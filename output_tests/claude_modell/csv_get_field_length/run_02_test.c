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

static void populate_buffer_with_data(CSV_BUFFER *buf, const char *file_name)
{
    csv_load(buf, (char *)file_name);
}

static CSV_BUFFER *build_simple_buffer(void)
{
    CSV_BUFFER *buf = csv_create_buffer();
    if (buf == NULL)
        return NULL;

    /* Use csv_set_field to populate rows and entries.
     * csv_set_field will create rows/entries as needed. */
    csv_set_field(buf, 0, 0, "hello");
    csv_set_field(buf, 0, 1, "world");
    csv_set_field(buf, 1, 0, "foo");
    csv_set_field(buf, 1, 1, "bar");
    csv_set_field(buf, 1, 2, "baz");

    return buf;
}

/* Test: valid row and entry returns correct length */
void test_csv_get_field_length_valid_entry_returns_correct_length(void)
{
    CSV_BUFFER *buf = build_simple_buffer();
    TEST_ASSERT_NOT_NULL(buf);

    /* "hello" has length 5, stored length = 6 (including '\0'), so result = 5 */
    int len = csv_get_field_length(buf, 0, 0);
    TEST_ASSERT_EQUAL_INT(5, len);

    csv_destroy_buffer(buf);
}

/* Test: second entry in first row */
void test_csv_get_field_length_second_entry_first_row(void)
{
    CSV_BUFFER *buf = build_simple_buffer();
    TEST_ASSERT_NOT_NULL(buf);

    /* "world" has length 5 */
    int len = csv_get_field_length(buf, 0, 1);
    TEST_ASSERT_EQUAL_INT(5, len);

    csv_destroy_buffer(buf);
}

/* Test: entry in second row */
void test_csv_get_field_length_entry_in_second_row(void)
{
    CSV_BUFFER *buf = build_simple_buffer();
    TEST_ASSERT_NOT_NULL(buf);

    /* "foo" has length 3 */
    int len = csv_get_field_length(buf, 1, 0);
    TEST_ASSERT_EQUAL_INT(3, len);

    csv_destroy_buffer(buf);
}

/* Test: last entry in second row */
void test_csv_get_field_length_last_entry_second_row(void)
{
    CSV_BUFFER *buf = build_simple_buffer();
    TEST_ASSERT_NOT_NULL(buf);

    /* "baz" has length 3 */
    int len = csv_get_field_length(buf, 1, 2);
    TEST_ASSERT_EQUAL_INT(3, len);

    csv_destroy_buffer(buf);
}

/* Test: row index out of bounds returns 0 */
void test_csv_get_field_length_row_out_of_bounds_returns_zero(void)
{
    CSV_BUFFER *buf = build_simple_buffer();
    TEST_ASSERT_NOT_NULL(buf);

    /* buf has 2 rows (0 and 1), so row 2 is out of bounds */
    int len = csv_get_field_length(buf, 2, 0);
    TEST_ASSERT_EQUAL_INT(0, len);

    csv_destroy_buffer(buf);
}

/* Test: entry index out of bounds returns 0 */
void test_csv_get_field_length_entry_out_of_bounds_returns_zero(void)
{
    CSV_BUFFER *buf = build_simple_buffer();
    TEST_ASSERT_NOT_NULL(buf);

    /* row 0 has 2 entries (0 and 1), so entry 2 is out of bounds */
    int len = csv_get_field_length(buf, 0, 2);
    TEST_ASSERT_EQUAL_INT(0, len);

    csv_destroy_buffer(buf);
}

/* Test: large row index out of bounds returns 0 */
void test_csv_get_field_length_large_row_index_returns_zero(void)
{
    CSV_BUFFER *buf = build_simple_buffer();
    TEST_ASSERT_NOT_NULL(buf);

    int len = csv_get_field_length(buf, 999, 0);
    TEST_ASSERT_EQUAL_INT(0, len);

    csv_destroy_buffer(buf);
}

/* Test: large entry index out of bounds returns 0 */
void test_csv_get_field_length_large_entry_index_returns_zero(void)
{
    CSV_BUFFER *buf = build_simple_buffer();
    TEST_ASSERT_NOT_NULL(buf);

    int len = csv_get_field_length(buf, 0, 999);
    TEST_ASSERT_EQUAL_INT(0, len);

    csv_destroy_buffer(buf);
}

