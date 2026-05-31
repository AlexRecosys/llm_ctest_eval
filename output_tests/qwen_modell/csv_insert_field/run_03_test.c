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
    csv_clear_row(buffer, 0);
    csv_set_field(buffer, 0, 0, "first");
    csv_set_field(buffer, 0, 1, "second");

    int result = csv_insert_field(buffer, 0, 2, "third");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "csv_insert_field should return 0");

    verify_field(buffer, 0, 0, "first");
    verify_field(buffer, 0, 1, "second");
    verify_field(buffer, 0, 2, "third");
    verify_row_width(buffer, 0, 3);
}

void test_csv_insert_field_inserts_in_middle_and_shifts_right(void)
{
    setup_initial_row_with_fields(buffer, 3);

    int result = csv_insert_field(buffer, 0, 1, "new");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "csv_insert_field should return 0");

    verify_field(buffer, 0, 0, "field0");
    verify_field(buffer, 0, 1, "new");
    verify_field(buffer, 0, 2, "field1");
    verify_field(buffer, 0, 3, "field2");
    verify_row_width(buffer, 0, 4);
}

void test_csv_insert_field_inserts_at_beginning_and_shifts_all_right(void)
{
    setup_initial_row_with_fields(buffer, 3);

    int result = csv_insert_field(buffer, 0, 0, "first_new");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "csv_insert_field should return 0");

    verify_field(buffer, 0, 0, "first_new");
    verify_field(buffer, 0, 1, "field0");
    verify_field(buffer, 0, 2, "field1");
    verify_field(buffer, 0, 3, "field2");
    verify_row_width(buffer, 0, 4);
}

void test_csv_insert_field_extends_row_when_index_exceeds_current_width(void)
{
    csv_clear_row(buffer, 0);
    csv_set_field(buffer, 0, 0, "only");

    int result = csv_insert_field(buffer, 0, 5, "far_out");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "csv_insert_field should return 0");

    verify_field(buffer, 0, 0, "only");
    verify_field(buffer, 0, 5, "far_out");
    verify_row_width(buffer, 0, 6);
}

void test_csv_insert_field_creates_new_row_when_row_index_exceeds_existing_rows(void)
{
    csv_clear_row(buffer, 0);
    csv_set_field(buffer, 0, 0, "row0_field0");

    int result = csv_insert_field(buffer, 2, 0, "row2_field0");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "csv_insert_field should return 0");

    verify_field(buffer, 0, 0, "row0_field0");
    verify_field(buffer, 2, 0, "row2_field0");
    verify_row_count(buffer, 3);
    verify_row_width(buffer, 1, 0);
    verify_row_width(buffer, 2, 1);
}