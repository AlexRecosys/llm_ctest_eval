#include "csv.c"
#include "unity.h"
#include <stdlib.h>
#include <string.h>

/* File-scope fixtures */
static CSV_BUFFER *buf;

/* Helper: populate a buffer with given rows x cols, filling each field with "rRcC" */
static void populate_buffer(CSV_BUFFER *b, int rows, int cols)
{
    int r, c;
    char text[32];
    for (r = 0; r < rows; r++) {
        append_row(b);
        for (c = 0; c < cols; c++) {
            append_field(b, r);
            snprintf(text, sizeof(text), "r%dc%d", r, c);
            set_field(b->field[r][c], text);
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
 * Test: remove the only row in a single-row buffer
 * ----------------------------------------------------------------------- */
void test_remove_row_single_row_becomes_empty(void)
{
    populate_buffer(buf, 1, 3);
    TEST_ASSERT_EQUAL_INT(1, csv_get_height(buf));

    int ret = csv_remove_row(buf, 0);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(0, csv_get_height(buf));
}

/* -----------------------------------------------------------------------
 * Test: remove the first row of a multi-row buffer
 * ----------------------------------------------------------------------- */
void test_remove_row_first_row_shifts_remaining(void)
{
    populate_buffer(buf, 3, 2);
    /* rows: r0c0/r0c1, r1c0/r1c1, r2c0/r2c1 */

    int ret = csv_remove_row(buf, 0);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(2, csv_get_height(buf));

    char field[32];
    csv_get_field(field, sizeof(field), buf, 0, 0);
    TEST_ASSERT_EQUAL_STRING("r1c0", field);

    csv_get_field(field, sizeof(field), buf, 0, 1);
    TEST_ASSERT_EQUAL_STRING("r1c1", field);

    csv_get_field(field, sizeof(field), buf, 1, 0);
    TEST_ASSERT_EQUAL_STRING("r2c0", field);

    csv_get_field(field, sizeof(field), buf, 1, 1);
    TEST_ASSERT_EQUAL_STRING("r2c1", field);
}

/* -----------------------------------------------------------------------
 * Test: remove the last row of a multi-row buffer
 * ----------------------------------------------------------------------- */
void test_remove_row_last_row(void)
{
    populate_buffer(buf, 3, 2);

    int ret = csv_remove_row(buf, 2);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(2, csv_get_height(buf));

    char field[32];
    csv_get_field(field, sizeof(field), buf, 0, 0);
    TEST_ASSERT_EQUAL_STRING("r0c0", field);

    csv_get_field(field, sizeof(field), buf, 1, 0);
    TEST_ASSERT_EQUAL_STRING("r1c0", field);
}

/* -----------------------------------------------------------------------
 * Test: remove a middle row of a multi-row buffer
 * ----------------------------------------------------------------------- */
void test_remove_row_middle_row(void)
{
    populate_buffer(buf, 4, 2);
    /* rows 0,1,2,3 */

    int ret = csv_remove_row(buf, 1);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(3, csv_get_height(buf));

    char field[32];

    /* row 0 unchanged */
    csv_get_field(field, sizeof(field), buf, 0, 0);
    TEST_ASSERT_EQUAL_STRING("r0c0", field);

    /* old row 2 is now row 1 */
    csv_get_field(field, sizeof(field), buf, 1, 0);
    TEST_ASSERT_EQUAL_STRING("r2c0", field);

    /* old row 3 is now row 2 */
    csv_get_field(field, sizeof(field), buf, 2, 0);
    TEST_ASSERT_EQUAL_STRING("r3c0", field);
}

/* -----------------------------------------------------------------------
 * Test: row index out of bounds returns 0 and buffer is unchanged
 * ----------------------------------------------------------------------- */
void test_remove_row_out_of_bounds_returns_0(void)
{
    populate_buffer(buf, 2, 2);

    int ret = csv_remove_row(buf, 5);

    TEST_ASSERT_EQUAL_INT(0, ret);
    /* buffer must be untouched */
    TEST_ASSERT_EQUAL_INT(2, csv_get_height(buf));

    char field[32];
    csv_get_field(field, sizeof(field), buf, 0, 0);
    TEST_ASSERT_EQUAL_STRING("r0c0", field);

    csv_get_field(field, sizeof(field), buf, 1, 0);
    TEST_ASSERT_EQUAL_STRING("r1c0", field);
}

/* -----------------------------------------------------------------------
 * Test: row index exactly equal to rows-1 (last valid row) is removed
 * ----------------------------------------------------------------------- */
void test_remove_row_exact_last_valid_index(void)
{
    populate_buffer(buf, 3, 1);

    int ret = csv_remove_row(buf, 2); /* rows-1 == 2 */

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(2, csv_get_height(buf));
}

/* -----------------------------------------------------------------------
 * Test: row index == rows (one past last) is rejected
 * ----------------------------------------------------------------------- */
void test_remove_row_one_past_last_is_rejected(void)
{
    populate_buffer(buf, 3, 1);

    int ret = csv_remove_row(buf, 3); /* rows == 3, rows-1 == 2 */

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(3, csv_get_height(buf));
}

/* -----------------------------------------------------------------------
 * Test: width of shifted rows is preserved correctly
 * ----------------------------------------------------------------------- */
void test_remove_row_width_preserved_after_shift(void)
{
    populate_buffer(buf, 3, 4);

    csv_remove_row(buf, 0);

    /* old row 1 (now row 0) had 4 fields */
    TEST_ASSERT_EQUAL_INT(4, csv_get_width(buf, 0));
    /* old row 2 (now row 1) had 4 fields */
    TEST_ASSERT_EQUAL_INT(4, csv_get_width(buf, 1));
}

/* -----------------------------------------------------------------------
 * Test: remove row from buffer with rows of different widths
 * ----------------------------------------------------------------------- */
void test_remove_row_different_widths(void)
{
    /* row 0: 1 field, row 1: 3 fields, row 2: 2 fields */
    append_row(buf);
    append_field(buf, 0);
    set_field(buf->field[0][0], "A");

    append_row(buf);
    append_field(buf, 1);
    set_field(buf->field[1][0], "B0");
    append_field(buf, 1);
    set_field(buf->field[1][1], "B1");
    append_field(buf, 1);
    set_field(buf->field[1][2], "B2");

    append_row(buf);
    append_field(buf, 2);
    set_field(buf->field[2][0], "C0");
    append_field(buf, 2);
    set_field(buf->field[2][1], "C1");

    /* remove row 1 (the wide one) */
    int ret = csv_remove_row(buf, 1);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(2, csv_get_height(buf));

    /* row 0 still has 1 field */
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buf, 0));

    /* old row 2 is now row 1 with 2 fields */
    TEST_ASSERT_EQUAL_INT(2, csv_get_width(buf, 1));

    char field[32];
    csv_get_field(field, sizeof(field), buf, 1, 0);
    TEST_ASSERT_EQUAL_STRING("C0", field);

    csv_get_field(field, sizeof(field), buf, 1, 1);
    TEST_ASSERT_EQUAL_STRING("C1", field);
}

/* -----------------------------------------------------------------------
 * Test: repeated removal reduces height each time
 * ----------------------------------------------------------------------- */
void test_remove_row_repeated_removal(void)
{
    populate_buffer(buf, 5, 1);

    csv_remove_row(buf, 0);
    TEST_ASSERT_EQUAL_INT(4, csv_get_height(buf));

    csv_remove_row(buf, 0);
    TEST_ASSERT_EQUAL_INT(3, csv_get_height(buf));

    csv_remove_row(buf, 0);
    TEST_ASSERT_EQUAL_INT(2, csv_get_height(buf));

    csv_remove_row(buf, 0);
    TEST_ASSERT_EQUAL_INT(1, csv_get_height(buf));

    csv_remove_row(buf, 0);
    TEST_ASSERT_EQUAL_INT(0, csv_get_height(buf));
}

/* -----------------------------------------------------------------------
 * Test: field content integrity after removing row 0 multiple times
 * ----------------------------------------------------------------------- */
void test_remove_row_content_integrity_after_multiple_removals(void)
{
    populate_buffer(buf, 4, 2);

    /* remove row 0 twice; what was row 2 should now be row 0 */
    csv_remove_row(buf, 0);
    csv_remove_row(buf, 0);

    char field[32];
    csv_get_field(field, sizeof(field), buf, 0, 0);
    TEST_ASSERT_EQUAL_STRING("r2c0", field);

    csv_get_field(field, sizeof(field), buf, 0, 1);
    TEST_ASSERT_EQUAL_STRING("r2c1", field);

    csv_get_field(field, sizeof(field), buf, 1, 0);
    TEST_ASSERT_EQUAL_STRING("r3c0", field);
}

/* -----------------------------------------------------------------------
 * main
 * ----------------------------------------------------------------------- */
int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_remove_row_single_row_becomes_empty);
    RUN_TEST(test_remove_row_first_row_shifts_remaining);
    RUN_TEST(test_remove_row_last_row);
    RUN_TEST(test_remove_row_middle_row);
    RUN_TEST(test_remove_row_out_of_bounds_returns_0);
    RUN_TEST(test_remove_row_exact_last_valid_index);
    RUN_TEST(test_remove_row_one_past_last_is_rejected);
    RUN_TEST(test_remove_row_width_preserved_after_shift);
    RUN_TEST(test_remove_row_different_widths);
    RUN_TEST(test_remove_row_repeated_removal);
    RUN_TEST(test_remove_row_content_integrity_after_multiple_removals);
    return UNITY_END();
}