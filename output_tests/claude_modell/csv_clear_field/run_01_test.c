#include "csv.c"
#include "unity.h"
#include <stdlib.h>
#include <string.h>

/* File-scope fixtures */
static CSV_BUFFER *buf;

/* Helper: populate a buffer with given rows/cols and set field text */
static void populate_buffer(CSV_BUFFER *b, int rows, int cols, const char *val)
{
        int r, c;
        for (r = 0; r < rows; r++) {
                append_row(b);
                for (c = 0; c < cols; c++) {
                        append_field(b, (size_t)r);
                        set_field(b->field[r][c], (char *)val);
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
/* Test: clearing a field that is out of range (row too large)         */
/* ------------------------------------------------------------------ */
void test_clear_field_row_out_of_range_returns_zero(void)
{
        populate_buffer(buf, 1, 3, "hello");
        int ret = csv_clear_field(buf, 5, 0);
        TEST_ASSERT_EQUAL_INT(0, ret);
}

/* ------------------------------------------------------------------ */
/* Test: clearing a field that is out of range (entry too large)       */
/* ------------------------------------------------------------------ */
void test_clear_field_entry_out_of_range_returns_zero(void)
{
        populate_buffer(buf, 1, 3, "hello");
        int ret = csv_clear_field(buf, 0, 10);
        TEST_ASSERT_EQUAL_INT(0, ret);
}

/* ------------------------------------------------------------------ */
/* Test: clearing a field that is out of range does not change width   */
/* ------------------------------------------------------------------ */
void test_clear_field_out_of_range_does_not_change_width(void)
{
        populate_buffer(buf, 1, 3, "hello");
        csv_clear_field(buf, 0, 10);
        TEST_ASSERT_EQUAL_INT(3, (int)buf->width[0]);
}

/* ------------------------------------------------------------------ */
/* Test: clearing a middle field sets its text to empty string         */
/* ------------------------------------------------------------------ */
void test_clear_field_middle_field_sets_empty_string(void)
{
        char dest[64];
        populate_buffer(buf, 1, 3, "hello");
        int ret = csv_clear_field(buf, 0, 1);
        TEST_ASSERT_EQUAL_INT(0, ret);
        csv_get_field(dest, sizeof(dest), buf, 0, 1);
        /* After clearing, the field should be empty */
        TEST_ASSERT_EQUAL_INT(2, csv_get_field(dest, sizeof(dest), buf, 0, 1));
}

/* ------------------------------------------------------------------ */
/* Test: clearing a middle field does not change row width             */
/* ------------------------------------------------------------------ */
void test_clear_field_middle_field_does_not_change_width(void)
{
        populate_buffer(buf, 1, 3, "hello");
        csv_clear_field(buf, 0, 1);
        TEST_ASSERT_EQUAL_INT(3, (int)buf->width[0]);
}

/* ------------------------------------------------------------------ */
/* Test: clearing the first field (entry 0) sets it to empty string   */
/* ------------------------------------------------------------------ */
void test_clear_field_first_field_sets_empty_string(void)
{
        char dest[64];
        populate_buffer(buf, 1, 3, "hello");
        int ret = csv_clear_field(buf, 0, 0);
        TEST_ASSERT_EQUAL_INT(0, ret);
        TEST_ASSERT_EQUAL_INT(2, csv_get_field(dest, sizeof(dest), buf, 0, 0));
}

/* ------------------------------------------------------------------ */
/* Test: clearing the first field does not change row width            */
/* ------------------------------------------------------------------ */
void test_clear_field_first_field_does_not_change_width(void)
{
        populate_buffer(buf, 1, 3, "hello");
        csv_clear_field(buf, 0, 0);
        TEST_ASSERT_EQUAL_INT(3, (int)buf->width[0]);
}

/* ------------------------------------------------------------------ */
/* Test: clearing the last field (not entry 0) removes it             */
/* ------------------------------------------------------------------ */
void test_clear_field_last_field_not_first_removes_field(void)
{
        populate_buffer(buf, 1, 3, "hello");
        /* entry 2 is last and not entry 0 */
        int ret = csv_clear_field(buf, 0, 2);
        TEST_ASSERT_EQUAL_INT(0, ret);
        TEST_ASSERT_EQUAL_INT(2, (int)buf->width[0]);
}

/* ------------------------------------------------------------------ */
/* Test: clearing the last field decrements width by one              */
/* ------------------------------------------------------------------ */
void test_clear_field_last_field_decrements_width(void)
{
        populate_buffer(buf, 1, 4, "data");
        csv_clear_field(buf, 0, 3);
        TEST_ASSERT_EQUAL_INT(3, (int)buf->width[0]);
}

/* ------------------------------------------------------------------ */
/* Test: clearing the only field (entry 0, last) sets it to empty     */
/* ------------------------------------------------------------------ */
void test_clear_field_only_field_sets_empty_string(void)
{
        char dest[64];
        populate_buffer(buf, 1, 1, "only");
        /* entry == 0 and entry == width-1, but entry == 0 so set_field path */
        int ret = csv_clear_field(buf, 0, 0);
        TEST_ASSERT_EQUAL_INT(0, ret);
        TEST_ASSERT_EQUAL_INT(2, csv_get_field(dest, sizeof(dest), buf, 0, 0));
}

/* ------------------------------------------------------------------ */
/* Test: clearing the only field does not change width                 */
/* ------------------------------------------------------------------ */
void test_clear_field_only_field_does_not_change_width(void)
{
        populate_buffer(buf, 1, 1, "only");
        csv_clear_field(buf, 0, 0);
        TEST_ASSERT_EQUAL_INT(1, (int)buf->width[0]);
}

/* ------------------------------------------------------------------ */
/* Test: clearing a field in a second row works correctly              */
/* ------------------------------------------------------------------ */
void test_clear_field_second_row_middle_field(void)
{
        char dest[64];
        populate_buffer(buf, 2, 3, "test");
        int ret = csv_clear_field(buf, 1, 1);
        TEST_ASSERT_EQUAL_INT(0, ret);
        TEST_ASSERT_EQUAL_INT(2, csv_get_field(dest, sizeof(dest), buf, 1, 1));
}

/* ------------------------------------------------------------------ */
/* Test: clearing last field in second row removes it                  */
/* ------------------------------------------------------------------ */
void test_clear_field_second_row_last_field_removes(void)
{
        populate_buffer(buf, 2, 3, "test");
        csv_clear_field(buf, 1, 2);
        TEST_ASSERT_EQUAL_INT(2, (int)buf->width[1]);
}

/* ------------------------------------------------------------------ */
/* Test: clearing last field does not affect other rows                */
/* ------------------------------------------------------------------ */
void test_clear_field_does_not_affect_other_rows(void)
{
        populate_buffer(buf, 2, 3, "test");
        csv_clear_field(buf, 0, 2);
        /* Row 1 should still have width 3 */
        TEST_ASSERT_EQUAL_INT(3, (int)buf->width[1]);
}

/* ------------------------------------------------------------------ */
/* Test: clearing a field on empty buffer (0 rows) returns 0           */
/* ------------------------------------------------------------------ */
void test_clear_field_empty_buffer_returns_zero(void)
{
        int ret = csv_clear_field(buf, 0, 0);
        TEST_ASSERT_EQUAL_INT(0, ret);
}

/* ------------------------------------------------------------------ */
/* Test: return value is always 0 for valid in-range field             */
/* ------------------------------------------------------------------ */
void test_clear_field_returns_zero_for_valid_field(void)
{
        populate_buffer(buf, 1, 2, "value");
        int ret = csv_clear_field(buf, 0, 0);
        TEST_ASSERT_EQUAL_INT(0, ret);
}

/* ------------------------------------------------------------------ */
/* Test: adjacent fields are unaffected when middle field is cleared   */
/* ------------------------------------------------------------------ */
void test_clear_field_adjacent_fields_unaffected(void)
{
        char dest[64];
        populate_buffer(buf, 1, 3, "abc");
        csv_clear_field(buf, 0, 1);
        csv_get_field(dest, sizeof(dest), buf, 0, 0);
        TEST_ASSERT_EQUAL_STRING("abc", dest);
        csv_get_field(dest, sizeof(dest), buf, 0, 2);
        TEST_ASSERT_EQUAL_STRING("abc", dest);
}

/* ------------------------------------------------------------------ */
/* Test: clearing last field twice reduces width by one each time      */
/* ------------------------------------------------------------------ */
void test_clear_field_last_field_twice_reduces_width_twice(void)
{
        populate_buffer(buf, 1, 4, "x");
        csv_clear_field(buf, 0, 3);
        TEST_ASSERT_EQUAL_INT(3, (int)buf->width[0]);
        csv_clear_field(buf, 0, 2);
        TEST_ASSERT_EQUAL_INT(2, (int)buf->width[0]);
}

/* ------------------------------------------------------------------ */
/* Test: clearing out-of-range entry on valid row returns 0            */
/* ------------------------------------------------------------------ */
void test_clear_field_out_of_range_entry_valid_row_returns_zero(void)
{
        populate_buffer(buf, 2, 2, "z");
        int ret = csv_clear_field(buf, 1, 99);
        TEST_ASSERT_EQUAL_INT(0, ret);
}

int main(void)
{
        unity_install_sighandler();
        UNITY_BEGIN();
        RUN_TEST(test_clear_field_row_out_of_range_returns_zero);
        RUN_TEST(test_clear_field_entry_out_of_range_returns_zero);
        RUN_TEST(test_clear_field_out_of_range_does_not_change_width);
        RUN_TEST(test_clear_field_middle_field_sets_empty_string);
        RUN_TEST(test_clear_field_middle_field_does_not_change_width);
        RUN_TEST(test_clear_field_first_field_sets_empty_string);
        RUN_TEST(test_clear_field_first_field_does_not_change_width);
        RUN_TEST(test_clear_field_last_field_not_first_removes_field);
        RUN_TEST(test_clear_field_last_field_decrements_width);
        RUN_TEST(test_clear_field_only_field_sets_empty_string);
        RUN_TEST(test_clear_field_only_field_does_not_change_width);
        RUN_TEST(test_clear_field_second_row_middle_field);
        RUN_TEST(test_clear_field_second_row_last_field_removes);
        RUN_TEST(test_clear_field_does_not_affect_other_rows);
        RUN_TEST(test_clear_field_empty_buffer_returns_zero);
        RUN_TEST(test_clear_field_returns_zero_for_valid_field);
        RUN_TEST(test_clear_field_adjacent_fields_unaffected);
        RUN_TEST(test_clear_field_last_field_twice_reduces_width_twice);
        RUN_TEST(test_clear_field_out_of_range_entry_valid_row_returns_zero);
        return UNITY_END();
}