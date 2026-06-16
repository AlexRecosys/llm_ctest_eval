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

static void populate_buffer_with_field(CSV_BUFFER *buf, const char *text)
{
    append_row(buf);
    append_field(buf, 0);
    set_field(buf->field[0][0], (char *)text);
}

void test_csv_get_field_length_returns_zero_for_row_out_of_bounds(void)
{
    /* Buffer has 0 rows, so any row index should return 0 */
    int result = csv_get_field_length(buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT(0, result);
}

void test_csv_get_field_length_returns_zero_for_row_beyond_last(void)
{
    append_row(buffer);
    append_field(buffer, 0);
    set_field(buffer->field[0][0], "hello");

    /* Row 1 does not exist (only row 0) */
    int result = csv_get_field_length(buffer, 1, 0);
    TEST_ASSERT_EQUAL_INT(0, result);
}

void test_csv_get_field_length_returns_zero_for_entry_out_of_bounds(void)
{
    append_row(buffer);
    append_field(buffer, 0);
    set_field(buffer->field[0][0], "hello");

    /* Entry 1 does not exist (only entry 0) */
    int result = csv_get_field_length(buffer, 0, 1);
    TEST_ASSERT_EQUAL_INT(0, result);
}

void test_csv_get_field_length_returns_correct_length_for_simple_string(void)
{
    /* "hello" has length 5, stored as 6 (including '\0'), so result = 6 - 1 = 5 */
    append_row(buffer);
    append_field(buffer, 0);
    set_field(buffer->field[0][0], "hello");

    int result = csv_get_field_length(buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT(5, result);
}

void test_csv_get_field_length_returns_correct_length_for_single_char(void)
{
    /* "a" has length 1, stored as 2 (including '\0'), so result = 2 - 1 = 1 */
    append_row(buffer);
    append_field(buffer, 0);
    set_field(buffer->field[0][0], "a");

    int result = csv_get_field_length(buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT(1, result);
}

void test_csv_get_field_length_returns_correct_length_for_empty_string(void)
{
    /* "" has length 0, stored as 1 (just '\0'), so result = 1 - 1 = 0 */
    append_row(buffer);
    append_field(buffer, 0);
    set_field(buffer->field[0][0], "");

    int result = csv_get_field_length(buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT(0, result);
}

void test_csv_get_field_length_returns_correct_length_for_longer_string(void)
{
    /* "Hello, World!" has length 13 */
    append_row(buffer);
    append_field(buffer, 0);
    set_field(buffer->field[0][0], "Hello, World!");

    int result = csv_get_field_length(buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT(13, result);
}

void test_csv_get_field_length_multiple_rows_correct_row(void)
{
    /* Row 0: "short", Row 1: "longer text" */
    append_row(buffer);
    append_field(buffer, 0);
    set_field(buffer->field[0][0], "short");

    append_row(buffer);
    append_field(buffer, 1);
    set_field(buffer->field[1][0], "longer text");

    int result0 = csv_get_field_length(buffer, 0, 0);
    int result1 = csv_get_field_length(buffer, 1, 0);

    TEST_ASSERT_EQUAL_INT(5, result0);
    TEST_ASSERT_EQUAL_INT(11, result1);
}

void test_csv_get_field_length_multiple_entries_in_row(void)
{
    /* Row 0 has two fields: "abc" and "defgh" */
    append_row(buffer);
    append_field(buffer, 0);
    set_field(buffer->field[0][0], "abc");

    append_field(buffer, 0);
    set_field(buffer->field[0][1], "defgh");

    int result0 = csv_get_field_length(buffer, 0, 0);
    int result1 = csv_get_field_length(buffer, 0, 1);

    TEST_ASSERT_EQUAL_INT(3, result0);
    TEST_ASSERT_EQUAL_INT(5, result1);
}

void test_csv_get_field_length_last_valid_row_index(void)
{
    /* Add 3 rows, check last row (index 2) */
    append_row(buffer);
    append_field(buffer, 0);
    set_field(buffer->field[0][0], "row0");

    append_row(buffer);
    append_field(buffer, 1);
    set_field(buffer->field[1][0], "row1");

    append_row(buffer);
    append_field(buffer, 2);
    set_field(buffer->field[2][0], "row2data");

    int result = csv_get_field_length(buffer, 2, 0);
    TEST_ASSERT_EQUAL_INT(8, result);
}

void test_csv_get_field_length_row_exactly_at_boundary(void)
{
    /* buffer->rows == 1, so row index 0 is valid, row index 1 is not */
    append_row(buffer);
    append_field(buffer, 0);
    set_field(buffer->field[0][0], "test");

    int valid   = csv_get_field_length(buffer, 0, 0);
    int invalid = csv_get_field_length(buffer, 1, 0);

    TEST_ASSERT_EQUAL_INT(4, valid);
    TEST_ASSERT_EQUAL_INT(0, invalid);
}

void test_csv_get_field_length_entry_exactly_at_boundary(void)
{
    /* Row 0 has 2 entries (indices 0 and 1), entry 2 is out of bounds */
    append_row(buffer);
    append_field(buffer, 0);
    set_field(buffer->field[0][0], "one");

    append_field(buffer, 0);
    set_field(buffer->field[0][1], "two");

    int valid   = csv_get_field_length(buffer, 0, 1);
    int invalid = csv_get_field_length(buffer, 0, 2);

    TEST_ASSERT_EQUAL_INT(3, valid);
    TEST_ASSERT_EQUAL_INT(0, invalid);
}

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_csv_get_field_length_returns_zero_for_row_out_of_bounds);
    RUN_TEST(test_csv_get_field_length_returns_zero_for_row_beyond_last);
    RUN_TEST(test_csv_get_field_length_returns_zero_for_entry_out_of_bounds);
    RUN_TEST(test_csv_get_field_length_returns_correct_length_for_simple_string);
    RUN_TEST(test_csv_get_field_length_returns_correct_length_for_single_char);
    RUN_TEST(test_csv_get_field_length_returns_correct_length_for_empty_string);
    RUN_TEST(test_csv_get_field_length_returns_correct_length_for_longer_string);
    RUN_TEST(test_csv_get_field_length_multiple_rows_correct_row);
    RUN_TEST(test_csv_get_field_length_multiple_entries_in_row);
    RUN_TEST(test_csv_get_field_length_last_valid_row_index);
    RUN_TEST(test_csv_get_field_length_row_exactly_at_boundary);
    RUN_TEST(test_csv_get_field_length_entry_exactly_at_boundary);
    return UNITY_END();
}