#include "csv.c"
#include "unity.h"
#include <stdlib.h>
#include <string.h>

/* File-scope fixtures */
static CSV_BUFFER *buffer;

/* Helper: build a buffer with a given number of rows and fields per row */
static void build_buffer(int num_rows, int fields_per_row, const char *fill_text)
{
    int r, f;
    for (r = 0; r < num_rows; r++) {
        append_row(buffer);
        for (f = 0; f < fields_per_row; f++) {
            append_field(buffer, r);
            set_field(buffer->field[r][f], (char *)fill_text);
        }
    }
}

/* setUp and tearDown */
void setUp(void)
{
    buffer = csv_create_buffer();
}

void tearDown(void)
{
    csv_destroy_buffer(buffer);
    buffer = NULL;
}

/* ------------------------------------------------------------------ */
/* Test: clear the last row (single row buffer) — should call          */
/* remove_last_row and return 0                                        */
/* ------------------------------------------------------------------ */
void test_csv_clear_row_last_row_single_row_returns_zero(void)
{
    append_row(buffer);
    append_field(buffer, 0);
    set_field(buffer->field[0][0], "hello");

    int ret = csv_clear_row(buffer, 0);

    TEST_ASSERT_EQUAL_INT(0, ret);
    /* The row should have been removed entirely */
    TEST_ASSERT_EQUAL_INT(0, (int)buffer->rows);
}

/* ------------------------------------------------------------------ */
/* Test: clear the last row of a multi-row buffer                      */
/* ------------------------------------------------------------------ */
void test_csv_clear_row_last_row_multi_row_returns_zero(void)
{
    build_buffer(3, 4, "data");

    int ret = csv_clear_row(buffer, 2);

    TEST_ASSERT_EQUAL_INT(0, ret);
    /* Buffer should now have 2 rows */
    TEST_ASSERT_EQUAL_INT(2, (int)buffer->rows);
}

/* ------------------------------------------------------------------ */
/* Test: clear a non-last row with multiple fields                     */
/* ------------------------------------------------------------------ */
void test_csv_clear_row_non_last_row_returns_zero(void)
{
    build_buffer(3, 5, "value");

    int ret = csv_clear_row(buffer, 0);

    TEST_ASSERT_EQUAL_INT(0, ret);
}

/* ------------------------------------------------------------------ */
/* Test: after clearing a non-last row, width becomes 1                */
/* ------------------------------------------------------------------ */
void test_csv_clear_row_non_last_row_width_becomes_one(void)
{
    build_buffer(3, 5, "value");

    csv_clear_row(buffer, 0);

    TEST_ASSERT_EQUAL_INT(1, (int)buffer->width[0]);
}

/* ------------------------------------------------------------------ */
/* Test: after clearing a non-last row, the remaining field is empty   */
/* ------------------------------------------------------------------ */
void test_csv_clear_row_non_last_row_field_is_empty(void)
{
    build_buffer(3, 5, "value");

    csv_clear_row(buffer, 0);

    char dest[64];
    memset(dest, 'X', sizeof(dest));
    int ret = csv_get_field(dest, sizeof(dest), buffer, 0, 0);
    /* csv_get_field returns 2 when cell is empty */
    TEST_ASSERT_EQUAL_INT(2, ret);
}

/* ------------------------------------------------------------------ */
/* Test: clearing a row with a single field (non-last row)             */
/* ------------------------------------------------------------------ */
void test_csv_clear_row_single_field_non_last_row_returns_zero(void)
{
    build_buffer(2, 1, "only");

    int ret = csv_clear_row(buffer, 0);

    TEST_ASSERT_EQUAL_INT(0, ret);
}

/* ------------------------------------------------------------------ */
/* Test: clearing a row with a single field (non-last row) width = 1  */
/* ------------------------------------------------------------------ */
void test_csv_clear_row_single_field_non_last_row_width_one(void)
{
    build_buffer(2, 1, "only");

    csv_clear_row(buffer, 0);

    TEST_ASSERT_EQUAL_INT(1, (int)buffer->width[0]);
}

/* ------------------------------------------------------------------ */
/* Test: other rows are unaffected after clearing a middle row         */
/* ------------------------------------------------------------------ */
void test_csv_clear_row_other_rows_unaffected(void)
{
    build_buffer(3, 3, "keep");

    csv_clear_row(buffer, 1);

    /* Row 0 should still have width 3 */
    TEST_ASSERT_EQUAL_INT(3, (int)buffer->width[0]);
    /* Row 2 should still have width 3 */
    TEST_ASSERT_EQUAL_INT(3, (int)buffer->width[2]);
}

/* ------------------------------------------------------------------ */
/* Test: data in other rows is preserved after clearing a middle row   */
/* ------------------------------------------------------------------ */
void test_csv_clear_row_other_rows_data_preserved(void)
{
    build_buffer(3, 2, "intact");

    csv_clear_row(buffer, 1);

    char dest[64];
    csv_get_field(dest, sizeof(dest), buffer, 0, 0);
    TEST_ASSERT_EQUAL_STRING("intact", dest);

    csv_get_field(dest, sizeof(dest), buffer, 2, 0);
    TEST_ASSERT_EQUAL_STRING("intact", dest);
}

