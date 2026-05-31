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
    char actual[256] = {0};
    int result = csv_get_field(actual, sizeof(actual), buf, row, entry);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "csv_get_field should return 0 on success");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(expected, actual, "Field content mismatch");
}

static void setup_initial_row_with_fields(CSV_BUFFER *buf, size_t row, const char *field0, const char *field1, const char *field2)
{
    csv_clear_row(buf, row);
    csv_set_field(buf, row, 0, field0);
    csv_set_field(buf, row, 1, field1);
    csv_set_field(buf, row, 2, field2);
}

static void setup_two_rows(CSV_BUFFER *buf)
{
    csv_clear_row(buf, 0);
    csv_clear_row(buf, 1);
    csv_set_field(buf, 0, 0, "r0c0");
    csv_set_field(buf, 0, 1, "r0c1");
    csv_set_field(buf, 1, 0, "r1c0");
}

TEST_CASE(csv_insert_field_inserts_at_end_when_index_equals_width)
{
    csv_clear_row(buffer, 0);
    csv_set_field(buffer, 0, 0, "first");
    csv_set_field(buffer, 0, 1, "second");

    int result = csv_insert_field(buffer, 0, 2, "third");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "csv_insert_field should return 0 on success");
    TEST_ASSERT_EQUAL_INT_MESSAGE(3, csv_get_width(buffer, 0), "Row width should be 3 after insertion at end");
    verify_field(buffer, 0, 0, "first");
    verify_field(buffer, 0, 1, "second");
    verify_field(buffer, 0, 2, "third");
}

TEST_CASE(csv_insert_field_shifts_existing_fields_when_inserting_in_middle)
{
    setup_initial_row_with_fields(buffer, 0, "A", "B", "C");

    int result = csv_insert_field(buffer, 0, 1, "INSERTED");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "csv_insert_field should return 0 on success");
    TEST_ASSERT_EQUAL_INT_MESSAGE(4, csv_get_width(buffer, 0), "Row width should increase by 1");
    verify_field(buffer, 0, 0, "A");
    verify_field(buffer, 0, 1, "INSERTED");
    verify_field(buffer, 0, 2, "B");
    verify_field(buffer, 0, 3, "C");
}

TEST_CASE(csv_insert_field_replaces_field_when_index_within_existing_range)
{
    setup_initial_row_with_fields(buffer, 0, "X", "Y", "Z");

    int result = csv_insert_field(buffer, 0, 1, "REPLACED");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "csv_insert_field should return 0 on success");
    TEST_ASSERT_EQUAL_INT_MESSAGE(4, csv_get_width(buffer, 0), "Row width should increase by 1");
    verify_field(buffer, 0, 0, "X");
    verify_field(buffer, 0, 1, "REPLACED");
    verify_field(buffer, 0, 2, "Y");
    verify_field(buffer, 0, 3, "Z");
}

TEST_CASE(csv_insert_field_creates_new_field_when_row_or_entry_does_not_exist)
{
    csv_clear_row(buffer, 0);
    csv_set_field(buffer, 0, 0, "existing");

    int result = csv_insert_field(buffer, 0, 5, "new_field");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "csv_insert_field should return 0 on success");
    TEST_ASSERT_EQUAL_INT_MESSAGE(6, csv_get_width(buffer, 0), "Row width should be 6 after inserting at index 5");
    verify_field(buffer, 0, 0, "existing");
    verify_field(buffer, 0, 1, "");
    verify_field(buffer, 0, 2, "");
    verify_field(buffer, 0, 3, "");
    verify_field(buffer, 0, 4, "");
    verify_field(buffer, 0, 5, "new_field");
}

TEST_CASE(csv_insert_field_inserts_into_second_row)
{
    setup_two_rows(buffer);

    int result = csv_insert_field(buffer, 1, 1, "MIDDLE");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "csv_insert_field should return 0 on success");
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, csv_get_width(buffer, 1), "Row 1 width should be 2 after insertion");
    verify_field(buffer, 1, 0, "r1c0");
    verify_field(buffer, 1, 1, "MIDDLE");
    verify_field(buffer, 0, 0, "r0c0");
    verify_field(buffer, 0, 1, "r0c1");
}