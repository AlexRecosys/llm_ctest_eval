#include "csv.c"
#include "unity.h"
#include <stdlib.h>
#include <string.h>

/* File-scope fixtures */
static CSV_BUFFER *buffer;

/* Helper: build a buffer with given content for testing */
static void build_buffer_from_string(CSV_BUFFER *buf, const char *csv_text)
{
    /* Write csv_text to a temp file, then load it */
    FILE *fp = fopen("_test_tmp.csv", "w");
    if (fp) {
        fputs(csv_text, fp);
        fclose(fp);
    }
    csv_load(buf, "_test_tmp.csv");
    remove("_test_tmp.csv");
}

void setUp(void)
{
    buffer = csv_create_buffer();
}

void tearDown(void)
{
    csv_destroy_buffer(buffer);
    buffer = NULL;
}

/* ------------------------------------------------------------------ */
/* Test cases                                                           */
/* ------------------------------------------------------------------ */

/* Row index out of bounds (row >= buffer->rows) returns 0 */
void test_row_out_of_bounds_returns_zero(void)
{
    build_buffer_from_string(buffer, "hello,world\n");
    /* buffer has 1 row (index 0); request row 1 */
    int result = csv_get_field_length(buffer, 1, 0);
    TEST_ASSERT_EQUAL_INT(0, result);
}

/* Entry index out of bounds (entry >= buffer->width[row]) returns 0 */
void test_entry_out_of_bounds_returns_zero(void)
{
    build_buffer_from_string(buffer, "hello,world\n");
    /* row 0 has 2 entries (indices 0 and 1); request entry 2 */
    int result = csv_get_field_length(buffer, 0, 2);
    TEST_ASSERT_EQUAL_INT(0, result);
}

/* Single character field: length should be 1 */
void test_single_char_field_length(void)
{
    build_buffer_from_string(buffer, "a\n");
    /* field "a" has text "a\0" so length stored = 2, returned = 1 */
    int result = csv_get_field_length(buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT(1, result);
}

/* Multi-character field: "hello" -> length 5 */
void test_multi_char_field_length(void)
{
    build_buffer_from_string(buffer, "hello\n");
    int result = csv_get_field_length(buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT(5, result);
}

/* Second field in a row */
void test_second_field_length(void)
{
    build_buffer_from_string(buffer, "hello,world\n");
    int result = csv_get_field_length(buffer, 0, 1);
    TEST_ASSERT_EQUAL_INT(5, result); /* "world" = 5 chars */
}

/* First field in a row with multiple fields */
void test_first_field_length_multi_field_row(void)
{
    build_buffer_from_string(buffer, "hello,world\n");
    int result = csv_get_field_length(buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT(5, result); /* "hello" = 5 chars */
}

/* Second row, first entry */
void test_second_row_first_entry_length(void)
{
    build_buffer_from_string(buffer, "hello,world\nfoo,bar\n");
    int result = csv_get_field_length(buffer, 1, 0);
    TEST_ASSERT_EQUAL_INT(3, result); /* "foo" = 3 chars */
}

/* Second row, second entry */
void test_second_row_second_entry_length(void)
{
    build_buffer_from_string(buffer, "hello,world\nfoo,bar\n");
    int result = csv_get_field_length(buffer, 1, 1);
    TEST_ASSERT_EQUAL_INT(3, result); /* "bar" = 3 chars */
}

/* Row out of bounds on a multi-row buffer */
void test_row_out_of_bounds_multi_row(void)
{
    build_buffer_from_string(buffer, "hello,world\nfoo,bar\n");
    int result = csv_get_field_length(buffer, 2, 0);
    TEST_ASSERT_EQUAL_INT(0, result);
}

/* Entry out of bounds on second row */
void test_entry_out_of_bounds_second_row(void)
{
    build_buffer_from_string(buffer, "hello,world\nfoo,bar\n");
    int result = csv_get_field_length(buffer, 1, 5);
    TEST_ASSERT_EQUAL_INT(0, result);
}

/* Empty buffer: row 0 out of bounds */
void test_empty_buffer_row_zero_returns_zero(void)
{
    /* buffer has 0 rows; any row request should return 0 */
    int result = csv_get_field_length(buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT(0, result);
}

/* Longer field */
void test_long_field_length(void)
{
    build_buffer_from_string(buffer, "abcdefghij\n");
    int result = csv_get_field_length(buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT(10, result); /* "abcdefghij" = 10 chars */
}

/* Field with spaces */
void test_field_with_spaces_length(void)
{
    build_buffer_from_string(buffer, "hello world\n");
    int result = csv_get_field_length(buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT(11, result); /* "hello world" = 11 chars */
}

/* Three fields in a row, check middle field */
void test_middle_field_of_three(void)
{
    build_buffer_from_string(buffer, "one,two,three\n");
    int result = csv_get_field_length(buffer, 0, 1);
    TEST_ASSERT_EQUAL_INT(3, result); /* "two" = 3 chars */
}

/* Three fields in a row, check last field */
void test_last_field_of_three(void)
{
    build_buffer_from_string(buffer, "one,two,three\n");
    int result = csv_get_field_length(buffer, 0, 2);
    TEST_ASSERT_EQUAL_INT(5, result); /* "three" = 5 chars */
}

/* Three rows, check third row */
void test_third_row_entry(void)
{
    build_buffer_from_string(buffer, "one,two\nthree,four\nfive,six\n");
    int result = csv_get_field_length(buffer, 2, 0);
    TEST_ASSERT_EQUAL_INT(4, result); /* "five" = 4 chars */
}

/* Three rows, row index 3 out of bounds */
void test_third_row_out_of_bounds(void)
{
    build_buffer_from_string(buffer, "one,two\nthree,four\nfive,six\n");
    int result = csv_get_field_length(buffer, 3, 0);
    TEST_ASSERT_EQUAL_INT(0, result);
}

/* ------------------------------------------------------------------ */
/* main                                                                 */
/* ------------------------------------------------------------------ */

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_row_out_of_bounds_returns_zero);
    RUN_TEST(test_entry_out_of_bounds_returns_zero);
    RUN_TEST(test_single_char_field_length);
    RUN_TEST(test_multi_char_field_length);
    RUN_TEST(test_second_field_length);
    RUN_TEST(test_first_field_length_multi_field_row);
    RUN_TEST(test_second_row_first_entry_length);
    RUN_TEST(test_second_row_second_entry_length);
    RUN_TEST(test_row_out_of_bounds_multi_row);
    RUN_TEST(test_entry_out_of_bounds_second_row);
    RUN_TEST(test_empty_buffer_row_zero_returns_zero);
    RUN_TEST(test_long_field_length);
    RUN_TEST(test_field_with_spaces_length);
    RUN_TEST(test_middle_field_of_three);
    RUN_TEST(test_last_field_of_three);
    RUN_TEST(test_third_row_entry);
    RUN_TEST(test_third_row_out_of_bounds);
    return UNITY_END();
}