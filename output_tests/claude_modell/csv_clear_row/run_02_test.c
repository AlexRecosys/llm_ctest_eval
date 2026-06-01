#include "csv.c"
#include "unity.h"
#include "csv.h"
#include <stdlib.h>
#include <string.h>
#include <signal.h>

/* File-scope buffer used across tests */
static CSV_BUFFER *buffer = NULL;

/* Signal handler for segmentation faults */
static void sigsegv_handler(int sig)
{
    (void)sig;
    TEST_FAIL_MESSAGE("Segmentation fault (SIGSEGV) caught during test execution");
}

/* Helper: populate a buffer with a given number of rows and fields per row */
static void populate_buffer(CSV_BUFFER *buf, size_t num_rows, size_t fields_per_row)
{
    size_t r, f;
    char text[64];
    for (r = 0; r < num_rows; r++) {
        for (f = 0; f < fields_per_row; f++) {
            snprintf(text, sizeof(text), "row%zu_field%zu", r, f);
            csv_set_field(buf, r, f, text);
        }
    }
}

void setUp(void)
{
    signal(SIGSEGV, sigsegv_handler);
    buffer = csv_create_buffer();
    TEST_ASSERT_NOT_NULL_MESSAGE(buffer, "csv_create_buffer() returned NULL in setUp");
}

void tearDown(void)
{
    if (buffer != NULL) {
        csv_destroy_buffer(buffer);
        buffer = NULL;
    }
    signal(SIGSEGV, SIG_DFL);
}

/* Test 1: Clear a row that is the last row in the buffer.
 * According to the implementation, if row == buffer->rows-1,
 * remove_last_row is called, reducing the row count by 1. */
void test_csv_clear_row_last_row_removes_it(void)
{
    /* Create two rows with some fields */
    populate_buffer(buffer, 2, 3);

    int height_before = csv_get_height(buffer);
    TEST_ASSERT_EQUAL_INT(2, height_before);

    /* Clear the last row (row index 1) */
    int result = csv_clear_row(buffer, 1);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "csv_clear_row on last row should return 0");

    /* Height should decrease by 1 */
    int height_after = csv_get_height(buffer);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, height_after, "Height should be 1 after clearing last row");
}

/* Test 2: Clear a non-last row with multiple fields.
 * The row should be reduced to width 1 and the single remaining field cleared. */
void test_csv_clear_row_non_last_row_reduces_width(void)
{
    /* Create three rows, each with 4 fields */
    populate_buffer(buffer, 3, 4);

    int width_before = csv_get_width(buffer, 0);
    TEST_ASSERT_EQUAL_INT(4, width_before);

    /* Clear row 0 (not the last row) */
    int result = csv_clear_row(buffer, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "csv_clear_row on non-last row should return 0");

    /* Width of row 0 should now be 1 */
    int width_after = csv_get_width(buffer, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, width_after, "Width of cleared row should be 1");

    /* The remaining field should be empty (cleared) */
    char dest[64];
    int get_result = csv_get_field(dest, sizeof(dest), buffer, 0, 0);
    /* get_result == 2 means the cell was empty */
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, get_result, "Cleared field should be empty");
}

/* Test 3: Clear a row that already has exactly one field.
 * The row should remain with width 1 and the field should be cleared. */
void test_csv_clear_row_single_field_row(void)
{
    /* Create two rows: row 0 with 1 field, row 1 with 2 fields (so row 0 is not last) */
    csv_set_field(buffer, 0, 0, "only_field");
    csv_set_field(buffer, 1, 0, "other_row_field0");
    csv_set_field(buffer, 1, 1, "other_row_field1");

    int width_before = csv_get_width(buffer, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, width_before, "Row 0 should have width 1 before clear");

    int result = csv_clear_row(buffer, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "csv_clear_row on single-field row should return 0");

    int width_after = csv_get_width(buffer, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, width_after, "Width should remain 1 after clearing single-field row");

    /* The field should now be empty */
    char dest[64];
    int get_result = csv_get_field(dest, sizeof(dest), buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, get_result, "Single field should be empty after clear");
}

/* Test 4: Clear a middle row in a buffer with many rows.
 * Verify that other rows are unaffected. */
void test_csv_clear_row_middle_row_other_rows_intact(void)
{
    /* Create 3 rows with 3 fields each */
    populate_buffer(buffer, 3, 3);

    /* Clear the middle row (row 1) */
    int result = csv_clear_row(buffer, 1);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "csv_clear_row on middle row should return 0");

    /* Row 1 should have width 1 and be empty */
    int width_row1 = csv_get_width(buffer, 1);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, width_row1, "Middle row width should be 1 after clear");

    char dest[64];
    int get_result = csv_get_field(dest, sizeof(dest), buffer, 1, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, get_result, "Middle row field should be empty after clear");

    /* Row 0 should be untouched: width 3 */
    int width_row0 = csv_get_width(buffer, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(3, width_row0, "Row 0 width should be unchanged");

    /* Row 0, field 0 should still have its original value */
    int get_r0f0 = csv_get_field(dest, sizeof(dest), buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, get_r0f0, "Row 0 field 0 should still be accessible");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("row0_field0", dest, "Row 0 field 0 content should be unchanged");

    /* Row 2 should be untouched: width 3 */
    int width_row2 = csv_get_width(buffer, 2);
    TEST_ASSERT_EQUAL_INT_MESSAGE(3, width_row2, "Row 2 width should be unchanged");

    int get_r2f2 = csv_get_field(dest, sizeof(dest), buffer, 2, 2);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, get_r2f2, "Row 2 field 2 should still be accessible");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("row2_field2", dest, "Row 2 field 2 content should be unchanged");
}

/* Test 5: Clear the only row in the buffer (row 0, which is also the last row).
 * Since row == buffer->rows-1, remove_last_row is called, leaving 0 rows. */
void test_csv_clear_row_only_row_in_buffer(void)
{
    /* Create a single row with multiple fields */
    csv_set_field(buffer, 0, 0, "field_a");
    csv_set_field(buffer, 0, 1, "field_b");
    csv_set_field(buffer, 0, 2, "field_c");

    int height_before = csv_get_height(buffer);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, height_before, "Buffer should have 1 row before clear");

    /* Clear row 0 — it is the last (and only) row */
    int result = csv_clear_row(buffer, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "csv_clear_row on only row should return 0");

    /* Buffer should now have 0 rows */
    int height_after = csv_get_height(buffer);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, height_after, "Buffer should have 0 rows after clearing only row");
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_csv_clear_row_last_row_removes_it);
    RUN_TEST(test_csv_clear_row_non_last_row_reduces_width);
    RUN_TEST(test_csv_clear_row_single_field_row);
    RUN_TEST(test_csv_clear_row_middle_row_other_rows_intact);
    RUN_TEST(test_csv_clear_row_only_row_in_buffer);
    return UNITY_END();
}