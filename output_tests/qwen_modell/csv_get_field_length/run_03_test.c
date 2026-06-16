#include "csv.c"
#include "unity.h"
#include <stdlib.h>
#include <string.h>

static CSV_BUFFER *buffer = NULL;

void setUp(void)
{
    buffer = csv_create_buffer();
    TEST_ASSERT_NOT_NULL(buffer);
}

void tearDown(void)
{
    csv_destroy_buffer(buffer);
    buffer = NULL;
}

static void setup_test_row_with_fields(size_t row, size_t num_fields)
{
    int ret;
    ret = append_row(buffer);
    TEST_ASSERT_EQUAL_INT(0, ret);

    for (size_t i = 0; i < num_fields; i++) {
        ret = append_field(buffer, row);
        TEST_ASSERT_EQUAL_INT(0, ret);
    }
}

static void set_field_text(size_t row, size_t entry, const char *text)
{
    int ret = csv_set_field(buffer, row, entry, (char *)text);
    TEST_ASSERT_EQUAL_INT(0, ret);
}

void test_csv_get_field_length_valid_field_nonempty(void)
{
    setup_test_row_with_fields(0, 1);
    set_field_text(0, 0, "hello");

    int len = csv_get_field_length(buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT(4, len);  // "hello" has length 5, minus 1 = 4
}

void test_csv_get_field_length_valid_field_empty(void)
{
    setup_test_row_with_fields(0, 1);
    set_field_text(0, 0, "");

    int len = csv_get_field_length(buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT(-1, len);  // empty string length 0, minus 1 = -1
}

void test_csv_get_field_length_row_out_of_bounds(void)
{
    setup_test_row_with_fields(0, 1);

    int len = csv_get_field_length(buffer, 1, 0);
    TEST_ASSERT_EQUAL_INT(0, len);
}

void test_csv_get_field_length_entry_out_of_bounds(void)
{
    setup_test_row_with_fields(0, 2);

    int len = csv_get_field_length(buffer, 0, 2);
    TEST_ASSERT_EQUAL_INT(0, len);
}

void test_csv_get_field_length_multiple_rows_and_entries(void)
{
    setup_test_row_with_fields(0, 3);
    setup_test_row_with_fields(1, 2);

    set_field_text(0, 0, "a");
    set_field_text(0, 1, "bb");
    set_field_text(0, 2, "ccc");
    set_field_text(1, 0, "dddd");
    set_field_text(1, 1, "eeeee");

    TEST_ASSERT_EQUAL_INT(0, csv_get_field_length(buffer, 0, 0));  // "a" -> 1-1=0
    TEST_ASSERT_EQUAL_INT(1, csv_get_field_length(buffer, 0, 1));  // "bb" -> 2-1=1
    TEST_ASSERT_EQUAL_INT(2, csv_get_field_length(buffer, 0, 2));  // "ccc" -> 3-1=2
    TEST_ASSERT_EQUAL_INT(3, csv_get_field_length(buffer, 1, 0));  // "dddd" -> 4-1=3
    TEST_ASSERT_EQUAL_INT(4, csv_get_field_length(buffer, 1, 1));  // "eeeee" -> 5-1=4
}

void test_csv_get_field_length_negative_length_for_empty_string(void)
{
    setup_test_row_with_fields(0, 1);
    set_field_text(0, 0, "");

    int len = csv_get_field_length(buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT(-1, len);
}

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_csv_get_field_length_valid_field_nonempty);
    RUN_TEST(test_csv_get_field_length_valid_field_empty);
    RUN_TEST(test_csv_get_field_length_row_out_of_bounds);
    RUN_TEST(test_csv_get_field_length_entry_out_of_bounds);
    RUN_TEST(test_csv_get_field_length_multiple_rows_and_entries);
    RUN_TEST(test_csv_get_field_length_negative_length_for_empty_string);
    return UNITY_END();
}