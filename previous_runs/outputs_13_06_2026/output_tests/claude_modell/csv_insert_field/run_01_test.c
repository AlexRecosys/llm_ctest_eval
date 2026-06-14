#include "csv.c"
#include "unity.h"
#include "csv.h"
#include <stdlib.h>
#include <string.h>
#include <signal.h>

/* File-scope buffer used across tests */
static CSV_BUFFER *buf = NULL;

/* Signal handler for SIGSEGV */
static void sigsegv_handler(int sig)
{
    (void)sig;
    TEST_FAIL_MESSAGE("Segmentation fault (SIGSEGV) caught during test");
}

void setUp(void)
{
    signal(SIGSEGV, sigsegv_handler);
    buf = csv_create_buffer();
    TEST_ASSERT_NOT_NULL_MESSAGE(buf, "csv_create_buffer returned NULL in setUp");
}

void tearDown(void)
{
    if (buf != NULL) {
        csv_destroy_buffer(buf);
        buf = NULL;
    }
    signal(SIGSEGV, SIG_DFL);
}

/* Helper: read a field into a stack buffer and return via strdup-like static storage */
static char field_result[256];
static const char *get_field_str(CSV_BUFFER *b, size_t row, size_t entry)
{
    memset(field_result, 0, sizeof(field_result));
    csv_get_field(field_result, sizeof(field_result), b, row, entry);
    return field_result;
}

/* -------------------------------------------------------------------------
 * Test 1: Insert into a non-existent row (row > rows-1) — should behave
 *         like csv_set_field and create the field.
 * ---------------------------------------------------------------------- */
void test_insert_field_into_nonexistent_row(void)
{
    /* Buffer is empty, row 0 does not exist */
    int ret = csv_insert_field(buf, 0, 0, "hello");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "csv_insert_field should return 0");

    /* The field should now be accessible */
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, csv_get_height(buf),
        "Buffer should have 1 row after insert into non-existent row");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("hello", get_field_str(buf, 0, 0),
        "Field at (0,0) should be 'hello'");
}

/* -------------------------------------------------------------------------
 * Test 2: Insert into an entry index beyond the current row width —
 *         should behave like csv_set_field (no shift needed).
 * ---------------------------------------------------------------------- */
void test_insert_field_beyond_row_width(void)
{
    /* Set up row 0 with two fields */
    csv_set_field(buf, 0, 0, "alpha");
    csv_set_field(buf, 0, 1, "beta");

    /* Insert at entry 5 (beyond width of 2) — acts like set_field */
    int ret = csv_insert_field(buf, 0, 5, "gamma");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "csv_insert_field should return 0");

    /* Original fields should still be intact */
    TEST_ASSERT_EQUAL_STRING_MESSAGE("alpha", get_field_str(buf, 0, 0),
        "Field (0,0) should still be 'alpha'");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("beta", get_field_str(buf, 0, 1),
        "Field (0,1) should still be 'beta'");
}

/* -------------------------------------------------------------------------
 * Test 3: Insert at the beginning of an existing row — all existing fields
 *         should shift right by one.
 * ---------------------------------------------------------------------- */
void test_insert_field_at_beginning_shifts_all(void)
{
    /* Set up row 0 with three fields */
    csv_set_field(buf, 0, 0, "one");
    csv_set_field(buf, 0, 1, "two");
    csv_set_field(buf, 0, 2, "three");

    TEST_ASSERT_EQUAL_INT_MESSAGE(3, csv_get_width(buf, 0),
        "Row 0 should have width 3 before insert");

    /* Insert "zero" at position 0 */
    int ret = csv_insert_field(buf, 0, 0, "zero");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "csv_insert_field should return 0");

    /* Width should have grown by 1 */
    TEST_ASSERT_EQUAL_INT_MESSAGE(4, csv_get_width(buf, 0),
        "Row 0 should have width 4 after insert");

    /* Check all positions */
    TEST_ASSERT_EQUAL_STRING_MESSAGE("zero",  get_field_str(buf, 0, 0),
        "Field (0,0) should be 'zero'");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("one",   get_field_str(buf, 0, 1),
        "Field (0,1) should be 'one'");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("two",   get_field_str(buf, 0, 2),
        "Field (0,2) should be 'two'");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("three", get_field_str(buf, 0, 3),
        "Field (0,3) should be 'three'");
}

