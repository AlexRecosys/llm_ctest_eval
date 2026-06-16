#include "csv.c"
#include "unity.h"

static CSV_BUFFER *buffer = NULL;

void setUp(void) {
    buffer = csv_create_buffer();
    TEST_ASSERT_NOT_NULL(buffer);
}

void tearDown(void) {
    csv_destroy_buffer(buffer);
    buffer = NULL;
}

void test_csv_get_height_returns_zero_for_new_buffer(void) {
    TEST_ASSERT_EQUAL_INT(0, csv_get_height(buffer));
}

void test_csv_get_height_returns_correct_height_after_adding_rows(void) {
    int result;

    result = append_row(buffer);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_INT(1, csv_get_height(buffer));

    result = append_row(buffer);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_INT(2, csv_get_height(buffer));

    result = append_row(buffer);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_INT(3, csv_get_height(buffer));
}

void test_csv_get_height_returns_correct_height_after_removing_rows(void) {
    int result;

    result = append_row(buffer);
    TEST_ASSERT_EQUAL_INT(0, result);
    result = append_row(buffer);
    TEST_ASSERT_EQUAL_INT(0, result);
    result = append_row(buffer);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_INT(3, csv_get_height(buffer));

    result = remove_last_row(buffer);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_INT(2, csv_get_height(buffer));

    result = remove_last_row(buffer);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_INT(1, csv_get_height(buffer));

    result = remove_last_row(buffer);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_INT(0, csv_get_height(buffer));
}

int main(void) {
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_csv_get_height_returns_zero_for_new_buffer);
    RUN_TEST(test_csv_get_height_returns_correct_height_after_adding_rows);
    RUN_TEST(test_csv_get_height_returns_correct_height_after_removing_rows);
    return UNITY_END();
}