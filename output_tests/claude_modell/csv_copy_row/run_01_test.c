#include "csv.c"
#include "unity.h"
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* File-scope fixtures                                                  */
/* ------------------------------------------------------------------ */
static CSV_BUFFER *src;
static CSV_BUFFER *dst;

/* ------------------------------------------------------------------ */
/* Helpers                                                              */
/* ------------------------------------------------------------------ */
static void fill_buffer_row(CSV_BUFFER *buf, int row,
                             const char **values, int count)
{
        int i;
        for (i = 0; i < count; i++) {
                csv_set_field(buf, row, i, (char *)values[i]);
        }
}

static void assert_row_equals(CSV_BUFFER *buf, int row,
                               const char **expected, int count)
{
        char tmp[256];
        int i;
        TEST_ASSERT_EQUAL_INT(count, csv_get_width(buf, row));
        for (i = 0; i < count; i++) {
                csv_get_field(tmp, sizeof(tmp), buf, row, i);
                TEST_ASSERT_EQUAL_STRING_MESSAGE(expected[i], tmp,
                        "field content mismatch");
        }
}

/* ------------------------------------------------------------------ */
/* setUp / tearDown                                                     */
/* ------------------------------------------------------------------ */
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
/* Test cases                                                           */
/* ------------------------------------------------------------------ */

/* 1. Copy a single-field row from source to an empty destination */
void test_copy_row_basic_single_field(void)
{
        const char *vals[] = {"hello"};
        csv_set_field(src, 0, 0, "hello");

        int ret = csv_copy_row(dst, 0, src, 0);

        TEST_ASSERT_EQUAL_INT(0, ret);
        assert_row_equals(dst, 0, vals, 1);
}

/* 2. Copy a multi-field row */
void test_copy_row_multiple_fields(void)
{
        const char *vals[] = {"alpha", "beta", "gamma"};
        fill_buffer_row(src, 0, vals, 3);

        int ret = csv_copy_row(dst, 0, src, 0);

        TEST_ASSERT_EQUAL_INT(0, ret);
        assert_row_equals(dst, 0, vals, 3);
}

/* 3. source_row beyond source->rows should clear dest row and return 0 */
void test_copy_row_source_row_out_of_range_clears_dest(void)
{
        /* populate dest row 0 first */
        csv_set_field(dst, 0, 0, "existing");

        /* src has no rows at all, so source_row 0 is out of range */
        int ret = csv_copy_row(dst, 0, src, 0);

        TEST_ASSERT_EQUAL_INT(0, ret);
        /* After csv_clear_row the row should exist with width 1 and empty */
        char tmp[64];
        csv_get_field(tmp, sizeof(tmp), dst, 0, 0);
        TEST_ASSERT_EQUAL_STRING("", tmp);
}

/* 4. Copy to a dest_row that does not yet exist (append_row must be called) */
void test_copy_row_dest_row_does_not_exist(void)
{
        const char *vals[] = {"x", "y"};
        fill_buffer_row(src, 0, vals, 2);

        /* dst has no rows; copy to row 0 */
        int ret = csv_copy_row(dst, 0, src, 0);

        TEST_ASSERT_EQUAL_INT(0, ret);
        TEST_ASSERT_EQUAL_INT(1, csv_get_height(dst));
        assert_row_equals(dst, 0, vals, 2);
}

/* 5. Copy to a dest_row that is several rows beyond current height */
void test_copy_row_dest_row_far_beyond_current_height(void)
{
        const char *vals[] = {"one", "two"};
        fill_buffer_row(src, 0, vals, 2);

        /* dst is empty; copy to row 2 — rows 0 and 1 must be created too */
        int ret = csv_copy_row(dst, 2, src, 0);

        TEST_ASSERT_EQUAL_INT(0, ret);
        TEST_ASSERT_EQUAL_INT(3, csv_get_height(dst));
        assert_row_equals(dst, 2, vals, 2);
}

/* 6. Destination row is wider than source row — fields must be removed */
void test_copy_row_dest_wider_than_source(void)
{
        const char *src_vals[] = {"a", "b"};
        const char *dst_vals[] = {"p", "q", "r", "s"};

        fill_buffer_row(src, 0, src_vals, 2);
        fill_buffer_row(dst, 0, dst_vals, 4);

        int ret = csv_copy_row(dst, 0, src, 0);

        TEST_ASSERT_EQUAL_INT(0, ret);
        assert_row_equals(dst, 0, src_vals, 2);
}

/* 7. Destination row is narrower than source row — fields must be appended */
void test_copy_row_dest_narrower_than_source(void)
{
        const char *src_vals[] = {"1", "2", "3", "4"};
        const char *dst_vals[] = {"x"};

        fill_buffer_row(src, 0, src_vals, 4);
        fill_buffer_row(dst, 0, dst_vals, 1);

        int ret = csv_copy_row(dst, 0, src, 0);

        TEST_ASSERT_EQUAL_INT(0, ret);
        assert_row_equals(dst, 0, src_vals, 4);
}

