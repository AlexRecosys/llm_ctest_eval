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
    int width = csv_get_width(buf, row);
    TEST_ASSERT_EQUAL_INT_MESSAGE((int)expected_width, width, "Row width mismatch");
}

static void verify_height(CSV_BUFFER *buf, size_t expected_height)
{
    int height = csv_get_height(buf);
    TEST_ASSERT_EQUAL_INT_MESSAGE((int)expected_height, height, "Buffer height mismatch");
}

static void setup_initial_row_with_fields(CSV_BUFFER *buf, size_t num_fields)
{
    csv_clear_row(buf, 0);
    for (size_t i = 0; i < num_fields; i++) {
        char field_text[32];
        snprintf(field_text, sizeof(field_text), "field_%zu", i);
        csv_set_field(buf, 0, i, field_text);
    }
}

void test_csv_insert_field_insert_at_end_of_row(void)
{
    setup_initial_row_with_fields(buffer, 3);
    verify_width(buffer, 0, 3);

    int result = csv_set_field(buffer, 0, 3, "new_field");
    TEST_ASSERT_EQUAL_INT(0, result);

    verify_field(buffer, 0, 3, "new_field");
    verify_width(buffer, 0, 4);
}

void test_csv_insert_field_insert_beyond_end_creates_new_field(void)
{
    setup_initial_row_with_fields(buffer, 2);
    verify_width(buffer, 0, 2);

    int result = csv_set_field(buffer, 0, 5, "new_field");
    TEST_ASSERT_EQUAL_INT(0, result);

    verify_field(buffer, 0, 5, "new_field");
    verify_width(buffer, 0, 6);
}

void test_csv_insert_field_insert_in_middle_shifts_fields(void)
{
    setup_initial_row_with_fields(buffer, 3);
    verify_width(buffer, 0, 3);

    int result = csv_set_field(buffer, 0, 1, "inserted");
    TEST_ASSERT_EQUAL_INT(0, result);

    verify_field(buffer, 0, 0, "field_0");
    verify_field(buffer, 0, 1, "inserted");
    verify_field(buffer, 0, 2, "field_1");
    verify_field(buffer, 0, 3, "field_2");
    verify_width(buffer, 0, 4);
}

void test_csv_insert_field_insert_at_start_shifts_all_fields(void)
{
    setup_initial_row_with_fields(buffer, 3);
    verify_width(buffer, 0, 3);

    int result = csv_set_field(buffer, 0, 0, "first");
    TEST_ASSERT_EQUAL_INT(0, result);

    verify_field(buffer, 0, 0, "first");
    verify_field(buffer, 0, 1, "field_0");
    verify_field(buffer, 0, 2, "field_1");
    verify_field(buffer, 0, 3, "field_2");
    verify_width(buffer, 0, 4);
}

void test_csv_insert_field_creates_new_row_if_needed(void)
{
    verify_height(buffer, 0);

    int result = csv_set_field(buffer, 2, 0, "new_row_field");
    TEST_ASSERT_EQUAL_INT(0, result);

    verify_height(buffer, 3);
    verify_width(buffer, 2, 1);
    verify_field(buffer, 2, 0, "new_row_field");
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_csv_insert_field_insert_at_end_of_row);
    RUN_TEST(test_csv_insert_field_insert_beyond_end_creates_new_field);
    RUN_TEST(test_csv_insert_field_insert_in_middle_shifts_fields);
    RUN_TEST(test_csv_insert_field_insert_at_start_shifts_all_fields);
    RUN_TEST(test_csv_insert_field_creates_new_row_if_needed);
    return UNITY_END();
}