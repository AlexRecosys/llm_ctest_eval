#include "csv.c"
#include "unity.h"
#include <stdlib.h>
#include <string.h>

/* File-scope fixtures */
static CSV_BUFFER *buf;

/* Helper: build a buffer with given rows and fields per row */
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
 * Test: clearing a field in an empty buffer (row out of range)
 * ----------------------------------------------------------------------- */
void test_clear_field_empty_buffer_returns_zero(void)
{
    int ret = csv_clear_field(buf, 0, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);
}

/* -----------------------------------------------------------------------
 * Test: row index out of range returns 0 without crashing
 * ----------------------------------------------------------------------- */
void test_clear_field_row_out_of_range_returns_zero(void)
{
    build_buffer(buf, 1, 3, "data");
    int ret = csv_clear_field(buf, 5, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);
}

/* -----------------------------------------------------------------------
 * Test: entry index out of range returns 0 without crashing
 * ----------------------------------------------------------------------- */
void test_clear_field_entry_out_of_range_returns_zero(void)
{
    build_buffer(buf, 1, 3, "data");
    int ret = csv_clear_field(buf, 0, 10);
    TEST_ASSERT_EQUAL_INT(0, ret);
}

/* -----------------------------------------------------------------------
 * Test: clearing a middle field sets it to empty string, width unchanged
 * ----------------------------------------------------------------------- */
void test_clear_field_middle_field_sets_empty_string(void)
{
    build_buffer(buf, 1, 3, "hello");
    size_t width_before = buf->width[0];

    int ret = csv_clear_field(buf, 0, 1);
    TEST_ASSERT_EQUAL_INT(0, ret);

    /* Width must not change */
    TEST_ASSERT_EQUAL_UINT(width_before, buf->width[0]);

    /* Field text should now be empty (null terminator only) */
    char dest[64];
    int get_ret = csv_get_field(dest, sizeof(dest), buf, 0, 1);
    /* get_ret == 2 means empty cell */
    TEST_ASSERT_EQUAL_INT(2, get_ret);
}

/* -----------------------------------------------------------------------
 * Test: clearing the first field (entry 0) sets it to empty, width unchanged
 * ----------------------------------------------------------------------- */
void test_clear_field_first_field_sets_empty_string(void)
{
    build_buffer(buf, 1, 3, "world");
    size_t width_before = buf->width[0];

    int ret = csv_clear_field(buf, 0, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);

    TEST_ASSERT_EQUAL_UINT(width_before, buf->width[0]);

    char dest[64];
    int get_ret = csv_get_field(dest, sizeof(dest), buf, 0, 0);
    TEST_ASSERT_EQUAL_INT(2, get_ret);
}

/* -----------------------------------------------------------------------
 * Test: clearing the last field (not entry 0) removes it — width decrements
 * ----------------------------------------------------------------------- */
void test_clear_field_last_field_removes_it(void)
{
    build_buffer(buf, 1, 3, "test");
    size_t width_before = buf->width[0]; /* should be 3 */

    int ret = csv_clear_field(buf, 0, 2); /* entry 2 is last */
    TEST_ASSERT_EQUAL_INT(0, ret);

    /* Width should have decreased by 1 */
    TEST_ASSERT_EQUAL_UINT(width_before - 1, buf->width[0]);
}

/* -----------------------------------------------------------------------
 * Test: clearing the only field in a row (entry 0, last) sets it empty,
 *       does NOT remove it (entry == 0 guard)
 * ----------------------------------------------------------------------- */
void test_clear_field_only_field_in_row_sets_empty_not_removed(void)
{
    build_buffer(buf, 1, 1, "solo");
    size_t width_before = buf->width[0]; /* should be 1 */

    int ret = csv_clear_field(buf, 0, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);

    /* Width must remain 1 */
    TEST_ASSERT_EQUAL_UINT(width_before, buf->width[0]);

    char dest[64];
    int get_ret = csv_get_field(dest, sizeof(dest), buf, 0, 0);
    TEST_ASSERT_EQUAL_INT(2, get_ret);
}

/* -----------------------------------------------------------------------
 * Test: clearing last field in second row does not affect first row
 * ----------------------------------------------------------------------- */
void test_clear_field_does_not_affect_other_rows(void)
{
    build_buffer(buf, 2, 3, "abc");
    size_t width_row0_before = buf->width[0];

    /* Clear last field of row 1 */
    int ret = csv_clear_field(buf, 1, 2);
    TEST_ASSERT_EQUAL_INT(0, ret);

    /* Row 0 width unchanged */
    TEST_ASSERT_EQUAL_UINT(width_row0_before, buf->width[0]);

    /* Row 0 fields still intact */
    char dest[64];
    csv_get_field(dest, sizeof(dest), buf, 0, 0);
    TEST_ASSERT_EQUAL_STRING("abc", dest);
}

