#include "unity.h"
#include "csv.h"
#include <stdlib.h>
#include <string.h>

// Helper to create a simple 1x1 buffer with one field
static CSV_BUFFER* create_simple_buffer(size_t rows, size_t cols, const char* initial_value) {
    CSV_BUFFER* buffer = csv_create_buffer();
    TEST_ASSERT_NOT_NULL(buffer);

    // Set fields to create the grid
    for (size_t r = 0; r < rows; r++) {
        for (size_t c = 0; c < cols; c++) {
            int ret = csv_set_field(buffer, r, c, initial_value);
            TEST_ASSERT_EQUAL_INT(0, ret);
        }
    }

    return buffer;
}

// Helper to verify field content at (row, col)
static void assert_field_equals(CSV_BUFFER* buffer, size_t row, size_t col, const char* expected) {
    TEST_ASSERT_TRUE(row < buffer->rows);
    TEST_ASSERT_TRUE(col < buffer->width[row]);
    TEST_ASSERT_NOT_NULL(buffer->field[row][col]);
    TEST_ASSERT_EQUAL_STRING(expected, buffer->field[row][col]->text);
}

// Test: Insert at existing position (middle of row)
void test_csv_insert_field_insert_in_middle(void) {
    // Create a 1x3 buffer: ["A", "B", "C"]
    CSV_BUFFER* buffer = create_simple_buffer(1, 3, "placeholder");
    csv_set_field(buffer, 0, 0, "A");
    csv_set_field(buffer, 0, 1, "B");
    csv_set_field(buffer, 0, 2, "C");

    // Insert "X" at position 1 (should shift B and C right)
    int ret = csv_insert_field(buffer, 0, 1, "X");
    TEST_ASSERT_EQUAL_INT(0, ret);

    // Verify: ["A", "X", "B", "C"]
    assert_field_equals(buffer, 0, 0, "A");
    assert_field_equals(buffer, 0, 1, "X");
    assert_field_equals(buffer, 0, 2, "B");
    assert_field_equals(buffer, 0, 3, "C");
    TEST_ASSERT_EQUAL_SIZE_T(4, buffer->width[0]);

    csv_destroy_buffer(buffer);
}

// Test: Insert at end of row (should behave like append)
void test_csv_insert_field_insert_at_end(void) {
    // Create a 1x2 buffer: ["A", "B"]
    CSV_BUFFER* buffer = create_simple_buffer(1, 2, "placeholder");
    csv_set_field(buffer, 0, 0, "A");
    csv_set_field(buffer, 0, 1, "B");

    // Insert "C" at position 2 (end)
    int ret = csv_insert_field(buffer, 0, 2, "C");
    TEST_ASSERT_EQUAL_INT(0, ret);

    // Verify: ["A", "B", "C"]
    assert_field_equals(buffer, 0, 0, "A");
    assert_field_equals(buffer, 0, 1, "B");
    assert_field_equals(buffer, 0, 2, "C");
    TEST_ASSERT_EQUAL_SIZE_T(3, buffer->width[0]);

    csv_destroy_buffer(buffer);
}

// Test: Insert beyond existing row/col (should call csv_set_field → expand grid)
void test_csv_insert_field_expand_grid(void) {
    // Start with empty buffer (0 rows)
    CSV_BUFFER* buffer = csv_create_buffer();
    TEST_ASSERT_NOT_NULL(buffer);

    // Insert at row=2, col=1 (should create rows 0,1,2 and expand row 2 to 2 cols)
    int ret = csv_insert_field(buffer, 2, 1, "NEW");
    TEST_ASSERT_EQUAL_INT(0, ret);

    // Verify grid expanded correctly
    TEST_ASSERT_EQUAL_SIZE_T(3, buffer->rows);
    TEST_ASSERT_EQUAL_SIZE_T(2, buffer->width[2]);
    assert_field_equals(buffer, 2, 1, "NEW");

    // Check other rows are empty (or default)
    TEST_ASSERT_EQUAL_SIZE_T(0, buffer->width[0]);
    TEST_ASSERT_EQUAL_SIZE_T(0, buffer->width[1]);

    csv_destroy_buffer(buffer);
}

// Test: Insert at beginning of row (shift all right)
void test_csv_insert_field_insert_at_start(void) {
    // Create a 1x2 buffer: ["A", "B"]
    CSV_BUFFER* buffer = create_simple_buffer(1, 2, "placeholder");
    csv_set_field(buffer, 0, 0, "A");
    csv_set_field(buffer, 0, 1, "B");

    // Insert "Z" at position 0
    int ret = csv_insert_field(buffer, 0, 0, "Z");
    TEST_ASSERT_EQUAL_INT(0, ret);

    // Verify: ["Z", "A", "B"]
    assert_field_equals(buffer, 0, 0, "Z");
    assert_field_equals(buffer, 0, 1, "A");
    assert_field_equals(buffer, 0, 2, "B");
    TEST_ASSERT_EQUAL_SIZE_T(3, buffer->width[0]);

    csv_destroy_buffer(buffer);
}

// Test: Insert into empty row (edge case: row exists but width=0)
void test_csv_insert_field_empty_row(void) {
    // Create buffer with 1 row, but no fields yet
    CSV_BUFFER* buffer = csv_create_buffer();
    TEST_ASSERT_NOT_NULL(buffer);

    // Add a row manually (via csv_set_field with col=0)
    csv_set_field(buffer, 0, 0, "initial");

    // Now insert at col=0 again (should shift existing field right)
    int ret = csv_insert_field(buffer, 0, 0, "new_first");
    TEST_ASSERT_EQUAL_INT(0, ret);

    // Verify: ["new_first", "initial"]
    assert_field_equals(buffer, 0, 0, "new_first");
    assert_field_equals(buffer, 0, 1, "initial");
    TEST_ASSERT_EQUAL_SIZE_T(2, buffer->width[0]);

    csv_destroy_buffer(buffer);
}