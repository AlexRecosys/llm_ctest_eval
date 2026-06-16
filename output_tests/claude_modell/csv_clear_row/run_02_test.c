#include "csv.c"
#include "unity.h"
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

/* Helper: retrieve field text safely */
static const char *field_text(CSV_BUFFER *b, size_t row, size_t col)
{
    if (b->field[row][col]->text == NULL)
        return "";
    return b->field[row][col]->text;
}

void setUp(void)
{
    buf = csv_create_buffer();
}

void tearDown(void)
{
    csv_destroy_buffer(buf);
    buf = NULL;
}

/* -----------------------------------------------------------------------
 * Test: clearing the last row removes it entirely (row == rows-1 path)
 * --------------------------------------------------------------------- */
void test_clear_last_row_removes_it(void)
{
    /* Build two rows so that row 1 is the last */
    build_buffer(buf, 2, 3, "data");

    size_t rows_before = buf->rows;
    int ret = csv_clear_row(buf, 1);

    TEST_ASSERT_EQUAL_INT(0, ret);
    /* The last row should have been removed */
    TEST_ASSERT_EQUAL_INT((int)(rows_before - 1), (int)buf->rows);
}

/* -----------------------------------------------------------------------
 * Test: clearing the only row (which is also the last) removes it
 * --------------------------------------------------------------------- */
void test_clear_only_row_removes_it(void)
{
    build_buffer(buf, 1, 4, "hello");

    int ret = csv_clear_row(buf, 0);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(0, (int)buf->rows);
}

/* -----------------------------------------------------------------------
 * Test: clearing a non-last row reduces its width to 1
 * --------------------------------------------------------------------- */
void test_clear_non_last_row_width_becomes_one(void)
{
    build_buffer(buf, 3, 5, "value");

    int ret = csv_clear_row(buf, 0);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(1, (int)buf->width[0]);
}

/* -----------------------------------------------------------------------
 * Test: after clearing a non-last row the remaining field is empty
 * --------------------------------------------------------------------- */
void test_clear_non_last_row_field_is_empty(void)
{
    build_buffer(buf, 3, 4, "nonempty");

    int ret = csv_clear_row(buf, 1);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(1, (int)buf->width[1]);
    /* The surviving field should have been set to "\0" — length 0 */
    TEST_ASSERT_EQUAL_INT(0, (int)buf->field[1][0]->length);
}

/* -----------------------------------------------------------------------
 * Test: clearing a non-last row does not affect other rows
 * --------------------------------------------------------------------- */
void test_clear_non_last_row_other_rows_intact(void)
{
    build_buffer(buf, 3, 3, "keep");

    csv_clear_row(buf, 0);

    /* Row 1 and row 2 should still have 3 fields with "keep" */
    TEST_ASSERT_EQUAL_INT(3, (int)buf->width[1]);
    TEST_ASSERT_EQUAL_INT(3, (int)buf->width[2]);
    TEST_ASSERT_EQUAL_STRING("keep", field_text(buf, 1, 0));
    TEST_ASSERT_EQUAL_STRING("keep", field_text(buf, 2, 2));
}

/* -----------------------------------------------------------------------
 * Test: clearing a row with a single field (non-last) sets it to empty
 * --------------------------------------------------------------------- */
void test_clear_non_last_row_single_field_becomes_empty(void)
{
    build_buffer(buf, 2, 1, "single");

    int ret = csv_clear_row(buf, 0);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(1, (int)buf->width[0]);
    TEST_ASSERT_EQUAL_INT(0, (int)buf->field[0][0]->length);
}

/* -----------------------------------------------------------------------
 * Test: total row count is unchanged when clearing a non-last row
 * --------------------------------------------------------------------- */
void test_clear_non_last_row_row_count_unchanged(void)
{
    build_buffer(buf, 4, 3, "abc");

    size_t rows_before = buf->rows;
    csv_clear_row(buf, 1);

    TEST_ASSERT_EQUAL_INT((int)rows_before, (int)buf->rows);
}

/* -----------------------------------------------------------------------
 * Test: clearing the first of many rows leaves last row accessible
 * --------------------------------------------------------------------- */
void test_clear_first_row_last_row_still_accessible(void)
{
    build_buffer(buf, 3, 2, "z");

    csv_clear_row(buf, 0);

    /* Last row (index 2) should still have 2 fields */
    TEST_ASSERT_EQUAL_INT(2, (int)buf->width[2]);
    TEST_ASSERT_EQUAL_STRING("z", field_text(buf, 2, 0));
    TEST_ASSERT_EQUAL_STRING("z", field_text(buf, 2, 1));
}

