#include "unity.h"
#include "csv.h"
#include <stdlib.h>
#include <string.h>

/* File-scope static fixture */
static CSV_BUFFER *buffer;

/* ---------------------------------------------------------------------------
 * Helper functions
 * --------------------------------------------------------------------------- */

/**
 * Create a fresh CSV_BUFFER with the given number of rows, each row having
 * `cols` fields pre-populated with the supplied text values.
 *
 * The caller is responsible for freeing the buffer via csv_destroy() or
 * equivalent teardown logic.
 */
static CSV_BUFFER *create_buffer_with_data(size_t rows, size_t cols,
                                            const char *fill_text)
{
    CSV_BUFFER *buf = csv_create(',', '"');
    if (buf == NULL) return NULL;

    size_t r, c;
    for (r = 0; r < rows; r++) {
        csv_append_row(buf);
        for (c = 1; c < cols; c++) {
            csv_append_field(buf, r);
        }
        for (c = 0; c < cols; c++) {
            csv_set_field(buf, r, c, fill_text);
        }
    }
    return buf;
}

/* ---------------------------------------------------------------------------
 * setUp / tearDown
 * --------------------------------------------------------------------------- */

void setUp(void)
{
    buffer = csv_create(',', '"');
    TEST_ASSERT_NOT_NULL_MESSAGE(buffer, "csv_create() returned NULL in setUp");
}

void tearDown(void)
{
    if (buffer != NULL) {
        csv_destroy(buffer);
        buffer = NULL;
    }
}

/* ---------------------------------------------------------------------------
 * Test cases
 * --------------------------------------------------------------------------- */

/**
 * test_csv_clear_row_middle_row_reduces_width_to_one
 *
 * A row in the middle of the buffer (not the last row) should be cleared so
 * that its width becomes 1 and the single remaining field contains an empty
 * string.
 */
void test_csv_clear_row_middle_row_reduces_width_to_one(void)
{
    /* Build a 3-row buffer, each row with 4 fields */
    csv_append_row(buffer);                          /* row 0 */
    csv_append_field(buffer, 0);
    csv_append_field(buffer, 0);
    csv_append_field(buffer, 0);
    csv_set_field(buffer, 0, 0, "A");
    csv_set_field(buffer, 0, 1, "B");
    csv_set_field(buffer, 0, 2, "C");
    csv_set_field(buffer, 0, 3, "D");

    csv_append_row(buffer);                          /* row 1 */
    csv_append_field(buffer, 1);
    csv_append_field(buffer, 1);
    csv_append_field(buffer, 1);
    csv_set_field(buffer, 1, 0, "E");
    csv_set_field(buffer, 1, 1, "F");
    csv_set_field(buffer, 1, 2, "G");
    csv_set_field(buffer, 1, 3, "H");

    csv_append_row(buffer);                          /* row 2 */
    csv_append_field(buffer, 2);
    csv_set_field(buffer, 2, 0, "I");
    csv_set_field(buffer, 2, 1, "J");

    int ret = csv_clear_row(buffer, 1);

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret,
        "csv_clear_row should return 0 on success");
    TEST_ASSERT_EQUAL_UINT_MESSAGE(1, buffer->width[1],
        "Width of cleared middle row should be 1");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("",
        csv_get_field(buffer, 1, 0),
        "Remaining field in cleared row should be empty string");
    /* Surrounding rows must be untouched */
    TEST_ASSERT_EQUAL_UINT_MESSAGE(4, buffer->width[0],
        "Row 0 width should be unchanged after clearing row 1");
    TEST_ASSERT_EQUAL_UINT_MESSAGE(2, buffer->width[2],
        "Row 2 width should be unchanged after clearing row 1");
}

/**
 * test_csv_clear_row_last_row_removes_row
 *
 * Clearing the last row should remove it entirely, decrementing the row count.
 */
void test_csv_clear_row_last_row_removes_row(void)
{
    csv_append_row(buffer);                          /* row 0 */
    csv_set_field(buffer, 0, 0, "keep");

    csv_append_row(buffer);                          /* row 1 – last */
    csv_append_field(buffer, 1);
    csv_set_field(buffer, 1, 0, "gone0");
    csv_set_field(buffer, 1, 1, "gone1");

    size_t rows_before = buffer->rows;

    int ret = csv_clear_row(buffer, rows_before - 1);

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret,
        "csv_clear_row on last row should return 0");
    TEST_ASSERT_EQUAL_UINT_MESSAGE(rows_before - 1, buffer->rows,
        "Row count should decrease by 1 after clearing last row");
    /* The surviving row must still be intact */
    TEST_ASSERT_EQUAL_STRING_MESSAGE("keep",
        csv_get_field(buffer, 0, 0),
        "Row 0 field should be untouched after last row removal");
}

