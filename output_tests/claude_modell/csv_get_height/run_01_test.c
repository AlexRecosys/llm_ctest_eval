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

/* Test 1: A freshly created buffer has height 0 */
void test_csv_get_height_empty_buffer_returns_zero(void)
{
    int height = csv_get_height(g_buffer);
    TEST_ASSERT_EQUAL_INT(0, height);
}

/* Test 2: After adding one row (via csv_set_field), height is 1 */
void test_csv_get_height_one_row(void)
{
    csv_set_field(g_buffer, 0, 0, "hello");
    int height = csv_get_height(g_buffer);
    TEST_ASSERT_EQUAL_INT(1, height);
}

/* Test 3: After adding multiple rows, height reflects the correct count */
void test_csv_get_height_multiple_rows(void)
{
    csv_set_field(g_buffer, 0, 0, "row0col0");
    csv_set_field(g_buffer, 1, 0, "row1col0");
    csv_set_field(g_buffer, 2, 0, "row2col0");
    int height = csv_get_height(g_buffer);
    TEST_ASSERT_EQUAL_INT(3, height);
}

/* Test 4: Height matches the rows field directly in the struct */
void test_csv_get_height_matches_struct_rows_field(void)
{
    csv_set_field(g_buffer, 0, 0, "a");
    csv_set_field(g_buffer, 1, 0, "b");
    csv_set_field(g_buffer, 2, 0, "c");
    csv_set_field(g_buffer, 3, 0, "d");

    int height = csv_get_height(g_buffer);
    TEST_ASSERT_EQUAL_INT((int)g_buffer->rows, height);
}

/* Test 5: After removing a row, height decreases by one */
void test_csv_get_height_after_remove_row(void)
{
    csv_set_field(g_buffer, 0, 0, "first");
    csv_set_field(g_buffer, 1, 0, "second");
    csv_set_field(g_buffer, 2, 0, "third");

    int height_before = csv_get_height(g_buffer);
    TEST_ASSERT_EQUAL_INT(3, height_before);

    csv_remove_row(g_buffer, 2);

    int height_after = csv_get_height(g_buffer);
    TEST_ASSERT_EQUAL_INT(2, height_after);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_csv_get_height_empty_buffer_returns_zero);
    RUN_TEST(test_csv_get_height_one_row);
    RUN_TEST(test_csv_get_height_multiple_rows);
    RUN_TEST(test_csv_get_height_matches_struct_rows_field);
    RUN_TEST(test_csv_get_height_after_remove_row);
    return UNITY_END();
}