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

void setUp(void) {
    buffer = csv_create_buffer();
    TEST_ASSERT_NOT_NULL(buffer);
}

void tearDown(void) {
    csv_destroy_buffer(buffer);
    buffer = NULL;
}

/* Test Case 1: Insert at existing position (middle of row) */
void test_csv_insert_field_insert_in_middle_of_row(void) {
    size_t widths[] = {3};
    init_buffer_with_rows_and_widths(buffer, 1, widths);

    /* Before: ["R0C0", "R0C1", "R0C2"] */
    char inserted[] = "INSERTED";
    int result = csv_insert_field(buffer, 0, 1, inserted);
    TEST_ASSERT_EQUAL_INT(0, result);

    /* After: ["R0C0", "INSERTED", "R0C1", "R0C2"] */
    TEST_ASSERT_EQUAL_UINT(4, buffer->width[0]);

    char dest[64] = {0};
    int status;

    status = csv_get_field(dest, sizeof(dest), buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT(0, status);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("R0C0", dest, "Field 0 should be unchanged");

    status = csv_get_field(dest, sizeof(dest), buffer, 0, 1);
    TEST_ASSERT_EQUAL_INT(0, status);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("INSERTED", dest, "Inserted field should be at position 1");

    status = csv_get_field(dest, sizeof(dest), buffer, 0, 2);
    TEST_ASSERT_EQUAL_INT(0, status);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("R0C1", dest, "Field 2 should be previous field 1");

    status = csv_get_field(dest, sizeof(dest), buffer, 0, 3);
    TEST_ASSERT_EQUAL_INT(0, status);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("R0C2", dest, "Field 3 should be previous field 2");
}

/* Test Case 2: Insert at end of row (should behave like append) */
void test_csv_insert_field_insert_at_end_of_row(void) {
    size_t widths[] = {2};
    init_buffer_with_rows_and_widths(buffer, 1, widths);

    /* Before: ["R0C0", "R0C1"] */
    char inserted[] = "NEW_END";
    int result = csv_insert_field(buffer, 0, 2, inserted);
    TEST_ASSERT_EQUAL_INT(0, result);

    /* After: ["R0C0", "R0C1", "NEW_END"] */
    TEST_ASSERT_EQUAL_UINT(3, buffer->width[0]);

    char dest[64] = {0};
    int status;

    status = csv_get_field(dest, sizeof(dest), buffer, 0, 2);
    TEST_ASSERT_EQUAL_INT(0, status);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("NEW_END", dest, "Inserted field should be at end");
}

/* Test Case 3: Insert beyond row bounds (should call csv_set_field, i.e., expand row) */
void test_csv_insert_field_beyond_row_bounds_expands_row(void) {
    size_t widths[] = {1};
    init_buffer_with_rows_and_widths(buffer, 1, widths);

    /* Before: ["R0C0"] */
    char inserted[] = "FAR_END";
    int result = csv_insert_field(buffer, 0, 5, inserted);
    TEST_ASSERT_EQUAL_INT(0, result);

    /* After: ["R0C0", "", "", "", "", "FAR_END"] (5 empty fields + inserted) */
    TEST_ASSERT_EQUAL_UINT(6, buffer->width[0]);

    char dest[64] = {0};
    int status;

    status = csv_get_field(dest, sizeof(dest), buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT(0, status);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("R0C0", dest, "Original field preserved");

    status = csv_get_field(dest, sizeof(dest), buffer, 0, 5);
    TEST_ASSERT_EQUAL_INT(0, status);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("FAR_END", dest, "Inserted field at index 5");

    /* Check intermediate fields are empty */
    for (size_t i = 1; i < 5; i++) {
        status = csv_get_field(dest, sizeof(dest), buffer, 0, i);
        TEST_ASSERT_EQUAL_INT(2, status, "Intermediate fields should be empty");
    }
}

/* Test Case 4: Insert at row 0, entry 0 (shift all fields right) */
void test_csv_insert_field_at_start_of_row(void) {
    size_t widths[] = {2};
    init_buffer_with_rows_and_widths(buffer, 1, widths);

    /* Before: ["R0C0", "R0C1"] */
    char inserted[] = "FIRST";
    int result = csv_insert_field(buffer, 0, 0, inserted);
    TEST_ASSERT_EQUAL_INT(0, result);

    /* After: ["FIRST", "R0C0", "R0C1"] */
    TEST_ASSERT_EQUAL_UINT(3, buffer->width[0]);

    char dest[64] = {0};
    int status;

    status = csv_get_field(dest, sizeof(dest), buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT(0, status);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("FIRST", dest, "Inserted field at position 0");

    status = csv_get_field(dest, sizeof(dest), buffer, 0, 1);
    TEST_ASSERT_EQUAL_INT(0, status);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("R0C0", dest, "Previous field 0 shifted to 1");

    status = csv_get_field(dest, sizeof(dest), buffer, 0, 2);
    TEST_ASSERT_EQUAL_INT(0, status);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("R0C1", dest, "Previous field 1 shifted to 2");
}

/* Test Case 5: Insert into non-existent row (should create row and insert) */
void test_csv_insert_field_into_nonexistent_row_creates_row(void) {
    size_t widths[] = {1};
    init_buffer_with_rows_and_widths(buffer, 1, widths);

    /* buffer has 1 row (row 0), insert into row 1 */
    char inserted[] = "NEW_ROW_FIELD";
    int result = csv_insert_field(buffer, 1, 0, inserted);
    TEST_ASSERT_EQUAL_INT(0, result);

    /* Row 1 should now exist with width 1 */
    TEST_ASSERT_EQUAL_UINT(2, buffer->rows);
    TEST_ASSERT_EQUAL_UINT(1, buffer->width[1]);

    char dest[64] = {0};
    int status = csv_get_field(dest, sizeof(dest), buffer, 1, 0);
    TEST_ASSERT_EQUAL_INT(0, status);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("NEW_ROW_FIELD", dest, "Field inserted into new row");
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_csv_insert_field_insert_in_middle_of_row);
    RUN_TEST(test_csv_insert_field_insert_at_end_of_row);
    RUN_TEST(test_csv_insert_field_beyond_row_bounds_expands_row);
    RUN_TEST(test_csv_insert_field_at_start_of_row);
    RUN_TEST(test_csv_insert_field_into_nonexistent_row_creates_row);

    return UNITY_END();
}