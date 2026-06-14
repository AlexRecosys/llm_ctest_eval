#include "csv.c"
#include "unity.h"
#include "csv.h"
#include <stdlib.h>
#include <string.h>
#include <signal.h>

/* File-scope buffer used across tests */
static CSV_BUFFER *g_buffer = NULL;

/* Signal handler for segmentation faults */
static void sigsegv_handler(int sig)
{
    (void)sig;
    TEST_FAIL_MESSAGE("Segmentation fault (SIGSEGV) caught during test execution");
}

/* Helper: build a buffer with `rows` rows, each having `cols` fields,
   filled with "RxCy" style strings. */
static void build_buffer(CSV_BUFFER *buf, int rows, int cols)
{
    int r, c;
    char text[32];
    for (r = 0; r < rows; r++) {
        for (c = 0; c < cols; c++) {
            snprintf(text, sizeof(text), "R%dC%d", r, c);
            csv_set_field(buf, (size_t)r, (size_t)c, text);
        }
    }
}

void setUp(void)
{
    signal(SIGSEGV, sigsegv_handler);
    g_buffer = csv_create_buffer();
    TEST_ASSERT_NOT_NULL_MESSAGE(g_buffer, "csv_create_buffer() returned NULL in setUp");
}

void tearDown(void)
{
    if (g_buffer != NULL) {
        csv_destroy_buffer(g_buffer);
        g_buffer = NULL;
    }
    signal(SIGSEGV, SIG_DFL);
}

/* -----------------------------------------------------------------------
 * Test 1: Clear the last row of a single-row buffer.
 * The function should call remove_last_row internally, leaving 0 rows.
 * ----------------------------------------------------------------------- */
void test_csv_clear_row_last_row_is_removed(void)
{
    /* Build a buffer with 1 row, 3 fields */
    build_buffer(g_buffer, 1, 3);

    TEST_ASSERT_EQUAL_INT_MESSAGE(1, csv_get_height(g_buffer),
        "Buffer should have 1 row before clear");

    int ret = csv_clear_row(g_buffer, 0);

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret,
        "csv_clear_row should return 0 on success");

    /* The last row is removed entirely, so height drops to 0 */
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, csv_get_height(g_buffer),
        "Buffer height should be 0 after clearing the only (last) row");
}

/* -----------------------------------------------------------------------
 * Test 2: Clear a non-last row in a multi-row buffer.
 * Row 0 of a 3-row buffer is cleared; it should become width=1 with
 * an empty field. Rows 1 and 2 should be untouched.
 * ----------------------------------------------------------------------- */
void test_csv_clear_row_non_last_row_becomes_width_one(void)
{
    /* Build 3 rows, 4 fields each */
    build_buffer(g_buffer, 3, 4);

    int ret = csv_clear_row(g_buffer, 0);

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret,
        "csv_clear_row should return 0 on success");

    /* Height unchanged */
    TEST_ASSERT_EQUAL_INT_MESSAGE(3, csv_get_height(g_buffer),
        "Buffer height should remain 3 after clearing row 0");

    /* Cleared row width should be 1 */
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, csv_get_width(g_buffer, 0),
        "Cleared row should have width 1");

    /* The single remaining field should be empty */
    char dest[64];
    int gret = csv_get_field(dest, sizeof(dest), g_buffer, 0, 0);
    /* Return value 2 means empty cell */
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, gret,
        "The cleared field should be reported as empty");
}

/* -----------------------------------------------------------------------
 * Test 3: Clear a middle row in a multi-row buffer.
 * Rows before and after the cleared row should retain their data.
 * ----------------------------------------------------------------------- */
void test_csv_clear_row_middle_row_neighbors_intact(void)
{
    /* Build 3 rows, 3 fields each */
    build_buffer(g_buffer, 3, 3);

    /* Clear the middle row (row 1) */
    int ret = csv_clear_row(g_buffer, 1);

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret,
        "csv_clear_row should return 0 on success");

    /* Row 0 field 0 should still be "R0C0" */
    char dest[64];
    csv_get_field(dest, sizeof(dest), g_buffer, 0, 0);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("R0C0", dest,
        "Row 0 field 0 should be untouched after clearing row 1");

    /* Row 2 field 2 should still be "R2C2" */
    csv_get_field(dest, sizeof(dest), g_buffer, 2, 2);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("R2C2", dest,
        "Row 2 field 2 should be untouched after clearing row 1");

    /* Cleared row 1 should have width 1 */
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, csv_get_width(g_buffer, 1),
        "Cleared middle row should have width 1");
}

/* -----------------------------------------------------------------------
 * Test 4: Clear a row that already has exactly one field.
 * The row is NOT the last row, so it should stay (width remains 1)
 * and the field should be cleared.
 * ----------------------------------------------------------------------- */
void test_csv_clear_row_single_field_row_not_last(void)
{
    /* Build 2 rows: row 0 has 1 field, row 1 has 2 fields */
    csv_set_field(g_buffer, 0, 0, "OnlyField");
    csv_set_field(g_buffer, 1, 0, "R1C0");
    csv_set_field(g_buffer, 1, 1, "R1C1");

    TEST_ASSERT_EQUAL_INT_MESSAGE(2, csv_get_height(g_buffer),
        "Buffer should have 2 rows before clear");
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, csv_get_width(g_buffer, 0),
        "Row 0 should have width 1 before clear");

    int ret = csv_clear_row(g_buffer, 0);

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret,
        "csv_clear_row should return 0 on success");

    /* Height unchanged */
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, csv_get_height(g_buffer),
        "Buffer height should remain 2");

    /* Width of row 0 should still be 1 */
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, csv_get_width(g_buffer, 0),
        "Row 0 width should remain 1 after clearing a single-field row");

    /* The field should now be empty */
    char dest[64];
    int gret = csv_get_field(dest, sizeof(dest), g_buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, gret,
        "The single field in row 0 should be empty after csv_clear_row");
}

/* -----------------------------------------------------------------------
 * Test 5: Clear the last row of a multi-row buffer (last row removal path).
 * With 3 rows, clearing row 2 (the last) should reduce height to 2
 * and leave rows 0 and 1 intact.
 * ----------------------------------------------------------------------- */
void test_csv_clear_row_last_row_of_multi_row_buffer(void)
{
    /* Build 3 rows, 2 fields each */
    build_buffer(g_buffer, 3, 2);

    int ret = csv_clear_row(g_buffer, 2);

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret,
        "csv_clear_row should return 0 when removing the last row");

    /* Height should now be 2 */
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, csv_get_height(g_buffer),
        "Buffer height should be 2 after removing the last row");

    /* Row 0 and row 1 should be intact */
    char dest[64];
    csv_get_field(dest, sizeof(dest), g_buffer, 0, 0);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("R0C0", dest,
        "Row 0 field 0 should be intact after removing last row");

    csv_get_field(dest, sizeof(dest), g_buffer, 1, 1);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("R1C1", dest,
        "Row 1 field 1 should be intact after removing last row");
}

/* -----------------------------------------------------------------------
 * main
 * ----------------------------------------------------------------------- */
int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_csv_clear_row_last_row_is_removed);
    RUN_TEST(test_csv_clear_row_non_last_row_becomes_width_one);
    RUN_TEST(test_csv_clear_row_middle_row_neighbors_intact);
    RUN_TEST(test_csv_clear_row_single_field_row_not_last);
    RUN_TEST(test_csv_clear_row_last_row_of_multi_row_buffer);
    return UNITY_END();
}