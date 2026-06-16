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

/* ------------------------------------------------------------------ */
/* Test: copy a single-field row from source row 0 to dest row 0      */
/* ------------------------------------------------------------------ */
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

/* ------------------------------------------------------------------ */
/* Test: copy a multi-field row                                        */
/* ------------------------------------------------------------------ */
void test_csv_copy_row_multiple_fields(void)
{
        append_row(src);
        append_field(src, 0); set_field(src->field[0][0], "alpha");
        append_field(src, 0); set_field(src->field[0][1], "beta");
        append_field(src, 0); set_field(src->field[0][2], "gamma");

        int ret = csv_copy_row(dst, 0, src, 0);

        TEST_ASSERT_EQUAL_INT(0, ret);
        TEST_ASSERT_EQUAL_INT(3, (int)dst->width[0]);
        TEST_ASSERT_EQUAL_STRING("alpha", get_text(dst, 0, 0));
        TEST_ASSERT_EQUAL_STRING("beta",  get_text(dst, 0, 1));
        TEST_ASSERT_EQUAL_STRING("gamma", get_text(dst, 0, 2));
}

/* ------------------------------------------------------------------ */
/* Test: source_row beyond source->rows triggers csv_clear_row        */
/* ------------------------------------------------------------------ */
void test_csv_copy_row_source_row_out_of_range(void)
{
        fill_buffer(src, 1, 2, "data");

        /* dest has a row with content */
        append_row(dst);
        append_field(dst, 0); set_field(dst->field[0][0], "keep");
        append_field(dst, 0); set_field(dst->field[0][1], "keep2");

        /* source_row = 5 does not exist in a 1-row source */
        int ret = csv_copy_row(dst, 0, src, 5);

        TEST_ASSERT_EQUAL_INT(0, ret);
}

/* ------------------------------------------------------------------ */
/* Test: dest row is automatically appended when it does not exist     */
/* ------------------------------------------------------------------ */
void test_csv_copy_row_dest_row_auto_appended(void)
{
        fill_buffer(src, 1, 2, "x");

        /* dst is empty — dest_row 0 does not exist yet */
        TEST_ASSERT_EQUAL_INT(0, (int)dst->rows);

        int ret = csv_copy_row(dst, 0, src, 0);

        TEST_ASSERT_EQUAL_INT(0, ret);
        TEST_ASSERT_EQUAL_INT(1, (int)dst->rows);
        TEST_ASSERT_EQUAL_INT(2, (int)dst->width[0]);
}

/* ------------------------------------------------------------------ */
/* Test: dest row is appended even when dest_row > 0 and dst is empty */
/* ------------------------------------------------------------------ */
void test_csv_copy_row_dest_row_2_auto_appended(void)
{
        fill_buffer(src, 3, 2, "v");

        int ret = csv_copy_row(dst, 2, src, 2);

        TEST_ASSERT_EQUAL_INT(0, ret);
        TEST_ASSERT_TRUE((int)dst->rows >= 3);
        TEST_ASSERT_EQUAL_INT(2, (int)dst->width[2]);
        TEST_ASSERT_EQUAL_STRING("v", get_text(dst, 2, 0));
        TEST_ASSERT_EQUAL_STRING("v", get_text(dst, 2, 1));
}

/* ------------------------------------------------------------------ */
/* Test: dest row wider than source — extra fields are removed         */
/* ------------------------------------------------------------------ */
void test_csv_copy_row_dest_wider_than_source(void)
{
        /* source: 1 row, 2 fields */
        append_row(src);
        append_field(src, 0); set_field(src->field[0][0], "A");
        append_field(src, 0); set_field(src->field[0][1], "B");

        /* dest: 1 row, 4 fields */
        append_row(dst);
        append_field(dst, 0); set_field(dst->field[0][0], "1");
        append_field(dst, 0); set_field(dst->field[0][1], "2");
        append_field(dst, 0); set_field(dst->field[0][2], "3");
        append_field(dst, 0); set_field(dst->field[0][3], "4");

        int ret = csv_copy_row(dst, 0, src, 0);

        TEST_ASSERT_EQUAL_INT(0, ret);
        TEST_ASSERT_EQUAL_INT(2, (int)dst->width[0]);
        TEST_ASSERT_EQUAL_STRING("A", get_text(dst, 0, 0));
        TEST_ASSERT_EQUAL_STRING("B", get_text(dst, 0, 1));
}