/* -----------------------------------------------------------------------
 * Test: return value is always 0 for valid in-range field
 * ----------------------------------------------------------------------- */
void test_clear_field_returns_zero_for_valid_field(void)
{
    build_buffer(buf, 1, 5, "value");
    int ret = csv_clear_field(buf, 0, 3);
    TEST_ASSERT_EQUAL_INT(0, ret);
}

/* -----------------------------------------------------------------------
 * Test: clearing last field repeatedly reduces width each time
 * ----------------------------------------------------------------------- */
void test_clear_field_repeated_last_field_removal(void)
{
    build_buffer(buf, 1, 4, "item");
    /* width = 4 */

    /* Clear entry 3 (last, not 0) -> width becomes 3 */
    csv_clear_field(buf, 0, 3);
    TEST_ASSERT_EQUAL_UINT(3, buf->width[0]);

    /* Clear entry 2 (last, not 0) -> width becomes 2 */
    csv_clear_field(buf, 0, 2);
    TEST_ASSERT_EQUAL_UINT(2, buf->width[0]);

    /* Clear entry 1 (last, not 0) -> width becomes 1 */
    csv_clear_field(buf, 0, 1);
    TEST_ASSERT_EQUAL_UINT(1, buf->width[0]);

    /* Clear entry 0 (last AND entry 0) -> width stays 1, field cleared */
    csv_clear_field(buf, 0, 0);
    TEST_ASSERT_EQUAL_UINT(1, buf->width[0]);

    char dest[64];
    int get_ret = csv_get_field(dest, sizeof(dest), buf, 0, 0);
    TEST_ASSERT_EQUAL_INT(2, get_ret);
}

/* -----------------------------------------------------------------------
 * Test: clearing a non-last field leaves surrounding fields intact
 * ----------------------------------------------------------------------- */
void test_clear_field_middle_leaves_neighbors_intact(void)
{
    build_buffer(buf, 1, 3, "keep");
    /* Set distinct values */
    set_field(buf->field[0][0], "first");
    set_field(buf->field[0][1], "middle");
    set_field(buf->field[0][2], "last");

    csv_clear_field(buf, 0, 1);

    char dest[64];

    csv_get_field(dest, sizeof(dest), buf, 0, 0);
    TEST_ASSERT_EQUAL_STRING("first", dest);

    csv_get_field(dest, sizeof(dest), buf, 0, 2);
    TEST_ASSERT_EQUAL_STRING("last", dest);
}

/* -----------------------------------------------------------------------
 * Test: row boundary — exactly at last valid row
 * ----------------------------------------------------------------------- */
void test_clear_field_exactly_last_valid_row(void)
{
    build_buffer(buf, 3, 2, "x");
    /* buf->rows == 3, last valid row index == 2 */
    int ret = csv_clear_field(buf, 2, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);
}

/* -----------------------------------------------------------------------
 * Test: row one past the end is out of range
 * ----------------------------------------------------------------------- */
void test_clear_field_one_past_last_row_is_out_of_range(void)
{
    build_buffer(buf, 3, 2, "x");
    /* buf->rows == 3, row 3 is out of range */
    size_t width_before = buf->width[2];
    int ret = csv_clear_field(buf, 3, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);
    /* Nothing changed */
    TEST_ASSERT_EQUAL_UINT(width_before, buf->width[2]);
}

/* -----------------------------------------------------------------------
 * main
 * ----------------------------------------------------------------------- */
int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_clear_field_empty_buffer_returns_zero);
    RUN_TEST(test_clear_field_row_out_of_range_returns_zero);
    RUN_TEST(test_clear_field_entry_out_of_range_returns_zero);
    RUN_TEST(test_clear_field_middle_field_sets_empty_string);
    RUN_TEST(test_clear_field_first_field_sets_empty_string);
    RUN_TEST(test_clear_field_last_field_removes_it);
    RUN_TEST(test_clear_field_only_field_in_row_sets_empty_not_removed);
    RUN_TEST(test_clear_field_does_not_affect_other_rows);
    RUN_TEST(test_clear_field_returns_zero_for_valid_field);
    RUN_TEST(test_clear_field_repeated_last_field_removal);
    RUN_TEST(test_clear_field_middle_leaves_neighbors_intact);
    RUN_TEST(test_clear_field_exactly_last_valid_row);
    RUN_TEST(test_clear_field_one_past_last_row_is_out_of_range);
    return UNITY_END();
}