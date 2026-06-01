#include "csv.c"
#include "unity.h"
#include "csv.h"
#include <stdlib.h>
#include <string.h>
#include <signal.h>

/* File-scope buffer used across tests */
static CSV_BUFFER *buf = NULL;

/* Signal handler for segmentation faults */
static void sigsegv_handler(int sig)
{
    (void)sig;
    TEST_FAIL_MESSAGE("Segmentation fault (SIGSEGV) caught during test execution");
}

void setUp(void)
{
    signal(SIGSEGV, sigsegv_handler);
    buf = csv_create_buffer();
    TEST_ASSERT_NOT_NULL_MESSAGE(buf, "csv_create_buffer() returned NULL in setUp");
}

void tearDown(void)
{
    if (buf != NULL) {
        csv_destroy_buffer(buf);
        buf = NULL;
    }
    signal(SIGSEGV, SIG_DFL);
}

/* Helper: retrieve field text into a stack buffer and return via strdup-like copy */
static void get_field_str(CSV_BUFFER *b, size_t row, size_t entry,
                           char *out, size_t out_len)
{
    csv_get_field(out, out_len, b, row, entry);
}

/* -------------------------------------------------------------------------
 * Test 1: Insert into a non-existent row/entry — should behave like set_field
 * ------------------------------------------------------------------------- */
void test_insert_into_nonexistent_row_acts_as_set(void)
{
    char result[64];

    /* Buffer is empty; row 0 does not exist */
    int ret = csv_insert_field(buf, 0, 0, "hello");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "csv_insert_field should return 0");

    /* The field should now be set */
    get_field_str(buf, 0, 0, result, sizeof(result));
    TEST_ASSERT_EQUAL_STRING_MESSAGE("hello", result,
        "Field at (0,0) should be 'hello' after insert into empty buffer");
}

/* -------------------------------------------------------------------------
 * Test 2: Insert at the beginning of an existing row shifts all fields right
 * ------------------------------------------------------------------------- */
void test_insert_at_beginning_shifts_fields_right(void)
{
    char result[64];

    /* Set up row 0 with three fields: A, B, C */
    csv_set_field(buf, 0, 0, "A");
    csv_set_field(buf, 0, 1, "B");
    csv_set_field(buf, 0, 2, "C");

    TEST_ASSERT_EQUAL_INT_MESSAGE(3, csv_get_width(buf, 0),
        "Row 0 should have width 3 before insert");

    /* Insert "X" at position 0 */
    int ret = csv_insert_field(buf, 0, 0, "X");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "csv_insert_field should return 0");

    /* Width should now be 4 */
    TEST_ASSERT_EQUAL_INT_MESSAGE(4, csv_get_width(buf, 0),
        "Row 0 should have width 4 after insert");

    /* Check all positions */
    get_field_str(buf, 0, 0, result, sizeof(result));
    TEST_ASSERT_EQUAL_STRING_MESSAGE("X", result, "Field at (0,0) should be 'X'");

    get_field_str(buf, 0, 1, result, sizeof(result));
    TEST_ASSERT_EQUAL_STRING_MESSAGE("A", result, "Field at (0,1) should be 'A'");

    get_field_str(buf, 0, 2, result, sizeof(result));
    TEST_ASSERT_EQUAL_STRING_MESSAGE("B", result, "Field at (0,2) should be 'B'");

    get_field_str(buf, 0, 3, result, sizeof(result));
    TEST_ASSERT_EQUAL_STRING_MESSAGE("C", result, "Field at (0,3) should be 'C'");
}

/* -------------------------------------------------------------------------
 * Test 3: Insert in the middle of an existing row shifts only right-side fields
 * ------------------------------------------------------------------------- */