/* 8. Copy a row within the same buffer (src == dst, different rows) */
void test_copy_row_same_buffer_different_rows(void)
{
        const char *vals[] = {"same", "buffer"};
        fill_buffer_row(src, 0, vals, 2);

        int ret = csv_copy_row(src, 1, src, 0);

        TEST_ASSERT_EQUAL_INT(0, ret);
        assert_row_equals(src, 1, vals, 2);
        /* original row must be unchanged */
        assert_row_equals(src, 0, vals, 2);
}

/* 9. Copy a non-zero source row */
void test_copy_row_non_zero_source_row(void)
{
        const char *row0[] = {"ignore", "me"};
        const char *row1[] = {"copy", "this", "row"};

        fill_buffer_row(src, 0, row0, 2);
        fill_buffer_row(src, 1, row1, 3);

        int ret = csv_copy_row(dst, 0, src, 1);

        TEST_ASSERT_EQUAL_INT(0, ret);
        assert_row_equals(dst, 0, row1, 3);
}

/* 10. Copy to a non-zero dest row */
void test_copy_row_non_zero_dest_row(void)
{
        const char *vals[] = {"data"};
        fill_buffer_row(src, 0, vals, 1);

        /* pre-populate dst row 0 so row 1 is the target */
        const char *filler[] = {"filler"};
        fill_buffer_row(dst, 0, filler, 1);

        int ret = csv_copy_row(dst, 1, src, 0);

        TEST_ASSERT_EQUAL_INT(0, ret);
        TEST_ASSERT_EQUAL_INT(2, csv_get_height(dst));
        assert_row_equals(dst, 1, vals, 1);
        /* row 0 must be untouched */
        assert_row_equals(dst, 0, filler, 1);
}

/* 11. Copy a row with an empty string field */
void test_copy_row_empty_string_field(void)
{
        const char *vals[] = {"", "nonempty", ""};
        fill_buffer_row(src, 0, vals, 3);

        int ret = csv_copy_row(dst, 0, src, 0);

        TEST_ASSERT_EQUAL_INT(0, ret);
        assert_row_equals(dst, 0, vals, 3);
}

/* 12. Return value is 0 on a successful copy */
void test_copy_row_return_value_success(void)
{
        csv_set_field(src, 0, 0, "test");
        int ret = csv_copy_row(dst, 0, src, 0);
        TEST_ASSERT_EQUAL_INT(0, ret);
}

/* 13. source_row == source->rows - 1 (last valid row) should succeed */
void test_copy_row_last_valid_source_row(void)
{
        const char *row0[] = {"r0c0"};
        const char *row1[] = {"r1c0", "r1c1"};

        fill_buffer_row(src, 0, row0, 1);
        fill_buffer_row(src, 1, row1, 2);

        int ret = csv_copy_row(dst, 0, src, 1);

        TEST_ASSERT_EQUAL_INT(0, ret);
        assert_row_equals(dst, 0, row1, 2);
}

/* 14. source_row == source->rows (one past the end) clears dest */
void test_copy_row_source_row_one_past_end_clears_dest(void)
{
        const char *row0[] = {"existing_data"};
        fill_buffer_row(src, 0, row0, 1);
        fill_buffer_row(dst, 0, row0, 1);

        /* source has 1 row (index 0); request row 1 which is out of range */
        int ret = csv_copy_row(dst, 0, src, 1);

        TEST_ASSERT_EQUAL_INT(0, ret);
        char tmp[64];
        csv_get_field(tmp, sizeof(tmp), dst, 0, 0);
        TEST_ASSERT_EQUAL_STRING("", tmp);
}

/* 15. Copying a row with a long string value */
void test_copy_row_long_field_value(void)
{
        char long_str[200];
        memset(long_str, 'A', 199);
        long_str[199] = '\0';

        csv_set_field(src, 0, 0, long_str);

        int ret = csv_copy_row(dst, 0, src, 0);

        TEST_ASSERT_EQUAL_INT(0, ret);

        char tmp[256];
        csv_get_field(tmp, sizeof(tmp), dst, 0, 0);
        TEST_ASSERT_EQUAL_STRING(long_str, tmp);
}

/* ------------------------------------------------------------------ */
/* main                                                                 */
/* ------------------------------------------------------------------ */
int main(void)
{
        unity_install_sighandler();
        UNITY_BEGIN();
        RUN_TEST(test_copy_row_basic_single_field);
        RUN_TEST(test_copy_row_multiple_fields);
        RUN_TEST(test_copy_row_source_row_out_of_range_clears_dest);
        RUN_TEST(test_copy_row_dest_row_does_not_exist);
        RUN_TEST(test_copy_row_dest_row_far_beyond_current_height);
        RUN_TEST(test_copy_row_dest_wider_than_source);
        RUN_TEST(test_copy_row_dest_narrower_than_source);
        RUN_TEST(test_copy_row_same_buffer_different_rows);
        RUN_TEST(test_copy_row_non_zero_source_row);
        RUN_TEST(test_copy_row_non_zero_dest_row);
        RUN_TEST(test_copy_row_empty_string_field);
        RUN_TEST(test_copy_row_return_value_success);
        RUN_TEST(test_copy_row_last_valid_source_row);
        RUN_TEST(test_copy_row_source_row_one_past_end_clears_dest);
        RUN_TEST(test_copy_row_long_field_value);
        return UNITY_END();
}