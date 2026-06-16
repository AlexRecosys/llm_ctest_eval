#include "csv.c"
#include "unity.h"
#include <stdlib.h>
#include <string.h>

static CSV_BUFFER *dest_buffer = NULL;
static CSV_BUFFER *source_buffer = NULL;

void setUp(void)
{
    dest_buffer = csv_create_buffer();
    source_buffer = csv_create_buffer();
    TEST_ASSERT_NOT_NULL(dest_buffer);
    TEST_ASSERT_NOT_NULL(source_buffer);
}

void tearDown(void)
{
    csv_destroy_buffer(dest_buffer);
    csv_destroy_buffer(source_buffer);
    dest_buffer = NULL;
    source_buffer = NULL;
}

static void setup_source_with_data(void)
{
    int ret;
    ret = append_row(source_buffer);
    TEST_ASSERT_EQUAL_INT(0, ret);
    ret = append_field(source_buffer, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);
    ret = set_field(source_buffer->field[0][0], "test_value");
    TEST_ASSERT_EQUAL_INT(0, ret);

    ret = append_field(source_buffer, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);
    ret = set_field(source_buffer->field[0][1], "another_value");
    TEST_ASSERT_EQUAL_INT(0, ret);

    ret = append_row(source_buffer);
    TEST_ASSERT_EQUAL_INT(0, ret);
    ret = append_field(source_buffer, 1);
    TEST_ASSERT_EQUAL_INT(0, ret);
    ret = set_field(source_buffer->field[1][0], "row2_col1");
    TEST_ASSERT_EQUAL_INT(0, ret);
}

static void setup_dest_with_rows_and_fields(void)
{
    int ret;
    ret = append_row(dest_buffer);
    TEST_ASSERT_EQUAL_INT(0, ret);
    ret = append_field(dest_buffer, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);
    ret = set_field(dest_buffer->field[0][0], "old_value");
    TEST_ASSERT_EQUAL_INT(0, ret);
}

static void setup_dest_with_multiple_rows_and_fields(void)
{
    int ret;
    ret = append_row(dest_buffer);
    TEST_ASSERT_EQUAL_INT(0, ret);
    ret = append_field(dest_buffer, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);
    ret = set_field(dest_buffer->field[0][0], "dest_old_0_0");
    TEST_ASSERT_EQUAL_INT(0, ret);
    ret = append_field(dest_buffer, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);
    ret = set_field(dest_buffer->field[0][1], "dest_old_0_1");
    TEST_ASSERT_EQUAL_INT(0, ret);

    ret = append_row(dest_buffer);
    TEST_ASSERT_EQUAL_INT(0, ret);
    ret = append_field(dest_buffer, 1);
    TEST_ASSERT_EQUAL_INT(0, ret);
    ret = set_field(dest_buffer->field[1][0], "dest_old_1_0");
    TEST_ASSERT_EQUAL_INT(0, ret);
}

static void assert_field_equals(CSV_BUFFER *buf, int row, int entry, const char *expected)
{
    TEST_ASSERT_NOT_NULL(buf);
    TEST_ASSERT_NOT_NULL(buf->field);
    TEST_ASSERT_NOT_NULL(buf->field[row]);
    TEST_ASSERT_NOT_NULL(buf->field[row][entry]);
    TEST_ASSERT_NOT_NULL(buf->field[row][entry]->text);
    TEST_ASSERT_EQUAL_STRING(expected, buf->field[row][entry]->text);
}

static void assert_field_empty(CSV_BUFFER *buf, int row, int entry)
{
    TEST_ASSERT_NOT_NULL(buf);
    TEST_ASSERT_NOT_NULL(buf->field);
    TEST_ASSERT_NOT_NULL(buf->field[row]);
    TEST_ASSERT_NOT_NULL(buf->field[row][entry]);
    TEST_ASSERT_NULL(buf->field[row][entry]->text);
    TEST_ASSERT_EQUAL_SIZE_T(0, buf->field[row][entry]->length);
}

void test_csv_copy_field_basic_copy(void)
{
    setup_source_with_data();
    setup_dest_with_rows_and_fields();

    int ret = csv_copy_field(dest_buffer, 0, 0, source_buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);
    assert_field_equals(dest_buffer, 0, 0, "test_value");
}