/* -------------------------------------------------------------------------
 * Test 4: Insert in the middle of an existing row — only fields at and
 *         after the insertion point should shift right.
 * ---------------------------------------------------------------------- */
void test_insert_field_in_middle_shifts_partial(void)
{
    /* Set up row 0: A, B, C, D */
    csv_set_field(buf, 0, 0, "A");
    csv_set_field(buf, 0, 1, "B");
    csv_set_field(buf, 0, 2, "C");
    csv_set_field(buf, 0, 3, "D");

    /* Insert "X" at position 2 */
    int ret = csv_insert_field(buf, 0, 2, "X");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "csv_insert_field should return 0");

    TEST_ASSERT_EQUAL_INT_MESSAGE(5, csv_get_width(buf, 0),
        "Row 0 should have width 5 after insert");

    TEST_ASSERT_EQUAL_STRING_MESSAGE("A", get_field_str(buf, 0, 0),
        "Field (0,0) should be 'A'");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("B", get_field_str(buf, 0, 1),
        "Field (0,1) should be 'B'");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("X", get_field_str(buf, 0, 2),
        "Field (0,2) should be 'X' (newly inserted)");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("C", get_field_str(buf, 0, 3),
        "Field (0,3) should be 'C' (shifted)");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("D", get_field_str(buf, 0, 4),
        "Field (0,4) should be 'D' (shifted)");
}

/* -------------------------------------------------------------------------
 * Test 5: Insert into a specific row in a multi-row buffer — other rows
 *         must remain unaffected.
 * ---------------------------------------------------------------------- */
void test_insert_field_does_not_affect_other_rows(void)
{
    /* Row 0: X, Y */
    csv_set_field(buf, 0, 0, "X");
    csv_set_field(buf, 0, 1, "Y");

    /* Row 1: P, Q, R */
    csv_set_field(buf, 1, 0, "P");
    csv_set_field(buf, 1, 1, "Q");
    csv_set_field(buf, 1, 2, "R");

    TEST_ASSERT_EQUAL_INT_MESSAGE(2, csv_get_height(buf),
        "Buffer should have 2 rows before insert");

    /* Insert "NEW" at row 1, position 1 */
    int ret = csv_insert_field(buf, 1, 1, "NEW");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "csv_insert_field should return 0");

    /* Row 0 must be unchanged */
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, csv_get_width(buf, 0),
        "Row 0 width should still be 2");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("X", get_field_str(buf, 0, 0),
        "Row 0, field 0 should still be 'X'");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("Y", get_field_str(buf, 0, 1),
        "Row 0, field 1 should still be 'Y'");

    /* Row 1 should have grown and shifted correctly */
    TEST_ASSERT_EQUAL_INT_MESSAGE(4, csv_get_width(buf, 1),
        "Row 1 width should be 4 after insert");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("P",   get_field_str(buf, 1, 0),
        "Row 1, field 0 should be 'P'");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("NEW", get_field_str(buf, 1, 1),
        "Row 1, field 1 should be 'NEW'");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("Q",   get_field_str(buf, 1, 2),
        "Row 1, field 2 should be 'Q' (shifted)");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("R",   get_field_str(buf, 1, 3),
        "Row 1, field 3 should be 'R' (shifted)");

    /* Overall height must not change */
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, csv_get_height(buf),
        "Buffer height should still be 2 after insert");
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_insert_field_into_nonexistent_row);
    RUN_TEST(test_insert_field_beyond_row_width);
    RUN_TEST(test_insert_field_at_beginning_shifts_all);
    RUN_TEST(test_insert_field_in_middle_shifts_partial);
    RUN_TEST(test_insert_field_does_not_affect_other_rows);
    return UNITY_END();
}