#include "unity.h"
#include "csv.h"

static CSV_BUFFER *buffer = NULL;

void setUp(void)
{
    buffer = csv_create_buffer();
    TEST_ASSERT_NOT_NULL_MESSAGE(buffer, "Failed to create CSV buffer");
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
    char dest[256] = {0};
    int result = csv_get_field(dest, sizeof(dest), buf, row, entry);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "csv_get_field should return 0 on success");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(expected, dest, "Field content mismatch");
}

static void setup_initial_row_with_fields(CSV_BUFFER *buf, size_t num_fields)
{
    csv_clear_row(buf, 0);
    for (size_t i = 0; i < num_fields; i++) {
        char field_text[32];
        snprintf(field_text, sizeof(field_text), "field%zu", i);
        csv_set_field(buf, 0, i, field_text);
    }
}

static void setup_two_rows(CSV_BUFFER *buf)
{
    csv_clear_row(buf, 0);
    csv_set_field(buf, 0, 0, "row0_col0");
    csv_set_field(buf, 0, 1, "row0_col1");

    csv_clear_row(buf, 1);
    csv_set_field(buf, 1, 0, "row1_col0");
}

static void verify_row_width(CSV_BUFFER *buf, size_t row, size_t expected_width)
{
    int width = csv_get_width(buf, row);
    TEST_ASSERT_EQUAL_INT_MESSAGE((int)expected_width, width, "Row width mismatch");
}

static void verify_row_count(CSV_BUFFER *buf, size_t expected_rows)
{
    int height = csv_get_height(buf);
    TEST_ASSERT_EQUAL_INT_MESSAGE((int)expected_rows, height, "Row count mismatch");
}

void test_csv_insert_field_inserts_at_end_when_index_equals_width(void)
{
    setup_initial_row_with_fields(buffer, 2);
    verify_row_width(buffer, 0, 2);

    int result = csv_insert_field(buffer, 0, 2, "new_field");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "csv_insert_field should return 0");

    verify_row_width(buffer, 0, 3);
    verify_field(buffer, 0, 2, "new_field");
}

void test_csv_insert_field_shifts_existing_fields_to_right(void)
{
    setup_initial_row_with_fields(buffer, 3);
    verify_row_width(buffer, 0, 3);
    verify_field(buffer, 0, 0, "field0");
    verify_field(buffer, 0, 1, "field1");
    verify_field(buffer, 0, 2, "field2");

    int result = csv_insert_field(buffer, 0, 1, "inserted");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "csv_insert_field should return 0");

    verify_row_width(buffer, 0, 4);
    verify_field(buffer, 0, 0, "field0");
    verify_field(buffer, 0, 1, "inserted");
    verify_field(buffer, 0, 2, "field1");
    verify_field(buffer, 0, 3, "field2");
}

void test_csv_insert_field_inserts_at_beginning(void)
{
    setup_initial_row_with_fields(buffer, 2);
    verify_row_width(buffer, 0, 2);

    int result = csv_insert_field(buffer, 0, 0, "first");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "csv_insert_field should return 0");

    verify_row_width(buffer, 0, 3);
    verify_field(buffer, 0, 0, "first");
    verify_field(buffer, 0, 1, "field0");
    verify_field(buffer, 0, 2, "field1");
}

void test_csv_insert_field_creates_new_row_if_row_index_out_of_bounds(void)
{
    setup_initial_row_with_fields(buffer, 2);
    verify_row_count(buffer, 2);

    int result = csv_insert_field(buffer, 2, 0, "new_row_field");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "csv_insert_field should return 0");

    verify_row_count(buffer, 3);
    verify_row_width(buffer, 2, 1);
    verify_field(buffer, 2, 0, "new_row_field");
}

void test_csv_insert_field_creates_new_field_if_entry_index_out_of_bounds(void)
{
    setup_initial_row_with_fields(buffer, 2);
    verify_row_width(buffer, 0, 2);

    int result = csv_insert_field(buffer, 0, 5, "far_field");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "csv_insert_field should return 0");

    verify_row_width(buffer, 0, 6);
    verify_field(buffer, 0, 5, "far_field");
    verify_field(buffer, 0, 0, "field0");
    verify_field(buffer, 0, 1, "field1");
    // Verify intermediate fields are empty
    char dest[256] = {0};
    csv_get_field(dest, sizeof(dest), buffer, 0, 2);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("", dest, "Intermediate field should be empty");
    csv_get_field(dest, sizeof(dest), buffer, 0, 3);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("", dest, "Intermediate field should be empty");
    csv_get_field(dest, sizeof(dest), buffer, 0, 4);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("", dest, "Intermediate field should be empty");
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_csv_insert_field_inserts_at_end_when_index_equals_width);
    RUN_TEST(test_csv_insert_field_shifts_existing_fields_to_right);
    RUN_TEST(test_csv_insert_field_inserts_at_beginning);
    RUN_TEST(test_csv_insert_field_creates_new_row_if_row_index_out_of_bounds);
    RUN_TEST(test_csv_insert_field_creates_new_field_if_entry_index_out_of_bounds);

    return UNITY_END();
}