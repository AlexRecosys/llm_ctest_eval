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
    if (buf) {
        csv_destroy_buffer(buf);
        buf = NULL;
    }
}

/* -----------------------------------------------------------------------
 * Test: insert into an empty buffer (row does not exist) — falls through
 * to csv_set_field path.
 * --------------------------------------------------------------------- */
void test_insert_into_empty_buffer_creates_field(void)
{
    int ret = csv_insert_field(buf, 0, 0, "hello");
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(0, field_equals(buf, 0, 0, "hello"));
}

/* -----------------------------------------------------------------------
 * Test: insert at row that does not yet exist (row > rows-1).
 * --------------------------------------------------------------------- */
void test_insert_nonexistent_row_delegates_to_set_field(void)
{
    /* Seed row 0 first */
    csv_set_field(buf, 0, 0, "seed");

    int ret = csv_insert_field(buf, 5, 0, "far_row");
    TEST_ASSERT_EQUAL_INT(0, ret);
    /* The field should be reachable */
    TEST_ASSERT_EQUAL_INT(0, field_equals(buf, 5, 0, "far_row"));
}

/* -----------------------------------------------------------------------
 * Test: insert at entry that does not yet exist in an existing row
 * (entry > width[row]-1) — delegates to csv_set_field.
 * --------------------------------------------------------------------- */
void test_insert_nonexistent_entry_delegates_to_set_field(void)
{
    csv_set_field(buf, 0, 0, "A");
    /* entry 5 does not exist in row 0 which has width 1 */
    int ret = csv_insert_field(buf, 0, 5, "Z");
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(0, field_equals(buf, 0, 5, "Z"));
}

/* -----------------------------------------------------------------------
 * Test: insert at the beginning of an existing row shifts all fields
 * right by one.
 * --------------------------------------------------------------------- */
void test_insert_at_beginning_shifts_fields_right(void)
{
    /* Build row 0: A, B, C */
    csv_set_field(buf, 0, 0, "A");
    csv_set_field(buf, 0, 1, "B");
    csv_set_field(buf, 0, 2, "C");

    int ret = csv_insert_field(buf, 0, 0, "NEW");
    TEST_ASSERT_EQUAL_INT(0, ret);

    /* Width should now be 4 */
    TEST_ASSERT_EQUAL_INT(4, csv_get_width(buf, 0));

    /* Check positions */
    TEST_ASSERT_EQUAL_INT(0, field_equals(buf, 0, 0, "NEW"));
    TEST_ASSERT_EQUAL_INT(0, field_equals(buf, 0, 1, "A"));
    TEST_ASSERT_EQUAL_INT(0, field_equals(buf, 0, 2, "B"));
    TEST_ASSERT_EQUAL_INT(0, field_equals(buf, 0, 3, "C"));
}

/* -----------------------------------------------------------------------
 * Test: insert in the middle of an existing row.
 * --------------------------------------------------------------------- */
void test_insert_in_middle_shifts_tail_right(void)
{
    csv_set_field(buf, 0, 0, "X");
    csv_set_field(buf, 0, 1, "Y");
    csv_set_field(buf, 0, 2, "Z");

    /* Insert at position 1 */
    int ret = csv_insert_field(buf, 0, 1, "MID");
    TEST_ASSERT_EQUAL_INT(0, ret);

    TEST_ASSERT_EQUAL_INT(4, csv_get_width(buf, 0));
    TEST_ASSERT_EQUAL_INT(0, field_equals(buf, 0, 0, "X"));
    TEST_ASSERT_EQUAL_INT(0, field_equals(buf, 0, 1, "MID"));
    TEST_ASSERT_EQUAL_INT(0, field_equals(buf, 0, 2, "Y"));
    TEST_ASSERT_EQUAL_INT(0, field_equals(buf, 0, 3, "Z"));
}

/* -----------------------------------------------------------------------
 * Test: insert at the last valid position of an existing row.
 * --------------------------------------------------------------------- */
void test_insert_at_last_position_shifts_last_field(void)
{
    csv_set_field(buf, 0, 0, "P");
    csv_set_field(buf, 0, 1, "Q");
    csv_set_field(buf, 0, 2, "R");

    /* Insert at last valid index (2) */
    int ret = csv_insert_field(buf, 0, 2, "INS");
    TEST_ASSERT_EQUAL_INT(0, ret);

    TEST_ASSERT_EQUAL_INT(4, csv_get_width(buf, 0));
    TEST_ASSERT_EQUAL_INT(0, field_equals(buf, 0, 0, "P"));
    TEST_ASSERT_EQUAL_INT(0, field_equals(buf, 0, 1, "Q"));
    TEST_ASSERT_EQUAL_INT(0, field_equals(buf, 0, 2, "INS"));
    TEST_ASSERT_EQUAL_INT(0, field_equals(buf, 0, 3, "R"));
}

/* -----------------------------------------------------------------------
 * Test: insert into a single-field row at position 0.
 * --------------------------------------------------------------------- */
