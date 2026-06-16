#include "csv.c"
#include "unity.h"
#include <stdlib.h>
#include <string.h>

/* File-scope fixtures */
static CSV_BUFFER *src;
static CSV_BUFFER *dst;

/* Helper: populate a buffer with given rows/cols and string values */
static void fill_buffer(CSV_BUFFER *buf, int rows, int cols, const char *val)
{
        int r, c;
        for (r = 0; r < rows; r++) {
                append_row(buf);
                for (c = 0; c < cols; c++) {
                        append_field(buf, r);
                        set_field(buf->field[r][c], (char *)val);
                }
        }
}

/* Helper: read a field text safely */
static const char *get_text(CSV_BUFFER *buf, int row, int col)
{
        if (buf == NULL) return NULL;
        if ((size_t)row >= buf->rows) return NULL;
        if ((size_t)col >= buf->width[row]) return NULL;
        if (buf->field[row][col] == NULL) return NULL;
        return buf->field[row][col]->text;
}

void setUp(void)
{
        src = csv_create_buffer();
        dst = csv_create_buffer();
}

void tearDown(void)
{
        csv_destroy_buffer(src);
        csv_destroy_buffer(dst);
        src = NULL;
        dst = NULL;
}

/* -----------------------------------------------------------------------
 * Test: copy a single-field row from source row 0 to dest row 0
 * ----------------------------------------------------------------------- */
void test_csv_copy_row_basic_single_field(void)
{
        fill_buffer(src, 1, 1, "hello");
        append_row(dst);
        append_field(dst, 0);
        set_field(dst->field[0][0], "old");

        int ret = csv_copy_row(dst, 0, src, 0);

        TEST_ASSERT_EQUAL_INT(0, ret);
        TEST_ASSERT_EQUAL_INT(1, (int)dst->width[0]);
        TEST_ASSERT_EQUAL_STRING("hello", get_text(dst, 0, 0));
}

/* -----------------------------------------------------------------------
 * Test: copy a multi-field row
 * ----------------------------------------------------------------------- */
void test_csv_copy_row_multiple_fields(void)
{
        append_row(src);
        append_field(src, 0);
        set_field(src->field[0][0], "alpha");
        append_field(src, 0);
        set_field(src->field[0][1], "beta");
        append_field(src, 0);
        set_field(src->field[0][2], "gamma");

        int ret = csv_copy_row(dst, 0, src, 0);

        TEST_ASSERT_EQUAL_INT(0, ret);
        TEST_ASSERT_EQUAL_INT(3, (int)dst->width[0]);
        TEST_ASSERT_EQUAL_STRING("alpha", get_text(dst, 0, 0));
        TEST_ASSERT_EQUAL_STRING("beta",  get_text(dst, 0, 1));
        TEST_ASSERT_EQUAL_STRING("gamma", get_text(dst, 0, 2));
}

/* -----------------------------------------------------------------------
 * Test: source_row beyond source->rows causes csv_clear_row on dest
 * ----------------------------------------------------------------------- */
void test_csv_copy_row_source_row_out_of_range_clears_dest(void)
{
        /* dest has row 0 with data */
        append_row(dst);
        append_field(dst, 0);
        set_field(dst->field[0][0], "data");

        /* source has 1 row (index 0), request row 5 */
        fill_buffer(src, 1, 2, "x");

        int ret = csv_copy_row(dst, 0, src, 5);

        TEST_ASSERT_EQUAL_INT(0, ret);
        /* csv_clear_row leaves row with width 1 and empty field */
        TEST_ASSERT_EQUAL_INT(1, (int)dst->width[0]);
}

/* -----------------------------------------------------------------------
 * Test: dest_row does not exist yet — rows are appended automatically
 * ----------------------------------------------------------------------- */
void test_csv_copy_row_dest_row_appended_automatically(void)
{
        fill_buffer(src, 1, 2, "auto");

        /* dst starts empty */
        int ret = csv_copy_row(dst, 0, src, 0);

        TEST_ASSERT_EQUAL_INT(0, ret);
        TEST_ASSERT_EQUAL_INT(1, (int)dst->rows);
        TEST_ASSERT_EQUAL_INT(2, (int)dst->width[0]);
        TEST_ASSERT_EQUAL_STRING("auto", get_text(dst, 0, 0));
        TEST_ASSERT_EQUAL_STRING("auto", get_text(dst, 0, 1));
}

/* -----------------------------------------------------------------------
 * Test: dest row has MORE fields than source — excess fields removed
 * ----------------------------------------------------------------------- */
