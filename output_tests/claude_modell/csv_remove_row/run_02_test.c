#include "csv.c"
#include "unity.h"
#include "csv.h"
#include <stdlib.h>
#include <string.h>
#include <signal.h>

/* File-scope fixture */
static CSV_BUFFER *buf = NULL;

/* Signal handler for segfaults */
static void sigsegv_handler(int sig)
{
    (void)sig;
    TEST_FAIL_MESSAGE("Segmentation fault (SIGSEGV) caught during test");
}

/* Helper: populate a buffer with N rows, each having one field */
static void populate_buffer(CSV_BUFFER *b, int num_rows)
{
    int i;
    char text[32];
    for (i = 0; i < num_rows; i++) {
        snprintf(text, sizeof(text), "row%d", i);
        csv_set_field(b, (size_t)i, 0, text);
    }
}

/* Helper: read a field string from the buffer */
static void get_field_str(CSV_BUFFER *b, size_t row, size_t entry,
                           char *out, size_t out_len)
{
    csv_get_field(out, out_len, b, row, entry);
}

void setUp(void)
{
    signal(SIGSEGV, sigsegv_handler);
    buf = csv_create_buffer();
    TEST_ASSERT_NOT_NULL_MESSAGE(buf, "csv_create_buffer returned NULL in setUp");
}

void tearDown(void)
{
    if (buf != NULL) {
        csv_destroy_buffer(buf);
        buf = NULL;
    }
    signal(SIGSEGV, SIG_DFL);
}

/* -----------------------------------------------------------------------
 * Test 1: Remove the only row in a single-row buffer.
 * After removal the buffer should have 0 rows.
 * ----------------------------------------------------------------------- */
void test_remove_only_row(void)
{
    /* Create one row with one field */
    csv_set_field(buf, 0, 0, "only");
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, csv_get_height(buf),
        "Buffer should have 1 row before removal");

    int ret = csv_remove_row(buf, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "csv_remove_row should return 0");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, csv_get_height(buf),
        "Buffer should have 0 rows after removing the only row");
}

/* -----------------------------------------------------------------------
 * Test 2: Remove the first row of a multi-row buffer.
 * Remaining rows should shift up correctly.
 * ----------------------------------------------------------------------- */
void test_remove_first_row(void)
{
    /* Populate 3 rows: "row0", "row1", "row2" */
    populate_buffer(buf, 3);
    TEST_ASSERT_EQUAL_INT(3, csv_get_height(buf));

    int ret = csv_remove_row(buf, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "csv_remove_row should return 0");
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, csv_get_height(buf),
        "Buffer should have 2 rows after removing first row");

    char field[64];
    get_field_str(buf, 0, 0, field, sizeof(field));
    TEST_ASSERT_EQUAL_STRING_MESSAGE("row1", field,
        "After removing row0, new row0 should contain 'row1'");

    get_field_str(buf, 1, 0, field, sizeof(field));
    TEST_ASSERT_EQUAL_STRING_MESSAGE("row2", field,
        "After removing row0, new row1 should contain 'row2'");
}

/* -----------------------------------------------------------------------
 * Test 3: Remove the last row of a multi-row buffer.
 * Preceding rows should be unaffected.
 * ----------------------------------------------------------------------- */
void test_remove_last_row(void)
{
    populate_buffer(buf, 3);
    TEST_ASSERT_EQUAL_INT(3, csv_get_height(buf));

    int ret = csv_remove_row(buf, 2);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "csv_remove_row should return 0");
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, csv_get_height(buf),
        "Buffer should have 2 rows after removing last row");

    char field[64];
    get_field_str(buf, 0, 0, field, sizeof(field));
    TEST_ASSERT_EQUAL_STRING_MESSAGE("row0", field,
        "row0 should be unchanged after removing last row");

    get_field_str(buf, 1, 0, field, sizeof(field));
    TEST_ASSERT_EQUAL_STRING_MESSAGE("row1", field,
        "row1 should be unchanged after removing last row");
}

/* -----------------------------------------------------------------------
 * Test 4: Remove a middle row of a multi-row buffer.
 * Rows after the removed one should shift up.
 * ----------------------------------------------------------------------- */
void test_remove_middle_row(void)
{
    populate_buffer(buf, 4);
    TEST_ASSERT_EQUAL_INT(4, csv_get_height(buf));

    /* Remove row index 1 ("row1") */
    int ret = csv_remove_row(buf, 1);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "csv_remove_row should return 0");
    TEST_ASSERT_EQUAL_INT_MESSAGE(3, csv_get_height(buf),
        "Buffer should have 3 rows after removing middle row");

    char field[64];
    get_field_str(buf, 0, 0, field, sizeof(field));
    TEST_ASSERT_EQUAL_STRING_MESSAGE("row0", field,
        "row0 should be unchanged");

    get_field_str(buf, 1, 0, field, sizeof(field));
    TEST_ASSERT_EQUAL_STRING_MESSAGE("row2", field,
        "New row1 should contain old row2 data");

    get_field_str(buf, 2, 0, field, sizeof(field));
    TEST_ASSERT_EQUAL_STRING_MESSAGE("row3", field,
        "New row2 should contain old row3 data");
}

/* -----------------------------------------------------------------------
 * Test 5: Attempt to remove a row index that is out of bounds.
 * The function should return 0 (as per implementation) and the buffer
 * height should remain unchanged.
 * ----------------------------------------------------------------------- */
void test_remove_out_of_bounds_row(void)
{
    populate_buffer(buf, 2);
    TEST_ASSERT_EQUAL_INT(2, csv_get_height(buf));

    /* Row index 5 does not exist */
    int ret = csv_remove_row(buf, 5);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret,
        "csv_remove_row should return 0 for out-of-bounds row");
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, csv_get_height(buf),
        "Buffer height should be unchanged after out-of-bounds removal");

    /* Verify existing data is intact */
    char field[64];
    get_field_str(buf, 0, 0, field, sizeof(field));
    TEST_ASSERT_EQUAL_STRING_MESSAGE("row0", field,
        "row0 data should be intact after failed removal");

    get_field_str(buf, 1, 0, field, sizeof(field));
    TEST_ASSERT_EQUAL_STRING_MESSAGE("row1", field,
        "row1 data should be intact after failed removal");
}

/* -----------------------------------------------------------------------
 * main
 * ----------------------------------------------------------------------- */
int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_remove_only_row);
    RUN_TEST(test_remove_first_row);
    RUN_TEST(test_remove_last_row);
    RUN_TEST(test_remove_middle_row);
    RUN_TEST(test_remove_out_of_bounds_row);
    return UNITY_END();
}