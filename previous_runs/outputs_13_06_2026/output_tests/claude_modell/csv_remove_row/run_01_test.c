#include "csv.c"
#include "unity.h"
#include "csv.h"
#include <stdlib.h>
#include <string.h>
#include <signal.h>

/* File-scope buffer used across tests */
static CSV_BUFFER *g_buffer = NULL;

/* Signal handler for SIGSEGV */
static void sigsegv_handler(int sig)
{
    (void)sig;
    TEST_FAIL_MESSAGE("Segmentation fault (SIGSEGV) caught during test execution");
}

/* Helper: populate a buffer with a given number of rows and columns */
static void populate_buffer(CSV_BUFFER *buf, int rows, int cols)
{
    int r, c;
    char text[64];
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
}

void tearDown(void)
{
    if (g_buffer != NULL) {
        csv_destroy_buffer(g_buffer);
        g_buffer = NULL;
    }
    signal(SIGSEGV, SIG_DFL);
}

/* Test 1: Remove the first row from a multi-row buffer.
 * After removal, the original row 1 becomes row 0, etc. */
void test_remove_first_row_shifts_remaining_rows(void)
{
    char dest[64];

    /* Create 3 rows, 2 columns each */
    populate_buffer(g_buffer, 3, 2);

    TEST_ASSERT_EQUAL_INT_MESSAGE(3, csv_get_height(g_buffer),
        "Buffer should have 3 rows before removal");

    int ret = csv_remove_row(g_buffer, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "csv_remove_row should return 0 on success");

    TEST_ASSERT_EQUAL_INT_MESSAGE(2, csv_get_height(g_buffer),
        "Buffer should have 2 rows after removing row 0");

    /* What was row 1 (R1C0) should now be row 0 */
    csv_get_field(dest, sizeof(dest), g_buffer, 0, 0);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("R1C0", dest,
        "After removing row 0, former row 1 col 0 should be at row 0");

    csv_get_field(dest, sizeof(dest), g_buffer, 0, 1);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("R1C1", dest,
        "After removing row 0, former row 1 col 1 should be at row 0");

    /* What was row 2 (R2C0) should now be row 1 */
    csv_get_field(dest, sizeof(dest), g_buffer, 1, 0);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("R2C0", dest,
        "After removing row 0, former row 2 col 0 should be at row 1");
}

/* Test 2: Remove the last row from a multi-row buffer. */
void test_remove_last_row_reduces_height(void)
{
    char dest[64];

    populate_buffer(g_buffer, 3, 2);

    int ret = csv_remove_row(g_buffer, 2);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "csv_remove_row should return 0 on success");

    TEST_ASSERT_EQUAL_INT_MESSAGE(2, csv_get_height(g_buffer),
        "Buffer should have 2 rows after removing last row");

    /* Row 0 and row 1 should be unchanged */
    csv_get_field(dest, sizeof(dest), g_buffer, 0, 0);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("R0C0", dest, "Row 0 col 0 should be unchanged");

    csv_get_field(dest, sizeof(dest), g_buffer, 1, 0);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("R1C0", dest, "Row 1 col 0 should be unchanged");
}

/* Test 3: Remove a middle row from a buffer with 4 rows. */
void test_remove_middle_row(void)
{
    char dest[64];

    populate_buffer(g_buffer, 4, 2);

    /* Remove row 1 (R1C0, R1C1) */
    int ret = csv_remove_row(g_buffer, 1);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "csv_remove_row should return 0 on success");

    TEST_ASSERT_EQUAL_INT_MESSAGE(3, csv_get_height(g_buffer),
        "Buffer should have 3 rows after removing middle row");

    /* Row 0 unchanged */
    csv_get_field(dest, sizeof(dest), g_buffer, 0, 0);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("R0C0", dest, "Row 0 col 0 should be unchanged");

    /* Former row 2 is now row 1 */
    csv_get_field(dest, sizeof(dest), g_buffer, 1, 0);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("R2C0", dest,
        "Former row 2 col 0 should now be at row 1");

    /* Former row 3 is now row 2 */
    csv_get_field(dest, sizeof(dest), g_buffer, 2, 0);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("R3C0", dest,
        "Former row 3 col 0 should now be at row 2");
}

/* Test 4: Attempt to remove a row index that is out of bounds.
 * The function returns 0 (no-op) and the buffer is unchanged. */
void test_remove_row_out_of_bounds_returns_zero_and_no_change(void)
{
    populate_buffer(g_buffer, 3, 2);

    /* Row index 3 is out of bounds for a 3-row buffer (valid: 0,1,2) */
    int ret = csv_remove_row(g_buffer, 3);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret,
        "csv_remove_row should return 0 even for out-of-bounds row");

    TEST_ASSERT_EQUAL_INT_MESSAGE(3, csv_get_height(g_buffer),
        "Buffer height should be unchanged after out-of-bounds removal");

    /* Verify data integrity */
    char dest[64];
    csv_get_field(dest, sizeof(dest), g_buffer, 0, 0);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("R0C0", dest, "Row 0 col 0 should be unchanged");

    csv_get_field(dest, sizeof(dest), g_buffer, 2, 0);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("R2C0", dest, "Row 2 col 0 should be unchanged");
}

/* Test 5: Remove the only row in a single-row buffer. */
void test_remove_only_row_leaves_empty_buffer(void)
{
    /* Create a single row with 2 fields */
    csv_set_field(g_buffer, 0, 0, "OnlyField0");
    csv_set_field(g_buffer, 0, 1, "OnlyField1");

    TEST_ASSERT_EQUAL_INT_MESSAGE(1, csv_get_height(g_buffer),
        "Buffer should have 1 row before removal");

    int ret = csv_remove_row(g_buffer, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "csv_remove_row should return 0 on success");

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, csv_get_height(g_buffer),
        "Buffer should have 0 rows after removing the only row");
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_remove_first_row_shifts_remaining_rows);
    RUN_TEST(test_remove_last_row_reduces_height);
    RUN_TEST(test_remove_middle_row);
    RUN_TEST(test_remove_row_out_of_bounds_returns_zero_and_no_change);
    RUN_TEST(test_remove_only_row_leaves_empty_buffer);
    return UNITY_END();
}