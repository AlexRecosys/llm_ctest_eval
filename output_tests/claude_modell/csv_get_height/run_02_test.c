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

static void helper_load_rows(CSV_BUFFER *buf, int num_rows)
{
    int i;
    for (i = 0; i < num_rows; i++) {
        append_row(buf);
    }
}

void test_csv_get_height_returns_zero_for_empty_buffer(void)
{
    int height = csv_get_height(buffer);
    TEST_ASSERT_EQUAL_INT(0, height);
}

void test_csv_get_height_returns_one_after_one_row_appended(void)
{
    append_row(buffer);
    int height = csv_get_height(buffer);
    TEST_ASSERT_EQUAL_INT(1, height);
}

void test_csv_get_height_returns_two_after_two_rows_appended(void)
{
    append_row(buffer);
    append_row(buffer);
    int height = csv_get_height(buffer);
    TEST_ASSERT_EQUAL_INT(2, height);
}

void test_csv_get_height_returns_five_after_five_rows_appended(void)
{
    helper_load_rows(buffer, 5);
    int height = csv_get_height(buffer);
    TEST_ASSERT_EQUAL_INT(5, height);
}

void test_csv_get_height_returns_ten_after_ten_rows_appended(void)
{
    helper_load_rows(buffer, 10);
    int height = csv_get_height(buffer);
    TEST_ASSERT_EQUAL_INT(10, height);
}

void test_csv_get_height_decrements_after_remove_last_row(void)
{
    append_row(buffer);
    append_row(buffer);
    append_row(buffer);
    remove_last_row(buffer);
    int height = csv_get_height(buffer);
    TEST_ASSERT_EQUAL_INT(2, height);
}

void test_csv_get_height_returns_zero_after_all_rows_removed(void)
{
    append_row(buffer);
    append_row(buffer);
    remove_last_row(buffer);
    remove_last_row(buffer);
    int height = csv_get_height(buffer);
    TEST_ASSERT_EQUAL_INT(0, height);
}

void test_csv_get_height_reflects_rows_field_directly(void)
{
    buffer->rows = 42;
    int height = csv_get_height(buffer);
    TEST_ASSERT_EQUAL_INT(42, height);
}

void test_csv_get_height_after_single_append_and_remove(void)
{
    append_row(buffer);
    remove_last_row(buffer);
    int height = csv_get_height(buffer);
    TEST_ASSERT_EQUAL_INT(0, height);
}

void test_csv_get_height_multiple_appends_and_removes(void)
{
    helper_load_rows(buffer, 7);
    remove_last_row(buffer);
    remove_last_row(buffer);
    int height = csv_get_height(buffer);
    TEST_ASSERT_EQUAL_INT(5, height);
}

void test_csv_get_height_with_fields_added_does_not_change_height(void)
{
    append_row(buffer);
    append_field(buffer, 0);
    append_field(buffer, 0);
    int height = csv_get_height(buffer);
    TEST_ASSERT_EQUAL_INT(1, height);
}

void test_csv_get_height_two_buffers_independent(void)
{
    CSV_BUFFER *buffer2 = csv_create_buffer();
    helper_load_rows(buffer, 3);
    helper_load_rows(buffer2, 7);

    TEST_ASSERT_EQUAL_INT(3, csv_get_height(buffer));
    TEST_ASSERT_EQUAL_INT(7, csv_get_height(buffer2));

    csv_destroy_buffer(buffer2);
}

void test_csv_get_height_rows_field_zero_initially(void)
{
    TEST_ASSERT_EQUAL_INT(0, (int)buffer->rows);
    TEST_ASSERT_EQUAL_INT(0, csv_get_height(buffer));
}

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_csv_get_height_returns_zero_for_empty_buffer);
    RUN_TEST(test_csv_get_height_returns_one_after_one_row_appended);
    RUN_TEST(test_csv_get_height_returns_two_after_two_rows_appended);
    RUN_TEST(test_csv_get_height_returns_five_after_five_rows_appended);
    RUN_TEST(test_csv_get_height_returns_ten_after_ten_rows_appended);
    RUN_TEST(test_csv_get_height_decrements_after_remove_last_row);
    RUN_TEST(test_csv_get_height_returns_zero_after_all_rows_removed);
    RUN_TEST(test_csv_get_height_reflects_rows_field_directly);
    RUN_TEST(test_csv_get_height_after_single_append_and_remove);
    RUN_TEST(test_csv_get_height_multiple_appends_and_removes);
    RUN_TEST(test_csv_get_height_with_fields_added_does_not_change_height);
    RUN_TEST(test_csv_get_height_two_buffers_independent);
    RUN_TEST(test_csv_get_height_rows_field_zero_initially);
    return UNITY_END();
}