#include "unity.h"
#include "csv.h"

static CSV_BUFFER *buffer = NULL;

void setUp(void)
{
    buffer = csv_create_buffer();
    TEST_ASSERT_NOT_NULL_MESSAGE(buffer, "Failed to create buffer");
}

void tearDown(void)
{
    if (buffer != NULL) {
        csv_destroy_buffer(buffer);
        buffer = NULL;
    }
}

static void populate_row_with_fields(CSV_BUFFER *buf, size_t row, size_t field_count)
{
    int ret;
    ret = csv_set_field(buf, row, 0, "first");
    TEST_ASSERT_EQUAL_INT(0, ret);
    for (size_t i = 1; i < field_count; i++) {
        ret = csv_set_field(buf, row, i, "field");
        TEST_ASSERT_EQUAL_INT(0, ret);
    }
}

static void verify_row_cleared(CSV_BUFFER *buf, size_t row)
{
    char dest[256] = {0};
    int ret = csv_get_field(dest, sizeof(dest), buf, row, 0);
    TEST_ASSERT_EQUAL_INT(2, ret);  // empty field
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buf, row));
}

static void verify_row_not_cleared(CSV_BUFFER *buf, size_t row, size_t expected_width)
{
    TEST_ASSERT_EQUAL_INT((int)expected_width, csv_get_width(buf, row));
}

static void add_row_with_fields(CSV_BUFFER *buf, size_t field_count)
{
    int ret = csv_set_field(buf, buf->rows, 0, "first");
    TEST_ASSERT_EQUAL_INT(0, ret);
    for (size_t i = 1; i < field_count; i++) {
        ret = csv_set_field(buf, buf->rows, i, "field");
        TEST_ASSERT_EQUAL_INT(0, ret);
    }
}

static void add_rows_with_fields(CSV_BUFFER *buf, size_t row_count, size_t field_count)
{
    for (size_t r = 0; r < row_count; r++) {
        add_row_with_fields(buf, field_count);
    }
}

void test_csv_clear_row_last_row_simple_case(void)
{
    add_row_with_fields(buffer, 3);
    int ret = csv_clear_row(buffer, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buffer, 0));
    verify_row_cleared(buffer, 0);
}

void test_csv_clear_row_middle_row_clears_all_but_first(void)
{
    add_rows_with_fields(buffer, 3, 4);
    int ret = csv_clear_row(buffer, 1);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buffer, 1));
    verify_row_cleared(buffer, 1);
    TEST_ASSERT_EQUAL_INT(3, csv_get_height(buffer));
}

void test_csv_clear_row_first_row_with_many_fields(void)
{
    add_row_with_fields(buffer, 10);
    int ret = csv_clear_row(buffer, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buffer, 0));
    verify_row_cleared(buffer, 0);
}

void test_csv_clear_row_single_field_row_no_change(void)
{
    add_row_with_fields(buffer, 1);
    int ret = csv_clear_row(buffer, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buffer, 0));
    verify_row_cleared(buffer, 0);
}

void test_csv_clear_row_last_row_delegates_to_remove_last_row(void)
{
    add_rows_with_fields(buffer, 2, 5);
    size_t original_height = csv_get_height(buffer);
    int ret = csv_clear_row(buffer, 1);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(original_height, csv_get_height(buffer));
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buffer, 1));
    verify_row_cleared(buffer, 1);
}