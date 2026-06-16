#include "csv.c"
#include "unity.h"
#include <stdlib.h>
#include <string.h>

static CSV_BUFFER *buffer;

void setUp(void)
{
    buffer = csv_create_buffer();
}

void tearDown(void)
{
    if (buffer != NULL) {
        csv_destroy_buffer(buffer);
        buffer = NULL;
    }
}

/* Helper: populate buffer with a given number of rows, each with given width */
static void populate_buffer(CSV_BUFFER *buf, size_t num_rows, size_t cols_per_row)
{
    size_t r, c;
    for (r = 0; r < num_rows; r++) {
        append_row(buf);
        for (c = 0; c < cols_per_row; c++) {
            append_field(buf, r);
        }
    }
}

/* Helper: populate buffer with rows of varying widths */
static void populate_buffer_varying(CSV_BUFFER *buf, size_t num_rows, size_t *widths)
{
    size_t r, c;
    for (r = 0; r < num_rows; r++) {
        append_row(buf);
        for (c = 0; c < widths[r]; c++) {
            append_field(buf, r);
        }
    }
}

/* Test: row index 0 on a buffer with one row and zero fields */
void test_csv_get_width_single_row_zero_fields(void)
{
    append_row(buffer);
    int result = csv_get_width(buffer, 0);
    TEST_ASSERT_EQUAL_INT(0, result);
}

/* Test: row index 0 on a buffer with one row and one field */
void test_csv_get_width_single_row_one_field(void)
{
    append_row(buffer);
    append_field(buffer, 0);
    int result = csv_get_width(buffer, 0);
    TEST_ASSERT_EQUAL_INT(1, result);
}

/* Test: row index 0 on a buffer with one row and multiple fields */
void test_csv_get_width_single_row_multiple_fields(void)
{
    append_row(buffer);
    append_field(buffer, 0);
    append_field(buffer, 0);
    append_field(buffer, 0);
    int result = csv_get_width(buffer, 0);
    TEST_ASSERT_EQUAL_INT(3, result);
}

/* Test: row out of bounds returns 0 when buffer has no rows */
void test_csv_get_width_no_rows_returns_zero(void)
{
    int result = csv_get_width(buffer, 0);
    TEST_ASSERT_EQUAL_INT(0, result);
}

/* Test: row index exactly equal to rows - 1 (last valid row) */
void test_csv_get_width_last_valid_row(void)
{
    size_t widths[3] = {2, 4, 6};
    populate_buffer_varying(buffer, 3, widths);
    int result = csv_get_width(buffer, 2);
    TEST_ASSERT_EQUAL_INT(6, result);
}

/* Test: row index one beyond the last valid row returns 0 */
void test_csv_get_width_one_beyond_last_row_returns_zero(void)
{
    populate_buffer(buffer, 3, 5);
    int result = csv_get_width(buffer, 3);
    TEST_ASSERT_EQUAL_INT(0, result);
}

/* Test: row index far beyond the last valid row returns 0 */
void test_csv_get_width_far_beyond_last_row_returns_zero(void)
{
    populate_buffer(buffer, 2, 3);
    int result = csv_get_width(buffer, 100);
    TEST_ASSERT_EQUAL_INT(0, result);
}

/* Test: multiple rows with different widths, check each row individually */
void test_csv_get_width_multiple_rows_varying_widths(void)
{
    size_t widths[4] = {1, 3, 2, 5};
    populate_buffer_varying(buffer, 4, widths);

    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buffer, 0));
    TEST_ASSERT_EQUAL_INT(3, csv_get_width(buffer, 1));
    TEST_ASSERT_EQUAL_INT(2, csv_get_width(buffer, 2));
    TEST_ASSERT_EQUAL_INT(5, csv_get_width(buffer, 3));
}

/* Test: first row of a multi-row buffer */
void test_csv_get_width_first_row_of_multi_row_buffer(void)
{
    size_t widths[3] = {7, 2, 4};
    populate_buffer_varying(buffer, 3, widths);
    int result = csv_get_width(buffer, 0);
    TEST_ASSERT_EQUAL_INT(7, result);
}

/* Test: middle row of a multi-row buffer */
void test_csv_get_width_middle_row_of_multi_row_buffer(void)
{
    size_t widths[5] = {1, 2, 9, 4, 5};
    populate_buffer_varying(buffer, 5, widths);
    int result = csv_get_width(buffer, 2);
    TEST_ASSERT_EQUAL_INT(9, result);
}

/* Test: width after csv_set_field adds fields via public API */
void test_csv_get_width_after_csv_set_field(void)
{
    append_row(buffer);
    csv_set_field(buffer, 0, 0, "hello");
    csv_set_field(buffer, 0, 1, "world");
    int result = csv_get_width(buffer, 0);
    TEST_ASSERT_EQUAL_INT(2, result);
}

/* Test: width of row 0 when buffer has exactly one row with many fields */
void test_csv_get_width_many_fields_single_row(void)
{
    append_row(buffer);
    size_t i;
    for (i = 0; i < 10; i++) {
        append_field(buffer, 0);
    }
    int result = csv_get_width(buffer, 0);
    TEST_ASSERT_EQUAL_INT(10, result);
}

/* Test: row index 0 is valid boundary when rows == 1 */
void test_csv_get_width_boundary_row_zero_when_rows_equals_one(void)
{
    append_row(buffer);
    append_field(buffer, 0);
    append_field(buffer, 0);
    /* row 0 is valid (rows - 1 == 0) */
    int result = csv_get_width(buffer, 0);
    TEST_ASSERT_EQUAL_INT(2, result);
}

/* Test: row index 1 is invalid when rows == 1, returns 0 */
void test_csv_get_width_row_one_invalid_when_rows_equals_one(void)
{
    append_row(buffer);
    append_field(buffer, 0);
    int result = csv_get_width(buffer, 1);
    TEST_ASSERT_EQUAL_INT(0, result);
}

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_csv_get_width_single_row_zero_fields);
    RUN_TEST(test_csv_get_width_single_row_one_field);
    RUN_TEST(test_csv_get_width_single_row_multiple_fields);
    RUN_TEST(test_csv_get_width_no_rows_returns_zero);
    RUN_TEST(test_csv_get_width_last_valid_row);
    RUN_TEST(test_csv_get_width_one_beyond_last_row_returns_zero);
    RUN_TEST(test_csv_get_width_far_beyond_last_row_returns_zero);
    RUN_TEST(test_csv_get_width_multiple_rows_varying_widths);
    RUN_TEST(test_csv_get_width_first_row_of_multi_row_buffer);
    RUN_TEST(test_csv_get_width_middle_row_of_multi_row_buffer);
    RUN_TEST(test_csv_get_width_after_csv_set_field);
    RUN_TEST(test_csv_get_width_many_fields_single_row);
    RUN_TEST(test_csv_get_width_boundary_row_zero_when_rows_equals_one);
    RUN_TEST(test_csv_get_width_row_one_invalid_when_rows_equals_one);
    return UNITY_END();
}