/* ------------------------------------------------------------------ */
/* Test: clearing the first row of a two-row buffer (non-last)         */
/* ------------------------------------------------------------------ */
void test_csv_clear_row_first_row_two_row_buffer(void)
{
    build_buffer(2, 4, "text");

    int ret = csv_clear_row(buffer, 0);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(1, (int)buffer->width[0]);
    /* Row 1 should be untouched */
    TEST_ASSERT_EQUAL_INT(4, (int)buffer->width[1]);
}

/* ------------------------------------------------------------------ */
/* Test: clearing the middle row of a 5-row buffer                     */
/* ------------------------------------------------------------------ */
void test_csv_clear_row_middle_row_five_row_buffer(void)
{
    build_buffer(5, 6, "mid");

    int ret = csv_clear_row(buffer, 2);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(1, (int)buffer->width[2]);
    /* Surrounding rows intact */
    TEST_ASSERT_EQUAL_INT(6, (int)buffer->width[1]);
    TEST_ASSERT_EQUAL_INT(6, (int)buffer->width[3]);
}

/* ------------------------------------------------------------------ */
/* Test: buffer row count unchanged after clearing a non-last row      */
/* ------------------------------------------------------------------ */
void test_csv_clear_row_non_last_row_count_unchanged(void)
{
    build_buffer(4, 3, "abc");

    csv_clear_row(buffer, 1);

    TEST_ASSERT_EQUAL_INT(4, (int)buffer->rows);
}

/* ------------------------------------------------------------------ */
/* Test: clearing a row with many fields reduces width to 1            */
/* ------------------------------------------------------------------ */
void test_csv_clear_row_many_fields_width_becomes_one(void)
{
    build_buffer(2, 100, "x");

    int ret = csv_clear_row(buffer, 0);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(1, (int)buffer->width[0]);
}

/* ------------------------------------------------------------------ */
/* Test: field pointer array is not NULL after clearing                */
/* ------------------------------------------------------------------ */
void test_csv_clear_row_field_array_not_null_after_clear(void)
{
    build_buffer(2, 5, "notnull");

    csv_clear_row(buffer, 0);

    TEST_ASSERT_NOT_NULL(buffer->field[0]);
    TEST_ASSERT_NOT_NULL(buffer->field[0][0]);
}

/* ------------------------------------------------------------------ */
/* Test: clearing last row of a 2-row buffer leaves 1 row             */
/* ------------------------------------------------------------------ */
void test_csv_clear_row_last_row_two_row_buffer_leaves_one_row(void)
{
    build_buffer(2, 3, "row");

    int ret = csv_clear_row(buffer, 1);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(1, (int)buffer->rows);
}

/* ------------------------------------------------------------------ */
/* Test: clearing last row of a 2-row buffer, first row intact         */
/* ------------------------------------------------------------------ */
void test_csv_clear_row_last_row_two_row_buffer_first_row_intact(void)
{
    build_buffer(2, 3, "safe");

    csv_clear_row(buffer, 1);

    TEST_ASSERT_EQUAL_INT(3, (int)buffer->width[0]);

    char dest[64];
    csv_get_field(dest, sizeof(dest), buffer, 0, 0);
    TEST_ASSERT_EQUAL_STRING("safe", dest);
}

/* ------------------------------------------------------------------ */
/* main                                                                */
/* ------------------------------------------------------------------ */
int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_csv_clear_row_last_row_single_row_returns_zero);
    RUN_TEST(test_csv_clear_row_last_row_multi_row_returns_zero);
    RUN_TEST(test_csv_clear_row_non_last_row_returns_zero);
    RUN_TEST(test_csv_clear_row_non_last_row_width_becomes_one);
    RUN_TEST(test_csv_clear_row_non_last_row_field_is_empty);
    RUN_TEST(test_csv_clear_row_single_field_non_last_row_returns_zero);
    RUN_TEST(test_csv_clear_row_single_field_non_last_row_width_one);
    RUN_TEST(test_csv_clear_row_other_rows_unaffected);
    RUN_TEST(test_csv_clear_row_other_rows_data_preserved);
    RUN_TEST(test_csv_clear_row_first_row_two_row_buffer);
    RUN_TEST(test_csv_clear_row_middle_row_five_row_buffer);
    RUN_TEST(test_csv_clear_row_non_last_row_count_unchanged);
    RUN_TEST(test_csv_clear_row_many_fields_width_becomes_one);
    RUN_TEST(test_csv_clear_row_field_array_not_null_after_clear);
    RUN_TEST(test_csv_clear_row_last_row_two_row_buffer_leaves_one_row);
    RUN_TEST(test_csv_clear_row_last_row_two_row_buffer_first_row_intact);
    return UNITY_END();
}