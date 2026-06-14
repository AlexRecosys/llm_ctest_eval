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
    TEST_FAIL_MESSAGE("Segmentation fault (SIGSEGV) caught during test execution");
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
    int height = csv_get_height(g_buffer);
    TEST_ASSERT_EQUAL_INT(0, height);
}

/* Test 2: After adding one row (via csv_set_field), height is 1 */
void test_csv_get_height_one_row(void)
{
    /* csv_set_field will create the row if it doesn't exist */
    int result = csv_set_field(g_buffer, 0, 0, "hello");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "csv_set_field should succeed for row 0");

    int height = csv_get_height(g_buffer);
    TEST_ASSERT_EQUAL_INT(1, height);
}

/* Test 3: After adding multiple rows, height reflects the correct count */
void test_csv_get_height_multiple_rows(void)
{
    csv_set_field(g_buffer, 0, 0, "row0col0");
    csv_set_field(g_buffer, 1, 0, "row1col0");
    csv_set_field(g_buffer, 2, 0, "row2col0");
    csv_set_field(g_buffer, 3, 0, "row3col0");
    csv_set_field(g_buffer, 4, 0, "row4col0");

    int height = csv_get_height(g_buffer);
    TEST_ASSERT_EQUAL_INT_MESSAGE(5, height, "Height should be 5 after adding 5 rows");
}

/* Test 4: Height is greater than zero after adding rows */
void test_csv_get_height_greater_than_zero_after_adding_rows(void)
{
    csv_set_field(g_buffer, 0, 0, "data");
    csv_set_field(g_buffer, 1, 0, "more data");

    int height = csv_get_height(g_buffer);
    TEST_ASSERT_GREATER_THAN(0, height);
}

/* Test 5: Height matches the rows field directly */
void test_csv_get_height_matches_rows_field(void)
{
    csv_set_field(g_buffer, 0, 0, "a");
    csv_set_field(g_buffer, 1, 0, "b");
    csv_set_field(g_buffer, 2, 0, "c");

    int height = csv_get_height(g_buffer);
    /* Verify the returned value matches the internal rows field */
    TEST_ASSERT_EQUAL_INT_MESSAGE((int)g_buffer->rows, height,
        "csv_get_height should return the same value as buffer->rows");
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_csv_get_height_empty_buffer_returns_zero);
    RUN_TEST(test_csv_get_height_one_row);
    RUN_TEST(test_csv_get_height_multiple_rows);
    RUN_TEST(test_csv_get_height_greater_than_zero_after_adding_rows);
    RUN_TEST(test_csv_get_height_matches_rows_field);
    return UNITY_END();
}