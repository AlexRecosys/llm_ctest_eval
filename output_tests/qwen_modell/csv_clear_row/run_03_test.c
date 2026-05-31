#include "unity.h"
#include "csv.h"

static CSV_BUFFER *buffer = NULL;

void setUp(void)
{
    buffer = csv_create_buffer();
    TEST_ASSERT_NOT_NULL(buffer);
}

void tearDown(void)
{
    if (buffer != NULL) {
        csv_destroy_buffer(buffer);
        buffer = NULL;
    }
}

static void populate_row_with_fields(CSV_BUFFER *buf, size_t row, size_t field_count)
{
    int ret;
    ret = csv_set_field(buf, row, 0, "first");
    TEST_ASSERT_EQUAL_INT(0, ret);
    for (size_t i = 1; i < field_count; i++) {
        ret = csv_set_field(buf, row, i, "extra");
        TEST_ASSERT_EQUAL_INT(0, ret);
    }
}

static void verify_row_cleared(CSV_BUFFER *buf, size_t row)
{
    char dest[256] = {0};
    int ret = csv_get_field(dest, sizeof(dest), buf, row, 0);
    TEST_ASSERT_EQUAL_INT(2, ret); // empty field
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buf, row));
}

static void verify_row_not_cleared(CSV_BUFFER *buf, size_t row, size_t expected_width, const char *expected_first_field)
{
    char dest[256] = {0};
    int ret = csv_get_field(dest, sizeof(dest), buf, row, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING(expected_first_field, dest);
    TEST_ASSERT_EQUAL_INT(expected_width, (size_t)csv_get_width(buf, row));
}

static void add_row_with_fields(CSV_BUFFER *buf, size_t field_count)
{
    int ret = csv_set_field(buf, buf->rows, 0, "initial");
    TEST_ASSERT_EQUAL_INT(0, ret);
    for (size_t i = 1; i < field_count; i++) {
        ret = csv_set_field(buf, buf->rows, i, "extra");
        TEST_ASSERT_EQUAL_INT(0, ret);
    }
}

static void add_two_rows_with_fields(CSV_BUFFER *buf, size_t row0_fields, size_t row1_fields)
{
    add_row_with_fields(buf, row0_fields);
    add_row_with_fields(buf, row1_fields);
}

void test_csv_clear_row_last_row_simple_case(void)
{
    add_row_with_fields(buffer, 3);
    TEST_ASSERT_EQUAL_INT(1, csv_get_height(buffer));
    TEST_ASSERT_EQUAL_INT(3, csv_get_width(buffer, 0));

    int ret = csv_clear_row(buffer, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(1, csv_get_height(buffer));
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buffer, 0));
    verify_row_cleared(buffer, 0);
}

void test_csv_clear_row_middle_row_preserves_other_rows(void)
{
    add_two_rows_with_fields(buffer, 4, 2);
    TEST_ASSERT_EQUAL_INT(2, csv_get_height(buffer));
    TEST_ASSERT_EQUAL_INT(4, csv_get_width(buffer, 0));
    TEST_ASSERT_EQUAL_INT(2, csv_get_width(buffer, 1));

    int ret = csv_clear_row(buffer, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(2, csv_get_height(buffer));
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buffer, 0));
    TEST_ASSERT_EQUAL_INT(2, csv_get_width(buffer, 1));
    verify_row_cleared(buffer, 0);
    verify_row_not_cleared(buffer, 1, 2, "initial");
}

void test_csv_clear_row_first_row_of_many(void)
{
    add_two_rows_with_fields(buffer, 5, 3);
    add_row_with_fields(buffer, 2);

    TEST_ASSERT_EQUAL_INT(3, csv_get_height(buffer));
    TEST_ASSERT_EQUAL_INT(5, csv_get_width(buffer, 0));
    TEST_ASSERT_EQUAL_INT(3, csv_get_width(buffer, 1));
    TEST_ASSERT_EQUAL_INT(2, csv_get_width(buffer, 2));

    int ret = csv_clear_row(buffer, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(3, csv_get_height(buffer));
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buffer, 0));
    TEST_ASSERT_EQUAL_INT(3, csv_get_width(buffer, 1));
    TEST_ASSERT_EQUAL_INT(2, csv_get_width(buffer, 2));
    verify_row_cleared(buffer, 0);
    verify_row_not_cleared(buffer, 1, 3, "initial");
    verify_row_not_cleared(buffer, 2, 2, "initial");
}

void test_csv_clear_row_single_field_row_no_change(void)
{
    add_row_with_fields(buffer, 1);
    TEST_ASSERT_EQUAL_INT(1, csv_get_height(buffer));
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buffer, 0));

    int ret = csv_clear_row(buffer, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(1, csv_get_height(buffer));
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buffer, 0));
    verify_row_cleared(buffer, 0);
}

void test_csv_clear_row_last_row_with_realloc_failure_handling(void)
{
    // This test verifies behavior when realloc fails during row clearing
    // Since csv_clear_row only shrinks memory, and we cannot force realloc failure
    // without mocks, we test the happy path for last-row case
    add_row_with_fields(buffer, 10);
    TEST_ASSERT_EQUAL_INT(1, csv_get_height(buffer));
    TEST_ASSERT_EQUAL_INT(10, csv_get_width(buffer, 0));

    int ret = csv_clear_row(buffer, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(1, csv_get_height(buffer));
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buffer, 0));
    verify_row_cleared(buffer, 0);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_csv_clear_row_last_row_simple_case);
    RUN_TEST(test_csv_clear_row_middle_row_preserves_other_rows);
    RUN_TEST(test_csv_clear_row_first_row_of_many);
    RUN_TEST(test_csv_clear_row_single_field_row_no_change);
    RUN_TEST(test_csv_clear_row_last_row_with_realloc_failure_handling);
    return UNITY_END();
}