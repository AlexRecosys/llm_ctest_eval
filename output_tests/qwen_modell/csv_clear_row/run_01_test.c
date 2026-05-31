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

static void populate_row_with_fields(CSV_BUFFER *buf, size_t row, size_t field_count)
{
    for (size_t i = 0; i < field_count; i++) {
        char field_text[32];
        snprintf(field_text, sizeof(field_text), "field_%zu_%zu", row, i);
        TEST_ASSERT_EQUAL_INT_MESSAGE(0, csv_set_field(buf, row, i, field_text),
                                      "Failed to set field");
    }
}

static void verify_row_cleared(CSV_BUFFER *buf, size_t row, const char *expected_text)
{
    char dest[256] = {0};
    int result = csv_get_field(dest, sizeof(dest), buf, row, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "csv_get_field should succeed");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(expected_text, dest, "Field content mismatch");
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, csv_get_width(buf, row),
                                  "Row width should be 1 after clear");
}

static void verify_row_not_cleared(CSV_BUFFER *buf, size_t row, size_t expected_width,
                                   const char *expected_first_field)
{
    char dest[256] = {0};
    int result = csv_get_field(dest, sizeof(dest), buf, row, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "csv_get_field should succeed");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(expected_first_field, dest, "First field content mismatch");
    TEST_ASSERT_EQUAL_INT_MESSAGE(expected_width, csv_get_width(buf, row),
                                  "Row width should be unchanged");
}

TEST(csv_clear_row, should_clear_single_field_row)
{
    // Setup: Add one row with one field
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, csv_set_field(buffer, 0, 0, "initial"),
                                  "Failed to set initial field");

    // Act
    int result = csv_clear_row(buffer, 0);

    // Assert
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "Function should return 0 on success");
    verify_row_cleared(buffer, 0, "");
}

TEST(csv_clear_row, should_clear_multi_field_row)
{
    // Setup: Add one row with 3 fields
    populate_row_with_fields(buffer, 0, 3);

    // Act
    int result = csv_clear_row(buffer, 0);

    // Assert
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "Function should return 0 on success");
    verify_row_cleared(buffer, 0, "");
}

TEST(csv_clear_row, should_clear_last_row_via_remove_last_row_path)
{
    // Setup: Add two rows, then clear the last one (row 1)
    populate_row_with_fields(buffer, 0, 2);
    populate_row_with_fields(buffer, 1, 3);

    // Act: Clear the last row (row 1)
    int result = csv_clear_row(buffer, 1);

    // Assert
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "Function should return 0 on success");
    verify_row_cleared(buffer, 1, "");
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, csv_get_height(buffer),
                                  "Buffer height should remain 2");
}

TEST(csv_clear_row, should_fail_gracefully_on_invalid_row)
{
    // Setup: Empty buffer (0 rows)
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, csv_get_height(buffer), "Buffer should start empty");

    // Act
    int result = csv_clear_row(buffer, 0);

    // Assert: Function should return 1 (error) for invalid row
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, result, "Function should return 1 for invalid row");
}

TEST(csv_clear_row, should_preserve_other_rows_when_clearing_non_last_row)
{
    // Setup: Two rows, each with multiple fields
    populate_row_with_fields(buffer, 0, 3);
    populate_row_with_fields(buffer, 1, 2);

    // Act: Clear row 0
    int result = csv_clear_row(buffer, 0);

    // Assert
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "Function should return 0 on success");

    // Verify row 0 is cleared
    verify_row_cleared(buffer, 0, "");

    // Verify row 1 is unchanged
    char dest[256] = {0};
    int get_result = csv_get_field(dest, sizeof(dest), buffer, 1, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, get_result, "csv_get_field should succeed for row 1");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("field_1_0", dest, "Row 1 first field should be unchanged");
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, csv_get_width(buffer, 1),
                                  "Row 1 width should be unchanged");
}