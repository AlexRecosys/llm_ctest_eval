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

void setUp(void)
{
        buf = csv_create_buffer();
}

void tearDown(void)
{
        if (buf != NULL) {
                csv_destroy_buffer(buf);
                buf = NULL;
        }
}

/* -----------------------------------------------------------------------
 * Test: insert into a completely empty buffer (row 0 does not exist yet)
 * The function should fall through to csv_set_field path.
 * ----------------------------------------------------------------------- */
void test_insert_into_empty_buffer_creates_field(void)
{
        int ret = csv_insert_field(buf, 0, 0, "hello");
        TEST_ASSERT_EQUAL_INT(0, ret);

        char dest[64];
        csv_get_field(dest, sizeof(dest), buf, 0, 0);
        TEST_ASSERT_EQUAL_STRING("hello", dest);
}

/* -----------------------------------------------------------------------
 * Test: insert at a row that does not exist yet (row > rows-1)
 * ----------------------------------------------------------------------- */
void test_insert_nonexistent_row_uses_set_field(void)
{
        /* Seed row 0 first */
        csv_set_field(buf, 0, 0, "seed");

        int ret = csv_insert_field(buf, 5, 0, "far_row");
        TEST_ASSERT_EQUAL_INT(0, ret);

        char dest[64];
        csv_get_field(dest, sizeof(dest), buf, 5, 0);
        TEST_ASSERT_EQUAL_STRING("far_row", dest);
}

/* -----------------------------------------------------------------------
 * Test: insert at an entry index beyond the current row width
 * The function should fall through to csv_set_field path.
 * ----------------------------------------------------------------------- */
void test_insert_beyond_row_width_uses_set_field(void)
{
        csv_set_field(buf, 0, 0, "A");
        csv_set_field(buf, 0, 1, "B");

        /* entry 10 is beyond width-1 */
        int ret = csv_insert_field(buf, 0, 10, "Z");
        TEST_ASSERT_EQUAL_INT(0, ret);

        char dest[64];
        csv_get_field(dest, sizeof(dest), buf, 0, 10);
        TEST_ASSERT_EQUAL_STRING("Z", dest);
}

/* -----------------------------------------------------------------------
 * Test: insert at position 0 of an existing row shifts all fields right
 * ----------------------------------------------------------------------- */
void test_insert_at_beginning_shifts_fields_right(void)
{
        csv_set_field(buf, 0, 0, "A");
        csv_set_field(buf, 0, 1, "B");
        csv_set_field(buf, 0, 2, "C");

        int ret = csv_insert_field(buf, 0, 0, "NEW");
        TEST_ASSERT_EQUAL_INT(0, ret);

        char dest[64];

        csv_get_field(dest, sizeof(dest), buf, 0, 0);
        TEST_ASSERT_EQUAL_STRING("NEW", dest);

        csv_get_field(dest, sizeof(dest), buf, 0, 1);
        TEST_ASSERT_EQUAL_STRING("A", dest);

        csv_get_field(dest, sizeof(dest), buf, 0, 2);
        TEST_ASSERT_EQUAL_STRING("B", dest);

        csv_get_field(dest, sizeof(dest), buf, 0, 3);
        TEST_ASSERT_EQUAL_STRING("C", dest);
}

/* -----------------------------------------------------------------------
 * Test: insert in the middle of a row shifts only the fields after it
 * ----------------------------------------------------------------------- */
void test_insert_in_middle_shifts_trailing_fields(void)
{
        csv_set_field(buf, 0, 0, "A");
        csv_set_field(buf, 0, 1, "B");
        csv_set_field(buf, 0, 2, "C");
        csv_set_field(buf, 0, 3, "D");

        int ret = csv_insert_field(buf, 0, 2, "MID");
        TEST_ASSERT_EQUAL_INT(0, ret);

        char dest[64];

        csv_get_field(dest, sizeof(dest), buf, 0, 0);
        TEST_ASSERT_EQUAL_STRING("A", dest);

        csv_get_field(dest, sizeof(dest), buf, 0, 1);
        TEST_ASSERT_EQUAL_STRING("B", dest);

        csv_get_field(dest, sizeof(dest), buf, 0, 2);
        TEST_ASSERT_EQUAL_STRING("MID", dest);

        csv_get_field(dest, sizeof(dest), buf, 0, 3);
        TEST_ASSERT_EQUAL_STRING("C", dest);

        csv_get_field(dest, sizeof(dest), buf, 0, 4);
        TEST_ASSERT_EQUAL_STRING("D", dest);
}

/* -----------------------------------------------------------------------
 * Test: insert at the last valid index shifts only the last field
 * ----------------------------------------------------------------------- */
void test_insert_at_last_valid_index_shifts_last_field(void)
{
        csv_set_field(buf, 0, 0, "X");
        csv_set_field(buf, 0, 1, "Y");
        csv_set_field(buf, 0, 2, "Z");

        /* last valid index is 2 (width-1) */
        int ret = csv_insert_field(buf, 0, 2, "INS");
        TEST_ASSERT_EQUAL_INT(0, ret);

        char dest[64];

        csv_get_field(dest, sizeof(dest), buf, 0, 0);
        TEST_ASSERT_EQUAL_STRING("X", dest);

        csv_get_field(dest, sizeof(dest), buf, 0, 1);
        TEST_ASSERT_EQUAL_STRING("Y", dest);

        csv_get_field(dest, sizeof(dest), buf, 0, 2);
        TEST_ASSERT_EQUAL_STRING("INS", dest);

        csv_get_field(dest, sizeof(dest), buf, 0, 3);
        TEST_ASSERT_EQUAL_STRING("Z", dest);
}