/**
 * test_csv_clear_row_single_field_row_becomes_empty
 *
 * A row that already has only one field should be cleared to an empty string
 * and its width should remain 1.
 */
void test_csv_clear_row_single_field_row_becomes_empty(void)
{
    csv_append_row(buffer);                          /* row 0 */
    csv_set_field(buffer, 0, 0, "only_field");

    csv_append_row(buffer);                          /* row 1 – last, so add a guard */
    csv_set_field(buffer, 1, 0, "guard");

    int ret = csv_clear_row(buffer, 0);

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret,
        "csv_clear_row on single-field row should return 0");
    TEST_ASSERT_EQUAL_UINT_MESSAGE(1, buffer->width[0],
        "Width should remain 1 after clearing a single-field row");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("",
        csv_get_field(buffer, 0, 0),
        "Single remaining field should be empty after clear");
}

/**
 * test_csv_clear_row_total_row_count_unchanged_for_non_last_row
 *
 * Clearing a non-last row must NOT change the total number of rows in the
 * buffer.
 */
void test_csv_clear_row_total_row_count_unchanged_for_non_last_row(void)
{
    /* Three rows */
    csv_append_row(buffer);                          /* row 0 */
    csv_append_field(buffer, 0);
    csv_set_field(buffer, 0, 0, "r0c0");
    csv_set_field(buffer, 0, 1, "r0c1");

    csv_append_row(buffer);                          /* row 1 */
    csv_append_field(buffer, 1);
    csv_set_field(buffer, 1, 0, "r1c0");
    csv_set_field(buffer, 1, 1, "r1c1");

    csv_append_row(buffer);                          /* row 2 */
    csv_set_field(buffer, 2, 0, "r2c0");

    size_t rows_before = buffer->rows;

    int ret = csv_clear_row(buffer, 0);

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret,
        "csv_clear_row should succeed");
    TEST_ASSERT_EQUAL_UINT_MESSAGE(rows_before, buffer->rows,
        "Total row count must not change when clearing a non-last row");
}

/**
 * test_csv_clear_row_other_rows_data_intact_after_clear
 *
 * After clearing a middle row, all other rows must retain their original
 * field data and widths.
 */
void test_csv_clear_row_other_rows_data_intact_after_clear(void)
{
    /* Row 0: 3 fields */
    csv_append_row(buffer);
    csv_append_field(buffer, 0);
    csv_append_field(buffer, 0);
    csv_set_field(buffer, 0, 0, "alpha");
    csv_set_field(buffer, 0, 1, "beta");
    csv_set_field(buffer, 0, 2, "gamma");

    /* Row 1: 2 fields – this is the row we will clear */
    csv_append_row(buffer);
    csv_append_field(buffer, 1);
    csv_set_field(buffer, 1, 0, "del0");
    csv_set_field(buffer, 1, 1, "del1");

    /* Row 2: 2 fields – last row, acts as guard */
    csv_append_row(buffer);
    csv_append_field(buffer, 2);
    csv_set_field(buffer, 2, 0, "delta");
    csv_set_field(buffer, 2, 1, "epsilon");

    int ret = csv_clear_row(buffer, 1);

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret,
        "csv_clear_row should return 0");

    /* Row 0 integrity */
    TEST_ASSERT_EQUAL_UINT_MESSAGE(3, buffer->width[0],
        "Row 0 width must be unchanged");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("alpha",
        csv_get_field(buffer, 0, 0), "Row 0, col 0 must be 'alpha'");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("beta",
        csv_get_field(buffer, 0, 1), "Row 0, col 1 must be 'beta'");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("gamma",
        csv_get_field(buffer, 0, 2), "Row 0, col 2 must be 'gamma'");

    /* Row 2 integrity */
    TEST_ASSERT_EQUAL_UINT_MESSAGE(2, buffer->width[2],
        "Row 2 width must be unchanged");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("delta",
        csv_get_field(buffer, 2, 0), "Row 2, col 0 must be 'delta'");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("epsilon",
        csv_get_field(buffer, 2, 1), "Row 2, col 1 must be 'epsilon'");
}

/* ---------------------------------------------------------------------------
 * main
 * --------------------------------------------------------------------------- */

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_csv_clear_row_middle_row_reduces_width_to_one);
    RUN_TEST(test_csv_clear_row_last_row_removes_row);
    RUN_TEST(test_csv_clear_row_single_field_row_becomes_empty);
    RUN_TEST(test_csv_clear_row_total_row_count_unchanged_for_non_last_row);
    RUN_TEST(test_csv_clear_row_other_rows_data_intact_after_clear);

    return UNITY_END();
}