#include "csv.c"
#include "unity.h"

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
        ret = set_field(buffer->field[row][i], "field");
        TEST_ASSERT_EQUAL_INT(0, ret);
    }
}

static void assert_field_equals(size_t row, size_t entry, const char *expected)
{
    char buf[64] = {0};
    int ret = csv_get_field(buf, sizeof(buf), buffer, row, entry);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING(expected, buf);
}

static void assert_row_width(size_t row, size_t expected_width)
{
    size_t width = csv_get_width(buffer, row);
    TEST_ASSERT_EQUAL_INT_MESSAGE((int)expected_width, (int)width,
                                  "Row width mismatch");
}

static void assert_buffer_height(size_t expected_height)
{
    int height = csv_get_height(buffer);
    TEST_ASSERT_EQUAL_INT_MESSAGE((int)expected_height, height,
                                  "Buffer height mismatch");
}

void test_csv_remove_field_valid_middle_field(void)
{
    setup_test_row_with_fields(0, 5);
    assert_row_width(0, 5);

    int ret = csv_remove_field(buffer, 0, 2);
    TEST_ASSERT_EQUAL_INT(0, ret);
    assert_row_width(0, 4);

    assert_field_equals(0, 0, "field");
    assert_field_equals(0, 1, "field");
    assert_field_equals(0, 2, "field");
    assert_field_equals(0, 3, "field");
}

void test_csv_remove_field_first_field(void)
{
    setup_test_row_with_fields(0, 3);
    assert_row_width(0, 3);

    int ret = csv_remove_field(buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);
    assert_row_width(0, 2);

    assert_field_equals(0, 0, "field");
    assert_field_equals(0, 1, "field");
}

void test_csv_remove_field_last_field(void)
{
    setup_test_row_with_fields(0, 3);
    assert_row_width(0, 3);

    int ret = csv_remove_field(buffer, 0, 2);
    TEST_ASSERT_EQUAL_INT(0, ret);
    assert_row_width(0, 2);

    assert_field_equals(0, 0, "field");
    assert_field_equals(0, 1, "field");
}

void test_csv_remove_field_single_field_row(void)
{
    setup_test_row_with_fields(0, 1);
    assert_row_width(0, 1);

    int ret = csv_remove_field(buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);
    assert_row_width(0, 0);
}

void test_csv_remove_field_invalid_row(void)
{
    setup_test_row_with_fields(0, 3);
    int ret = csv_remove_field(buffer, 1, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);
    assert_buffer_height(1);
}

void test_csv_remove_field_invalid_entry(void)
{
    setup_test_row_with_fields(0, 2);
    int ret = csv_remove_field(buffer, 0, 5);
    TEST_ASSERT_EQUAL_INT(0, ret);
    assert_row_width(0, 2);
}

void test_csv_remove_field_multiple_rows(void)
{
    setup_test_row_with_fields(0, 4);
    setup_test_row_with_fields(1, 3);

    int ret = csv_remove_field(buffer, 0, 1);
    TEST_ASSERT_EQUAL_INT(0, ret);
    assert_row_width(0, 3);
    assert_row_width(1, 3);

    ret = csv_remove_field(buffer, 1, 2);
    TEST_ASSERT_EQUAL_INT(0, ret);
    assert_row_width(0, 3);
    assert_row_width(1, 2);
}

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_csv_remove_field_valid_middle_field);
    RUN_TEST(test_csv_remove_field_first_field);
    RUN_TEST(test_csv_remove_field_last_field);
    RUN_TEST(test_csv_remove_field_single_field_row);
    RUN_TEST(test_csv_remove_field_invalid_row);
    RUN_TEST(test_csv_remove_field_invalid_entry);
    RUN_TEST(test_csv_remove_field_multiple_rows);
    return UNITY_END();
}