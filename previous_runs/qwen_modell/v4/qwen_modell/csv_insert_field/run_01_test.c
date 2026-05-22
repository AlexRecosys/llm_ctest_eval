#include "unity.h"
#include "csv.h"
#include <stdlib.h>
#include <string.h>

// Helper to create a simple 1x1 buffer with one field
static CSV_BUFFER* create_simple_buffer(size_t rows, size_t cols) {
    CSV_BUFFER *buf = csv_create_buffer();
    TEST_ASSERT_NOT_NULL(buf);

    // Set fields to ensure buffer has correct dimensions
    for (size_t r = 0; r < rows; r++) {
        for (size_t c = 0; c < cols; c++) {
            int res = csv_set_field(buf, r, c, "initial");
            TEST_ASSERT_EQUAL_INT(0, res);
        }
    }

    return buf;
}

// Helper to verify field content at (row, col)
static void assert_field_equals(CSV_BUFFER *buf, size_t row, size_t col, const char *expected) {
    TEST_ASSERT_TRUE(row < buf->rows);
    TEST_ASSERT_TRUE(col < buf->width[row]);
    TEST_ASSERT_NOT_NULL(buf->field[row][col]);
    TEST_ASSERT_EQUAL_STRING(expected, buf->field[row][col]->text);
}

// Test: Insert at existing position (middle of row)
TEST(csv_insert_field, insert_existing_position_middle) {
    CSV_BUFFER *buf = create_simple_buffer(1, 3);
    // Buffer now has 1 row, 3 columns: ["initial", "initial", "initial"]

    int res = csv_insert_field(buf, 0, 1, "new_value");
    TEST_ASSERT_EQUAL_INT(0, res);

    // After insert: ["initial", "new_value", "initial", "initial"]
    TEST_ASSERT_EQUAL_SIZE(4, buf->width[0]);
    assert_field_equals(buf, 0, 0, "initial");
    assert_field_equals(buf, 0, 1, "new_value");
    assert_field_equals(buf, 0, 2, "initial");
    assert_field_equals(buf, 0, 3, "initial");

    csv_destroy_buffer(buf);
}

// Test: Insert at beginning of row
TEST(csv_insert_field, insert_existing_position_beginning) {
    CSV_BUFFER *buf = create_simple_buffer(1, 2);
    // ["initial", "initial"]

    int res = csv_insert_field(buf, 0, 0, "first");
    TEST_ASSERT_EQUAL_INT(0, res);

    // After insert: ["first", "initial", "initial"]
    TEST_ASSERT_EQUAL_SIZE(3, buf->width[0]);
    assert_field_equals(buf, 0, 0, "first");
    assert_field_equals(buf, 0, 1, "initial");
    assert_field_equals(buf, 0, 2, "initial");

    csv_destroy_buffer(buf);
}

// Test: Insert at end of existing row (i.e., at current width)
TEST(csv_insert_field, insert_existing_position_end) {
    CSV_BUFFER *buf = create_simple_buffer(1, 2);
    // ["initial", "initial"]

    int res = csv_insert_field(buf, 0, 2, "last");
    TEST_ASSERT_EQUAL_INT(0, res);

    // After insert: ["initial", "initial", "last"]
    TEST_ASSERT_EQUAL_SIZE(3, buf->width[0]);
    assert_field_equals(buf, 0, 0, "initial");
    assert_field_equals(buf, 0, 1, "initial");
    assert_field_equals(buf, 0, 2, "last");

    csv_destroy_buffer(buf);
}

// Test: Insert beyond existing row/columns → should call csv_set_field (expand)
TEST(csv_insert_field, insert_beyond_bounds_expands) {
    CSV_BUFFER *buf = create_simple_buffer(1, 1);
    // ["initial"]

    int res = csv_insert_field(buf, 0, 5, "expanded");
    TEST_ASSERT_EQUAL_INT(0, res);

    // csv_set_field should have expanded row to 6 columns
    TEST_ASSERT_EQUAL_SIZE(6, buf->width[0]);
    assert_field_equals(buf, 0, 5, "expanded");

    // All intermediate fields should be "initial" (set by csv_set_field)
    for (size_t i = 0; i < 5; i++) {
        assert_field_equals(buf, 0, i, "initial");
    }

    csv_destroy_buffer(buf);
}

// Test: Insert into new row (row > rows-1)
TEST(csv_insert_field, insert_new_row_expands_rows) {
    CSV_BUFFER *buf = create_simple_buffer(1, 1);
    // 1 row, 1 column

    int res = csv_insert_field(buf, 2, 0, "new_row_field");
    TEST_ASSERT_EQUAL_INT(0, res);

    // Should have 3 rows now (0,1,2), row 2 has 1 column
    TEST_ASSERT_EQUAL_SIZE(3, buf->rows);
    TEST_ASSERT_EQUAL_SIZE(1, buf->width[2]);
    assert_field_equals(buf, 2, 0, "new_row_field");

    // Rows 0 and 1 should be untouched (still have 1 column each)
    TEST_ASSERT_EQUAL_SIZE(1, buf->width[0]);
    TEST_ASSERT_EQUAL_SIZE(1, buf->width[1]);

    csv_destroy_buffer(buf);
}

// Test: Insert into empty buffer (0 rows)
TEST(csv_insert_field, insert_into_empty_buffer) {
    CSV_BUFFER *buf = csv_create_buffer();
    TEST_ASSERT_EQUAL_SIZE(0, buf->rows);

    int res = csv_insert_field(buf, 0, 0, "first_field");
    TEST_ASSERT_EQUAL_INT(0, res);

    // Should have created 1 row, 1 column
    TEST_ASSERT_EQUAL_SIZE(1, buf->rows);
    TEST_ASSERT_EQUAL_SIZE(1, buf->width[0]);
    assert_field_equals(buf, 0, 0, "first_field");

    csv_destroy_buffer(buf);
}

// Test: Insert at valid position with empty string
TEST(csv_insert_field, insert_empty_string) {
    CSV_BUFFER *buf = create_simple_buffer(1, 2);
    // ["initial", "initial"]

    int res = csv_insert_field(buf, 0, 1, "");
    TEST_ASSERT_EQUAL_INT(0, res);

    // Should have inserted empty string
    TEST_ASSERT_EQUAL_SIZE(3, buf->width[0]);
    assert_field_equals(buf, 0, 1, "");

    csv_destroy_buffer(buf);
}