void test_csv_copy_row_dest_wider_than_source(void)
{
        /* source: 1 row, 2 fields */
        append_row(src);
        append_field(src, 0);
        set_field(src->field[0][0], "one");
        append_field(src, 0);
        set_field(src->field[0][1], "two");

        /* dest: 1 row, 5 fields */
        append_row(dst);
        int i;
        for (i = 0; i < 5; i++) {
                append_field(dst, 0);
                set_field(dst->field[0][i], "old");
        }

        int ret = csv_copy_row(dst, 0, src, 0);

        TEST_ASSERT_EQUAL_INT(0, ret);
        TEST_ASSERT_EQUAL_INT(2, (int)dst->width[0]);
        TEST_ASSERT_EQUAL_STRING("one", get_text(dst, 0, 0));
        TEST_ASSERT_EQUAL_STRING("two", get_text(dst, 0, 1));
}

/* -----------------------------------------------------------------------
 * Test: dest row has FEWER fields than source — fields appended
 * ----------------------------------------------------------------------- */
void test_csv_copy_row_dest_narrower_than_source(void)
{
        /* source: 1 row, 4 fields */
        append_row(src);
        int i;
        for (i = 0; i < 4; i++) {
                append_field(src, 0);
                set_field(src->field[0][i], "src_val");
        }

        /* dest: 1 row, 1 field */
        append_row(dst);
        append_field(dst, 0);
        set_field(dst->field[0][0], "old");

        int ret = csv_copy_row(dst, 0, src, 0);

        TEST_ASSERT_EQUAL_INT(0, ret);
        TEST_ASSERT_EQUAL_INT(4, (int)dst->width[0]);
        for (i = 0; i < 4; i++) {
                TEST_ASSERT_EQUAL_STRING("src_val", get_text(dst, 0, i));
        }
}

/* -----------------------------------------------------------------------
 * Test: copy second source row (index 1) to dest row 0
 * ----------------------------------------------------------------------- */
void test_csv_copy_row_second_source_row(void)
{
        /* source row 0 */
        append_row(src);
        append_field(src, 0);
        set_field(src->field[0][0], "row0");

        /* source row 1 */
        append_row(src);
        append_field(src, 1);
        set_field(src->field[1][0], "row1_col0");
        append_field(src, 1);
        set_field(src->field[1][1], "row1_col1");

        int ret = csv_copy_row(dst, 0, src, 1);

        TEST_ASSERT_EQUAL_INT(0, ret);
        TEST_ASSERT_EQUAL_INT(2, (int)dst->width[0]);
        TEST_ASSERT_EQUAL_STRING("row1_col0", get_text(dst, 0, 0));
        TEST_ASSERT_EQUAL_STRING("row1_col1", get_text(dst, 0, 1));
}

/* -----------------------------------------------------------------------
 * Test: copy to a non-zero dest_row, intermediate rows created
 * ----------------------------------------------------------------------- */
void test_csv_copy_row_dest_row_nonzero_creates_intermediate_rows(void)
{
        fill_buffer(src, 1, 1, "target");

        /* copy to dest row 2 — rows 0,1,2 must be created */
        int ret = csv_copy_row(dst, 2, src, 0);

        TEST_ASSERT_EQUAL_INT(0, ret);
        TEST_ASSERT_TRUE((int)dst->rows >= 3);
        TEST_ASSERT_EQUAL_STRING("target", get_text(dst, 2, 0));
}

/* -----------------------------------------------------------------------
 * Test: deep copy — modifying source after copy does not affect dest
 * ----------------------------------------------------------------------- */
void test_csv_copy_row_is_deep_copy(void)
{
        append_row(src);
        append_field(src, 0);
        set_field(src->field[0][0], "original");

        csv_copy_row(dst, 0, src, 0);

        /* Mutate source */
        set_field(src->field[0][0], "mutated");

        TEST_ASSERT_EQUAL_STRING("original", get_text(dst, 0, 0));
        TEST_ASSERT_EQUAL_STRING("mutated",  get_text(src, 0, 0));
}

/* -----------------------------------------------------------------------
 * Test: copy within same buffer (src == dst, different rows)
 * ----------------------------------------------------------------------- */