void test_insert_in_middle_shifts_right_side_only(void)
{
    char result[64];

    /* Set up row 0: "first", "second", "third" */
    csv_set_field(buf, 0, 0, "first");
    csv_set_field(buf, 0, 1, "second");
    csv_set_field(buf, 0, 2, "third");

    /* Insert "middle" at position 1 */
    int ret = csv_insert_field(buf, 0, 1, "middle");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "csv_insert_field should return 0");

    TEST_ASSERT_EQUAL_INT_MESSAGE(4, csv_get_width(buf, 0),
        "Row 0 should have width 4 after middle insert");

    get_field_str(buf, 0, 0, result, sizeof(result));
    TEST_ASSERT_EQUAL_STRING_MESSAGE("first", result, "Field (0,0) should be 'first'");

    get_field_str(buf, 0, 1, result, sizeof(result));
    TEST_ASSERT_EQUAL_STRING_MESSAGE("middle", result, "Field (0,1) should be 'middle'");

    get_field_str(buf, 0, 2, result, sizeof(result));
    TEST_ASSERT_EQUAL_STRING_MESSAGE("second", result, "Field (0,2) should be 'second'");

    get_field_str(buf, 0, 3, result, sizeof(result));
    TEST_ASSERT_EQUAL_STRING_MESSAGE("third", result, "Field (0,3) should be 'third'");
}

/* -------------------------------------------------------------------------
 * Test 4: Insert at the last valid position shifts only the last field
 * ------------------------------------------------------------------------- */
void test_insert_at_last_valid_position(void)
{
    char result[64];

    /* Set up row 0: "alpha", "beta" */
    csv_set_field(buf, 0, 0, "alpha");
    csv_set_field(buf, 0, 1, "beta");

    TEST_ASSERT_EQUAL_INT_MESSAGE(2, csv_get_width(buf, 0),
        "Row 0 should have width 2 before insert");

    /* Insert "gamma" at position 1 (last valid index) */
    int ret = csv_insert_field(buf, 0, 1, "gamma");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "csv_insert_field should return 0");

    TEST_ASSERT_EQUAL_INT_MESSAGE(3, csv_get_width(buf, 0),
        "Row 0 should have width 3 after insert at last position");

    get_field_str(buf, 0, 0, result, sizeof(result));
    TEST_ASSERT_EQUAL_STRING_MESSAGE("alpha", result, "Field (0,0) should be 'alpha'");

    get_field_str(buf, 0, 1, result, sizeof(result));
    TEST_ASSERT_EQUAL_STRING_MESSAGE("gamma", result, "Field (0,1) should be 'gamma'");

    get_field_str(buf, 0, 2, result, sizeof(result));
    TEST_ASSERT_EQUAL_STRING_MESSAGE("beta", result, "Field (0,2) should be 'beta'");
}

/* -------------------------------------------------------------------------
 * Test 5: Insert into a non-existent entry index beyond current width
 *         acts as set_field (no shift needed)
 * ------------------------------------------------------------------------- */
void test_insert_beyond_width_acts_as_set(void)
{
    char result[64];

    /* Set up row 0 with one field */
    csv_set_field(buf, 0, 0, "only");

    TEST_ASSERT_EQUAL_INT_MESSAGE(1, csv_get_width(buf, 0),
        "Row 0 should have width 1 before insert");

    /* Insert at entry 5 — beyond current width of 1 */
    int ret = csv_insert_field(buf, 0, 5, "far");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "csv_insert_field should return 0");

    /* The field at (0,5) should be "far" */
    get_field_str(buf, 0, 5, result, sizeof(result));
    TEST_ASSERT_EQUAL_STRING_MESSAGE("far", result,
        "Field at (0,5) should be 'far' after insert beyond width");

    /* The original field at (0,0) should still be "only" */
    get_field_str(buf, 0, 0, result, sizeof(result));
    TEST_ASSERT_EQUAL_STRING_MESSAGE("only", result,
        "Field at (0,0) should still be 'only'");
}

/* -------------------------------------------------------------------------
 * main
 * ------------------------------------------------------------------------- */
int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_insert_into_nonexistent_row_acts_as_set);
    RUN_TEST(test_insert_at_beginning_shifts_fields_right);
    RUN_TEST(test_insert_in_middle_shifts_right_side_only);
    RUN_TEST(test_insert_at_last_valid_position);
    RUN_TEST(test_insert_beyond_width_acts_as_set);
    return UNITY_END();
}