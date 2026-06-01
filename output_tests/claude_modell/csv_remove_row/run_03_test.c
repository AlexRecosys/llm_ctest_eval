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

/* Helper: populate a buffer with a given number of rows and columns.
 * Each cell is set to "row<r>col<c>" style text. */
static void populate_buffer(CSV_BUFFER *buf, int rows, int cols)
{
    char text[64];
    int r, c;
    for (r = 0; r < rows; r++) {
        for (c = 0; c < cols; c++) {
            snprintf(text, sizeof(text), "r%dc%d", r, c);
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

/* Test 1: Remove the first row from a multi-row buffer.
 * After removal, the original second row should become the first row. */
void test_csv_remove_row_removes_first_row(void)
{
    /* Build a 3-row, 2-column buffer */
    populate_buffer(g_buffer, 3, 2);

    TEST_ASSERT_EQUAL_INT_MESSAGE(3, csv_get_height(g_buffer),
        "Buffer should have 3 rows before removal");

    int ret = csv_remove_row(g_buffer, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "csv_remove_row should return 0 on success");

    /* Height should now be 2 */
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, csv_get_height(g_buffer),
        "Buffer should have 2 rows after removing first row");

    /* What was row 1 ("r1c0") should now be row 0 */
    char dest[64];
    csv_get_field(dest, sizeof(dest), g_buffer, 0, 0);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("r1c0", dest,
        "After removing row 0, old row 1 should be new row 0");

    /* What was row 2 ("r2c0") should now be row 1 */
    csv_get_field(dest, sizeof(dest), g_buffer, 1, 0);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("r2c0", dest,
        "After removing row 0, old row 2 should be new row 1");
}

/* Test 2: Remove the last row from a multi-row buffer. */
void test_csv_remove_row_removes_last_row(void)
{
    populate_buffer(g_buffer, 3, 2);

    TEST_ASSERT_EQUAL_INT_MESSAGE(3, csv_get_height(g_buffer),
        "Buffer should have 3 rows before removal");

    int ret = csv_remove_row(g_buffer, 2);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "csv_remove_row should return 0 on success");

    TEST_ASSERT_EQUAL_INT_MESSAGE(2, csv_get_height(g_buffer),
        "Buffer should have 2 rows after removing last row");

    /* Rows 0 and 1 should be unchanged */
    char dest[64];
    csv_get_field(dest, sizeof(dest), g_buffer, 0, 0);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("r0c0", dest,
        "Row 0 should be unchanged after removing last row");

    csv_get_field(dest, sizeof(dest), g_buffer, 1, 0);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("r1c0", dest,
        "Row 1 should be unchanged after removing last row");
}

/* Test 3: Remove a middle row from a multi-row buffer. */
void test_csv_remove_row_removes_middle_row(void)
{
    populate_buffer(g_buffer, 4, 2);

    TEST_ASSERT_EQUAL_INT_MESSAGE(4, csv_get_height(g_buffer),
        "Buffer should have 4 rows before removal");

    /* Remove row 1 (middle) */
    int ret = csv_remove_row(g_buffer, 1);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "csv_remove_row should return 0 on success");

    TEST_ASSERT_EQUAL_INT_MESSAGE(3, csv_get_height(g_buffer),
        "Buffer should have 3 rows after removing middle row");

    char dest[64];

    /* Row 0 unchanged */
    csv_get_field(dest, sizeof(dest), g_buffer, 0, 0);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("r0c0", dest,
        "Row 0 should be unchanged");

    /* Old row 2 is now row 1 */
    csv_get_field(dest, sizeof(dest), g_buffer, 1, 0);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("r2c0", dest,
        "Old row 2 should now be row 1 after removing row 1");

    /* Old row 3 is now row 2 */
    csv_get_field(dest, sizeof(dest), g_buffer, 2, 0);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("r3c0", dest,
        "Old row 3 should now be row 2 after removing row 1");
}

/* Test 4: Attempt to remove a row index that is out of bounds.
 * The function should return 0 (as per implementation) and leave
 * the buffer unchanged. */
void test_csv_remove_row_out_of_bounds_index(void)
{
    populate_buffer(g_buffer, 2, 2);

    TEST_ASSERT_EQUAL_INT_MESSAGE(2, csv_get_height(g_buffer),
        "Buffer should have 2 rows before out-of-bounds removal attempt");

    /* Row index 5 does not exist */
    int ret = csv_remove_row(g_buffer, 5);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret,
        "csv_remove_row should return 0 even for out-of-bounds row");

    /* Buffer should be unchanged */
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, csv_get_height(g_buffer),
        "Buffer height should be unchanged after out-of-bounds removal");

    char dest[64];
    csv_get_field(dest, sizeof(dest), g_buffer, 0, 0);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("r0c0", dest,
        "Row 0 should be unchanged after out-of-bounds removal");

    csv_get_field(dest, sizeof(dest), g_buffer, 1, 0);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("r1c0", dest,
        "Row 1 should be unchanged after out-of-bounds removal");
}

/* Test 5: Remove the only row in a single-row buffer.
 * After removal the buffer should have 0 rows. */
void test_csv_remove_row_single_row_buffer(void)
{
    /* Create a single row with 2 fields */
    csv_set_field(g_buffer, 0, 0, "only_field_0");
    csv_set_field(g_buffer, 0, 1, "only_field_1");

    TEST_ASSERT_EQUAL_INT_MESSAGE(1, csv_get_height(g_buffer),
        "Buffer should have exactly 1 row before removal");

    int ret = csv_remove_row(g_buffer, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret,
        "csv_remove_row should return 0 when removing the only row");

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, csv_get_height(g_buffer),
        "Buffer should have 0 rows after removing the only row");
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_csv_remove_row_removes_first_row);
    RUN_TEST(test_csv_remove_row_removes_last_row);
    RUN_TEST(test_csv_remove_row_removes_middle_row);
    RUN_TEST(test_csv_remove_row_out_of_bounds_index);
    RUN_TEST(test_csv_remove_row_single_row_buffer);
    return UNITY_END();
}