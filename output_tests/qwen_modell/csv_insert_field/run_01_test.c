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

static void setup_initial_row_with_fields(CSV_BUFFER *buf, size_t row, const char *fields[], size_t count)
{
    csv_clear_row(buf, row);
    for (size_t i = 0; i < count; i++) {
        int res = csv_set_field(buf, row, i, (char *)fields[i]);
        TEST_ASSERT_EQUAL_INT_MESSAGE(0, res, "csv_set_field should succeed");
    }
}

static void verify_row_width(CSV_BUFFER *buf, size_t row, size_t expected_width)
{
    int width = csv_get_width(buf, row);
    TEST_ASSERT_EQUAL_INT_MESSAGE((int)expected_width, width, "Row width mismatch");
}

static void verify_buffer_height(CSV_BUFFER *buf, size_t expected_height)
{
    int height = csv_get_height(buf);
    TEST_ASSERT_EQUAL_INT_MESSAGE((int)expected_height, height, "Buffer height mismatch");
}

void test_csv_insert_field_inserts_at_end_when_index_equals_current_width(void)
{
    // Setup: create a row with 2 fields
    const char *initial_fields[] = {"A", "B"};
    setup_initial_row_with_fields(buffer, 0, initial_fields, 2);
    verify_row_width(buffer, 0, 2);

    // Act: insert at index 2 (end of current fields)
    int result = csv_set_field(buffer, 0, 2, "C");
    TEST_ASSERT_EQUAL_INT(0, result);

    // Verify: field inserted at end
    verify_field(buffer, 0, 2, "C");
    verify_row_width(buffer, 0, 3);
}

void test_csv_insert_field_inserts_in_middle_and_shifts_right(void)
{
    // Setup: create a row with 3 fields
    const char *initial_fields[] = {"A", "B", "C"};
    setup_initial_row_with_fields(buffer, 0, initial_fields, 3);
    verify_row_width(buffer, 0, 3);

    // Act: insert "X" at index 1
    int res = csv_set_field(buffer, 0, 1, "X");
    TEST_ASSERT_EQUAL_INT(0, res);

    // Verify: original fields shifted right
    verify_field(buffer, 0, 0, "A");
    verify_field(buffer, 0, 1, "X");
    verify_field(buffer, 0, 2, "B");
    verify_field(buffer, 0, 3, "C");
    verify_row_width(buffer, 0, 4);
}

void test_csv_insert_field_inserts_at_beginning_and_shifts_all_right(void)
{
    // Setup: create a row with 2 fields
    const char *initial_fields[] = {"B", "C"};
    setup_initial_row_with_fields(buffer, 0, initial_fields, 2);
    verify_row_width(buffer, 0, 2);

    // Act: insert "A" at index 0
    int res = csv_set_field(buffer, 0, 0, "A");
    TEST_ASSERT_EQUAL_INT(0, res);

    // Verify: all fields shifted right
    verify_field(buffer, 0, 0, "A");
    verify_field(buffer, 0, 1, "B");
    verify_field(buffer, 0, 2, "C");
    verify_row_width(buffer, 0, 3);
}

void test_csv_insert_field_creates_new_row_if_row_does_not_exist(void)
{
    // Setup: buffer starts with 0 rows
    verify_buffer_height(buffer, 0);

    // Act: insert field at row 0, entry 0 (row doesn't exist yet)
    int res = csv_set_field(buffer, 0, 0, "New");
    TEST_ASSERT_EQUAL_INT(0, res);

    // Verify: row created and field set
    verify_buffer_height(buffer, 1);
    verify_field(buffer, 0, 0, "New");
    verify_row_width(buffer, 0, 1);
}

void test_csv_insert_field_creates_new_entry_if_entry_does_not_exist(void)
{
    // Setup: create row with 1 field
    const char *initial_fields[] = {"A"};
    setup_initial_row_with_fields(buffer, 0, initial_fields, 1);
    verify_row_width(buffer, 0, 1);

    // Act: insert field at index 5 (beyond current width)
    int res = csv_set_field(buffer, 0, 5, "F");
    TEST_ASSERT_EQUAL_INT(0, res);

    // Verify: intermediate entries filled with empty fields, new field set
    verify_field(buffer, 0, 0, "A");
    verify_field(buffer, 0, 1, "");
    verify_field(buffer, 0, 2, "");
    verify_field(buffer, 0, 3, "");
    verify_field(buffer, 0, 4, "");
    verify_field(buffer, 0, 5, "F");
    verify_row_width(buffer, 0, 6);
}