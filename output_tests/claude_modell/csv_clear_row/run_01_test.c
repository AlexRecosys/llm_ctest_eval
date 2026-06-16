#include "csv.c"
#include "unity.h"
#include "csv.h"
#include <stdlib.h>
#include <string.h>

/* File-scope fixtures */
static CSV_BUFFER *buf;

/* Helper: build a buffer with a given number of rows and fields per row */
static void build_buffer(CSV_BUFFER *b, size_t num_rows, size_t fields_per_row,
                         const char *fill_text)
{
    size_t r, f;
    for (r = 0; r < num_rows; r++) {
        append_row(b);
        for (f = 0; f < fields_per_row; f++) {
            append_field(b, r);
            set_field(b->field[r][f], (char *)fill_text);
        }
    }
}

/* setUp / tearDown */
void setUp(void)
{
    buf = csv_create_buffer();
}

void tearDown(void)
{
    csv_destroy_buffer(buf);
    buf = NULL;
}

/* ------------------------------------------------------------------ */
/* Test: clearing the last row (row == buffer->rows - 1) removes it   */
/* ------------------------------------------------------------------ */
void test_clear_last_row_removes_it(void)
{
    /* Build two rows so that row 1 is the last */
    build_buffer(buf, 2, 3, "data");

    size_t rows_before = buf->rows;
    int ret = csv_clear_row(buf, 1);

    TEST_ASSERT_EQUAL_INT(0, ret);
    /* The last row should have been removed entirely */
    TEST_ASSERT_EQUAL_INT((int)(rows_before - 1), (int)buf->rows);
}

/* ------------------------------------------------------------------ */
/* Test: clearing a non-last row with multiple fields leaves width==1  */
/* ------------------------------------------------------------------ */
void test_clear_non_last_row_reduces_width_to_one(void)
{
    /* Three rows; clear the middle one (row 1) */
    build_buffer(buf, 3, 5, "hello");

    int ret = csv_clear_row(buf, 1);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(1, (int)buf->width[1]);
}

/* ------------------------------------------------------------------ */
/* Test: after clearing a non-last row the remaining field is empty    */
/* ------------------------------------------------------------------ */
void test_clear_non_last_row_field_is_empty(void)
{
    build_buffer(buf, 3, 4, "content");

    int ret = csv_clear_row(buf, 0);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(1, (int)buf->width[0]);

    char dest[64];
    int gr = csv_get_field(dest, sizeof(dest), buf, 0, 0);
    /* csv_get_field returns 2 when the cell is empty */
    TEST_ASSERT_EQUAL_INT(2, gr);
}

/* ------------------------------------------------------------------ */
/* Test: clearing a row with exactly one field (non-last row)          */
/* ------------------------------------------------------------------ */
void test_clear_non_last_row_single_field(void)
{
    build_buffer(buf, 2, 1, "single");

    int ret = csv_clear_row(buf, 0);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(1, (int)buf->width[0]);
    /* Row count must be unchanged */
    TEST_ASSERT_EQUAL_INT(2, (int)buf->rows);
}

/* ------------------------------------------------------------------ */
/* Test: other rows are not affected when a middle row is cleared      */
/* ------------------------------------------------------------------ */
void test_clear_middle_row_does_not_affect_other_rows(void)
{
    build_buffer(buf, 3, 3, "keep");

    csv_clear_row(buf, 1);

    /* Row 0 and row 2 should still have width 3 */
    TEST_ASSERT_EQUAL_INT(3, (int)buf->width[0]);
    TEST_ASSERT_EQUAL_INT(3, (int)buf->width[2]);

    char dest[64];
    csv_get_field(dest, sizeof(dest), buf, 0, 0);
    TEST_ASSERT_EQUAL_STRING("keep", dest);

    csv_get_field(dest, sizeof(dest), buf, 2, 0);
    TEST_ASSERT_EQUAL_STRING("keep", dest);
}

/* ------------------------------------------------------------------ */
/* Test: total row count is unchanged when clearing a non-last row     */
/* ------------------------------------------------------------------ */
void test_clear_non_last_row_row_count_unchanged(void)
{
    build_buffer(buf, 4, 3, "x");

    size_t rows_before = buf->rows;
    csv_clear_row(buf, 2);

    TEST_ASSERT_EQUAL_INT((int)rows_before, (int)buf->rows);
}

/* ------------------------------------------------------------------ */
/* Test: clearing the only row (row 0, which is also the last)         */
/* ------------------------------------------------------------------ */
void test_clear_only_row_removes_it(void)
{
    build_buffer(buf, 1, 3, "only");

    int ret = csv_clear_row(buf, 0);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(0, (int)buf->rows);
}