void test_csv_copy_row_same_buffer_different_rows(void)
{
        csv_destroy_buffer(dst);
        dst = NULL;

        /* Use src as both source and destination */
        append_row(src);
        append_field(src, 0);
        set_field(src->field[0][0], "same_buf");
        append_field(src, 0);
        set_field(src->field[0][1], "field2");

        int ret = csv_copy_row(src, 1, src, 0);

        TEST_ASSERT_EQUAL_INT(0, ret);
        TEST_ASSERT_TRUE((int)src->rows >= 2);
        TEST_ASSERT_EQUAL_INT(2, (int)src->width[1]);
        TEST_ASSERT_EQUAL_STRING("same_buf", get_text(src, 1, 0));
        TEST_ASSERT_EQUAL_STRING("field2",   get_text(src, 1, 1));

        dst = csv_create_buffer();
}

/* -----------------------------------------------------------------------
 * Test: source row exactly at boundary (last valid row)
 * ----------------------------------------------------------------------- */
void test_csv_copy_row_source_row_last_valid(void)
{
        fill_buffer(src, 3, 2, "boundary");

        int ret = csv_copy_row(dst, 0, src, 2);

        TEST_ASSERT_EQUAL_INT(0, ret);
        TEST_ASSERT_EQUAL_INT(2, (int)dst->width[0]);
        TEST_ASSERT_EQUAL_STRING("boundary", get_text(dst, 0, 0));
        TEST_ASSERT_EQUAL_STRING("boundary", get_text(dst, 0, 1));
}

/* -----------------------------------------------------------------------
 * Test: source row one past the last valid row triggers clear
 * ----------------------------------------------------------------------- */
void test_csv_copy_row_source_row_one_past_last(void)
{
        fill_buffer(src, 3, 2, "data");

        append_row(dst);
        append_field(dst, 0);
        set_field(dst->field[0][0], "should_clear");

        /* source has rows 0,1,2 — row 3 is out of range */
        int ret = csv_copy_row(dst, 0, src, 3);

        TEST_ASSERT_EQUAL_INT(0, ret);
        TEST_ASSERT_EQUAL_INT(1, (int)dst->width[0]);
}

/* -----------------------------------------------------------------------
 * Test: empty source row (0 fields) copied to dest
 * ----------------------------------------------------------------------- */
void test_csv_copy_row_empty_source_row(void)
{
        /* source row with no fields */
        append_row(src);
        /* width[0] == 0 */

        append_row(dst);
        append_field(dst, 0);
        set_field(dst->field[0][0], "remove_me");

        int ret = csv_copy_row(dst, 0, src, 0);

        TEST_ASSERT_EQUAL_INT(0, ret);
        TEST_ASSERT_EQUAL_INT(0, (int)dst->width[0]);
}

/* -----------------------------------------------------------------------
 * Test: return value is 0 on success
 * ----------------------------------------------------------------------- */
void test_csv_copy_row_returns_zero_on_success(void)
{
        fill_buffer(src, 1, 3, "ok");
        int ret = csv_copy_row(dst, 0, src, 0);
        TEST_ASSERT_EQUAL_INT(0, ret);
}

/* -----------------------------------------------------------------------
 * Test: width of dest row matches source row after copy
 * ----------------------------------------------------------------------- */
void test_csv_copy_row_dest_width_matches_source_width(void)
{
        fill_buffer(src, 1, 7, "w");
        csv_copy_row(dst, 0, src, 0);
        TEST_ASSERT_EQUAL_INT((int)src->width[0], (int)dst->width[0]);
}

/* -----------------------------------------------------------------------
 * main
 * ----------------------------------------------------------------------- */
int main(void)
{
        unity_install_sighandler();
        UNITY_BEGIN();
        RUN_TEST(test_csv_copy_row_basic_single_field);
        RUN_TEST(test_csv_copy_row_multiple_fields);
        RUN_TEST(test_csv_copy_row_source_row_out_of_range_clears_dest);
        RUN_TEST(test_csv_copy_row_dest_row_appended_automatically);
        RUN_TEST(test_csv_copy_row_dest_wider_than_source);
        RUN_TEST(test_csv_copy_row_dest_narrower_than_source);
        RUN_TEST(test_csv_copy_row_second_source_row);
        RUN_TEST(test_csv_copy_row_dest_row_nonzero_creates_intermediate_rows);
        RUN_TEST(test_csv_copy_row_is_deep_copy);
        RUN_TEST(test_csv_copy_row_same_buffer_different_rows);
        RUN_TEST(test_csv_copy_row_source_row_last_valid);
        RUN_TEST(test_csv_copy_row_source_row_one_past_last);
        RUN_TEST(test_csv_copy_row_empty_source_row);
        RUN_TEST(test_csv_copy_row_returns_zero_on_success);
        RUN_TEST(test_csv_copy_row_dest_width_matches_source_width);
        return UNITY_END();
}