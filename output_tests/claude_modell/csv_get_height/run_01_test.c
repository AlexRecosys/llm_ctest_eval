#include "csv.c"
#include "unity.h"

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

static void helper_set_rows(CSV_BUFFER *buf, size_t rows)
{
    buf->rows = rows;
}

void test_csv_get_height_returns_zero_for_new_buffer(void)
{
    TEST_ASSERT_NOT_NULL(buffer);
    TEST_ASSERT_EQUAL_INT(0, csv_get_height(buffer));
}

void test_csv_get_height_returns_one_after_one_row_loaded(void)
{
    TEST_ASSERT_NOT_NULL(buffer);
    helper_set_rows(buffer, 1);
    TEST_ASSERT_EQUAL_INT(1, csv_get_height(buffer));
}

void test_csv_get_height_returns_five_after_five_rows(void)
{
    TEST_ASSERT_NOT_NULL(buffer);
    helper_set_rows(buffer, 5);
    TEST_ASSERT_EQUAL_INT(5, csv_get_height(buffer));
}

void test_csv_get_height_returns_large_value(void)
{
    TEST_ASSERT_NOT_NULL(buffer);
    helper_set_rows(buffer, 1000);
    TEST_ASSERT_EQUAL_INT(1000, csv_get_height(buffer));
}

void test_csv_get_height_reflects_rows_field_directly(void)
{
    TEST_ASSERT_NOT_NULL(buffer);
    buffer->rows = 42;
    TEST_ASSERT_EQUAL_INT(42, csv_get_height(buffer));
}

void test_csv_get_height_after_append_row_once(void)
{
    TEST_ASSERT_NOT_NULL(buffer);
    append_row(buffer);
    TEST_ASSERT_EQUAL_INT(1, csv_get_height(buffer));
}

void test_csv_get_height_after_append_row_multiple_times(void)
{
    TEST_ASSERT_NOT_NULL(buffer);
    append_row(buffer);
    append_row(buffer);
    append_row(buffer);
    TEST_ASSERT_EQUAL_INT(3, csv_get_height(buffer));
}

void test_csv_get_height_after_append_then_remove_row(void)
{
    TEST_ASSERT_NOT_NULL(buffer);
    append_row(buffer);
    append_row(buffer);
    remove_last_row(buffer);
    TEST_ASSERT_EQUAL_INT(1, csv_get_height(buffer));
}

void test_csv_get_height_after_remove_all_rows(void)
{
    TEST_ASSERT_NOT_NULL(buffer);
    append_row(buffer);
    remove_last_row(buffer);
    TEST_ASSERT_EQUAL_INT(0, csv_get_height(buffer));
}

void test_csv_get_height_two_separate_buffers_independent(void)
{
    CSV_BUFFER *buffer2 = csv_create_buffer();
    TEST_ASSERT_NOT_NULL(buffer);
    TEST_ASSERT_NOT_NULL(buffer2);

    append_row(buffer);
    append_row(buffer);

    append_row(buffer2);

    TEST_ASSERT_EQUAL_INT(2, csv_get_height(buffer));
    TEST_ASSERT_EQUAL_INT(1, csv_get_height(buffer2));

    csv_destroy_buffer(buffer2);
}

void test_csv_get_height_returns_zero_rows_field_zero(void)
{
    TEST_ASSERT_NOT_NULL(buffer);
    buffer->rows = 0;
    TEST_ASSERT_EQUAL_INT(0, csv_get_height(buffer));
}

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_csv_get_height_returns_zero_for_new_buffer);
    RUN_TEST(test_csv_get_height_returns_one_after_one_row_loaded);
    RUN_TEST(test_csv_get_height_returns_five_after_five_rows);
    RUN_TEST(test_csv_get_height_returns_large_value);
    RUN_TEST(test_csv_get_height_reflects_rows_field_directly);
    RUN_TEST(test_csv_get_height_after_append_row_once);
    RUN_TEST(test_csv_get_height_after_append_row_multiple_times);
    RUN_TEST(test_csv_get_height_after_append_then_remove_row);
    RUN_TEST(test_csv_get_height_after_remove_all_rows);
    RUN_TEST(test_csv_get_height_two_separate_buffers_independent);
    RUN_TEST(test_csv_get_height_returns_zero_rows_field_zero);
    return UNITY_END();
}