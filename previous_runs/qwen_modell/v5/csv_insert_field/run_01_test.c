#include "unity.h"
#include "csv.h"
#include <stdlib.h>
#include <string.h>

// Helper to create a simple 1x1 buffer with one field for testing
static CSV_BUFFER* create_test_buffer_1x1(const char* initial_value)
{
    CSV_BUFFER* buffer = csv_create_buffer();
    TEST_ASSERT_NOT_NULL(buffer);
    int res = csv_set_field(buffer, 0, 0, initial_value);
    TEST_ASSERT_EQUAL_INT(0, res);
    return buffer;
}

// Helper to verify field content at (row, col)
static void assert_field_equals(CSV_BUFFER* buffer, size_t row, size_t col, const char* expected)
{
    TEST_ASSERT_TRUE(row < buffer->rows);
    TEST_ASSERT_TRUE(col < buffer->width[row]);
    TEST_ASSERT_NOT_NULL(buffer->field[row][col]);
    TEST_ASSERT_EQUAL_STRING(expected, buffer->field[row][col]->text);
}

// Test: insert at existing position (middle of row)
void test_csv_insert_field_insert_in_middle(void)
{
    // Setup: create a 1x3 buffer: ["A", "B", "C"]
    CSV_BUFFER* buffer = csv_create_buffer();
    TEST_ASSERT_NOT_NULL(buffer);

    int res = csv_set_field(buffer, 0, 0, "A");
    TEST_ASSERT_EQUAL_INT(0, res);
    res = csv_set_field(buffer, 0, 1, "B");
    TEST_ASSERT_EQUAL_INT(0, res);
    res = csv_set_field(buffer, 0, 2, "C");
    TEST_ASSERT_EQUAL_INT(0, res);

    // Verify initial state
    assert_field_equals(buffer, 0, 0, "A");
    assert_field_equals(buffer, 0, 1, "B");
    assert_field_equals(buffer, 0, 2, "C");

    // Act: insert "X" at position 1 (should shift B and C right)
    res = csv_insert_field(buffer, 0, 1, "X");
    TEST_ASSERT_EQUAL_INT(0, res);

    // Assert: width should increase to 4, and fields should be ["A", "X", "B", "C"]
    TEST_ASSERT_EQUAL_SIZE(4, buffer->width[0]);
    assert_field_equals(buffer, 0, 0, "A");
    assert_field_equals(buffer, 0, 1, "X");
    assert_field_equals(buffer, 0, 2, "B");
    assert_field_equals(buffer, 0, 3, "C");

    csv_destroy_buffer(buffer);
}

// Test: insert at end (should behave like set_field)
void test_csv_insert_field_insert_at_end(void)
{
    CSV_BUFFER* buffer = create_test_buffer_1x1("A");
    TEST_ASSERT_EQUAL_SIZE(1, buffer->width[0]);

    int res = csv_insert_field(buffer, 0, 1, "B");
    TEST_ASSERT_EQUAL_INT(0, res);

    // Should have expanded to 2 fields
    TEST_ASSERT_EQUAL_SIZE(2, buffer->width[0]);
    assert_field_equals(buffer, 0, 0, "A");
    assert_field_equals(buffer, 0, 1, "B");

    csv_destroy_buffer(buffer);
}

// Test: insert beyond current row (should create new row)
void test_csv_insert_field_new_row(void)
{
    CSV_BUFFER* buffer = create_test_buffer_1x1("A");
    TEST_ASSERT_EQUAL_SIZE(1, buffer->width[0]);
    TEST_ASSERT_EQUAL_SIZE(1, buffer->rows);

    int res = csv_insert_field(buffer, 1, 0, "B");
    TEST_ASSERT_EQUAL_INT(0, res);

    // Should have created new row
    TEST_ASSERT_EQUAL_SIZE(2, buffer->rows);
    TEST_ASSERT_EQUAL_SIZE(1, buffer->width[1]);
    assert_field_equals(buffer, 1, 0, "B");

    csv_destroy_buffer(buffer);
}

// Test: insert beyond current column in new row (should expand row width)
void test_csv_insert_field_new_row_wide(void)
{
    CSV_BUFFER* buffer = create_test_buffer_1x1("A");

    int res = csv_insert_field(buffer, 1, 2, "C");
    TEST_ASSERT_EQUAL_INT(0, res);

    // Row 1 should have width 3 (0,1,2)
    TEST_ASSERT_EQUAL_SIZE(3, buffer->width[1]);
    assert_field_equals(buffer, 1, 0, "");   // auto-created empty fields
    assert_field_equals(buffer, 1, 1, "");   // auto-created empty fields
    assert_field_equals(buffer, 1, 2, "C");

    csv_destroy_buffer(buffer);
}

// Test: insert at position 0 (shift everything right)
void test_csv_insert_field_at_zero(void)
{
    CSV_BUFFER* buffer = csv_create_buffer();
    TEST_ASSERT_NOT_NULL(buffer);

    int res = csv_set_field(buffer, 0, 0, "B");
    TEST_ASSERT_EQUAL_INT(0, res);
    res = csv_set_field(buffer, 0, 1, "C");
    TEST_ASSERT_EQUAL_INT(0, res);

    res = csv_insert_field(buffer, 0, 0, "A");
    TEST_ASSERT_EQUAL_INT(0, res);

    TEST_ASSERT_EQUAL_SIZE(3, buffer->width[0]);
    assert_field_equals(buffer, 0, 0, "A");
    assert_field_equals(buffer, 0, 1, "B");
    assert_field_equals(buffer, 0, 2, "C");

    csv_destroy_buffer(buffer);
}

// Test: insert into empty buffer (row 0, col 0)
void test_csv_insert_field_empty_buffer(void)
{
    CSV_BUFFER* buffer = csv_create_buffer();
    TEST_ASSERT_NOT_NULL(buffer);

    int res = csv_insert_field(buffer, 0, 0, "Hello");
    TEST_ASSERT_EQUAL_INT(0, res);

    TEST_ASSERT_EQUAL_SIZE(1, buffer->rows);
    TEST_ASSERT_EQUAL_SIZE(1, buffer->width[0]);
    assert_field_equals(buffer, 0, 0, "Hello");

    csv_destroy_buffer(buffer);
}

// Test: insert into existing row but beyond current width (should auto-grow)
void test_csv_insert_field_beyond_width(void)
{
    CSV_BUFFER* buffer = create_test_buffer_1x1("A");

    int res = csv_insert_field(buffer, 0, 5, "F");
    TEST_ASSERT_EQUAL_INT(0, res);

    // Should have grown to width 6
    TEST_ASSERT_EQUAL_SIZE(6, buffer->width[0]);
    assert_field_equals(buffer, 0, 0, "A");
    assert_field_equals(buffer, 0, 5, "F");
    // Check middle fields are empty (as per csv_set_field behavior)
    for (size_t i = 1; i < 5; i++) {
        assert_field_equals(buffer, 0, i, "");
    }

    csv_destroy_buffer(buffer);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_csv_insert_field_insert_in_middle);
    RUN_TEST(test_csv_insert_field_insert_at_end);
    RUN_TEST(test_csv_insert_field_new_row);
    RUN_TEST(test_csv_insert_field_new_row_wide);
    RUN_TEST(test_csv_insert_field_at_zero);
    RUN_TEST(test_csv_insert_field_empty_buffer);
    RUN_TEST(test_csv_insert_field_beyond_width);

    return UNITY_END();
}