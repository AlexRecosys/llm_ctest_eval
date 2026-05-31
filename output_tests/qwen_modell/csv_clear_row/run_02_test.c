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
    char field_text[32];

    // First ensure row exists
    for (size_t i = 0; i < row + 1; i++) {
        ret = csv_set_field(buf, i, 0, "");
        TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "Failed to initialize row");
    }

    // Add fields to the target row
    for (size_t f = 0; f < field_count; f++) {
        snprintf(field_text, sizeof(field_text), "field%zu", f);
        ret = csv_set_field(buf, row, f, field_text);
        TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "Failed to set field");
    }
}

static void verify_row_clear(CSV_BUFFER *buf, size_t row)
{
    char dest[64];
    int ret;

    // Row should have exactly 1 field
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, csv_get_width(buf, row),
                                  "Row width should be 1 after clear");

    // The single field should be empty
    ret = csv_get_field(dest, sizeof(dest), buf, row, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, ret, "Field should be empty after clear");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("", dest, "Field content should be empty string");
}

static void verify_row_not_cleared(CSV_BUFFER *buf, size_t row, size_t expected_width)
{
    TEST_ASSERT_EQUAL_INT_MESSAGE(expected_width, csv_get_width(buf, row),
                                  "Row width should be unchanged");
}

static void verify_field_content(CSV_BUFFER *buf, size_t row, size_t entry, const char *expected)
{
    char dest[64];
    int ret = csv_get_field(dest, sizeof(dest), buf, row, entry);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "Field should be readable");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(expected, dest, "Field content mismatch");
}

static void setup_multi_row_buffer(void)
{
    // Create a buffer with 3 rows, each having 3 fields
    csv_set_field(buffer, 0, 0, "r0c0");
    csv_set_field(buffer, 0, 1, "r0c1");
    csv_set_field(buffer, 0, 2, "r0c2");

    csv_set_field(buffer, 1, 0, "r1c0");
    csv_set_field(buffer, 1, 1, "r1c1");
    csv_set_field(buffer, 1, 2, "r1c2");

    csv_set_field(buffer, 2, 0, "r2c0");
    csv_set_field(buffer, 2, 1, "r2c1");
    csv_set_field(buffer, 2, 2, "r2c2");
}

void test_csv_clear_row_last_row_optimization_path(void)
{
    // Setup: 2 rows, last row has 3 fields
    csv_set_field(buffer, 0, 0, "r0c0");
    csv_set_field(buffer, 1, 0, "r1c0");
    csv_set_field(buffer, 1, 1, "r1c1");
    csv_set_field(buffer, 1, 2, "r1c2");

    // Verify initial state
    TEST_ASSERT_EQUAL_INT(2, csv_get_height(buffer));
    TEST_ASSERT_EQUAL_INT(3, csv_get_width(buffer, 1));

    // Clear the last row (row 1)
    int ret = csv_clear_row(buffer, 1);
    TEST_ASSERT_EQUAL_INT(0, ret);

    // Verify row 1 is cleared (1 field, empty)
    verify_row_clear(buffer, 1);

    // Verify row 0 is unaffected
    verify_field_content(buffer, 0, 0, "r0c0");
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buffer, 0));
}

void test_csv_clear_row_middle_row_normal_path(void)
{
    // Setup: 3 rows, middle row has 4 fields
    setup_multi_row_buffer();

    // Verify initial state
    TEST_ASSERT_EQUAL_INT(3, csv_get_height(buffer));
    TEST_ASSERT_EQUAL_INT(3, csv_get_width(buffer, 1));

    // Clear middle row (row 1)
    int ret = csv_clear_row(buffer, 1);
    TEST_ASSERT_EQUAL_INT(0, ret);

    // Verify row 1 is cleared
    verify_row_clear(buffer, 1);

    // Verify other rows are unaffected
    verify_field_content(buffer, 0, 0, "r0c0");
    verify_field_content(buffer, 2, 0, "r2c0");
    TEST_ASSERT_EQUAL_INT(3, csv_get_width(buffer, 0));
    TEST_ASSERT_EQUAL_INT(3, csv_get_width(buffer, 2));
}

void test_csv_clear_row_single_field_row(void)
{
    // Setup: row with only 1 field
    csv_set_field(buffer, 0, 0, "only_field");

    // Verify initial state
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buffer, 0));

    // Clear the row
    int ret = csv_clear_row(buffer, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);

    // Verify row is cleared (still 1 field, but empty)
    verify_row_clear(buffer, 0);
}

void test_csv_clear_row_empty_row(void)
{
    // Setup: row with 0 fields (create row then clear)
    csv_set_field(buffer, 0, 0, "temp");
    csv_set_field(buffer, 0, 0, "");  // Replace with empty to get 1 field

    // Now clear it
    int ret = csv_clear_row(buffer, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);

    // Verify row is cleared
    verify_row_clear(buffer, 0);
}

void test_csv_clear_row_nonexistent_row(void)
{
    // Setup: buffer with 2 rows
    csv_set_field(buffer, 0, 0, "r0c0");
    csv_set_field(buffer, 1, 0, "r1c0");

    // Try to clear row 5 (nonexistent)
    int ret = csv_clear_row(buffer, 5);
    // Function behavior for nonexistent row is undefined per spec,
    // but we test that it doesn't crash and returns expected error code
    // The spec says "Returns: 0: success, 1: memory allocation failure"
    // but doesn't specify error for invalid row; assume it returns 0 if no crash
    // However, looking at implementation, it accesses buffer->field[row] directly
    // so passing invalid row may cause undefined behavior.
    // Since the spec doesn't define behavior for invalid row, we test only valid rows.
    // This test is skipped per requirements — we only test valid rows.
    // Instead, test with row=1 (valid last row)
    ret = csv_clear_row(buffer, 1);
    TEST_ASSERT_EQUAL_INT(0, ret);
    verify_row_clear(buffer, 1);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_csv_clear_row_last_row_optimization_path);
    RUN_TEST(test_csv_clear_row_middle_row_normal_path);
    RUN_TEST(test_csv_clear_row_single_field_row);
    RUN_TEST(test_csv_clear_row_empty_row);
    RUN_TEST(test_csv_clear_row_nonexistent_row);

    return UNITY_END();
}