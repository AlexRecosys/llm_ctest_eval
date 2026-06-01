#include "csv.c"
#include "unity.h"
#include "csv.h"
#include <stdlib.h>
#include <string.h>
#include <signal.h>

/* File-scope buffer used across tests */
static CSV_BUFFER *buffer;

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
    /* Restore default SIGSEGV handler */
    signal(SIGSEGV, SIG_DFL);
}

/* -----------------------------------------------------------------------
 * Test 1: Clear the last row in a single-row buffer.
 * Expected: the row is removed entirely (buffer height becomes 0).
 * ----------------------------------------------------------------------- */
void test_csv_clear_row_last_row_is_removed(void)
{
    /* Create one row with two fields */
    int rc = csv_set_field(buffer, 0, 0, "hello");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, rc, "csv_set_field row0 field0 failed");
    rc = csv_set_field(buffer, 0, 1, "world");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, rc, "csv_set_field row0 field1 failed");

    TEST_ASSERT_EQUAL_INT_MESSAGE(1, csv_get_height(buffer), "Expected 1 row before clear");

    /* Row 0 is the last row, so csv_clear_row should call remove_last_row */
    rc = csv_clear_row(buffer, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, rc, "csv_clear_row on last row should return 0");

    /* Buffer should now have 0 rows */
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, csv_get_height(buffer), "Buffer height should be 0 after removing last row");
}

/* -----------------------------------------------------------------------
 * Test 2: Clear a non-last row that has multiple fields.
 * Expected: row width becomes 1, field content is cleared (empty string).
 * ----------------------------------------------------------------------- */
void test_csv_clear_row_non_last_row_width_becomes_one(void)
{
    /* Create two rows so row 0 is NOT the last */
    int rc;
    rc = csv_set_field(buffer, 0, 0, "alpha");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, rc, "csv_set_field row0 field0 failed");
    rc = csv_set_field(buffer, 0, 1, "beta");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, rc, "csv_set_field row0 field1 failed");
    rc = csv_set_field(buffer, 0, 2, "gamma");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, rc, "csv_set_field row0 field2 failed");
    rc = csv_set_field(buffer, 1, 0, "anchor");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, rc, "csv_set_field row1 field0 failed");

    TEST_ASSERT_EQUAL_INT_MESSAGE(2, csv_get_height(buffer), "Expected 2 rows");
    TEST_ASSERT_EQUAL_INT_MESSAGE(3, csv_get_width(buffer, 0), "Expected width 3 for row 0");

    rc = csv_clear_row(buffer, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, rc, "csv_clear_row should return 0");

    /* Width of row 0 should now be 1 */
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, csv_get_width(buffer, 0), "Row 0 width should be 1 after clear");

    /* The remaining field should be empty */
    char dest[64];
    int get_rc = csv_get_field(dest, sizeof(dest), buffer, 0, 0);
    /* Return code 2 means empty cell */
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, get_rc, "Cleared field should be empty (csv_get_field returns 2)");
}

/* -----------------------------------------------------------------------
 * Test 3: Clear a non-last row that already has exactly one field.
 * Expected: returns 0, width stays 1, field is cleared.
 * ----------------------------------------------------------------------- */
void test_csv_clear_row_single_field_row(void)
{
    /* Two rows: row 0 has one field, row 1 anchors it as non-last */
    int rc;
    rc = csv_set_field(buffer, 0, 0, "only_field");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, rc, "csv_set_field row0 field0 failed");
    rc = csv_set_field(buffer, 1, 0, "anchor");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, rc, "csv_set_field row1 field0 failed");

    TEST_ASSERT_EQUAL_INT_MESSAGE(1, csv_get_width(buffer, 0), "Row 0 should have width 1 before clear");

    rc = csv_clear_row(buffer, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, rc, "csv_clear_row on single-field non-last row should return 0");

    TEST_ASSERT_EQUAL_INT_MESSAGE(1, csv_get_width(buffer, 0), "Row 0 width should still be 1");

    char dest[64];
    int get_rc = csv_get_field(dest, sizeof(dest), buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, get_rc, "Field should be empty after clear");
}

/* -----------------------------------------------------------------------
 * Test 4: Clear a middle row in a multi-row buffer.
 * Expected: only the target row is affected; other rows remain intact.
 * ----------------------------------------------------------------------- */