/* ------------------------------------------------------------------ */
/* Test: dest row narrower than source — fields are appended           */
/* ------------------------------------------------------------------ */
void test_csv_copy_row_dest_narrower_than_source(void)
{
        /* source: 1 row, 4 fields */
        append_row(src);
        append_field(src, 0); set_field(src->field[0][0], "W");
        append_field(src, 0); set_field(src->field[0][1], "X");
        append_field(src, 0); set_field(src->field[0][2], "Y");
        append_field(src, 0); set_field(src->field[0][3], "Z");

        /* dest: 1 row, 1 field */
        append_row(dst);
        append_field(dst, 0); set_field(dst->field[0][0], "old");

        int ret = csv_copy_row(dst, 0, src, 0);

        TEST_ASSERT_EQUAL_INT(0, ret);
        TEST_ASSERT_EQUAL_INT(4, (int)dst->width[0]);
        TEST_ASSERT_EQUAL_STRING("W", get_text(dst, 0, 0));
        TEST_ASSERT_EQUAL_STRING("X", get_text(dst, 0, 1));
        TEST_ASSERT_EQUAL_STRING("Y", get_text(dst, 0, 2));
        TEST_ASSERT_EQUAL_STRING("Z", get_text(dst, 0, 3));
}

/* ------------------------------------------------------------------ */
/* Test: copy within the same buffer (src == dst, different rows)      */
/* ------------------------------------------------------------------ */
void test_csv_copy_row_same_buffer_different_rows(void)
{
        /* Use a single buffer as both source and destination */
        CSV_BUFFER *buf = csv_create_buffer();

        append_row(buf);
        append_field(buf, 0); set_field(buf->field[0][0], "row0col0");
        append_field(buf, 0); set_field(buf->field[0][1], "row0col1");

        append_row(buf);
        append_field(buf, 1); set_field(buf->field[1][0], "row1col0");

        int ret = csv_copy_row(buf, 1, buf, 0);

        TEST_ASSERT_EQUAL_INT(0, ret);
        TEST_ASSERT_EQUAL_INT(2, (int)buf->width[1]);
        TEST_ASSERT_EQUAL_STRING("row0col0", get_text(buf, 1, 0));
        TEST_ASSERT_EQUAL_STRING("row0col1", get_text(buf, 1, 1));

        csv_destroy_buffer(buf);
}

/* ------------------------------------------------------------------ */
/* Test: copy preserves existing rows in dest that are not the target  */
/* ------------------------------------------------------------------ */
void test_csv_copy_row_does_not_corrupt_other_dest_rows(void)
{
        fill_buffer(src, 1, 2, "new");

        append_row(dst);
        append_field(dst, 0); set_field(dst->field[0][0], "untouched");

        append_row(dst);
        append_field(dst, 1); set_field(dst->field[1][0], "old");

        int ret = csv_copy_row(dst, 1, src, 0);

        TEST_ASSERT_EQUAL_INT(0, ret);
        /* row 0 must be untouched */
        TEST_ASSERT_EQUAL_STRING("untouched", get_text(dst, 0, 0));
        /* row 1 must reflect the copy */
        TEST_ASSERT_EQUAL_INT(2, (int)dst->width[1]);
        TEST_ASSERT_EQUAL_STRING("new", get_text(dst, 1, 0));
        TEST_ASSERT_EQUAL_STRING("new", get_text(dst, 1, 1));
}

/* ------------------------------------------------------------------ */
/* Test: copy row with empty string fields                             */
/* ------------------------------------------------------------------ */
void test_csv_copy_row_empty_string_fields(void)
{
        append_row(src);
        append_field(src, 0); set_field(src->field[0][0], "");
        append_field(src, 0); set_field(src->field[0][1], "");

        int ret = csv_copy_row(dst, 0, src, 0);

        TEST_ASSERT_EQUAL_INT(0, ret);
        TEST_ASSERT_EQUAL_INT(2, (int)dst->width[0]);
        TEST_ASSERT_EQUAL_STRING("", get_text(dst, 0, 0));
        TEST_ASSERT_EQUAL_STRING("", get_text(dst, 0, 1));
}

