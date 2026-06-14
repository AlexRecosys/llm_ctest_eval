#include "csv.c"
#include "unity.h"
#include "csv.h"
#include <stdlib.h>
#include <string.h>
#include <signal.h>

/* File-scope fixture */
static CSV_BUFFER *g_buffer;

/* Signal handler for SIGSEGV */
static void sigsegv_handler(int sig)
{
    (void)sig;
    TEST_FAIL_MESSAGE("Segmentation fault (SIGSEGV) caught during test");
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

/* Test 1: Freshly created buffer has height 0 */
void test_csv_get_height_empty_buffer_returns_zero(void)
{
    TEST_ASSERT_NOT_NULL_MESSAGE(g_buffer, "csv_create_buffer returned NULL");
    int height = csv_get_height(g_buffer);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, height,
        "Freshly created buffer should have height 0");
}

/* Test 2: After adding one row (via csv_set_field), height should be 1 */
void test_csv_get_height_one_row(void)
{
    TEST_ASSERT_NOT_NULL_MESSAGE(g_buffer, "csv_create_buffer returned NULL");
    /* csv_set_field will create the row/field as needed */
    int ret = csv_set_field(g_buffer, 0, 0, "hello");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "csv_set_field should succeed");
    int height = csv_get_height(g_buffer);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, height,
        "Buffer with one row should have height 1");
}

/* Test 3: After adding multiple rows, height reflects the correct count */
void test_csv_get_height_multiple_rows(void)
{
    TEST_ASSERT_NOT_NULL_MESSAGE(g_buffer, "csv_create_buffer returned NULL");
    int ret;
    ret = csv_set_field(g_buffer, 0, 0, "row0col0");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "csv_set_field row 0 should succeed");
    ret = csv_set_field(g_buffer, 1, 0, "row1col0");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "csv_set_field row 1 should succeed");
    ret = csv_set_field(g_buffer, 2, 0, "row2col0");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "csv_set_field row 2 should succeed");
    int height = csv_get_height(g_buffer);
    TEST_ASSERT_EQUAL_INT_MESSAGE(3, height,
        "Buffer with three rows should have height 3");
}

/* Test 4: Height matches the rows field directly */
void test_csv_get_height_matches_rows_field(void)
{
    TEST_ASSERT_NOT_NULL_MESSAGE(g_buffer, "csv_create_buffer returned NULL");
    csv_set_field(g_buffer, 0, 0, "a");
    csv_set_field(g_buffer, 1, 0, "b");
    csv_set_field(g_buffer, 2, 0, "c");
    csv_set_field(g_buffer, 3, 0, "d");
    int height = csv_get_height(g_buffer);
    /* Verify the return value equals the internal rows field */
    TEST_ASSERT_EQUAL_INT_MESSAGE((int)g_buffer->rows, height,
        "csv_get_height should return the value of buffer->rows");
}

/* Test 5: Height after removing a row is decremented */
void test_csv_get_height_after_remove_row(void)
{
    TEST_ASSERT_NOT_NULL_MESSAGE(g_buffer, "csv_create_buffer returned NULL");
    int ret;
    ret = csv_set_field(g_buffer, 0, 0, "first");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "csv_set_field row 0 should succeed");
    ret = csv_set_field(g_buffer, 1, 0, "second");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "csv_set_field row 1 should succeed");
    ret = csv_set_field(g_buffer, 2, 0, "third");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "csv_set_field row 2 should succeed");

    int height_before = csv_get_height(g_buffer);
    TEST_ASSERT_EQUAL_INT_MESSAGE(3, height_before,
        "Buffer should have 3 rows before removal");

    ret = csv_remove_row(g_buffer, 2);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "csv_remove_row should succeed");

    int height_after = csv_get_height(g_buffer);
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, height_after,
        "Buffer should have 2 rows after removing one row");
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_csv_get_height_empty_buffer_returns_zero);
    RUN_TEST(test_csv_get_height_one_row);
    RUN_TEST(test_csv_get_height_multiple_rows);
    RUN_TEST(test_csv_get_height_matches_rows_field);
    RUN_TEST(test_csv_get_height_after_remove_row);
    return UNITY_END();
}