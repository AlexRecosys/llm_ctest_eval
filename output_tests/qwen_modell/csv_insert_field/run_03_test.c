#include "unity.h"
#include "csv.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* File-scope static variables / fixtures */
static CSV_BUFFER *buffer = NULL;

/* Helper functions and macros */
static void init_buffer_with_rows_and_widths(CSV_BUFFER *buf, size_t num_rows, size_t *widths) {
    for (size_t i = 0; i < num_rows; i++) {
        TEST_ASSERT_EQUAL_INT(0, append_row(buf));
        for (size_t j = 0; j < widths[i]; j++) {
            TEST_ASSERT_EQUAL_INT(0, append_field(buf, i));
            char field_text[32];
            snprintf(field_text, sizeof(field_text), "R%zuC%zu", i, j);
            TEST_ASSERT_EQUAL_INT(0, csv_set_field(buf, i, j, field_text));
        }
    }
}

static void assert_field_equals(CSV_BUFFER *buf, size_t row, size_t entry, const char *expected) {
    char actual[256] = {0};
    int result = csv_get_field(actual, sizeof(actual), buf, row, entry);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_STRING_MESSAGE(expected, actual, "Field value mismatch");
}

static void assert_row_width_equals(CSV_BUFFER *buf, size_t row, size_t expected_width) {
    size_t actual_width = csv_get_width(buf, row);
    TEST_ASSERT_EQUAL_UINT_MESSAGE(expected_width, actual_width, "Row width mismatch");
}

/* setUp and tearDown (non-static, visible to Unity) */
void setUp(void) {
    buffer = csv_create_buffer();
    TEST_ASSERT_NOT_NULL(buffer);
}

void tearDown(void) {
    csv_destroy_buffer(buffer);
    buffer = NULL;
}

/* Test Cases */

void test_csv_insert_field_inserts_at_end_when_row_exists_and_entry_is_beyond_current_width(void) {
    size_t widths[] = {2};
    init_buffer_with_rows_and_widths(buffer, 1, widths);

    /* Insert at entry 2 (beyond current width=2), should append */
    TEST_ASSERT_EQUAL_INT(0, csv_insert_field(buffer, 0, 2, "NEW"));

    assert_row_width_equals(buffer, 0, 3);
    assert_field_equals(buffer, 0, 0, "R0C0");
    assert_field_equals(buffer, 0, 1, "R0C1");
    assert_field_equals(buffer, 0, 2, "NEW");
}

void test_csv_insert_field_inserts_at_new_row_by_appending_row_and_field(void) {
    size_t widths[] = {2};
    init_buffer_with_rows_and_widths(buffer, 1, widths);

    /* Insert at row 1 (beyond current rows=1), should append row and field */
    TEST_ASSERT_EQUAL_INT(0, csv_insert_field(buffer, 1, 0, "FIRST"));

    assert_row_width_equals(buffer, 1, 1);
    assert_field_equals(buffer, 1, 0, "FIRST");
    TEST_ASSERT_EQUAL_INT(2, csv_get_height(buffer));
}

void test_csv_insert_field_shifts_existing_fields_to_right_when_inserting_in_middle(void) {
    size_t widths[] = {3};
    init_buffer_with_rows_and_widths(buffer, 1, widths);

    /* Insert at entry 1 (middle of row with width=3) */
    TEST_ASSERT_EQUAL_INT(0, csv_insert_field(buffer, 0, 1, "INSERTED"));

    assert_row_width_equals(buffer, 0, 4);
    assert_field_equals(buffer, 0, 0, "R0C0");
    assert_field_equals(buffer, 0, 1, "INSERTED");
    assert_field_equals(buffer, 0, 2, "R0C0"); /* shifted from original index 1 */
    assert_field_equals(buffer, 0, 3, "R0C1"); /* shifted from original index 2 */
}

void test_csv_insert_field_shifts_all_fields_right_when_inserting_at_beginning(void) {
    size_t widths[] = {2};
    init_buffer_with_rows_and_widths(buffer, 1, widths);

    /* Insert at entry 0 (beginning of row) */
    TEST_ASSERT_EQUAL_INT(0, csv_insert_field(buffer, 0, 0, "FIRST"));

    assert_row_width_equals(buffer, 0, 3);
    assert_field_equals(buffer, 0, 0, "FIRST");
    assert_field_equals(buffer, 0, 1, "R0C0");
    assert_field_equals(buffer, 0, 2, "R0C1");
}

void test_csv_insert_field_handles_empty_row_by_appending_field(void) {
    /* Start with empty buffer */
    TEST_ASSERT_EQUAL_INT(0, append_row(buffer));
    assert_row_width_equals(buffer, 0, 0);

    /* Insert into empty row at entry 0 */
    TEST_ASSERT_EQUAL_INT(0, csv_insert_field(buffer, 0, 0, "ONLY"));

    assert_row_width_equals(buffer, 0, 1);
    assert_field_equals(buffer, 0, 0, "ONLY");
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_csv_insert_field_inserts_at_end_when_row_exists_and_entry_is_beyond_current_width);
    RUN_TEST(test_csv_insert_field_inserts_at_new_row_by_appending_row_and_field);
    RUN_TEST(test_csv_insert_field_shifts_existing_fields_to_right_when_inserting_in_middle);
    RUN_TEST(test_csv_insert_field_shifts_all_fields_right_when_inserting_at_beginning);
    RUN_TEST(test_csv_insert_field_handles_empty_row_by_appending_field);

    return UNITY_END();
}