/* ------------------------------------------------------------------ */
/* Test: clearing the first of two rows (non-last) works correctly     */
/* ------------------------------------------------------------------ */
void test_clear_first_of_two_rows(void)
{
    build_buffer(buf, 2, 4, "abc");

    int ret = csv_clear_row(buf, 0);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(1, (int)buf->width[0]);
    /* Second row must be intact */
    TEST_ASSERT_EQUAL_INT(4, (int)buf->width[1]);
}

/* ------------------------------------------------------------------ */
/* Test: clearing a row with many fields leaves exactly one field      */
/* ------------------------------------------------------------------ */
void test_clear_row_with_many_fields(void)
{
    build_buffer(buf, 2, 20, "lots");

    int ret = csv_clear_row(buf, 0);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(1, (int)buf->width[0]);
}

/* ------------------------------------------------------------------ */
/* Test: field pointer array is not NULL after clearing a non-last row */
/* ------------------------------------------------------------------ */
void test_clear_non_last_row_field_array_not_null(void)
{
    build_buffer(buf, 2, 3, "ptr");

    csv_clear_row(buf, 0);

    TEST_ASSERT_NOT_NULL(buf->field[0]);
    TEST_ASSERT_NOT_NULL(buf->field[0][0]);
}

/* ------------------------------------------------------------------ */
/* Test: clearing last row of a single-row buffer leaves rows == 0     */
/* ------------------------------------------------------------------ */
void test_clear_last_row_single_row_buffer(void)
{
    build_buffer(buf, 1, 2, "last");

    int ret = csv_clear_row(buf, 0);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(0, (int)buf->rows);
}

/* ------------------------------------------------------------------ */
/* Test: clearing last row of multi-row buffer leaves correct count    */
/* ------------------------------------------------------------------ */
void test_clear_last_row_multi_row_buffer(void)
{
    build_buffer(buf, 5, 3, "multi");

    int ret = csv_clear_row(buf, 4);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(4, (int)buf->rows);
}

/* ------------------------------------------------------------------ */
/* Test: after clearing a non-last row, csv_get_width returns 1        */
/* ------------------------------------------------------------------ */
void test_csv_get_width_after_clear(void)
{
    build_buffer(buf, 3, 7, "wide");

    csv_clear_row(buf, 1);

    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buf, 1));
}

/* ------------------------------------------------------------------ */
/* Test: csv_get_height unchanged after clearing a non-last row        */
/* ------------------------------------------------------------------ */
void test_csv_get_height_unchanged_after_clear_non_last(void)
{
    build_buffer(buf, 3, 3, "h");

    int height_before = csv_get_height(buf);
    csv_clear_row(buf, 0);

    TEST_ASSERT_EQUAL_INT(height_before, csv_get_height(buf));
}

/* ------------------------------------------------------------------ */
/* Test: csv_get_height decremented after clearing the last row        */
/* ------------------------------------------------------------------ */
void test_csv_get_height_decremented_after_clear_last(void)
{
    build_buffer(buf, 3, 3, "h");

    int height_before = csv_get_height(buf);
    csv_clear_row(buf, 2);

    TEST_ASSERT_EQUAL_INT(height_before - 1, csv_get_height(buf));
}

/* ------------------------------------------------------------------ */
/* main                                                                */
/* ------------------------------------------------------------------ */
int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_clear_last_row_removes_it);
    RUN_TEST(test_clear_non_last_row_reduces_width_to_one);
    RUN_TEST(test_clear_non_last_row_field_is_empty);
    RUN_TEST(test_clear_non_last_row_single_field);
    RUN_TEST(test_clear_middle_row_does_not_affect_other_rows);
    RUN_TEST(test_clear_non_last_row_row_count_unchanged);
    RUN_TEST(test_clear_only_row_removes_it);
    RUN_TEST(test_clear_first_of_two_rows);
    RUN_TEST(test_clear_row_with_many_fields);
    RUN_TEST(test_clear_non_last_row_field_array_not_null);
    RUN_TEST(test_clear_last_row_single_row_buffer);
    RUN_TEST(test_clear_last_row_multi_row_buffer);
    RUN_TEST(test_csv_get_width_after_clear);
    RUN_TEST(test_csv_get_height_unchanged_after_clear_non_last);
    RUN_TEST(test_csv_get_height_decremented_after_clear_last);
    return UNITY_END();
}