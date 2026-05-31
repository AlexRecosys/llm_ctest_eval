#include "unity.h"
#include "csv.h"

static CSV_BUFFER *buffer = NULL;

void setUp(void)
{
    buffer = csv_create_buffer();
    TEST_ASSERT_NOT_NULL_MESSAGE(buffer, "Failed to create buffer");
}

void tearDown(void)
{
    if (buffer != NULL) {
        csv_destroy_buffer(buffer);
        buffer = NULL;
    }
}

static void verify_field(CSV_BUFFER *buf, size_t row, size_t entry, const char *expected)
{
    char actual[256] = {0};
    int result = csv_get_field(actual, sizeof(actual), buf, row, entry);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "csv_get_field should return 0 on success");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(expected, actual, "Field content mismatch");
}

static void setup_simple_2x2_buffer(void)
{
    csv_set_field(buffer, 0, 0, "A1");
    csv_set_field(buffer, 0, 1, "B1");
    csv_set_field(buffer, 1, 0, "A2");
    csv_set_field(buffer, 1, 1, "B2");
}

static void setup_1x3_buffer(void)
{
    csv_set_field(buffer, 0, 0, "X1");
    csv_set_field(buffer, 0, 1, "X2");
    csv_set_field(buffer, 0, 2, "X3");
}

static void setup_3x1_buffer(void)
{
    csv_set_field(buffer, 0, 0, "Y1");
    csv_set_field(buffer, 1, 0, "Y2");
    csv_set_field(buffer, 2, 0, "Y3");
}

void test_csv_insert_field_insert_at_end_of_row(void)
{
    setup_1x3_buffer();
    int result = csv_insert_field(buffer, 0, 3, "NEW");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "csv_insert_field should return 0 on success");
    TEST_ASSERT_EQUAL_INT_MESSAGE(4, csv_get_width(buffer, 0), "Row width should increase by 1");
    verify_field(buffer, 0, 3, "NEW");
    verify_field(buffer, 0, 0, "X1");
    verify_field(buffer, 0, 1, "X2");
    verify_field(buffer, 0, 2, "X3");
}

void test_csv_insert_field_insert_in_middle_of_row(void)
{
    setup_1x3_buffer();
    int result = csv_insert_field(buffer, 0, 1, "INSERTED");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "csv_insert_field should return 0 on success");
    TEST_ASSERT_EQUAL_INT_MESSAGE(4, csv_get_width(buffer, 0), "Row width should increase by 1");
    verify_field(buffer, 0, 0, "X1");
    verify_field(buffer, 0, 1, "INSERTED");
    verify_field(buffer, 0, 2, "X2");
    verify_field(buffer, 0, 3, "X3");
}

void test_csv_insert_field_insert_at_beginning_of_row(void)
{
    setup_1x3_buffer();
    int result = csv_insert_field(buffer, 0, 0, "FIRST");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "csv_insert_field should return 0 on success");
    TEST_ASSERT_EQUAL_INT_MESSAGE(4, csv_get_width(buffer, 0), "Row width should increase by 1");
    verify_field(buffer, 0, 0, "FIRST");
    verify_field(buffer, 0, 1, "X1");
    verify_field(buffer, 0, 2, "X2");
    verify_field(buffer, 0, 3, "X3");
}

void test_csv_insert_field_insert_beyond_existing_row_creates_new_field(void)
{
    setup_1x3_buffer();
    int result = csv_insert_field(buffer, 0, 5, "NEW");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "csv_insert_field should return 0 on success");
    TEST_ASSERT_EQUAL_INT_MESSAGE(6, csv_get_width(buffer, 0), "Row width should be 6 (5+1)");
    verify_field(buffer, 0, 5, "NEW");
    verify_field(buffer, 0, 0, "X1");
    verify_field(buffer, 0, 1, "X2");
    verify_field(buffer, 0, 2, "X3");
    // Verify empty fields in between are cleared
    char empty[2] = {0};
    int ret = csv_get_field(empty, sizeof(empty), buffer, 0, 3);
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, ret, "Empty field should return 2");
    ret = csv_get_field(empty, sizeof(empty), buffer, 0, 4);
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, ret, "Empty field should return 2");
}

void test_csv_insert_field_insert_into_new_row(void)
{
    setup_1x3_buffer();
    int result = csv_insert_field(buffer, 1, 0, "NEW_ROW");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "csv_insert_field should return 0 on success");
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, csv_get_width(buffer, 1), "New row should have width 1");
    verify_field(buffer, 1, 0, "NEW_ROW");
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, csv_get_height(buffer), "Buffer height should increase to 2");
}