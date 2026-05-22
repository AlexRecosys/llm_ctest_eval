#include "unity.h"
#include "csv.h"
#include <stdlib.h>
#include <string.h>

// Helper: creates a 1x1 buffer with one field set to "initial"
static CSV_BUFFER *create_one_by_one_buffer(const char *initial_value)
{
    CSV_BUFFER *buffer = csv_create_buffer();
    TEST_ASSERT_NOT_NULL(buffer);

    // Set field at (0,0) to initial_value
    int result = csv_set_field(buffer, 0, 0, initial_value);
    TEST_ASSERT_EQUAL_INT(0, result);

    return buffer;
}

// Helper: verifies that field at (row, col) equals expected
static void assert_field_equals(CSV_BUFFER *buffer, size_t row, size_t col, const char *expected)
{
    TEST_ASSERT_TRUE(row < buffer->rows);
    TEST_ASSERT_TRUE(col < buffer->width[row]);
    TEST_ASSERT_NOT_NULL(buffer->field[row][col]);
    TEST_ASSERT_EQUAL_STRING(expected, buffer->field[row][col]->text);
}

// Helper: verifies buffer dimensions
static void assert_buffer_dimensions(CSV_BUFFER *buffer, size_t expected_rows, size_t expected_cols_row0)
{
    TEST_ASSERT_EQUAL_SIZE(expected_rows, buffer->rows);
    if (expected_rows > 0) {
        TEST_ASSERT_EQUAL_SIZE(expected_cols_row0, buffer->width[0]);
    }
}

void setUp(void)
{
    // Nothing needed — no global state
}

void tearDown(void)
{
    // Nothing needed — cleanup done per-test
}

// Test: insert at existing position (middle of row) → shifts right
void test_csv_insert_field_insert_in_middle_shifts_right(void)
{
    // Create buffer with 1 row, 2 fields: ["A", "B"]
    CSV_BUFFER *buffer = csv_create_buffer();
    TEST_ASSERT_NOT_NULL(buffer);

    // Set row 0, col 0 and 1
    TEST_ASSERT_EQUAL_INT(0, csv_set_field(buffer, 0, 0, "A"));
    TEST_ASSERT_EQUAL_INT(0, csv_set_field(buffer, 0, 1, "B"));

    // Now buffer is 1x2: ["A", "B"]
    assert_buffer_dimensions(buffer, 1, 2);
    assert_field_equals(buffer, 0, 0, "A");
    assert_field_equals(buffer, 0, 1, "B");

    // Insert "X" at position 1 → should shift "B" to position 2
    int result = csv_insert_field(buffer, 0, 1, "X");
    TEST_ASSERT_EQUAL_INT(0, result);

    // Now should be 1x3: ["A", "X", "B"]
    assert_buffer_dimensions(buffer, 1, 3);
    assert_field_equals(buffer, 0, 0, "A");
    assert_field_equals(buffer, 0, 1, "X");
    assert_field_equals(buffer, 0, 2, "B");

    csv_destroy_buffer(buffer);
}

// Test: insert at end of row → same as set_field (no shift)
void test_csv_insert_field_insert_at_end_no_shift(void)
{
    CSV_BUFFER *buffer = create_one_by_one_buffer("A");
    // buffer is 1x1: ["A"]

    // Insert at position 1 (end) → should expand width and set field
    int result = csv_insert_field(buffer, 0, 1, "B");
    TEST_ASSERT_EQUAL_INT(0, result);

    // Should be 1x2: ["A", "B"]
    assert_buffer_dimensions(buffer, 1, 2);
    assert_field_equals(buffer, 0, 0, "A");
    assert_field_equals(buffer, 0, 1, "B");

    csv_destroy_buffer(buffer);
}

// Test: insert beyond existing row → creates row and sets field (via csv_set_field path)
void test_csv_insert_field_insert_beyond_rows_creates_row(void)
{
    CSV_BUFFER *buffer = create_one_by_one_buffer("A");
    // buffer is 1x1: ["A"]

    // Insert at row=1, col=0 → should create row 1 and set field
    int result = csv_insert_field(buffer, 1, 0, "C");
    TEST_ASSERT_EQUAL_INT(0, result);

    // Should be 2x1: ["A"], ["C"]
    assert_buffer_dimensions(buffer, 2, 1);
    assert_field_equals(buffer, 0, 0, "A");
    assert_field_equals(buffer, 1, 0, "C");

    csv_destroy_buffer(buffer);
}

// Test: insert beyond existing column in existing row → expands width (via csv_set_field path)
void test_csv_insert_field_insert_beyond_columns_expands_width(void)
{
    CSV_BUFFER *buffer = create_one_by_one_buffer("A");
    // buffer is 1x1: ["A"]

    // Insert at row=0, col=3 → should expand width to 4 and set field at 3
    int result = csv_insert_field(buffer, 0, 3, "D");
    TEST_ASSERT_EQUAL_INT(0, result);

    // Should be 1x4: ["A", NULL?, NULL?, "D"]? 
    // But csv_set_field fills gaps with empty fields (per typical behavior)
    // Let's verify only the set fields:
    assert_buffer_dimensions(buffer, 1, 4);
    assert_field_equals(buffer, 0, 0, "A");
    // Positions 1 and 2 may be empty or default — but we only test what we set
    assert_field_equals(buffer, 0, 3, "D");

    csv_destroy_buffer(buffer);
}

// Test: insert at position 0 of non-empty row → shifts all right
void test_csv_insert_field_insert_at_zero_shifts_all(void)
{
    CSV_BUFFER *buffer = csv_create_buffer();
    TEST_ASSERT_NOT_NULL(buffer);

    // Build 1x3: ["X", "Y", "Z"]
    TEST_ASSERT_EQUAL_INT(0, csv_set_field(buffer, 0, 0, "X"));
    TEST_ASSERT_EQUAL_INT(0, csv_set_field(buffer, 0, 1, "Y"));
    TEST_ASSERT_EQUAL_INT(0, csv_set_field(buffer, 0, 2, "Z"));

    // Insert "A" at 0 → should shift all right
    int result = csv_insert_field(buffer, 0, 0, "A");
    TEST_ASSERT_EQUAL_INT(0, result);

    // Should be 1x4: ["A", "X", "Y", "Z"]
    assert_buffer_dimensions(buffer, 1, 4);
    assert_field_equals(buffer, 0, 0, "A");
    assert_field_equals(buffer, 0, 1, "X");
    assert_field_equals(buffer, 0, 2, "Y");
    assert_field_equals(buffer, 0, 3, "Z");

    csv_destroy_buffer(buffer);
}

// Test: insert into empty buffer (row=0, col=0)
void test_csv_insert_field_into_empty_buffer(void)
{
    CSV_BUFFER *buffer = csv_create_buffer();
    TEST_ASSERT_NOT_NULL(buffer);

    // Insert first field
    int result = csv_insert_field(buffer, 0, 0, "First");
    TEST_ASSERT_EQUAL_INT(0, result);

    assert_buffer_dimensions(buffer, 1, 1);
    assert_field_equals(buffer, 0, 0, "First");

    csv_destroy_buffer(buffer);
}