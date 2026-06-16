#include "csv.c"
#include "unity.h"
#include <stdlib.h>
#include <string.h>

/* File-scope fixtures */
static CSV_BUFFER *buf;

/* Helper: populate a buffer with known data */
static void populate_buffer(CSV_BUFFER *b, int rows, int cols, const char *val)
{
    int r, c;
    for (r = 0; r < rows; r++) {
        for (c = 0; c < cols; c++) {
            csv_set_field(b, (size_t)r, (size_t)c, (char *)val);
        }
    }
}

/* Helper: read a field into a local buffer and return strcmp result */
static int field_equals(CSV_BUFFER *b, size_t row, size_t entry, const char *expected)
{
    char tmp[256];
    memset(tmp, 0, sizeof(tmp));
    csv_get_field(tmp, sizeof(tmp), b, row, entry);
    return strcmp(tmp, expected);
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
 * Test: insert into an empty buffer (row and entry do not exist yet)
 * Expected: csv_set_field path is taken; field is stored at [0][0]
 * ----------------------------------------------------------------------- */
void test_insert_into_empty_buffer(void)
{
    int ret = csv_insert_field(buf, 0, 0, "hello");
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(0, field_equals(buf, 0, 0, "hello"));
}

/* -----------------------------------------------------------------------
 * Test: insert at a row that does not exist yet
 * Expected: csv_set_field path; new row is created
 * ----------------------------------------------------------------------- */
void test_insert_row_beyond_existing(void)
{
    /* Create row 0 first */
    csv_set_field(buf, 0, 0, "existing");

    int ret = csv_insert_field(buf, 2, 0, "newrow");
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(0, field_equals(buf, 2, 0, "newrow"));
}

/* -----------------------------------------------------------------------
 * Test: insert at an entry index beyond the current row width
 * Expected: csv_set_field path; field appended at the new position
 * ----------------------------------------------------------------------- */
void test_insert_entry_beyond_row_width(void)
{
    csv_set_field(buf, 0, 0, "a");
    csv_set_field(buf, 0, 1, "b");

    /* entry 5 does not exist in a 2-wide row */
    int ret = csv_insert_field(buf, 0, 5, "far");
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(0, field_equals(buf, 0, 5, "far"));
}

/* -----------------------------------------------------------------------
 * Test: insert at the beginning of an existing row (entry 0)
 * Expected: all existing fields shift right by one; new field at [0][0]
 * ----------------------------------------------------------------------- */
void test_insert_at_beginning_shifts_fields_right(void)
{
    csv_set_field(buf, 0, 0, "first");
    csv_set_field(buf, 0, 1, "second");
    csv_set_field(buf, 0, 2, "third");

    int ret = csv_insert_field(buf, 0, 0, "inserted");
    TEST_ASSERT_EQUAL_INT(0, ret);

    /* New field at position 0 */
    TEST_ASSERT_EQUAL_INT(0, field_equals(buf, 0, 0, "inserted"));
    /* Old fields shifted right */
    TEST_ASSERT_EQUAL_INT(0, field_equals(buf, 0, 1, "first"));
    TEST_ASSERT_EQUAL_INT(0, field_equals(buf, 0, 2, "second"));
    TEST_ASSERT_EQUAL_INT(0, field_equals(buf, 0, 3, "third"));
}

/* -----------------------------------------------------------------------
 * Test: insert in the middle of an existing row
 * Expected: fields at and after the insertion point shift right
 * ----------------------------------------------------------------------- */
void test_insert_in_middle_shifts_fields_right(void)
{
    csv_set_field(buf, 0, 0, "A");
    csv_set_field(buf, 0, 1, "B");
    csv_set_field(buf, 0, 2, "C");
    csv_set_field(buf, 0, 3, "D");

    int ret = csv_insert_field(buf, 0, 2, "X");
    TEST_ASSERT_EQUAL_INT(0, ret);

    TEST_ASSERT_EQUAL_INT(0, field_equals(buf, 0, 0, "A"));
    TEST_ASSERT_EQUAL_INT(0, field_equals(buf, 0, 1, "B"));
    TEST_ASSERT_EQUAL_INT(0, field_equals(buf, 0, 2, "X"));
    TEST_ASSERT_EQUAL_INT(0, field_equals(buf, 0, 3, "C"));
    TEST_ASSERT_EQUAL_INT(0, field_equals(buf, 0, 4, "D"));
}

/* -----------------------------------------------------------------------
 * Test: insert at the last valid index of an existing row
 * Expected: last field shifts right; new field placed at that index
 * ----------------------------------------------------------------------- */
void test_insert_at_last_valid_index(void)
{
    csv_set_field(buf, 0, 0, "alpha");
    csv_set_field(buf, 0, 1, "beta");
    csv_set_field(buf, 0, 2, "gamma");

    /* Last valid index is 2 */
    int ret = csv_insert_field(buf, 0, 2, "inserted");
    TEST_ASSERT_EQUAL_INT(0, ret);

    TEST_ASSERT_EQUAL_INT(0, field_equals(buf, 0, 0, "alpha"));
    TEST_ASSERT_EQUAL_INT(0, field_equals(buf, 0, 1, "beta"));
    TEST_ASSERT_EQUAL_INT(0, field_equals(buf, 0, 2, "inserted"));
    TEST_ASSERT_EQUAL_INT(0, field_equals(buf, 0, 3, "gamma"));
}

/* -----------------------------------------------------------------------
 * Test: row width increases by one after a shift-insert
 * ----------------------------------------------------------------------- */
void test_insert_increases_row_width(void)
{
    csv_set_field(buf, 0, 0, "one");
    csv_set_field(buf, 0, 1, "two");

    int width_before = csv_get_width(buf, 0);
    csv_insert_field(buf, 0, 0, "zero");
    int width_after = csv_get_width(buf, 0);

    TEST_ASSERT_EQUAL_INT(width_before + 1, width_after);
}

/* -----------------------------------------------------------------------
 * Test: insert into a single-field row at entry 0
 * Expected: existing field shifts to index 1; new field at index 0
 * ----------------------------------------------------------------------- */
void test_insert_into_single_field_row(void)
{
    csv_set_field(buf, 0, 0, "only");

    int ret = csv_insert_field(buf, 0, 0, "new");
    TEST_ASSERT_EQUAL_INT(0, ret);

    TEST_ASSERT_EQUAL_INT(0, field_equals(buf, 0, 0, "new"));
    TEST_ASSERT_EQUAL_INT(0, field_equals(buf, 0, 1, "only"));
    TEST_ASSERT_EQUAL_INT(2, csv_get_width(buf, 0));
}

/* -----------------------------------------------------------------------
 * Test: insert does not disturb other rows
 * ----------------------------------------------------------------------- */
void test_insert_does_not_affect_other_rows(void)
{
    csv_set_field(buf, 0, 0, "row0col0");
    csv_set_field(buf, 0, 1, "row0col1");
    csv_set_field(buf, 1, 0, "row1col0");
    csv_set_field(buf, 1, 1, "row1col1");
    csv_set_field(buf, 2, 0, "row2col0");

    csv_insert_field(buf, 1, 0, "inserted");

    /* Row 0 unchanged */
    TEST_ASSERT_EQUAL_INT(0, field_equals(buf, 0, 0, "row0col0"));
    TEST_ASSERT_EQUAL_INT(0, field_equals(buf, 0, 1, "row0col1"));

    /* Row 1 modified correctly */
    TEST_ASSERT_EQUAL_INT(0, field_equals(buf, 1, 0, "inserted"));
    TEST_ASSERT_EQUAL_INT(0, field_equals(buf, 1, 1, "row1col0"));
    TEST_ASSERT_EQUAL_INT(0, field_equals(buf, 1, 2, "row1col1"));

    /* Row 2 unchanged */
    TEST_ASSERT_EQUAL_INT(0, field_equals(buf, 2, 0, "row2col0"));
}

/* -----------------------------------------------------------------------
 * Test: insert an empty string
 * ----------------------------------------------------------------------- */
void test_insert_empty_string(void)
{
    csv_set_field(buf, 0, 0, "existing");

    int ret = csv_insert_field(buf, 0, 0, "");
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(0, field_equals(buf, 0, 0, ""));
    TEST_ASSERT_EQUAL_INT(0, field_equals(buf, 0, 1, "existing"));
}

/* -----------------------------------------------------------------------
 * Test: return value is always 0
 * ----------------------------------------------------------------------- */
void test_insert_always_returns_zero_set_path(void)
{
    int ret = csv_insert_field(buf, 0, 0, "test");
    TEST_ASSERT_EQUAL_INT(0, ret);
}

void test_insert_always_returns_zero_shift_path(void)
{
    csv_set_field(buf, 0, 0, "a");
    csv_set_field(buf, 0, 1, "b");

    int ret = csv_insert_field(buf, 0, 0, "new");
    TEST_ASSERT_EQUAL_INT(0, ret);
}

/* -----------------------------------------------------------------------
 * Test: insert with a longer string preserves content integrity
 * ----------------------------------------------------------------------- */
void test_insert_long_string(void)
{
    const char *long_str = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyz";
    csv_set_field(buf, 0, 0, "before");
    csv_set_field(buf, 0, 1, "after");

    int ret = csv_insert_field(buf, 0, 1, (char *)long_str);
    TEST_ASSERT_EQUAL_INT(0, ret);

    TEST_ASSERT_EQUAL_INT(0, field_equals(buf, 0, 0, "before"));
    TEST_ASSERT_EQUAL_INT(0, field_equals(buf, 0, 1, long_str));
    TEST_ASSERT_EQUAL_INT(0, field_equals(buf, 0, 2, "after"));
}

/* -----------------------------------------------------------------------
 * Test: multiple consecutive inserts at position 0
 * ----------------------------------------------------------------------- */
void test_multiple_inserts_at_position_zero(void)
{
    csv_set_field(buf, 0, 0, "original");

    csv_insert_field(buf, 0, 0, "second");
    csv_insert_field(buf, 0, 0, "third");
    csv_insert_field(buf, 0, 0, "fourth");

    TEST_ASSERT_EQUAL_INT(0, field_equals(buf, 0, 0, "fourth"));
    TEST_ASSERT_EQUAL_INT(0, field_equals(buf, 0, 1, "third"));
    TEST_ASSERT_EQUAL_INT(0, field_equals(buf, 0, 2, "second"));
    TEST_ASSERT_EQUAL_INT(0, field_equals(buf, 0, 3, "original"));
    TEST_ASSERT_EQUAL_INT(4, csv_get_width(buf, 0));
}

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_insert_into_empty_buffer);
    RUN_TEST(test_insert_row_beyond_existing);
    RUN_TEST(test_insert_entry_beyond_row_width);
    RUN_TEST(test_insert_at_beginning_shifts_fields_right);
    RUN_TEST(test_insert_in_middle_shifts_fields_right);
    RUN_TEST(test_insert_at_last_valid_index);
    RUN_TEST(test_insert_increases_row_width);
    RUN_TEST(test_insert_into_single_field_row);
    RUN_TEST(test_insert_does_not_affect_other_rows);
    RUN_TEST(test_insert_empty_string);
    RUN_TEST(test_insert_always_returns_zero_set_path);
    RUN_TEST(test_insert_always_returns_zero_shift_path);
    RUN_TEST(test_insert_long_string);
    RUN_TEST(test_multiple_inserts_at_position_zero);
    return UNITY_END();
}