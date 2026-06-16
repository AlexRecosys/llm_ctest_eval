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
    size_t r, f;
    char text[32];

    for (r = 0; r < num_rows; r++) {
        append_row(buf);
        for (f = 0; f < fields_per_row; f++) {
            append_field(buf, r);
            snprintf(text, sizeof(text), "r%zu_f%zu", r, f);
            csv_set_field(buf, r, f, text);
        }
    }
}

/* Helper: populate buffer with variable widths per row */
static void populate_buffer_variable(CSV_BUFFER *buf, size_t num_rows, size_t *widths)
{
    size_t r, f;
    char text[32];

    for (r = 0; r < num_rows; r++) {
        append_row(buf);
        for (f = 0; f < widths[r]; f++) {
            append_field(buf, r);
            snprintf(text, sizeof(text), "r%zu_f%zu", r, f);
            csv_set_field(buf, r, f, text);
        }
    }
}

/* Test: row index exactly at boundary (last valid row) */
void test_csv_get_width_last_valid_row(void)
{
    populate_buffer(buffer, 3, 4);
    int width = csv_get_width(buffer, 2);
    TEST_ASSERT_EQUAL_INT(4, width);
}

/* Test: row index 0 with single row */
void test_csv_get_width_single_row_index_zero(void)
{
    populate_buffer(buffer, 1, 5);
    int width = csv_get_width(buffer, 0);
    TEST_ASSERT_EQUAL_INT(5, width);
}

/* Test: row index beyond last row returns 0 */
void test_csv_get_width_row_out_of_bounds_returns_zero(void)
{
    populate_buffer(buffer, 2, 3);
    int width = csv_get_width(buffer, 2);
    TEST_ASSERT_EQUAL_INT(0, width);
}

/* Test: row index far beyond last row returns 0 */
void test_csv_get_width_row_far_out_of_bounds_returns_zero(void)
{
    populate_buffer(buffer, 2, 3);
    int width = csv_get_width(buffer, 100);
    TEST_ASSERT_EQUAL_INT(0, width);
}

/* Test: empty buffer (0 rows), any row index returns 0 */
void test_csv_get_width_empty_buffer_returns_zero(void)
{
    /* buffer has 0 rows after creation */
    int width = csv_get_width(buffer, 0);
    TEST_ASSERT_EQUAL_INT(0, width);
}

/* Test: first row of multi-row buffer */
void test_csv_get_width_first_row_of_multiple(void)
{
    populate_buffer(buffer, 4, 6);
    int width = csv_get_width(buffer, 0);
    TEST_ASSERT_EQUAL_INT(6, width);
}

/* Test: middle row of multi-row buffer */
void test_csv_get_width_middle_row_of_multiple(void)
{
    populate_buffer(buffer, 5, 7);
    int width = csv_get_width(buffer, 2);
    TEST_ASSERT_EQUAL_INT(7, width);
}

/* Test: variable widths per row — row 0 */
void test_csv_get_width_variable_widths_row0(void)
{
    size_t widths[] = {2, 5, 3};
    populate_buffer_variable(buffer, 3, widths);
    int width = csv_get_width(buffer, 0);
    TEST_ASSERT_EQUAL_INT(2, width);
}

/* Test: variable widths per row — row 1 */
void test_csv_get_width_variable_widths_row1(void)
{
    size_t widths[] = {2, 5, 3};
    populate_buffer_variable(buffer, 3, widths);
    int width = csv_get_width(buffer, 1);
    TEST_ASSERT_EQUAL_INT(5, width);
}

/* Test: variable widths per row — row 2 */
void test_csv_get_width_variable_widths_row2(void)
{
    size_t widths[] = {2, 5, 3};
    populate_buffer_variable(buffer, 3, widths);
    int width = csv_get_width(buffer, 2);
    TEST_ASSERT_EQUAL_INT(3, width);
}

/* Test: variable widths per row — out of bounds */
void test_csv_get_width_variable_widths_out_of_bounds(void)
{
    size_t widths[] = {2, 5, 3};
    populate_buffer_variable(buffer, 3, widths);
    int width = csv_get_width(buffer, 3);
    TEST_ASSERT_EQUAL_INT(0, width);
}

/* Test: single row with single field */
void test_csv_get_width_single_row_single_field(void)
{
    populate_buffer(buffer, 1, 1);
    int width = csv_get_width(buffer, 0);
    TEST_ASSERT_EQUAL_INT(1, width);
}

/* Test: row index 1 is out of bounds when only 1 row exists */
void test_csv_get_width_one_row_index_one_out_of_bounds(void)
{
    populate_buffer(buffer, 1, 3);
    int width = csv_get_width(buffer, 1);
    TEST_ASSERT_EQUAL_INT(0, width);
}

/* Test: width after csv_clear_row reduces to 1 */
void test_csv_get_width_after_clear_row(void)
{
    populate_buffer(buffer, 2, 5);
    csv_clear_row(buffer, 0);
    int width = csv_get_width(buffer, 0);
    TEST_ASSERT_EQUAL_INT(1, width);
}

/* Test: width after csv_clear_field on last field decrements width */
void test_csv_get_width_after_clear_last_field(void)
{
    populate_buffer(buffer, 1, 4);
    csv_clear_field(buffer, 0, 3);
    int width = csv_get_width(buffer, 0);
    TEST_ASSERT_EQUAL_INT(3, width);
}

/* Test: width is consistent after adding fields via csv_set_field on new entries */
void test_csv_get_width_multiple_rows_consistent(void)
{
    populate_buffer(buffer, 3, 3);
    TEST_ASSERT_EQUAL_INT(3, csv_get_width(buffer, 0));
    TEST_ASSERT_EQUAL_INT(3, csv_get_width(buffer, 1));
    TEST_ASSERT_EQUAL_INT(3, csv_get_width(buffer, 2));
}

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_csv_get_width_last_valid_row);
    RUN_TEST(test_csv_get_width_single_row_index_zero);
    RUN_TEST(test_csv_get_width_row_out_of_bounds_returns_zero);
    RUN_TEST(test_csv_get_width_row_far_out_of_bounds_returns_zero);
    RUN_TEST(test_csv_get_width_empty_buffer_returns_zero);
    RUN_TEST(test_csv_get_width_first_row_of_multiple);
    RUN_TEST(test_csv_get_width_middle_row_of_multiple);
    RUN_TEST(test_csv_get_width_variable_widths_row0);
    RUN_TEST(test_csv_get_width_variable_widths_row1);
    RUN_TEST(test_csv_get_width_variable_widths_row2);
    RUN_TEST(test_csv_get_width_variable_widths_out_of_bounds);
    RUN_TEST(test_csv_get_width_single_row_single_field);
    RUN_TEST(test_csv_get_width_one_row_index_one_out_of_bounds);
    RUN_TEST(test_csv_get_width_after_clear_row);
    RUN_TEST(test_csv_get_width_after_clear_last_field);
    RUN_TEST(test_csv_get_width_multiple_rows_consistent);
    return UNITY_END();
}