void test_insert_into_single_field_row_at_zero(void)
{
    csv_set_field(buf, 0, 0, "ONLY");

    int ret = csv_insert_field(buf, 0, 0, "FIRST");
    TEST_ASSERT_EQUAL_INT(0, ret);

    TEST_ASSERT_EQUAL_INT(2, csv_get_width(buf, 0));
    TEST_ASSERT_EQUAL_INT(0, field_equals(buf, 0, 0, "FIRST"));
    TEST_ASSERT_EQUAL_INT(0, field_equals(buf, 0, 1, "ONLY"));
}

/* -----------------------------------------------------------------------
 * Test: insert does not disturb other rows.
 * --------------------------------------------------------------------- */
void test_insert_does_not_affect_other_rows(void)
{
    csv_set_field(buf, 0, 0, "R0C0");
    csv_set_field(buf, 0, 1, "R0C1");
    csv_set_field(buf, 1, 0, "R1C0");
    csv_set_field(buf, 1, 1, "R1C1");

    csv_insert_field(buf, 0, 0, "INS");

    /* Row 1 must be untouched */
    TEST_ASSERT_EQUAL_INT(2, csv_get_width(buf, 1));
    TEST_ASSERT_EQUAL_INT(0, field_equals(buf, 1, 0, "R1C0"));
    TEST_ASSERT_EQUAL_INT(0, field_equals(buf, 1, 1, "R1C1"));
}

/* -----------------------------------------------------------------------
 * Test: return value is always 0.
 * --------------------------------------------------------------------- */
void test_insert_always_returns_zero(void)
{
    /* Case 1: new buffer */
    TEST_ASSERT_EQUAL_INT(0, csv_insert_field(buf, 0, 0, "v1"));

    /* Case 2: existing row, existing entry */
    csv_set_field(buf, 0, 0, "A");
    csv_set_field(buf, 0, 1, "B");
    TEST_ASSERT_EQUAL_INT(0, csv_insert_field(buf, 0, 0, "v2"));

    /* Case 3: non-existent row */
    TEST_ASSERT_EQUAL_INT(0, csv_insert_field(buf, 99, 0, "v3"));
}

/* -----------------------------------------------------------------------
 * Test: insert with an empty string field.
 * --------------------------------------------------------------------- */
void test_insert_empty_string_field(void)
{
    csv_set_field(buf, 0, 0, "A");
    csv_set_field(buf, 0, 1, "B");

    int ret = csv_insert_field(buf, 0, 1, "");
    TEST_ASSERT_EQUAL_INT(0, ret);

    TEST_ASSERT_EQUAL_INT(3, csv_get_width(buf, 0));
    TEST_ASSERT_EQUAL_INT(0, field_equals(buf, 0, 0, "A"));
    TEST_ASSERT_EQUAL_INT(0, field_equals(buf, 0, 1, ""));
    TEST_ASSERT_EQUAL_INT(0, field_equals(buf, 0, 2, "B"));
}

/* -----------------------------------------------------------------------
 * Test: multiple consecutive inserts at position 0 build correct order.
 * --------------------------------------------------------------------- */
void test_multiple_inserts_at_position_zero(void)
{
    csv_set_field(buf, 0, 0, "C");
    csv_insert_field(buf, 0, 0, "B");
    csv_insert_field(buf, 0, 0, "A");

    TEST_ASSERT_EQUAL_INT(3, csv_get_width(buf, 0));
    TEST_ASSERT_EQUAL_INT(0, field_equals(buf, 0, 0, "A"));
    TEST_ASSERT_EQUAL_INT(0, field_equals(buf, 0, 1, "B"));
    TEST_ASSERT_EQUAL_INT(0, field_equals(buf, 0, 2, "C"));
}

/* -----------------------------------------------------------------------
 * Test: insert preserves buffer height.
 * --------------------------------------------------------------------- */
void test_insert_preserves_buffer_height(void)
{
    csv_set_field(buf, 0, 0, "r0");
    csv_set_field(buf, 1, 0, "r1");
    csv_set_field(buf, 2, 0, "r2");

    int height_before = csv_get_height(buf);
    csv_insert_field(buf, 1, 0, "ins");
    int height_after = csv_get_height(buf);

    TEST_ASSERT_EQUAL_INT(height_before, height_after);
}

/* -----------------------------------------------------------------------
 * main
 * --------------------------------------------------------------------- */
int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_insert_into_empty_buffer_creates_field);
    RUN_TEST(test_insert_nonexistent_row_delegates_to_set_field);
    RUN_TEST(test_insert_nonexistent_entry_delegates_to_set_field);
    RUN_TEST(test_insert_at_beginning_shifts_fields_right);
    RUN_TEST(test_insert_in_middle_shifts_tail_right);
    RUN_TEST(test_insert_at_last_position_shifts_last_field);
    RUN_TEST(test_insert_into_single_field_row_at_zero);
    RUN_TEST(test_insert_does_not_affect_other_rows);
    RUN_TEST(test_insert_always_returns_zero);
    RUN_TEST(test_insert_empty_string_field);
    RUN_TEST(test_multiple_inserts_at_position_zero);
    RUN_TEST(test_insert_preserves_buffer_height);
    return UNITY_END();
}