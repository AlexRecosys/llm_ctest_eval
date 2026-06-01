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
    /* Restore default signal handler */
    signal(SIGSEGV, SIG_DFL);
}

/* -----------------------------------------------------------------------
 * Test 1: csv_clear_row on the last (and only) row removes it entirely.
 * When the buffer has a single row and we clear it, the function calls
 * remove_last_row internally, so the buffer height drops to 0.
 * ----------------------------------------------------------------------- */
void test_csv_clear_row_last_row_is_removed(void)
{
    /* Create one row with two fields */
    int rc = csv_set_field(buffer, 0, 0, "hello");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, rc, "csv_set_field row0 field0 failed");
    rc = csv_set_field(buffer, 0, 1, "world");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, rc, "csv_set_field row0 field1 failed");

    TEST_ASSERT_EQUAL_INT_MESSAGE(1, csv_get_height(buffer), "Expected 1 row before clear");

    rc = csv_clear_row(buffer, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, rc, "csv_clear_row should return 0 on success");

    /* The last row is removed, so height should be 0 */
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, csv_get_height(buffer), "Height should be 0 after clearing last row");
}

/* -----------------------------------------------------------------------
 * Test 2: csv_clear_row on a non-last row reduces its width to 1 and
 * clears the remaining field.
 * ----------------------------------------------------------------------- */
void test_csv_clear_row_non_last_row_width_becomes_one(void)
{
    /* Create two rows so row 0 is NOT the last row */
    int rc = populate_buffer(buffer, 2, 3);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, rc, "populate_buffer failed");

    TEST_ASSERT_EQUAL_INT_MESSAGE(2, csv_get_height(buffer), "Expected 2 rows");
    TEST_ASSERT_EQUAL_INT_MESSAGE(3, csv_get_width(buffer, 0), "Expected width 3 for row 0");

    rc = csv_clear_row(buffer, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, rc, "csv_clear_row should return 0");

    /* Width of row 0 should now be 1 */
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, csv_get_width(buffer, 0), "Width of cleared row should be 1");

    /* Height should remain 2 */
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, csv_get_height(buffer), "Height should still be 2");
}

/* -----------------------------------------------------------------------
 * Test 3: After csv_clear_row on a non-last row, the remaining field
 * is empty (cleared to "\0").
 * ----------------------------------------------------------------------- */
void test_csv_clear_row_non_last_row_field_is_empty(void)
{
    /* Create two rows; row 0 has multiple fields */
    int rc = csv_set_field(buffer, 0, 0, "alpha");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, rc, "csv_set_field row0 field0 failed");
    rc = csv_set_field(buffer, 0, 1, "beta");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, rc, "csv_set_field row0 field1 failed");
    rc = csv_set_field(buffer, 0, 2, "gamma");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, rc, "csv_set_field row0 field2 failed");
    /* Create row 1 so row 0 is not the last */
    rc = csv_set_field(buffer, 1, 0, "second_row");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, rc, "csv_set_field row1 field0 failed");

    rc = csv_clear_row(buffer, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, rc, "csv_clear_row should return 0");

    /* The single remaining field should be empty */
    char dest[64];
    int get_rc = csv_get_field(dest, sizeof(dest), buffer, 0, 0);
    /* get_rc == 2 means the cell was empty, which is what we expect */
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, get_rc, "Remaining field after clear should be empty");
}

/* -----------------------------------------------------------------------
 * Test 4: csv_clear_row on a row that already has exactly one field
 * (non-last row) should still succeed and keep width at 1.
 * ----------------------------------------------------------------------- */
void test_csv_clear_row_single_field_non_last_row(void)
{
    /* Row 0 has one field, row 1 exists to make row 0 non-last */
    int rc = csv_set_field(buffer, 0, 0, "only_field");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, rc, "csv_set_field row0 field0 failed");
    rc = csv_set_field(buffer, 1, 0, "row1_field0");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, rc, "csv_set_field row1 field0 failed");

    TEST_ASSERT_EQUAL_INT_MESSAGE(1, csv_get_width(buffer, 0), "Row 0 should have width 1 before clear");

    rc = csv_clear_row(buffer, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, rc, "csv_clear_row on single-field non-last row should return 0");

    TEST_ASSERT_EQUAL_INT_MESSAGE(1, csv_get_width(buffer, 0), "Width should remain 1 after clearing single-field row");
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, csv_get_height(buffer), "Height should remain 2");
}

/* -----------------------------------------------------------------------
 * Test 5: csv_clear_row does not disturb other rows. After clearing
 * row 0 (non-last), row 1's content should be intact.
 * ----------------------------------------------------------------------- */
void test_csv_clear_row_does_not_disturb_other_rows(void)
{
    /* Set up two rows */
    int rc = csv_set_field(buffer, 0, 0, "r0f0");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, rc, "csv_set_field row0 field0 failed");
    rc = csv_set_field(buffer, 0, 1, "r0f1");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, rc, "csv_set_field row0 field1 failed");
    rc = csv_set_field(buffer, 1, 0, "r1f0");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, rc, "csv_set_field row1 field0 failed");
    rc = csv_set_field(buffer, 1, 1, "r1f1");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, rc, "csv_set_field row1 field1 failed");

    /* Clear row 0 (non-last) */
    rc = csv_clear_row(buffer, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, rc, "csv_clear_row should return 0");

    /* Row 1 should be untouched */
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, csv_get_width(buffer, 1), "Row 1 width should still be 2");

    char dest[64];
    int get_rc = csv_get_field(dest, sizeof(dest), buffer, 1, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, get_rc, "csv_get_field for row1 field0 should succeed");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("r1f0", dest, "Row 1 field 0 content should be intact");

    get_rc = csv_get_field(dest, sizeof(dest), buffer, 1, 1);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, get_rc, "csv_get_field for row1 field1 should succeed");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("r1f1", dest, "Row 1 field 1 content should be intact");
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_csv_clear_row_last_row_is_removed);
    RUN_TEST(test_csv_clear_row_non_last_row_width_becomes_one);
    RUN_TEST(test_csv_clear_row_non_last_row_field_is_empty);
    RUN_TEST(test_csv_clear_row_single_field_non_last_row);
    RUN_TEST(test_csv_clear_row_does_not_disturb_other_rows);
    return UNITY_END();
}