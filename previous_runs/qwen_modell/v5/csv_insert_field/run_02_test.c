#include "unity.h"
#include "csv.h"
#include <stdlib.h>
#include <string.h>

// Helper to create a simple 1x1 buffer with one field for initial setup
static CSV_BUFFER* create_simple_buffer(const char* initial_value) {
    CSV_BUFFER* buffer = csv_create_buffer();
    TEST_ASSERT_NOT_NULL(buffer);
    
    // Set field at (0,0) to initialize buffer with one row and one column
    int result = csv_set_field(buffer, 0, 0, initial_value);
    TEST_ASSERT_EQUAL_INT(0, result);
    
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
    CSV_BUFFER* buffer = create_simple_buffer("A");
    
    // Expand row to 3 columns: ["A", "B", "C"]
    csv_set_field(buffer, 0, 1, "B");
    csv_set_field(buffer, 0, 2, "C");
    
    // Now insert "X" at position 1 → should shift "B" and "C" right
    int result = csv_insert_field(buffer, 0, 1, "X");
    TEST_ASSERT_EQUAL_INT(0, result);
    
    // Verify: ["A", "X", "B", "C"]
    assert_field_equals(buffer, 0, 0, "A");
    assert_field_equals(buffer, 0, 1, "X");
    assert_field_equals(buffer, 0, 2, "B");
    assert_field_equals(buffer, 0, 3, "C");
    
    csv_destroy_buffer(buffer);
}

// Test: Insert at end of row (should behave like append)
void test_csv_insert_field_insert_at_end(void) {
    CSV_BUFFER* buffer = create_simple_buffer("A");
    
    // Insert at position 1 (beyond current width=1) → should call csv_set_field
    int result = csv_insert_field(buffer, 0, 1, "B");
    TEST_ASSERT_EQUAL_INT(0, result);
    
    // Verify: ["A", "B"]
    assert_field_equals(buffer, 0, 0, "A");
    assert_field_equals(buffer, 0, 1, "B");
    
    csv_destroy_buffer(buffer);
}

// Test: Insert beyond existing row (should create new row)
void test_csv_insert_field_new_row(void) {
    CSV_BUFFER* buffer = create_simple_buffer("A");
    
    // Insert at row=1, col=0 → should create row 1
    int result = csv_insert_field(buffer, 1, 0, "NewRow");
    TEST_ASSERT_EQUAL_INT(0, result);
    
    // Verify new row exists and has correct value
    TEST_ASSERT_EQUAL_SIZE(2, buffer->rows);
    assert_field_equals(buffer, 1, 0, "NewRow");
    
    csv_destroy_buffer(buffer);
}

// Test: Insert at beginning of row (shift all right)
void test_csv_insert_field_insert_at_start(void) {
    CSV_BUFFER* buffer = create_simple_buffer("A");
    
    // Add second field first
    csv_set_field(buffer, 0, 1, "B");
    
    // Insert at position 0 → shift "A" and "B" right
    int result = csv_insert_field(buffer, 0, 0, "X");
    TEST_ASSERT_EQUAL_INT(0, result);
    
    // Verify: ["X", "A", "B"]
    assert_field_equals(buffer, 0, 0, "X");
    assert_field_equals(buffer, 0, 1, "A");
    assert_field_equals(buffer, 0, 2, "B");
    
    csv_destroy_buffer(buffer);
}

// Test: Insert into empty buffer (row=0, col=0)
void test_csv_insert_field_empty_buffer_first_field(void) {
    CSV_BUFFER* buffer = csv_create_buffer();
    TEST_ASSERT_NOT_NULL(buffer);
    
    // Insert first field directly
    int result = csv_insert_field(buffer, 0, 0, "First");
    TEST_ASSERT_EQUAL_INT(0, result);
    
    // Verify
    TEST_ASSERT_EQUAL_SIZE(1, buffer->rows);
    assert_field_equals(buffer, 0, 0, "First");
    
    csv_destroy_buffer(buffer);
}

// Test: Insert into non-existent column in existing row (edge case: entry == width[row])
void test_csv_insert_field_at_width_boundary(void) {
    CSV_BUFFER* buffer = create_simple_buffer("A");
    
    // width[0] = 1, so entry=1 is beyond current last column (0)
    // Should call csv_set_field (which expands row)
    int result = csv_insert_field(buffer, 0, 1, "B");
    TEST_ASSERT_EQUAL_INT(0, result);
    
    // Verify: ["A", "B"]
    assert_field_equals(buffer, 0, 0, "A");
    assert_field_equals(buffer, 0, 1, "B");
    
    csv_destroy_buffer(buffer);
}

// Main test runner
int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_csv_insert_field_insert_in_middle);
    RUN_TEST(test_csv_insert_field_insert_at_end);
    RUN_TEST(test_csv_insert_field_new_row);
    RUN_TEST(test_csv_insert_field_insert_at_start);
    RUN_TEST(test_csv_insert_field_empty_buffer_first_field);
    RUN_TEST(test_csv_insert_field_at_width_boundary);
    
    return UNITY_END();
}