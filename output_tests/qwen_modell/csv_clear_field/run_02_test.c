#include "csv.c"
#include "unity.h"
#include <string.h>
#include <stdlib.h>

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

static void populate_buffer_with_data(void)
{
    // Add 3 rows with varying widths
    append_row(buffer);
    append_field(buffer, 0);
    set_field(buffer->field[0][0], "A1");
    append_field(buffer, 0);
    set_field(buffer->field[0][1], "B1");
    append_field(buffer, 0);
    set_field(buffer->field[0][2], "C1");

    append_row(buffer);
    append_field(buffer, 1);
    set_field(buffer->field[1][0], "A2");
    append_field(buffer, 1);
    set_field(buffer->field[1][1], "B2");

    append_row(buffer);
    append_field(buffer, 2);
    set_field(buffer->field[2][0], "A3");
}

static void verify_field_is_cleared(CSV_BUFFER *buf, size_t row, size_t entry)
{
    char expected[2] = { '\0', '\0' };
    char actual[256] = { 0 };
    int result = csv_get_field(actual, sizeof(actual), buf, row, entry);
    TEST_ASSERT_EQUAL_INT(2, result); // empty or non-existent
    TEST_ASSERT_EQUAL_STRING_LEN(expected, actual, 1);
}

static void verify_field_preserved(CSV_BUFFER *buf, size_t row, size_t entry, const char *expected_text)
{
    char actual[256] = { 0 };
    int result = csv_get_field(actual, sizeof(actual), buf, row, entry);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_STRING(expected_text, actual);
}

static void verify_row_width(CSV_BUFFER *buf, size_t row, size_t expected_width)
{
    TEST_ASSERT_EQUAL_SIZE(expected_width, csv_get_width(buf, row));
}

static void verify_buffer_height(CSV_BUFFER *buf, size_t expected_height)
{
    TEST_ASSERT_EQUAL_SIZE(expected_height, csv_get_height(buf));
}

void test_csv_clear_field_clears_middle_field(void)
{
    populate_buffer_with_data();

    // Clear middle field (entry 1) in row 0
    int result = csv_clear_field(buffer, 0, 1);
    TEST_ASSERT_EQUAL_INT(0, result);

    // Verify field 1 is cleared
    verify_field_is_cleared(buffer, 0, 1);

    // Verify other fields remain unchanged
    verify_field_preserved(buffer, 0, 0, "A1");
    verify_field_preserved(buffer, 0, 2, "C1");

    // Row width unchanged (still 3 fields)
    verify_row_width(buffer, 0, 3);
}

void test_csv_clear_field_clears_last_field_leaves_spacer(void)
{
    populate_buffer_with_data();

    // Clear last field (entry 2) in row 0 (not the only field)
    int result = csv_clear_field(buffer, 0, 2);
    TEST_ASSERT_EQUAL_INT(0, result);

    // Verify field 2 is cleared
    verify_field_is_cleared(buffer, 0, 2);

    // Verify other fields remain unchanged
    verify_field_preserved(buffer, 0, 0, "A1");
    verify_field_preserved(buffer, 0, 1, "B1");

    // Row width unchanged (still 3 fields)
    verify_row_width(buffer, 0, 3);
}

void test_csv_clear_field_removes_last_field_when_only_one_field_left(void)
{
    populate_buffer_with_data();

    // Clear the only field in row 2 (entry 0)
    int result = csv_clear_field(buffer, 2, 0);
    TEST_ASSERT_EQUAL_INT(0, result);

    // Row should now have 1 field (a spacer)
    verify_row_width(buffer, 2, 1);

    // The field should be cleared
    verify_field_is_cleared(buffer, 2, 0);
}

void test_csv_clear_field_removes_last_field_when_not_first_field(void)
{
    populate_buffer_with_data();

    // Clear last field (entry 1) in row 1 (which has 2 fields)
    int result = csv_clear_field(buffer, 1, 1);
    TEST_ASSERT_EQUAL_INT(0, result);

    // Row should now have 1 field (the first field remains as spacer)
    verify_row_width(buffer, 1, 1);

    // The remaining field should be cleared
    verify_field_is_cleared(buffer, 1, 0);

    // Other rows unchanged
    verify_row_width(buffer, 0, 3);
    verify_row_width(buffer, 2, 1);
}

void test_csv_clear_field_out_of_range_row_returns_zero(void)
{
    populate_buffer_with_data();

    // Try to clear non-existent row
    int result = csv_clear_field(buffer, 100, 0);
    TEST_ASSERT_EQUAL_INT(0, result);

    // Buffer unchanged
    verify_buffer_height(buffer, 3);
}

void test_csv_clear_field_out_of_range_entry_returns_zero(void)
{
    populate_buffer_with_data();

    // Try to clear non-existent entry in row 1 (which has width 2)
    int result = csv_clear_field(buffer, 1, 100);
    TEST_ASSERT_EQUAL_INT(0, result);

    // Row width unchanged
    verify_row_width(buffer, 1, 2);
}

void test_csv_clear_field_clears_first_field_of_row_with_one_field(void)
{
    populate_buffer_with_data();

    // Clear the only field in row 2 (entry 0)
    int result = csv_clear_field(buffer, 2, 0);
    TEST_ASSERT_EQUAL_INT(0, result);

    // Row should still have 1 field (a spacer)
    verify_row_width(buffer, 2, 1);

    // The field should be cleared
    verify_field_is_cleared(buffer, 2, 0);
}

void test_csv_clear_field_clears_first_field_of_row_with_multiple_fields(void)
{
    populate_buffer_with_data();

    // Clear first field (entry 0) in row 0 (which has 3 fields)
    int result = csv_clear_field(buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT(0, result);

    // Row width unchanged (still 3 fields)
    verify_row_width(buffer, 0, 3);

    // First field should be cleared
    verify_field_is_cleared(buffer, 0, 0);

    // Other fields unchanged
    verify_field_preserved(buffer, 0, 1, "B1");
    verify_field_preserved(buffer, 0, 2, "C1");
}

void test_csv_clear_field_clears_empty_buffer(void)
{
    // Clear on empty buffer
    int result = csv_clear_field(buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT(0, result);

    // Buffer still empty
    verify_buffer_height(buffer, 0);
}

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_csv_clear_field_clears_middle_field);
    RUN_TEST(test_csv_clear_field_clears_last_field_leaves_spacer);
    RUN_TEST(test_csv_clear_field_removes_last_field_when_only_one_field_left);
    RUN_TEST(test_csv_clear_field_removes_last_field_when_not_first_field);
    RUN_TEST(test_csv_clear_field_out_of_range_row_returns_zero);
    RUN_TEST(test_csv_clear_field_out_of_range_entry_returns_zero);
    RUN_TEST(test_csv_clear_field_clears_first_field_of_row_with_one_field);
    RUN_TEST(test_csv_clear_field_clears_first_field_of_row_with_multiple_fields);
    RUN_TEST(test_csv_clear_field_clears_empty_buffer);
    return UNITY_END();
}