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
    memset(dest, 'X', sizeof(dest));
    int gret = csv_get_field(dest, sizeof(dest), buf, 0, 1);
    /* csv_get_field returns 2 when cell is empty */
    TEST_ASSERT_EQUAL_INT(2, gret);
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
    int gret = csv_get_field(dest, sizeof(dest), buf, 0, 0);
    TEST_ASSERT_EQUAL_INT(2, gret);
}

/* -----------------------------------------------------------------------
 * Test: clearing the last field (entry != 0) removes it (width decremented)
 * ----------------------------------------------------------------------- */
void test_clear_field_last_field_removes_it(void)
{
    build_buffer(buf, 1, 3, "test");
    size_t width_before = buf->width[0];   /* should be 3 */

    int ret = csv_clear_field(buf, 0, 2);  /* entry 2 is last */
    TEST_ASSERT_EQUAL_INT(0, ret);

    /* Width should decrease by 1 */
    TEST_ASSERT_EQUAL_UINT(width_before - 1, buf->width[0]);
}

/* -----------------------------------------------------------------------
 * Test: clearing the only field in a row (entry 0, width 1) sets it empty
 * ----------------------------------------------------------------------- */
void test_clear_field_only_field_in_row_sets_empty(void)
{
    build_buffer(buf, 1, 1, "solo");
    size_t width_before = buf->width[0];   /* should be 1 */

    int ret = csv_clear_field(buf, 0, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);

    /* Width must remain 1 (entry==0 so remove_last_field is NOT called) */
    TEST_ASSERT_EQUAL_UINT(width_before, buf->width[0]);

    char dest[64];
    int gret = csv_get_field(dest, sizeof(dest), buf, 0, 0);
    TEST_ASSERT_EQUAL_INT(2, gret);
}

/* -----------------------------------------------------------------------
 * Test: clearing last field of a row with exactly 2 fields removes it
 * ----------------------------------------------------------------------- */
void test_clear_field_last_of_two_fields_removes_it(void)
{
    build_buffer(buf, 1, 2, "ab");
    /* entry 1 is last and entry != 0 → remove_last_field */
    int ret = csv_clear_field(buf, 0, 1);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_UINT(1, buf->width[0]);
}

/* -----------------------------------------------------------------------
 * Test: other rows are unaffected when clearing a field in one row
 * ----------------------------------------------------------------------- */
void test_clear_field_does_not_affect_other_rows(void)
{
    build_buffer(buf, 2, 3, "keep");

    /* Clear middle field of row 0 */
    csv_clear_field(buf, 0, 1);

    /* Row 1 width must be unchanged */
    TEST_ASSERT_EQUAL_UINT(3, buf->width[1]);

    /* Row 1 fields must still contain "keep" */
    char dest[64];
    int gret = csv_get_field(dest, sizeof(dest), buf, 1, 0);
    TEST_ASSERT_EQUAL_INT(0, gret);
    TEST_ASSERT_EQUAL_STRING("keep", dest);
}

/* -----------------------------------------------------------------------
 * Test: return value is always 0 for valid in-range field
 * ----------------------------------------------------------------------- */
void test_clear_field_return_value_is_zero_for_valid_field(void)
{
    build_buffer(buf, 2, 4, "value");
    TEST_ASSERT_EQUAL_INT(0, csv_clear_field(buf, 0, 0));
    TEST_ASSERT_EQUAL_INT(0, csv_clear_field(buf, 0, 1));
    TEST_ASSERT_EQUAL_INT(0, csv_clear_field(buf, 1, 0));
}

/* -----------------------------------------------------------------------
 * Test: clearing a field that was already empty (set to "\0") is safe
 * ----------------------------------------------------------------------- */
void test_clear_field_already_empty_field_is_safe(void)
{
    build_buffer(buf, 1, 3, "data");
    /* Clear once */
    csv_clear_field(buf, 0, 1);
    /* Clear again — must not crash and must return 0 */
    int ret = csv_clear_field(buf, 0, 1);
    TEST_ASSERT_EQUAL_INT(0, ret);
}

/* -----------------------------------------------------------------------
 * Test: clearing last field repeatedly reduces width each time
 * ----------------------------------------------------------------------- */
void test_clear_field_repeated_last_field_removal(void)
{
    build_buffer(buf, 1, 4, "x");
    /* width starts at 4 */
    csv_clear_field(buf, 0, 3);   /* width → 3 */
    TEST_ASSERT_EQUAL_UINT(3, buf->width[0]);

    csv_clear_field(buf, 0, 2);   /* width → 2 */
    TEST_ASSERT_EQUAL_UINT(2, buf->width[0]);

    csv_clear_field(buf, 0, 1);   /* width → 1 */
    TEST_ASSERT_EQUAL_UINT(1, buf->width[0]);

    /* Now only entry 0 remains; clearing it should NOT remove (entry==0) */
    csv_clear_field(buf, 0, 0);
    TEST_ASSERT_EQUAL_UINT(1, buf->width[0]);
}

/* -----------------------------------------------------------------------
 * Test: row boundary — clearing row N-1 last field works correctly
 * ----------------------------------------------------------------------- */
void test_clear_field_last_row_last_field(void)
{
    build_buffer(buf, 3, 3, "z");
    size_t last_row = buf->rows - 1;   /* row 2 */
    size_t last_entry = buf->width[last_row] - 1;  /* entry 2 */

    int ret = csv_clear_field(buf, last_row, last_entry);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_UINT(2, buf->width[last_row]);
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
    RUN_TEST(test_clear_field_only_field_in_row_sets_empty);
    RUN_TEST(test_clear_field_last_of_two_fields_removes_it);
    RUN_TEST(test_clear_field_does_not_affect_other_rows);
    RUN_TEST(test_clear_field_return_value_is_zero_for_valid_field);
    RUN_TEST(test_clear_field_already_empty_field_is_safe);
    RUN_TEST(test_clear_field_repeated_last_field_removal);
    RUN_TEST(test_clear_field_last_row_last_field);
    return UNITY_END();
}