void test_csv_clear_row_middle_row_others_unaffected(void)
{
    int rc;
    /* Row 0 */
    rc = csv_set_field(buffer, 0, 0, "r0f0");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, rc, "set row0 field0");
    rc = csv_set_field(buffer, 0, 1, "r0f1");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, rc, "set row0 field1");

    /* Row 1 — will be cleared */
    rc = csv_set_field(buffer, 1, 0, "r1f0");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, rc, "set row1 field0");
    rc = csv_set_field(buffer, 1, 1, "r1f1");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, rc, "set row1 field1");
    rc = csv_set_field(buffer, 1, 2, "r1f2");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, rc, "set row1 field2");

    /* Row 2 — anchor / last row */
    rc = csv_set_field(buffer, 2, 0, "r2f0");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, rc, "set row2 field0");

    TEST_ASSERT_EQUAL_INT_MESSAGE(3, csv_get_height(buffer), "Expected 3 rows");

    /* Clear middle row (row 1) */
    rc = csv_clear_row(buffer, 1);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, rc, "csv_clear_row on middle row should return 0");

    /* Row 1 width should be 1 */
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, csv_get_width(buffer, 1), "Row 1 width should be 1 after clear");

    /* Row 0 should be untouched */
    char dest[64];
    int get_rc = csv_get_field(dest, sizeof(dest), buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, get_rc, "Row 0 field 0 should still exist");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("r0f0", dest, "Row 0 field 0 content should be unchanged");

    get_rc = csv_get_field(dest, sizeof(dest), buffer, 0, 1);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, get_rc, "Row 0 field 1 should still exist");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("r0f1", dest, "Row 0 field 1 content should be unchanged");

    /* Row 2 should be untouched */
    get_rc = csv_get_field(dest, sizeof(dest), buffer, 2, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, get_rc, "Row 2 field 0 should still exist");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("r2f0", dest, "Row 2 field 0 content should be unchanged");

    /* Buffer height should still be 3 */
    TEST_ASSERT_EQUAL_INT_MESSAGE(3, csv_get_height(buffer), "Buffer height should remain 3");
}

/* -----------------------------------------------------------------------
 * Test 5: Clear the last row in a multi-row buffer.
 * Expected: buffer height decreases by 1, other rows remain intact.
 * ----------------------------------------------------------------------- */
void test_csv_clear_row_last_row_in_multi_row_buffer(void)
{
    int rc;
    /* Row 0 */
    rc = csv_set_field(buffer, 0, 0, "keep0");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, rc, "set row0 field0");
    rc = csv_set_field(buffer, 0, 1, "keep1");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, rc, "set row0 field1");

    /* Row 1 — will be cleared (it is the last row) */
    rc = csv_set_field(buffer, 1, 0, "gone0");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, rc, "set row1 field0");
    rc = csv_set_field(buffer, 1, 1, "gone1");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, rc, "set row1 field1");

    TEST_ASSERT_EQUAL_INT_MESSAGE(2, csv_get_height(buffer), "Expected 2 rows before clear");

    /* Clear last row (row 1) */
    rc = csv_clear_row(buffer, 1);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, rc, "csv_clear_row on last row should return 0");

    /* Buffer height should now be 1 */
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, csv_get_height(buffer), "Buffer height should be 1 after removing last row");

    /* Row 0 should be untouched */
    char dest[64];
    int get_rc = csv_get_field(dest, sizeof(dest), buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, get_rc, "Row 0 field 0 should still exist");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("keep0", dest, "Row 0 field 0 content should be unchanged");

    get_rc = csv_get_field(dest, sizeof(dest), buffer, 0, 1);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, get_rc, "Row 0 field 1 should still exist");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("keep1", dest, "Row 0 field 1 content should be unchanged");
}

/* -----------------------------------------------------------------------
 * main
 * ----------------------------------------------------------------------- */
int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_csv_clear_row_last_row_is_removed);
    RUN_TEST(test_csv_clear_row_non_last_row_width_becomes_one);
    RUN_TEST(test_csv_clear_row_single_field_row);
    RUN_TEST(test_csv_clear_row_middle_row_others_unaffected);
    RUN_TEST(test_csv_clear_row_last_row_in_multi_row_buffer);
    return UNITY_END();
}