/* Test: single character field */
void test_csv_get_field_length_single_char_field(void)
{
    CSV_BUFFER *buf = csv_create_buffer();
    TEST_ASSERT_NOT_NULL(buf);

    csv_set_field(buf, 0, 0, "A");

    int len = csv_get_field_length(buf, 0, 0);
    TEST_ASSERT_EQUAL_INT(1, len);

    csv_destroy_buffer(buf);
}

/* Test: empty string field */
void test_csv_get_field_length_empty_string_field(void)
{
    CSV_BUFFER *buf = csv_create_buffer();
    TEST_ASSERT_NOT_NULL(buf);

    csv_set_field(buf, 0, 0, "");

    /* empty string: length stored = 1 (just '\0'), result = 0 */
    int len = csv_get_field_length(buf, 0, 0);
    TEST_ASSERT_EQUAL_INT(0, len);

    csv_destroy_buffer(buf);
}

/* Test: longer string field */
void test_csv_get_field_length_longer_string(void)
{
    CSV_BUFFER *buf = csv_create_buffer();
    TEST_ASSERT_NOT_NULL(buf);

    csv_set_field(buf, 0, 0, "abcdefghij");

    int len = csv_get_field_length(buf, 0, 0);
    TEST_ASSERT_EQUAL_INT(10, len);

    csv_destroy_buffer(buf);
}

/* Test: first valid entry in a buffer with one row and one entry */
void test_csv_get_field_length_single_row_single_entry(void)
{
    CSV_BUFFER *buf = csv_create_buffer();
    TEST_ASSERT_NOT_NULL(buf);

    csv_set_field(buf, 0, 0, "test");

    int len = csv_get_field_length(buf, 0, 0);
    TEST_ASSERT_EQUAL_INT(4, len);

    /* row 1 does not exist */
    int len2 = csv_get_field_length(buf, 1, 0);
    TEST_ASSERT_EQUAL_INT(0, len2);

    /* entry 1 does not exist in row 0 */
    int len3 = csv_get_field_length(buf, 0, 1);
    TEST_ASSERT_EQUAL_INT(0, len3);

    csv_destroy_buffer(buf);
}

/* Test: boundary — last valid row and last valid entry */
void test_csv_get_field_length_boundary_last_valid_row_and_entry(void)
{
    CSV_BUFFER *buf = build_simple_buffer();
    TEST_ASSERT_NOT_NULL(buf);

    /* Last valid row is 1, last valid entry in row 1 is 2 ("baz") */
    int len = csv_get_field_length(buf, 1, 2);
    TEST_ASSERT_EQUAL_INT(3, len);

    /* One past last valid row */
    int len2 = csv_get_field_length(buf, 2, 2);
    TEST_ASSERT_EQUAL_INT(0, len2);

    /* One past last valid entry in row 1 */
    int len3 = csv_get_field_length(buf, 1, 3);
    TEST_ASSERT_EQUAL_INT(0, len3);

    csv_destroy_buffer(buf);
}

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_csv_get_field_length_valid_entry_returns_correct_length);
    RUN_TEST(test_csv_get_field_length_second_entry_first_row);
    RUN_TEST(test_csv_get_field_length_entry_in_second_row);
    RUN_TEST(test_csv_get_field_length_last_entry_second_row);
    RUN_TEST(test_csv_get_field_length_row_out_of_bounds_returns_zero);
    RUN_TEST(test_csv_get_field_length_entry_out_of_bounds_returns_zero);
    RUN_TEST(test_csv_get_field_length_large_row_index_returns_zero);
    RUN_TEST(test_csv_get_field_length_large_entry_index_returns_zero);
    RUN_TEST(test_csv_get_field_length_single_char_field);
    RUN_TEST(test_csv_get_field_length_empty_string_field);
    RUN_TEST(test_csv_get_field_length_longer_string);
    RUN_TEST(test_csv_get_field_length_single_row_single_entry);
    RUN_TEST(test_csv_get_field_length_boundary_last_valid_row_and_entry);
    return UNITY_END();
}