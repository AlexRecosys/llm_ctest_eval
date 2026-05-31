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
    for (size_t i = 0; i < field_count; i++) {
        char field_text[32];
        snprintf(field_text, sizeof(field_text), "field_%zu_%zu", row, i);
        TEST_ASSERT_EQUAL_INT(0, csv_set_field(buf, row, i, field_text));
    }
}

static void verify_row_clear(CSV_BUFFER *buf, size_t row, const char *expected_text)
{
    char dest[256] = {0};
    int result = csv_get_field(dest, sizeof(dest), buf, row, 0);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_STRING(expected_text, dest);
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buf, row));
}

static void verify_row_not_cleared(CSV_BUFFER *buf, size_t row, size_t expected_width)
{
    TEST_ASSERT_EQUAL_INT((int)expected_width, csv_get_width(buf, row));
}

static void create_multi_row_buffer_with_fields(void)
{
    // Create 3 rows: row0 has 3 fields, row1 has 1 field, row2 has 5 fields
    TEST_ASSERT_EQUAL_INT(0, csv_set_field(buffer, 0, 0, "r0c0"));
    TEST_ASSERT_EQUAL_INT(0, csv_set_field(buffer, 0, 1, "r0c1"));
    TEST_ASSERT_EQUAL_INT(0, csv_set_field(buffer, 0, 2, "r0c2"));
    TEST_ASSERT_EQUAL_INT(0, csv_set_field(buffer, 1, 0, "r1c0"));
    TEST_ASSERT_EQUAL_INT(0, csv_set_field(buffer, 2, 0, "r2c0"));
    TEST_ASSERT_EQUAL_INT(0, csv_set_field(buffer, 2, 1, "r2c1"));
    TEST_ASSERT_EQUAL_INT(0, csv_set_field(buffer, 2, 2, "r2c2"));
    TEST_ASSERT_EQUAL_INT(0, csv_set_field(buffer, 2, 3, "r2c3"));
    TEST_ASSERT_EQUAL_INT(0, csv_set_field(buffer, 2, 4, "r2c4"));
}

TEST(CSVClearRow, ClearLastRowUsesRemoveLastRowPath)
{
    // Setup: Create buffer with 2 rows, row1 has 3 fields
    create_multi_row_buffer_with_fields();
    // Now add a 3rd row (row2) with 3 fields, then clear it (should use remove_last_row path)
    TEST_ASSERT_EQUAL_INT(0, csv_set_field(buffer, 2, 0, "temp0"));
    TEST_ASSERT_EQUAL_INT(0, csv_set_field(buffer, 2, 1, "temp1"));
    TEST_ASSERT_EQUAL_INT(0, csv_set_field(buffer, 2, 2, "temp2"));

    // Clear row2 (which is the last row)
    int result = csv_clear_row(buffer, 2);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_INT(2, csv_get_height(buffer));
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buffer, 0));
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buffer, 1));
}

TEST(CSVClearRow, ClearNonLastRowWithMultipleFields)
{
    // Setup: Create buffer with 2 rows, row0 has 3 fields
    create_multi_row_buffer_with_fields();

    // Clear row0 (non-last row with 3 fields)
    int result = csv_clear_row(buffer, 0);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_INT(3, csv_get_height(buffer));
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buffer, 0));
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buffer, 1));
    TEST_ASSERT_EQUAL_INT(5, csv_get_width(buffer, 2));

    // Verify cleared field content
    char dest[256] = {0};
    TEST_ASSERT_EQUAL_INT(0, csv_get_field(dest, sizeof(dest), buffer, 0, 0));
    TEST_ASSERT_EQUAL_STRING("", dest);
}

TEST(CSVClearRow, ClearRowWithSingleField)
{
    // Setup: Create buffer with 2 rows, row1 has 1 field
    create_multi_row_buffer_with_fields();

    // Clear row1 (which has only 1 field)
    int result = csv_clear_row(buffer, 1);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_INT(3, csv_get_height(buffer));
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buffer, 1));

    // Verify cleared field content
    char dest[256] = {0};
    TEST_ASSERT_EQUAL_INT(0, csv_get_field(dest, sizeof(dest), buffer, 1, 0));
    TEST_ASSERT_EQUAL_STRING("", dest);
}

TEST(CSVClearRow, ClearRowWithManyFieldsPreservesOtherRows)
{
    // Setup: Create buffer with 3 rows, row2 has 5 fields
    create_multi_row_buffer_with_fields();

    // Clear row2 (non-last row with 5 fields)
    int result = csv_clear_row(buffer, 2);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_INT(3, csv_get_height(buffer));
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buffer, 2));

    // Verify other rows unchanged
    TEST_ASSERT_EQUAL_INT(3, csv_get_width(buffer, 0));
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buffer, 1));

    // Verify cleared row content
    char dest[256] = {0};
    TEST_ASSERT_EQUAL_INT(0, csv_get_field(dest, sizeof(dest), buffer, 2, 0));
    TEST_ASSERT_EQUAL_STRING("", dest);
}

TEST(CSVClearRow, ClearFirstRowInMultiRowBuffer)
{
    // Setup: Create buffer with 2 rows, row0 has 3 fields
    create_multi_row_buffer_with_fields();

    // Clear row0 (first row with 3 fields)
    int result = csv_clear_row(buffer, 0);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_INT(3, csv_get_height(buffer));
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buffer, 0));
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buffer, 1));
    TEST_ASSERT_EQUAL_INT(5, csv_get_width(buffer, 2));

    // Verify cleared field content
    char dest[256] = {0};
    TEST_ASSERT_EQUAL_INT(0, csv_get_field(dest, sizeof(dest), buffer, 0, 0));
    TEST_ASSERT_EQUAL_STRING("", dest);
}