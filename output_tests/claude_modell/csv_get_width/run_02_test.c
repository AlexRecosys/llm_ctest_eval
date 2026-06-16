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

/* Helper: populate buffer with a given number of rows and fields per row */
static void populate_buffer(CSV_BUFFER *buf, size_t num_rows, size_t fields_per_row)
{
    for (size_t r = 0; r < num_rows; r++) {
        append_row(buf);
        for (size_t f = 0; f < fields_per_row; f++) {
            append_field(buf, r);
        }
    }
}

/* Helper: populate buffer with variable widths per row */
static void populate_buffer_variable(CSV_BUFFER *buf, size_t num_rows, size_t *widths)
{
    for (size_t r = 0; r < num_rows; r++) {
        append_row(buf);
        for (size_t f = 0; f < widths[r]; f++) {
            append_field(buf, r);
        }
    }
}

/* Test: row 0 out of bounds when buffer has no rows */
void test_csv_get_width_empty_buffer_returns_zero(void)
{
    int result = csv_get_width(buffer, 0);
    TEST_ASSERT_EQUAL_INT(0, result);
}

/* Test: row index exactly equal to rows (out of bounds) */
void test_csv_get_width_row_equals_rows_returns_zero(void)
{
    populate_buffer(buffer, 3, 2);
    /* buffer->rows == 3, so row index 3 is out of bounds */
    int result = csv_get_width(buffer, 3);
    TEST_ASSERT_EQUAL_INT(0, result);
}

/* Test: row index greater than rows (out of bounds) */
void test_csv_get_width_row_greater_than_rows_returns_zero(void)
{
    populate_buffer(buffer, 2, 4);
    int result = csv_get_width(buffer, 100);
    TEST_ASSERT_EQUAL_INT(0, result);
}

/* Test: single row with zero fields appended (width should be 0) */
void test_csv_get_width_single_row_no_fields(void)
{
    append_row(buffer);
    int result = csv_get_width(buffer, 0);
    TEST_ASSERT_EQUAL_INT(0, result);
}

/* Test: single row with one field */
void test_csv_get_width_single_row_one_field(void)
{
    append_row(buffer);
    append_field(buffer, 0);
    int result = csv_get_width(buffer, 0);
    TEST_ASSERT_EQUAL_INT(1, result);
}

/* Test: single row with multiple fields */
void test_csv_get_width_single_row_multiple_fields(void)
{
    append_row(buffer);
    append_field(buffer, 0);
    append_field(buffer, 0);
    append_field(buffer, 0);
    append_field(buffer, 0);
    append_field(buffer, 0);
    int result = csv_get_width(buffer, 0);
    TEST_ASSERT_EQUAL_INT(5, result);
}

/* Test: multiple rows with uniform width, check each row */
void test_csv_get_width_multiple_rows_uniform_width(void)
{
    populate_buffer(buffer, 4, 3);
    for (size_t r = 0; r < 4; r++) {
        int result = csv_get_width(buffer, r);
        TEST_ASSERT_EQUAL_INT_MESSAGE(3, result, "Expected width 3 for each row");
    }
}

/* Test: multiple rows with variable widths */
void test_csv_get_width_multiple_rows_variable_width(void)
{
    size_t widths[] = {1, 5, 3, 0, 7};
    populate_buffer_variable(buffer, 5, widths);

    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buffer, 0));
    TEST_ASSERT_EQUAL_INT(5, csv_get_width(buffer, 1));
    TEST_ASSERT_EQUAL_INT(3, csv_get_width(buffer, 2));
    TEST_ASSERT_EQUAL_INT(0, csv_get_width(buffer, 3));
    TEST_ASSERT_EQUAL_INT(7, csv_get_width(buffer, 4));
}

/* Test: last valid row index returns correct width */
void test_csv_get_width_last_valid_row(void)
{
    populate_buffer(buffer, 3, 6);
    /* Last valid row index is 2 */
    int result = csv_get_width(buffer, 2);
    TEST_ASSERT_EQUAL_INT(6, result);
}