void test_csv_copy_field_copy_to_different_field_in_same_buffer(void)
{
    setup_source_with_data();
    setup_dest_with_rows_and_fields();

    int ret = csv_copy_field(dest_buffer, 0, 1, source_buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);
    assert_field_equals(dest_buffer, 0, 1, "test_value");
}

void test_csv_copy_field_copy_to_different_row(void)
{
    setup_source_with_data();
    setup_dest_with_multiple_rows_and_fields();

    int ret = csv_copy_field(dest_buffer, 1, 0, source_buffer, 0, 1);
    TEST_ASSERT_EQUAL_INT(0, ret);
    assert_field_equals(dest_buffer, 1, 0, "another_value");
}

void test_csv_copy_field_copy_same_buffer(void)
{
    setup_source_with_data();

    int ret = csv_copy_field(source_buffer, 1, 0, source_buffer, 0, 1);
    TEST_ASSERT_EQUAL_INT(0, ret);
    assert_field_equals(source_buffer, 1, 0, "another_value");
}

void test_csv_copy_field_empty_source_field(void)
{
    int ret;
    ret = append_row(source_buffer);
    TEST_ASSERT_EQUAL_INT(0, ret);
    ret = append_field(source_buffer, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);
    ret = set_field(source_buffer->field[0][0], NULL);
    TEST_ASSERT_EQUAL_INT(0, ret);

    ret = append_row(dest_buffer);
    TEST_ASSERT_EQUAL_INT(0, ret);
    ret = append_field(dest_buffer, 1);
    TEST_ASSERT_EQUAL_INT(0, ret);
    ret = set_field(dest_buffer->field[1][0], "old");
    TEST_ASSERT_EQUAL_INT(0, ret);

    ret = csv_copy_field(dest_buffer, 1, 0, source_buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);
    assert_field_empty(dest_buffer, 1, 0);
}

void test_csv_copy_field_long_string(void)
{
    char long_str[1024];
    memset(long_str, 'x', sizeof(long_str) - 1);
    long_str[sizeof(long_str) - 1] = '\0';

    int ret = append_row(source_buffer);
    TEST_ASSERT_EQUAL_INT(0, ret);
    ret = append_field(source_buffer, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);
    ret = set_field(source_buffer->field[0][0], long_str);
    TEST_ASSERT_EQUAL_INT(0, ret);

    ret = append_row(dest_buffer);
    TEST_ASSERT_EQUAL_INT(0, ret);
    ret = append_field(dest_buffer, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);
    ret = set_field(dest_buffer->field[0][0], "old");
    TEST_ASSERT_EQUAL_INT(0, ret);

    ret = csv_copy_field(dest_buffer, 0, 0, source_buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);
    assert_field_equals(dest_buffer, 0, 0, long_str);
}

void test_csv_copy_field_invalid_source_row(void)
{
    setup_dest_with_rows_and_fields();

    int ret = csv_copy_field(dest_buffer, 0, 0, source_buffer, 99, 0);
    TEST_ASSERT_EQUAL_INT(1, ret);
}

void test_csv_copy_field_invalid_source_entry(void)
{
    setup_source_with_data();
    setup_dest_with_rows_and_fields();

    int ret = csv_copy_field(dest_buffer, 0, 0, source_buffer, 0, 99);
    TEST_ASSERT_EQUAL_INT(1, ret);
}

void test_csv_copy_field_invalid_dest_row(void)
{
    setup_source_with_data();

    int ret = csv_copy_field(dest_buffer, 99, 0, source_buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT(1, ret);
}

void test_csv_copy_field_invalid_dest_entry(void)
{
    setup_source_with_data();
    setup_dest_with_rows_and_fields();

    int ret = csv_copy_field(dest_buffer, 0, 99, source_buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT(1, ret);
}

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_csv_copy_field_basic_copy);
    RUN_TEST(test_csv_copy_field_copy_to_different_field_in_same_buffer);
    RUN_TEST(test_csv_copy_field_copy_to_different_row);
    RUN_TEST(test_csv_copy_field_same_buffer);
    RUN_TEST(test_csv_copy_field_empty_source_field);
    RUN_TEST(test_csv_copy_field_long_string);
    RUN_TEST(test_csv_copy_field_invalid_source_row);
    RUN_TEST(test_csv_copy_field_invalid_source_entry);
    RUN_TEST(test_csv_copy_field_invalid_dest_row);
    RUN_TEST(test_csv_copy_field_invalid_dest_entry);
    return UNITY_END();
}