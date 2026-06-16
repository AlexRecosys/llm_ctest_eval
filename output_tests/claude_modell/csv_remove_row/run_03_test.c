#include "csv.c"
#include "unity.h"

/* File-scope fixtures */
static CSV_BUFFER *buf;

/* Helper: populate buffer with known data using csv_set_field */
static void populate_buffer(CSV_BUFFER *b, int rows, int cols, const char *prefix)
{
    int r, c;
    char text[64];
    for (r = 0; r < rows; r++) {
        for (c = 0; c < cols; c++) {
            snprintf(text, sizeof(text), "%s_r%d_c%d", prefix, r, c);
            csv_set_field(b, (size_t)r, (size_t)c, text);
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

/* ------------------------------------------------------------------ */
/* Test: remove the only row in a single-row buffer                    */
/* ------------------------------------------------------------------ */
void test_remove_row_single_row_reduces_height_to_zero(void)
{
    csv_set_field(buf, 0, 0, "only");
    TEST_ASSERT_EQUAL_INT(1, csv_get_height(buf));

    int ret = csv_remove_row(buf, 0);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(0, csv_get_height(buf));
}

/* ------------------------------------------------------------------ */
/* Test: remove first row shifts remaining rows up                     */
/* ------------------------------------------------------------------ */
void test_remove_row_first_row_shifts_remaining(void)
{
    populate_buffer(buf, 3, 2, "v");
    /* rows: v_r0_c0, v_r0_c1 | v_r1_c0, v_r1_c1 | v_r2_c0, v_r2_c1 */

    int ret = csv_remove_row(buf, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(2, csv_get_height(buf));

    char dest[64];
    csv_get_field(dest, sizeof(dest), buf, 0, 0);
    TEST_ASSERT_EQUAL_STRING("v_r1_c0", dest);

    csv_get_field(dest, sizeof(dest), buf, 0, 1);
    TEST_ASSERT_EQUAL_STRING("v_r1_c1", dest);

    csv_get_field(dest, sizeof(dest), buf, 1, 0);
    TEST_ASSERT_EQUAL_STRING("v_r2_c0", dest);

    csv_get_field(dest, sizeof(dest), buf, 1, 1);
    TEST_ASSERT_EQUAL_STRING("v_r2_c1", dest);
}

/* ------------------------------------------------------------------ */
/* Test: remove middle row                                             */
/* ------------------------------------------------------------------ */
void test_remove_row_middle_row_shifts_remaining(void)
{
    populate_buffer(buf, 3, 2, "m");

    int ret = csv_remove_row(buf, 1);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(2, csv_get_height(buf));

    char dest[64];
    /* row 0 unchanged */
    csv_get_field(dest, sizeof(dest), buf, 0, 0);
    TEST_ASSERT_EQUAL_STRING("m_r0_c0", dest);

    /* old row 2 is now row 1 */
    csv_get_field(dest, sizeof(dest), buf, 1, 0);
    TEST_ASSERT_EQUAL_STRING("m_r2_c0", dest);

    csv_get_field(dest, sizeof(dest), buf, 1, 1);
    TEST_ASSERT_EQUAL_STRING("m_r2_c1", dest);
}

/* ------------------------------------------------------------------ */
/* Test: remove last row                                               */
/* ------------------------------------------------------------------ */
void test_remove_row_last_row_reduces_height(void)
{
    populate_buffer(buf, 3, 2, "l");

    int ret = csv_remove_row(buf, 2);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(2, csv_get_height(buf));

    char dest[64];
    csv_get_field(dest, sizeof(dest), buf, 0, 0);
    TEST_ASSERT_EQUAL_STRING("l_r0_c0", dest);

    csv_get_field(dest, sizeof(dest), buf, 1, 0);
    TEST_ASSERT_EQUAL_STRING("l_r1_c0", dest);
}

/* ------------------------------------------------------------------ */
/* Test: row index out of bounds returns 0 and leaves buffer intact   */
/* ------------------------------------------------------------------ */
void test_remove_row_out_of_bounds_returns_zero_no_change(void)
{
    populate_buffer(buf, 2, 2, "ob");

    int ret = csv_remove_row(buf, 5);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(2, csv_get_height(buf));

    char dest[64];
    csv_get_field(dest, sizeof(dest), buf, 0, 0);
    TEST_ASSERT_EQUAL_STRING("ob_r0_c0", dest);

    csv_get_field(dest, sizeof(dest), buf, 1, 0);
    TEST_ASSERT_EQUAL_STRING("ob_r1_c0", dest);
}

/* ------------------------------------------------------------------ */
/* Test: row index exactly equal to rows (one past last) returns 0    */
/* ------------------------------------------------------------------ */
void test_remove_row_index_equals_rows_returns_zero(void)
{
    populate_buffer(buf, 3, 1, "eq");

    /* rows == 3, valid indices 0-2; index 3 is out of bounds */
    int ret = csv_remove_row(buf, 3);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(3, csv_get_height(buf));
}

/* ------------------------------------------------------------------ */
/* Test: remove row preserves field widths of remaining rows          */
/* ------------------------------------------------------------------ */
void test_remove_row_preserves_widths_of_remaining_rows(void)
{
    /* row 0: 3 fields, row 1: 2 fields, row 2: 4 fields */
    csv_set_field(buf, 0, 0, "a");
    csv_set_field(buf, 0, 1, "b");
    csv_set_field(buf, 0, 2, "c");
    csv_set_field(buf, 1, 0, "d");
    csv_set_field(buf, 1, 1, "e");
    csv_set_field(buf, 2, 0, "f");
    csv_set_field(buf, 2, 1, "g");
    csv_set_field(buf, 2, 2, "h");
    csv_set_field(buf, 2, 3, "i");

    /* remove row 1 (2 fields) */
    int ret = csv_remove_row(buf, 1);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(2, csv_get_height(buf));

    TEST_ASSERT_EQUAL_INT(3, csv_get_width(buf, 0));
    TEST_ASSERT_EQUAL_INT(4, csv_get_width(buf, 1));
}

/* ------------------------------------------------------------------ */
/* Test: remove row 0 from two-row buffer leaves correct single row   */
/* ------------------------------------------------------------------ */
void test_remove_row_first_of_two_leaves_second(void)
{
    csv_set_field(buf, 0, 0, "first");
    csv_set_field(buf, 1, 0, "second");

    int ret = csv_remove_row(buf, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(1, csv_get_height(buf));

    char dest[64];
    csv_get_field(dest, sizeof(dest), buf, 0, 0);
    TEST_ASSERT_EQUAL_STRING("second", dest);
}

/* ------------------------------------------------------------------ */
/* Test: remove row 1 from two-row buffer leaves correct single row   */
/* ------------------------------------------------------------------ */
void test_remove_row_second_of_two_leaves_first(void)
{
    csv_set_field(buf, 0, 0, "first");
    csv_set_field(buf, 1, 0, "second");

    int ret = csv_remove_row(buf, 1);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(1, csv_get_height(buf));

    char dest[64];
    csv_get_field(dest, sizeof(dest), buf, 0, 0);
    TEST_ASSERT_EQUAL_STRING("first", dest);
}

/* ------------------------------------------------------------------ */
/* Test: successive removals work correctly                            */
/* ------------------------------------------------------------------ */
void test_remove_row_successive_removals(void)
{
    populate_buffer(buf, 4, 1, "s");
    /* rows: s_r0_c0, s_r1_c0, s_r2_c0, s_r3_c0 */

    csv_remove_row(buf, 0); /* removes s_r0_c0 */
    TEST_ASSERT_EQUAL_INT(3, csv_get_height(buf));

    char dest[64];
    csv_get_field(dest, sizeof(dest), buf, 0, 0);
    TEST_ASSERT_EQUAL_STRING("s_r1_c0", dest);

    csv_remove_row(buf, 1); /* removes s_r2_c0 */
    TEST_ASSERT_EQUAL_INT(2, csv_get_height(buf));

    csv_get_field(dest, sizeof(dest), buf, 0, 0);
    TEST_ASSERT_EQUAL_STRING("s_r1_c0", dest);

    csv_get_field(dest, sizeof(dest), buf, 1, 0);
    TEST_ASSERT_EQUAL_STRING("s_r3_c0", dest);
}

/* ------------------------------------------------------------------ */
/* Test: remove row from empty buffer (rows == 0) returns 0           */
/* ------------------------------------------------------------------ */
void test_remove_row_empty_buffer_returns_zero(void)
{
    /* buf has 0 rows; any index should be out of bounds */
    TEST_ASSERT_EQUAL_INT(0, csv_get_height(buf));
    int ret = csv_remove_row(buf, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(0, csv_get_height(buf));
}

/* ------------------------------------------------------------------ */
/* main                                                                */
/* ------------------------------------------------------------------ */
int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_remove_row_single_row_reduces_height_to_zero);
    RUN_TEST(test_remove_row_first_row_shifts_remaining);
    RUN_TEST(test_remove_row_middle_row_shifts_remaining);
    RUN_TEST(test_remove_row_last_row_reduces_height);
    RUN_TEST(test_remove_row_out_of_bounds_returns_zero_no_change);
    RUN_TEST(test_remove_row_index_equals_rows_returns_zero);
    RUN_TEST(test_remove_row_preserves_widths_of_remaining_rows);
    RUN_TEST(test_remove_row_first_of_two_leaves_second);
    RUN_TEST(test_remove_row_second_of_two_leaves_first);
    RUN_TEST(test_remove_row_successive_removals);
    RUN_TEST(test_remove_row_empty_buffer_returns_zero);
    return UNITY_END();
}