/* ------------------------------------------------------------------ */
/* Test: copy second row of multi-row source                           */
/* ------------------------------------------------------------------ */
void test_csv_copy_row_second_source_row(void)
{
        fill_buffer(src, 3, 2, "ignore");
        /* Override row 1 with distinct values */
        set_field(src->field[1][0], "r1c0");
        set_field(src->field[1][1], "r1c1");

        int ret = csv_copy_row(dst, 0, src, 1);

        TEST_ASSERT_EQUAL_INT(0, ret);
        TEST_ASSERT_EQUAL_INT(2, (int)dst->width[0]);
        TEST_ASSERT_EQUAL_STRING("r1c0", get_text(dst, 0, 0));
        TEST_ASSERT_EQUAL_STRING("r1c1", get_text(dst, 0, 1));
}

/* ------------------------------------------------------------------ */
/* Test: return value is 0 on success for a normal copy               */
/* ------------------------------------------------------------------ */
void test_csv_copy_row_return_zero_on_success(void)
{
        fill_buffer(src, 2, 3, "ok");
        int ret = csv_copy_row(dst, 0, src, 0);
        TEST_ASSERT_EQUAL_INT(0, ret);
}

/* ------------------------------------------------------------------ */
/* Test: source row exactly at boundary (last valid row)               */
/* ------------------------------------------------------------------ */
void test_csv_copy_row_source_row_at_boundary(void)
{
        fill_buffer(src, 3, 2, "boundary");

        /* source_row = 2 is the last valid row (rows-1) */
        int ret = csv_copy_row(dst, 0, src, 2);

        TEST_ASSERT_EQUAL_INT(0, ret);
        TEST_ASSERT_EQUAL_INT(2, (int)dst->width[0]);
        TEST_ASSERT_EQUAL_STRING("boundary", get_text(dst, 0, 0));
        TEST_ASSERT_EQUAL_STRING("boundary", get_text(dst, 0, 1));
}

/* ------------------------------------------------------------------ */
/* Test: source_row == source->rows (one past last) clears dest row   */
/* ------------------------------------------------------------------ */
void test_csv_copy_row_source_row_one_past_last(void)
{
        fill_buffer(src, 2, 2, "data");

        append_row(dst);
        append_field(dst, 0); set_field(dst->field[0][0], "will_clear");

        /* source has 2 rows (0,1); source_row=2 is out of range */
        int ret = csv_copy_row(dst, 0, src, 2);

        TEST_ASSERT_EQUAL_INT(0, ret);
}

/* ------------------------------------------------------------------ */
/* Test: deep copy — modifying source after copy does not affect dest  */
/* ------------------------------------------------------------------ */
void test_csv_copy_row_is_deep_copy(void)
{
        append_row(src);
        append_field(src, 0); set_field(src->field[0][0], "original");

        csv_copy_row(dst, 0, src, 0);

        /* Mutate source */
        set_field(src->field[0][0], "mutated");

        /* dest must still hold the original value */
        TEST_ASSERT_EQUAL_STRING("original", get_text(dst, 0, 0));
}

/* ------------------------------------------------------------------ */
/* main                                                                */
/* ------------------------------------------------------------------ */
int main(void)
{
        unity_install_sighandler();
        UNITY_BEGIN();
        RUN_TEST(test_csv_copy_row_basic_single_field);
        RUN_TEST(test_csv_copy_row_multiple_fields);
        RUN_TEST(test_csv_copy_row_source_row_out_of_range);
        RUN_TEST(test_csv_copy_row_dest_row_auto_appended);
        RUN_TEST(test_csv_copy_row_dest_row_2_auto_appended);
        RUN_TEST(test_csv_copy_row_dest_wider_than_source);
        RUN_TEST(test_csv_copy_row_dest_narrower_than_source);
        RUN_TEST(test_csv_copy_row_same_buffer_different_rows);
        RUN_TEST(test_csv_copy_row_does_not_corrupt_other_dest_rows);
        RUN_TEST(test_csv_copy_row_empty_string_fields);
        RUN_TEST(test_csv_copy_row_second_source_row);
        RUN_TEST(test_csv_copy_row_return_zero_on_success);
        RUN_TEST(test_csv_copy_row_source_row_at_boundary);
        RUN_TEST(test_csv_copy_row_source_row_one_past_last);
        RUN_TEST(test_csv_copy_row_is_deep_copy);
        return UNITY_END();
}