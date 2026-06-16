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

/* Helper: read a field string safely */
static void get_field_str(CSV_BUFFER *b, int row, int col, char *out, size_t len)
{
    csv_get_field(out, len, b, row, col);
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

/* -------------------------------------------------------------------------
 * Test: remove the only row in a single-row buffer
 * ------------------------------------------------------------------------- */
void test_remove_row_single_row_reduces_height_to_zero(void)
{
    populate_buffer(buf, 1, 3);
    TEST_ASSERT_EQUAL_INT(1, csv_get_height(buf));

    int ret = csv_remove_row(buf, 0);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(0, csv_get_height(buf));
}

/* -------------------------------------------------------------------------
 * Test: remove first row of a multi-row buffer
 * ------------------------------------------------------------------------- */
void test_remove_row_first_row_shifts_remaining_rows_up(void)
{
    populate_buffer(buf, 3, 2);
    /* rows: r0c0/r0c1, r1c0/r1c1, r2c0/r2c1 */

    int ret = csv_remove_row(buf, 0);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(2, csv_get_height(buf));

    char field[64];
    get_field_str(buf, 0, 0, field, sizeof(field));
    TEST_ASSERT_EQUAL_STRING("r1c0", field);

    get_field_str(buf, 0, 1, field, sizeof(field));
    TEST_ASSERT_EQUAL_STRING("r1c1", field);

    get_field_str(buf, 1, 0, field, sizeof(field));
    TEST_ASSERT_EQUAL_STRING("r2c0", field);

    get_field_str(buf, 1, 1, field, sizeof(field));
    TEST_ASSERT_EQUAL_STRING("r2c1", field);
}

/* -------------------------------------------------------------------------
 * Test: remove last row of a multi-row buffer
 * ------------------------------------------------------------------------- */
void test_remove_row_last_row_reduces_height_by_one(void)
{
    populate_buffer(buf, 3, 2);

    int ret = csv_remove_row(buf, 2);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(2, csv_get_height(buf));

    char field[64];
    get_field_str(buf, 0, 0, field, sizeof(field));
    TEST_ASSERT_EQUAL_STRING("r0c0", field);

    get_field_str(buf, 1, 0, field, sizeof(field));
    TEST_ASSERT_EQUAL_STRING("r1c0", field);
}

/* -------------------------------------------------------------------------
 * Test: remove middle row of a multi-row buffer
 * ------------------------------------------------------------------------- */
void test_remove_row_middle_row_shifts_subsequent_rows_up(void)
{
    populate_buffer(buf, 4, 2);
    /* rows 0..3 */

    int ret = csv_remove_row(buf, 1);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(3, csv_get_height(buf));

    char field[64];

    /* row 0 unchanged */
    get_field_str(buf, 0, 0, field, sizeof(field));
    TEST_ASSERT_EQUAL_STRING("r0c0", field);

    /* old row 2 is now row 1 */
    get_field_str(buf, 1, 0, field, sizeof(field));
    TEST_ASSERT_EQUAL_STRING("r2c0", field);

    get_field_str(buf, 1, 1, field, sizeof(field));
    TEST_ASSERT_EQUAL_STRING("r2c1", field);

    /* old row 3 is now row 2 */
    get_field_str(buf, 2, 0, field, sizeof(field));
    TEST_ASSERT_EQUAL_STRING("r3c0", field);
}

/* -------------------------------------------------------------------------
 * Test: out-of-bounds row index returns 0 and leaves buffer unchanged
 * ------------------------------------------------------------------------- */
void test_remove_row_out_of_bounds_returns_zero_and_leaves_buffer_unchanged(void)
{
    populate_buffer(buf, 2, 2);

    int ret = csv_remove_row(buf, 5);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(2, csv_get_height(buf));

    char field[64];
    get_field_str(buf, 0, 0, field, sizeof(field));
    TEST_ASSERT_EQUAL_STRING("r0c0", field);

    get_field_str(buf, 1, 0, field, sizeof(field));
    TEST_ASSERT_EQUAL_STRING("r1c0", field);
}

/* -------------------------------------------------------------------------
 * Test: row index exactly equal to rows-1 is valid (last row)
 * ------------------------------------------------------------------------- */
void test_remove_row_index_equal_to_last_valid_index_succeeds(void)
{
    populate_buffer(buf, 3, 1);

    int ret = csv_remove_row(buf, 2); /* rows-1 == 2 */

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(2, csv_get_height(buf));
}

/* -------------------------------------------------------------------------
 * Test: row index equal to rows (one past last) is rejected
 * ------------------------------------------------------------------------- */
void test_remove_row_index_equal_to_rows_is_out_of_bounds(void)
{
    populate_buffer(buf, 3, 1);

    int ret = csv_remove_row(buf, 3); /* rows == 3, valid indices 0..2 */

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(3, csv_get_height(buf));
}

/* -------------------------------------------------------------------------
 * Test: width of shifted rows is preserved correctly
 * ------------------------------------------------------------------------- */
void test_remove_row_preserves_width_of_remaining_rows(void)
{
    populate_buffer(buf, 3, 0);
    /* Give each row a different width */
    append_field(buf, 0); set_field(buf->field[0][0], "A");
    append_field(buf, 1); set_field(buf->field[1][0], "B");
    append_field(buf, 1); set_field(buf->field[1][1], "C");
    append_field(buf, 2); set_field(buf->field[2][0], "D");
    append_field(buf, 2); set_field(buf->field[2][1], "E");
    append_field(buf, 2); set_field(buf->field[2][2], "F");

    /* Remove row 0 (width 1) */
    csv_remove_row(buf, 0);

    TEST_ASSERT_EQUAL_INT(2, csv_get_height(buf));
    TEST_ASSERT_EQUAL_INT(2, csv_get_width(buf, 0));
    TEST_ASSERT_EQUAL_INT(3, csv_get_width(buf, 1));
}

/* -------------------------------------------------------------------------
 * Test: remove all rows one by one until buffer is empty
 * ------------------------------------------------------------------------- */
void test_remove_row_repeatedly_until_empty(void)
{
    populate_buffer(buf, 3, 2);

    csv_remove_row(buf, 0);
    TEST_ASSERT_EQUAL_INT(2, csv_get_height(buf));

    csv_remove_row(buf, 0);
    TEST_ASSERT_EQUAL_INT(1, csv_get_height(buf));

    csv_remove_row(buf, 0);
    TEST_ASSERT_EQUAL_INT(0, csv_get_height(buf));
}

/* -------------------------------------------------------------------------
 * Test: field content integrity after removing a row with different widths
 * ------------------------------------------------------------------------- */
void test_remove_row_field_content_integrity_after_removal(void)
{
    populate_buffer(buf, 3, 3);

    csv_remove_row(buf, 1);

    char field[64];

    get_field_str(buf, 0, 0, field, sizeof(field));
    TEST_ASSERT_EQUAL_STRING("r0c0", field);
    get_field_str(buf, 0, 1, field, sizeof(field));
    TEST_ASSERT_EQUAL_STRING("r0c1", field);
    get_field_str(buf, 0, 2, field, sizeof(field));
    TEST_ASSERT_EQUAL_STRING("r0c2", field);

    get_field_str(buf, 1, 0, field, sizeof(field));
    TEST_ASSERT_EQUAL_STRING("r2c0", field);
    get_field_str(buf, 1, 1, field, sizeof(field));
    TEST_ASSERT_EQUAL_STRING("r2c1", field);
    get_field_str(buf, 1, 2, field, sizeof(field));
    TEST_ASSERT_EQUAL_STRING("r2c2", field);
}

/* -------------------------------------------------------------------------
 * main
 * ------------------------------------------------------------------------- */
int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_remove_row_single_row_reduces_height_to_zero);
    RUN_TEST(test_remove_row_first_row_shifts_remaining_rows_up);
    RUN_TEST(test_remove_row_last_row_reduces_height_by_one);
    RUN_TEST(test_remove_row_middle_row_shifts_subsequent_rows_up);
    RUN_TEST(test_remove_row_out_of_bounds_returns_zero_and_leaves_buffer_unchanged);
    RUN_TEST(test_remove_row_index_equal_to_last_valid_index_succeeds);
    RUN_TEST(test_remove_row_index_equal_to_rows_is_out_of_bounds);
    RUN_TEST(test_remove_row_preserves_width_of_remaining_rows);
    RUN_TEST(test_remove_row_repeatedly_until_empty);
    RUN_TEST(test_remove_row_field_content_integrity_after_removal);
    return UNITY_END();
}