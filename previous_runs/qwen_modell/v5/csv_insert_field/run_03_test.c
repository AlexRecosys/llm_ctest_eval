#include "unity.h"
#include "csv.h"
#include <stdlib.h>
#include <string.h>

// Helper to create a simple 1x1 buffer with one field for setup
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
    TEST_ASSERT_NOT_NULL(buffer);
    TEST_ASSERT_LESS_THAN(buffer->rows, row);
    TEST_ASSERT_LESS_THAN(buffer->width[row], col);
    
    CSV_FIELD* field = buffer->field[row][col];
    TEST_ASSERT_NOT_NULL(field);
    TEST_ASSERT_EQUAL_STRING(expected, field->text);
}

// Test: Insert at existing position (middle of row) shifts fields right
void test_csv_insert_field_insert_in_middle(void) {
    CSV_BUFFER* buffer = create_simple_buffer("A");
    
    // Expand row to 3 columns: [A, B, C]
    csv_set_field(buffer, 0, 1, "B");
    csv_set_field(buffer, 0, 2, "C");
    
    // Now insert "X" at position 1 → should shift B and C right
    int result = csv_insert_field(buffer, 0, 1, "X");
    TEST_ASSERT_EQUAL_INT(0, result);
    
    // Verify: [A, X, B, C]
    assert_field_equals(buffer, 0, 0, "A");
    assert_field_equals(buffer, 0, 1, "X");
    assert_field_equals(buffer, 0, 2, "B");
    assert_field_equals(buffer, 0, 3, "C");
    
    csv_destroy_buffer(buffer);
}

// Test: Insert at end of row (should behave like append)
void test_csv_insert_field_insert_at_end(void) {
    CSV_BUFFER* buffer = create_simple_buffer("A");
    
    // Set second field
    csv_set_field(buffer, 0, 1, "B");
    
    // Insert at position 2 (beyond current width=2) → should call csv_set_field
    int result = csv_insert_field(buffer, 0, 2, "C");
    TEST_ASSERT_EQUAL_INT(0, result);
    
    // Verify: [A, B, C]
    assert_field_equals(buffer, 0, 0, "A");
    assert_field_equals(buffer, 0, 1, "B");
    assert_field_equals(buffer, 0, 2, "C");
    
    csv_destroy_buffer(buffer);
}

// Test: Insert beyond existing row (should create new row)
void test_csv_insert_field_insert_beyond_row(void) {
    CSV_BUFFER* buffer = create_simple_buffer("A");
    
    // Insert at row 1 (nonexistent) → should create row 1 and set field
    int result = csv_insert_field(buffer, 1, 0, "NewRow");
    TEST_ASSERT_EQUAL_INT(0, result);
    
    // Verify new row exists and field is set
    TEST_ASSERT_EQUAL_SIZE(2, buffer->rows);
    assert_field_equals(buffer, 1, 0, "NewRow");
    
    csv_destroy_buffer(buffer);
}

// Test: Insert at existing position in multi-column row
void test_csv_insert_field_multi_column_shift(void) {
    CSV_BUFFER* buffer = create_simple_buffer("A");
    
    // Build row: [A, B, C, D]
    csv_set_field(buffer, 0, 1, "B");
    csv_set_field(buffer, 0, 2, "C");
    csv_set_field(buffer, 0, 3, "D");
    
    // Insert "X" at position 2 → should shift C and D right
    int result = csv_insert_field(buffer, 0, 2, "X");
    TEST_ASSERT_EQUAL_INT(0, result);
    
    // Verify: [A, B, X, C, D]
    assert_field_equals(buffer, 0, 0, "A");
    assert_field_equals(buffer, 0, 1, "B");
    assert_field_equals(buffer, 0, 2, "X");
    assert_field_equals(buffer, 0, 3, "C");
    assert_field_equals(buffer, 0, 4, "D");
    
    csv_destroy_buffer(buffer);
}

// Test: Insert at position 0 (shift all right)
void test_csv_insert_field_insert_at_zero(void) {
    CSV_BUFFER* buffer = create_simple_buffer("A");
    csv_set_field(buffer, 0, 1, "B");
    
    // Insert at 0 → shift everything right
    int result = csv_insert_field(buffer, 0, 0, "First");
    TEST_ASSERT_EQUAL_INT(0, result);
    
    // Verify: [First, A, B]
    assert_field_equals(buffer, 0, 0, "First");
    assert_field_equals(buffer, 0, 1, "A");
    assert_field_equals(buffer, 0, 2, "B");
    
    csv_destroy_buffer(buffer);
}

// Test: Insert into empty buffer (row 0, col 0)
void test_csv_insert_field_empty_buffer(void) {
    CSV_BUFFER* buffer = csv_create_buffer();
    TEST_ASSERT_NOT_NULL(buffer);
    
    // Insert into empty buffer (row 0, col 0)
    int result = csv_insert_field(buffer, 0, 0, "First");
    TEST_ASSERT_EQUAL_INT(0, result);
    
    // Verify: [First]
    assert_field_equals(buffer, 0, 0, "First");
    
    csv_destroy_buffer(buffer);
}

// Main test runner
int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_csv_insert_field_insert_in_middle);
    RUN_TEST(test_csv_insert_field_insert_at_end);
    RUN_TEST(test_csv_insert_field_insert_beyond_row);
    RUN_TEST(test_csv_insert_field_multi_column_shift);
    RUN_TEST(test_csv_insert_field_insert_at_zero);
    RUN_TEST(test_csv_insert_field_empty_buffer);
    
    return UNITY_END();
}