/* -----------------------------------------------------------------------
 * Test: row width increases by one after a successful insert
 * ----------------------------------------------------------------------- */
void test_insert_increases_row_width_by_one(void)
{
        csv_set_field(buf, 0, 0, "A");
        csv_set_field(buf, 0, 1, "B");

        int width_before = csv_get_width(buf, 0);
        csv_insert_field(buf, 0, 0, "NEW");
        int width_after = csv_get_width(buf, 0);

        TEST_ASSERT_EQUAL_INT(width_before + 1, width_after);
}

/* -----------------------------------------------------------------------
 * Test: insert does not disturb other rows
 * ----------------------------------------------------------------------- */
void test_insert_does_not_affect_other_rows(void)
{
        csv_set_field(buf, 0, 0, "R0C0");
        csv_set_field(buf, 0, 1, "R0C1");
        csv_set_field(buf, 1, 0, "R1C0");
        csv_set_field(buf, 1, 1, "R1C1");

        csv_insert_field(buf, 0, 0, "INS");

        char dest[64];

        csv_get_field(dest, sizeof(dest), buf, 1, 0);
        TEST_ASSERT_EQUAL_STRING("R1C0", dest);

        csv_get_field(dest, sizeof(dest), buf, 1, 1);
        TEST_ASSERT_EQUAL_STRING("R1C1", dest);
}

/* -----------------------------------------------------------------------
 * Test: insert returns 0 on success (existing row, valid entry)
 * ----------------------------------------------------------------------- */
void test_insert_returns_zero_on_success(void)
{
        csv_set_field(buf, 0, 0, "only");
        int ret = csv_insert_field(buf, 0, 0, "new");
        TEST_ASSERT_EQUAL_INT(0, ret);
}

/* -----------------------------------------------------------------------
 * Test: insert returns 0 when falling through to set_field path
 * ----------------------------------------------------------------------- */
void test_insert_returns_zero_on_set_field_path(void)
{
        int ret = csv_insert_field(buf, 0, 0, "first");
        TEST_ASSERT_EQUAL_INT(0, ret);
}

/* -----------------------------------------------------------------------
 * Test: insert with an empty string field
 * ----------------------------------------------------------------------- */
void test_insert_empty_string_field(void)
{
        csv_set_field(buf, 0, 0, "A");
        csv_set_field(buf, 0, 1, "B");

        int ret = csv_insert_field(buf, 0, 1, "");
        TEST_ASSERT_EQUAL_INT(0, ret);

        char dest[64];

        csv_get_field(dest, sizeof(dest), buf, 0, 1);
        TEST_ASSERT_EQUAL_STRING("", dest);

        csv_get_field(dest, sizeof(dest), buf, 0, 2);
        TEST_ASSERT_EQUAL_STRING("B", dest);
}

/* -----------------------------------------------------------------------
 * Test: multiple inserts at position 0 accumulate correctly
 * ----------------------------------------------------------------------- */
void test_multiple_inserts_at_position_zero(void)
{
        csv_set_field(buf, 0, 0, "first");
        csv_insert_field(buf, 0, 0, "second");
        csv_insert_field(buf, 0, 0, "third");

        char dest[64];

        csv_get_field(dest, sizeof(dest), buf, 0, 0);
        TEST_ASSERT_EQUAL_STRING("third", dest);

        csv_get_field(dest, sizeof(dest), buf, 0, 1);
        TEST_ASSERT_EQUAL_STRING("second", dest);

        csv_get_field(dest, sizeof(dest), buf, 0, 2);
        TEST_ASSERT_EQUAL_STRING("first", dest);
}

/* -----------------------------------------------------------------------
 * Test: insert on row 1 when row 0 also has data
 * ----------------------------------------------------------------------- */
void test_insert_on_second_row(void)
{
        csv_set_field(buf, 0, 0, "R0C0");
        csv_set_field(buf, 1, 0, "R1C0");
        csv_set_field(buf, 1, 1, "R1C1");

        int ret = csv_insert_field(buf, 1, 0, "INS");
        TEST_ASSERT_EQUAL_INT(0, ret);

        char dest[64];

        csv_get_field(dest, sizeof(dest), buf, 1, 0);
        TEST_ASSERT_EQUAL_STRING("INS", dest);

        csv_get_field(dest, sizeof(dest), buf, 1, 1);
        TEST_ASSERT_EQUAL_STRING("R1C0", dest);

        csv_get_field(dest, sizeof(dest), buf, 1, 2);
        TEST_ASSERT_EQUAL_STRING("R1C1", dest);
}

/* -----------------------------------------------------------------------
 * main
 * ----------------------------------------------------------------------- */
int main(void)
{
        unity_install_sighandler();
        UNITY_BEGIN();
        RUN_TEST(test_insert_into_empty_buffer_creates_field);
        RUN_TEST(test_insert_nonexistent_row_uses_set_field);
        RUN_TEST(test_insert_beyond_row_width_uses_set_field);
        RUN_TEST(test_insert_at_beginning_shifts_fields_right);
        RUN_TEST(test_insert_in_middle_shifts_trailing_fields);
        RUN_TEST(test_insert_at_last_valid_index_shifts_last_field);
        RUN_TEST(test_insert_increases_row_width_by_one);
        RUN_TEST(test_insert_does_not_affect_other_rows);
        RUN_TEST(test_insert_returns_zero_on_success);
        RUN_TEST(test_insert_returns_zero_on_set_field_path);
        RUN_TEST(test_insert_empty_string_field);
        RUN_TEST(test_multiple_inserts_at_position_zero);
        RUN_TEST(test_insert_on_second_row);
        return UNITY_END();
}