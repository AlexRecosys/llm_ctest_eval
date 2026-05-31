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
    csv_destroy_buffer(buffer);
    buffer = NULL;
}

static void verify_field(CSV_BUFFER *buf, size_t row, size_t entry, const char *expected)
{
    char dest[256] = {0};
    int result = csv_get_field(dest, sizeof(dest), buf, row, entry);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "csv_get_field should succeed");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(expected, dest, "Field content mismatch");
}

static void verify_width(CSV_BUFFER *buf, size_t row, size_t expected_width)
{
    int actual = csv_get_width(buf, row);
    TEST_ASSERT_EQUAL_INT_MESSAGE((int)expected_width, actual, "Row width mismatch");
}

static void verify_height(CSV_BUFFER *buf, size_t expected_height)
{
    int actual = csv_get_height(buf);
    TEST_ASSERT_EQUAL_INT_MESSAGE((int)expected_height, actual, "Buffer height mismatch");
}

static void init_simple_2x2_buffer(void)
{
    csv_set_field(buffer, 0, 0, "A1");
    csv_set_field(buffer, 0, 1, "B1");
    csv_set_field(buffer, 1, 0, "A2");
    csv_set_field(buffer, 1, 1, "B2");
}

static void init_1x3_buffer(void)
{
    csv_set_field(buffer, 0, 0, "X0");
    csv_set_field(buffer, 0, 1, "X1");
    csv_set_field(buffer, 0, 2, "X2");
}

void test_csv_insert_field_insert_at_end_of_row(void)
{
    init_1x3_buffer();
    verify_width(buffer, 0, 3);

    int result = csv_set_field(buffer, 0, 3, "X3");
    TEST_ASSERT_EQUAL_INT(0, result);

    result = csv_insert_field(buffer, 0, 3, "NEW");
    TEST_ASSERT_EQUAL_INT(0, result);

    verify_width(buffer, 0, 4);
    verify_field(buffer, 0, 0, "X0");
    verify_field(buffer, 0, 1, "X1");
    verify_field(buffer, 0, 2, "X2");
    verify_field(buffer, 0, 3, "NEW");
}

void test_csv_insert_field_insert_in_middle_of_row(void)
{
    init_1x3_buffer();
    verify_width(buffer, 0, 3);

    int result = csv_insert_field(buffer, 0, 1, "INSERTED");
    TEST_ASSERT_EQUAL_INT(0, result);

    verify_width(buffer, 0, 4);
    verify_field(buffer, 0, 0, "X0");
    verify_field(buffer, 0, 1, "INSERTED");
    verify_field(buffer, 0, 2, "X1");
    verify_field(buffer, 0, 3, "X2");
}

void test_csv_insert_field_insert_at_beginning_of_row(void)
{
    init_1x3_buffer();

    int result = csv_insert_field(buffer, 0, 0, "FIRST");
    TEST_ASSERT_EQUAL_INT(0, result);

    verify_width(buffer, 0, 4);
    verify_field(buffer, 0, 0, "FIRST");
    verify_field(buffer, 0, 1, "X0");
    verify_field(buffer, 0, 2, "X1");
    verify_field(buffer, 0, 3, "X2");
}

void test_csv_insert_field_insert_beyond_existing_row_creates_row_and_sets_field(void)
{
    int result = csv_insert_field(buffer, 0, 0, "A1");
    TEST_ASSERT_EQUAL_INT(0, result);

    result = csv_insert_field(buffer, 1, 0, "A2");
    TEST_ASSERT_EQUAL_INT(0, result);

    verify_height(buffer, 2);
    verify_field(buffer, 0, 0, "A1");
    verify_field(buffer, 1, 0, "A2");
}

void test_csv_insert_field_insert_beyond_existing_entry_in_existing_row_calls_set_field(void)
{
    csv_set_field(buffer, 0, 0, "A1");
    csv_set_field(buffer, 0, 1, "B1");

    int result = csv_insert_field(buffer, 0, 3, "D1");
    TEST_ASSERT_EQUAL_INT(0, result);

    verify_width(buffer, 0, 4);
    verify_field(buffer, 0, 0, "A1");
    verify_field(buffer, 0, 1, "B1");
    verify_field(buffer, 0, 2, "");
    verify_field(buffer, 0, 3, "D1");
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_csv_insert_field_insert_at_end_of_row);
    RUN_TEST(test_csv_insert_field_insert_in_middle_of_row);
    RUN_TEST(test_csv_insert_field_insert_at_beginning_of_row);
    RUN_TEST(test_csv_insert_field_insert_beyond_existing_row_creates_row_and_sets_field);
    RUN_TEST(test_csv_insert_field_insert_beyond_existing_entry_in_existing_row_calls_set_field);
    return UNITY_END();
}