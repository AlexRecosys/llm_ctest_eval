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
    signal(SIGSEGV, SIG_DFL);
}

/* Test 1: Clear the last row in a single-row buffer.
 * When row == buffer->rows - 1, remove_last_row is called.
 * The buffer should have 0 rows after the call. */
void test_csv_clear_row_last_row_is_removed(void)
{
    /* Create one row with two fields */
    csv_set_field(buffer, 0, 0, "hello");
    csv_set_field(buffer, 0, 1, "world");

    TEST_ASSERT_EQUAL_INT(1, csv_get_height(buffer));
    TEST_ASSERT_EQUAL_INT(2, csv_get_width(buffer, 0));

    int result = csv_clear_row(buffer, 0);

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "csv_clear_row should return 0 for last row");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, csv_get_height(buffer),
        "Buffer should have 0 rows after clearing the only (last) row");
}

/* Test 2: Clear a non-last row with multiple fields.
 * The row should be reduced to width 1 and its field cleared. */
void test_csv_clear_row_non_last_row_reduces_width_to_one(void)
{
    /* Create two rows */
    csv_set_field(buffer, 0, 0, "alpha");
    csv_set_field(buffer, 0, 1, "beta");
    csv_set_field(buffer, 0, 2, "gamma");
    csv_set_field(buffer, 1, 0, "delta");

    TEST_ASSERT_EQUAL_INT(2, csv_get_height(buffer));
    TEST_ASSERT_EQUAL_INT(3, csv_get_width(buffer, 0));

    int result = csv_clear_row(buffer, 0);

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "csv_clear_row should return 0 on success");
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, csv_get_width(buffer, 0),
        "Row 0 width should be 1 after csv_clear_row");
    /* The buffer should still have 2 rows */
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, csv_get_height(buffer),
        "Buffer height should remain 2 after clearing row 0");
}

/* Test 3: After clearing a non-last row, the remaining field should be empty. */
void test_csv_clear_row_field_content_is_empty_after_clear(void)
{
    char dest[128];

    /* Create two rows so row 0 is not the last */
    csv_set_field(buffer, 0, 0, "data1");
    csv_set_field(buffer, 0, 1, "data2");
    csv_set_field(buffer, 1, 0, "other");

    int result = csv_clear_row(buffer, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "csv_clear_row should succeed");

    /* The cleared field should be empty (csv_get_field returns 2 for empty) */
    int get_result = csv_get_field(dest, sizeof(dest), buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, get_result,
        "csv_get_field should return 2 (empty) for the cleared field");
}

/* Test 4: Clear the last row in a multi-row buffer.
 * The last row should be removed, reducing height by 1. */
void test_csv_clear_row_last_row_in_multi_row_buffer(void)
{
    /* Create three rows */
    csv_set_field(buffer, 0, 0, "row0");
    csv_set_field(buffer, 1, 0, "row1");
    csv_set_field(buffer, 1, 1, "row1_f1");
    csv_set_field(buffer, 2, 0, "row2");

    TEST_ASSERT_EQUAL_INT(3, csv_get_height(buffer));

    /* Clear the last row (row index 2) */
    int result = csv_clear_row(buffer, 2);

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "csv_clear_row on last row should return 0");
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, csv_get_height(buffer),
        "Buffer height should be 2 after removing last row");
    /* Row 0 and row 1 should be intact */
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, csv_get_width(buffer, 0),
        "Row 0 width should be unchanged");
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, csv_get_width(buffer, 1),
        "Row 1 width should be unchanged");
}

/* Test 5: Clear a row that has exactly one field (non-last row).
 * The row should remain with width 1 and the field should be cleared. */
void test_csv_clear_row_single_field_non_last_row(void)
{
    char dest[128];

    /* Create two rows, row 0 has only one field */
    csv_set_field(buffer, 0, 0, "only_field");
    csv_set_field(buffer, 1, 0, "second_row");

    TEST_ASSERT_EQUAL_INT(2, csv_get_height(buffer));
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buffer, 0));

    int result = csv_clear_row(buffer, 0);

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result,
        "csv_clear_row on single-field non-last row should return 0");
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, csv_get_width(buffer, 0),
        "Row 0 width should still be 1 after clearing");
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, csv_get_height(buffer),
        "Buffer height should remain 2");

    /* The field content should now be empty */
    int get_result = csv_get_field(dest, sizeof(dest), buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, get_result,
        "The single field in row 0 should be empty after csv_clear_row");
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_csv_clear_row_last_row_is_removed);
    RUN_TEST(test_csv_clear_row_non_last_row_reduces_width_to_one);
    RUN_TEST(test_csv_clear_row_field_content_is_empty_after_clear);
    RUN_TEST(test_csv_clear_row_last_row_in_multi_row_buffer);
    RUN_TEST(test_csv_clear_row_single_field_non_last_row);
    return UNITY_END();
}