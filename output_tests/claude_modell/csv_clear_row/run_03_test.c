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
static int populate_buffer(CSV_BUFFER *buf, size_t num_rows, size_t fields_per_row)
{
    size_t r, f;
    char text[64];
    for (r = 0; r < num_rows; r++) {
        for (f = 0; f < fields_per_row; f++) {
            snprintf(text, sizeof(text), "row%zu_field%zu", r, f);
            if (csv_set_field(buf, r, f, text) != 0)
                return -1;
        }
    }
    return 0;
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

/* Test 1: Clear a row that is NOT the last row, with multiple fields.
 * After clearing, the row should have width 1 and the single field should be empty. */
void test_csv_clear_row_middle_row_multiple_fields(void)
{
    /* Create 3 rows, each with 4 fields */
    int ret = populate_buffer(buffer, 3, 4);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "populate_buffer failed");

    /* Verify initial state */
    TEST_ASSERT_EQUAL_INT(3, csv_get_height(buffer));
    TEST_ASSERT_EQUAL_INT(4, csv_get_width(buffer, 0));
    TEST_ASSERT_EQUAL_INT(4, csv_get_width(buffer, 1));
    TEST_ASSERT_EQUAL_INT(4, csv_get_width(buffer, 2));

    /* Clear row 1 (middle row) */
    int result = csv_clear_row(buffer, 1);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "csv_clear_row should return 0 on success");

    /* Row count should remain 3 */
    TEST_ASSERT_EQUAL_INT_MESSAGE(3, csv_get_height(buffer), "Row count should remain 3 after clear");

    /* Row 1 should now have width 1 */
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, csv_get_width(buffer, 1), "Cleared row should have width 1");

    /* The single remaining field should be empty */
    char dest[64];
    int get_ret = csv_get_field(dest, sizeof(dest), buffer, 1, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, get_ret, "Cleared field should be empty (return 2)");

    /* Other rows should be untouched */
    TEST_ASSERT_EQUAL_INT_MESSAGE(4, csv_get_width(buffer, 0), "Row 0 width should be unchanged");
    TEST_ASSERT_EQUAL_INT_MESSAGE(4, csv_get_width(buffer, 2), "Row 2 width should be unchanged");
}

/* Test 2: Clear the last row in the buffer.
 * The function should call remove_last_row, reducing the row count by 1. */
void test_csv_clear_row_last_row_is_removed(void)
{
    /* Create 3 rows, each with 3 fields */
    int ret = populate_buffer(buffer, 3, 3);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "populate_buffer failed");

    TEST_ASSERT_EQUAL_INT(3, csv_get_height(buffer));

    /* Clear the last row (row index 2) */
    int result = csv_clear_row(buffer, 2);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "csv_clear_row on last row should return 0");

    /* Row count should decrease by 1 */
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, csv_get_height(buffer), "Height should decrease by 1 after clearing last row");

    /* Remaining rows should be intact */
    TEST_ASSERT_EQUAL_INT_MESSAGE(3, csv_get_width(buffer, 0), "Row 0 should be unchanged");
    TEST_ASSERT_EQUAL_INT_MESSAGE(3, csv_get_width(buffer, 1), "Row 1 should be unchanged");
}

/* Test 3: Clear a row that already has exactly one field.
 * The row should remain with width 1 and the field should be cleared. */
void test_csv_clear_row_single_field_row(void)
{
    /* Create 2 rows: row 0 with 1 field, row 1 with 3 fields */
    int ret0 = csv_set_field(buffer, 0, 0, "only_field");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret0, "csv_set_field row 0 failed");

    int ret1 = populate_buffer(buffer, 2, 3);
    /* row 0 already exists with 1 field; set_field for row 1 */
    (void)ret1;

    /* Ensure row 0 has width 1 */
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, csv_get_width(buffer, 0), "Row 0 should have width 1");

    /* Clear row 0 (not the last row) */
    int result = csv_clear_row(buffer, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "csv_clear_row on single-field row should return 0");

    /* Row count unchanged */
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, csv_get_height(buffer), "Height should remain 2");

    /* Row 0 should still have width 1 */
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, csv_get_width(buffer, 0), "Row 0 should still have width 1");

    /* The field should now be empty */
    char dest[64];
    int get_ret = csv_get_field(dest, sizeof(dest), buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, get_ret, "Field should be empty after clear");
}