/* Test: first row index (0) is valid */
void test_csv_get_width_first_row_valid(void)
{
    populate_buffer(buffer, 5, 4);
    int result = csv_get_width(buffer, 0);
    TEST_ASSERT_EQUAL_INT(4, result);
}

/* Test: width after csv_set_field extends the row */
void test_csv_get_width_after_set_field(void)
{
    append_row(buffer);
    /* csv_set_field should extend the row as needed */
    csv_set_field(buffer, 0, 0, "hello");
    csv_set_field(buffer, 0, 1, "world");
    int result = csv_get_width(buffer, 0);
    TEST_ASSERT_EQUAL_INT(2, result);
}

/* Test: width after loading from a CSV file */
void test_csv_get_width_after_csv_load(void)
{
    /* Create a temporary CSV file */
    const char *tmp_file = "/tmp/test_csv_get_width.csv";
    FILE *fp = fopen(tmp_file, "w");
    if (fp == NULL) {
        TEST_IGNORE_MESSAGE("Could not create temp file for csv_load test");
        return;
    }
    fprintf(fp, "a,b,c\n");
    fprintf(fp, "d,e\n");
    fprintf(fp, "f,g,h,i\n");
    fclose(fp);

    int load_result = csv_load(buffer, (char *)tmp_file);
    if (load_result != 0) {
        TEST_IGNORE_MESSAGE("csv_load failed; skipping file-based width test");
        return;
    }

    TEST_ASSERT_EQUAL_INT(3, csv_get_width(buffer, 0));
    TEST_ASSERT_EQUAL_INT(2, csv_get_width(buffer, 1));
    TEST_ASSERT_EQUAL_INT(4, csv_get_width(buffer, 2));

    remove(tmp_file);
}

/* Test: row index just beyond last valid row */
void test_csv_get_width_one_past_last_row(void)
{
    populate_buffer(buffer, 5, 3);
    /* rows == 5, last valid index == 4, so index 5 is out of bounds */
    int result = csv_get_width(buffer, 5);
    TEST_ASSERT_EQUAL_INT(0, result);
}

/* Test: width of row 0 when only one row exists */
void test_csv_get_width_only_row_zero(void)
{
    append_row(buffer);
    append_field(buffer, 0);
    append_field(buffer, 0);
    int result = csv_get_width(buffer, 0);
    TEST_ASSERT_EQUAL_INT(2, result);
}

/* Test: width after removing last field */
void test_csv_get_width_after_remove_last_field(void)
{
    append_row(buffer);
    append_field(buffer, 0);
    append_field(buffer, 0);
    append_field(buffer, 0);
    remove_last_field(buffer, 0);
    int result = csv_get_width(buffer, 0);
    TEST_ASSERT_EQUAL_INT(2, result);
}

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_csv_get_width_empty_buffer_returns_zero);
    RUN_TEST(test_csv_get_width_row_equals_rows_returns_zero);
    RUN_TEST(test_csv_get_width_row_greater_than_rows_returns_zero);
    RUN_TEST(test_csv_get_width_single_row_no_fields);
    RUN_TEST(test_csv_get_width_single_row_one_field);
    RUN_TEST(test_csv_get_width_single_row_multiple_fields);
    RUN_TEST(test_csv_get_width_multiple_rows_uniform_width);
    RUN_TEST(test_csv_get_width_multiple_rows_variable_width);
    RUN_TEST(test_csv_get_width_last_valid_row);
    RUN_TEST(test_csv_get_width_first_row_valid);
    RUN_TEST(test_csv_get_width_after_set_field);
    RUN_TEST(test_csv_get_width_after_csv_load);
    RUN_TEST(test_csv_get_width_one_past_last_row);
    RUN_TEST(test_csv_get_width_only_row_zero);
    RUN_TEST(test_csv_get_width_after_remove_last_field);
    return UNITY_END();
}