/* -----------------------------------------------------------------------
 * Test: clearing a row with many fields (non-last) leaves width == 1
 * --------------------------------------------------------------------- */
void test_clear_wide_non_last_row(void)
{
    build_buffer(buf, 2, 100, "wide");

    int ret = csv_clear_row(buf, 0);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(1, (int)buf->width[0]);
}

/* -----------------------------------------------------------------------
 * Test: return value is 0 on success for last-row path
 * --------------------------------------------------------------------- */
void test_return_zero_on_success_last_row(void)
{
    build_buffer(buf, 2, 2, "x");

    int ret = csv_clear_row(buf, 1);

    TEST_ASSERT_EQUAL_INT(0, ret);
}

/* -----------------------------------------------------------------------
 * Test: return value is 0 on success for non-last-row path
 * --------------------------------------------------------------------- */
void test_return_zero_on_success_non_last_row(void)
{
    build_buffer(buf, 3, 3, "y");

    int ret = csv_clear_row(buf, 0);

    TEST_ASSERT_EQUAL_INT(0, ret);
}

/* -----------------------------------------------------------------------
 * Test: field pointer array is not NULL after clearing a non-last row
 * --------------------------------------------------------------------- */
void test_field_array_not_null_after_clear(void)
{
    build_buffer(buf, 2, 5, "ptr");

    csv_clear_row(buf, 0);

    TEST_ASSERT_NOT_NULL(buf->field[0]);
    TEST_ASSERT_NOT_NULL(buf->field[0][0]);
}

/* -----------------------------------------------------------------------
 * Test: clearing middle row in a 5-row buffer
 * --------------------------------------------------------------------- */
void test_clear_middle_row_five_rows(void)
{
    build_buffer(buf, 5, 4, "mid");

    int ret = csv_clear_row(buf, 2);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(1, (int)buf->width[2]);
    /* Surrounding rows untouched */
    TEST_ASSERT_EQUAL_INT(4, (int)buf->width[1]);
    TEST_ASSERT_EQUAL_INT(4, (int)buf->width[3]);
}

/* -----------------------------------------------------------------------
 * Test: after clearing last row, buffer height decrements by exactly 1
 * --------------------------------------------------------------------- */
void test_height_decrements_by_one_on_last_row_clear(void)
{
    build_buffer(buf, 5, 3, "h");

    int height_before = csv_get_height(buf);
    csv_clear_row(buf, 4);
    int height_after = csv_get_height(buf);

    TEST_ASSERT_EQUAL_INT(height_before - 1, height_after);
}

/* -----------------------------------------------------------------------
 * Test: csv_get_width returns 1 after clearing a non-last row
 * --------------------------------------------------------------------- */
void test_get_width_returns_one_after_clear(void)
{
    build_buffer(buf, 3, 7, "w");

    csv_clear_row(buf, 0);

    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buf, 0));
}

/* -----------------------------------------------------------------------
 * Test: cleared field text can be overwritten with csv_set_field
 * --------------------------------------------------------------------- */
void test_cleared_field_can_be_set_again(void)
{
    build_buffer(buf, 2, 3, "old");

    csv_clear_row(buf, 0);
    int ret = csv_set_field(buf, 0, 0, "new");

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING("new", field_text(buf, 0, 0));
}

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_clear_last_row_removes_it);
    RUN_TEST(test_clear_only_row_removes_it);
    RUN_TEST(test_clear_non_last_row_width_becomes_one);
    RUN_TEST(test_clear_non_last_row_field_is_empty);
    RUN_TEST(test_clear_non_last_row_other_rows_intact);
    RUN_TEST(test_clear_non_last_row_single_field_becomes_empty);
    RUN_TEST(test_clear_non_last_row_row_count_unchanged);
    RUN_TEST(test_clear_first_row_last_row_still_accessible);
    RUN_TEST(test_clear_wide_non_last_row);
    RUN_TEST(test_return_zero_on_success_last_row);
    RUN_TEST(test_return_zero_on_success_non_last_row);
    RUN_TEST(test_field_array_not_null_after_clear);
    RUN_TEST(test_clear_middle_row_five_rows);
    RUN_TEST(test_height_decrements_by_one_on_last_row_clear);
    RUN_TEST(test_get_width_returns_one_after_clear);
    RUN_TEST(test_cleared_field_can_be_set_again);
    return UNITY_END();
}