/* Test 4: Clear the first row (row 0) when there are multiple rows.
 * Row 0 is not the last row, so it should be cleared (not removed). */
void test_csv_clear_row_first_row_not_removed(void)
{
    /* Create 4 rows, each with 5 fields */
    int ret = populate_buffer(buffer, 4, 5);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "populate_buffer failed");

    TEST_ASSERT_EQUAL_INT(4, csv_get_height(buffer));

    /* Clear row 0 */
    int result = csv_clear_row(buffer, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "csv_clear_row on row 0 should return 0");

    /* Height should remain 4 */
    TEST_ASSERT_EQUAL_INT_MESSAGE(4, csv_get_height(buffer), "Height should remain 4");

    /* Row 0 should have width 1 */
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, csv_get_width(buffer, 0), "Row 0 should have width 1 after clear");

    /* The single field in row 0 should be empty */
    char dest[64];
    int get_ret = csv_get_field(dest, sizeof(dest), buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, get_ret, "Row 0 field 0 should be empty after clear");

    /* Other rows should be untouched */
    TEST_ASSERT_EQUAL_INT_MESSAGE(5, csv_get_width(buffer, 1), "Row 1 width should be unchanged");
    TEST_ASSERT_EQUAL_INT_MESSAGE(5, csv_get_width(buffer, 2), "Row 2 width should be unchanged");
    TEST_ASSERT_EQUAL_INT_MESSAGE(5, csv_get_width(buffer, 3), "Row 3 width should be unchanged");

    /* Verify content of another row is intact */
    char expected[64];
    snprintf(expected, sizeof(expected), "row1_field2");
    int get_ret2 = csv_get_field(dest, sizeof(dest), buffer, 1, 2);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, get_ret2, "Row 1 field 2 should be intact");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(expected, dest, "Row 1 field 2 content should be unchanged");
}

/* Test 5: Clear a row with many fields to verify all fields are destroyed
 * and only one empty field remains. Also verify the buffer is still usable
 * after the operation (can set fields again). */
void test_csv_clear_row_many_fields_then_reuse(void)
{
    /* Create 2 rows: row 0 with 10 fields, row 1 with 2 fields */
    size_t r, f;
    char text[64];
    for (f = 0; f < 10; f++) {
        snprintf(text, sizeof(text), "data_%zu", f);
        int sr = csv_set_field(buffer, 0, f, text);
        TEST_ASSERT_EQUAL_INT_MESSAGE(0, sr, "csv_set_field row 0 failed");
    }
    for (f = 0; f < 2; f++) {
        snprintf(text, sizeof(text), "other_%zu", f);
        int sr = csv_set_field(buffer, 1, f, text);
        TEST_ASSERT_EQUAL_INT_MESSAGE(0, sr, "csv_set_field row 1 failed");
    }
    (void)r;

    TEST_ASSERT_EQUAL_INT_MESSAGE(10, csv_get_width(buffer, 0), "Row 0 should have 10 fields");
    TEST_ASSERT_EQUAL_INT_MESSAGE(2,  csv_get_width(buffer, 1), "Row 1 should have 2 fields");

    /* Clear row 0 (not the last row) */
    int result = csv_clear_row(buffer, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "csv_clear_row should return 0");

    /* Row 0 should now have width 1 */
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, csv_get_width(buffer, 0), "Row 0 should have width 1 after clear");

    /* The single field should be empty */
    char dest[128];
    int get_ret = csv_get_field(dest, sizeof(dest), buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, get_ret, "Row 0 field 0 should be empty");

    /* Row 1 should be untouched */
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, csv_get_width(buffer, 1), "Row 1 width should be unchanged");

    /* Buffer should still be usable: set a new value in row 0 */
    int set_ret = csv_set_field(buffer, 0, 0, "new_value");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, set_ret, "Should be able to set field in cleared row");

    int get_ret2 = csv_get_field(dest, sizeof(dest), buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, get_ret2, "Should retrieve new value successfully");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("new_value", dest, "New value should match");
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_csv_clear_row_middle_row_multiple_fields);
    RUN_TEST(test_csv_clear_row_last_row_is_removed);
    RUN_TEST(test_csv_clear_row_single_field_row);
    RUN_TEST(test_csv_clear_row_first_row_not_removed);
    RUN_TEST(test_csv_clear_row_many_fields_then_reuse);
    return UNITY_END();
}