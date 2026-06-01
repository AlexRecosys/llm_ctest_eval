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

/* -----------------------------------------------------------------------
 * Helper: read a field from the buffer into a stack buffer and return
 * a freshly strdup'd copy so we can compare with TEST_ASSERT_EQUAL_STRING.
 * The caller must free() the returned pointer.
 * ----------------------------------------------------------------------- */
static char *get_field_str(CSV_BUFFER *b, size_t row, size_t entry)
{
    char tmp[256];
    memset(tmp, 0, sizeof(tmp));
    csv_get_field(tmp, sizeof(tmp), b, row, entry);
    return strdup(tmp);
}

/* =======================================================================
 * Test 1: Insert into a non-existent row/entry — should behave like set
 * ======================================================================= */
void test_insert_field_into_empty_buffer(void)
{
    /* Buffer has no rows yet; insert should fall through to csv_set_field */
    int ret = csv_insert_field(buf, 0, 0, "hello");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "csv_insert_field should return 0");

    char *val = get_field_str(buf, 0, 0);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("hello", val, "Field (0,0) should be 'hello'");
    free(val);

    TEST_ASSERT_EQUAL_INT_MESSAGE(1, csv_get_height(buf), "Buffer should have 1 row");
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, csv_get_width(buf, 0), "Row 0 should have width 1");
}

/* =======================================================================
 * Test 2: Insert at the beginning of an existing row — shifts all fields
 * ======================================================================= */
void test_insert_field_at_beginning_shifts_existing_fields(void)
{
    /* Set up row 0 with two fields: [A, B] */
    csv_set_field(buf, 0, 0, "A");
    csv_set_field(buf, 0, 1, "B");

    TEST_ASSERT_EQUAL_INT_MESSAGE(2, csv_get_width(buf, 0),
        "Row 0 should have width 2 before insert");

    /* Insert "X" at entry 0 — expected result: [X, A, B] */
    int ret = csv_insert_field(buf, 0, 0, "X");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "csv_insert_field should return 0");

    TEST_ASSERT_EQUAL_INT_MESSAGE(3, csv_get_width(buf, 0),
        "Row 0 should have width 3 after insert");

    char *f0 = get_field_str(buf, 0, 0);
    char *f1 = get_field_str(buf, 0, 1);
    char *f2 = get_field_str(buf, 0, 2);

    TEST_ASSERT_EQUAL_STRING_MESSAGE("X", f0, "Entry 0 should be 'X'");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("A", f1, "Entry 1 should be 'A'");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("B", f2, "Entry 2 should be 'B'");

    free(f0); free(f1); free(f2);
}

/* =======================================================================
 * Test 3: Insert in the middle of an existing row
 * ======================================================================= */
void test_insert_field_in_middle_shifts_tail(void)
{
    /* Set up row 0: [first, second, third] */
    csv_set_field(buf, 0, 0, "first");
    csv_set_field(buf, 0, 1, "second");
    csv_set_field(buf, 0, 2, "third");

    /* Insert "middle" at entry 1 — expected: [first, middle, second, third] */
    int ret = csv_insert_field(buf, 0, 1, "middle");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "csv_insert_field should return 0");

    TEST_ASSERT_EQUAL_INT_MESSAGE(4, csv_get_width(buf, 0),
        "Row 0 should have width 4 after insert");

    char *f0 = get_field_str(buf, 0, 0);
    char *f1 = get_field_str(buf, 0, 1);
    char *f2 = get_field_str(buf, 0, 2);
    char *f3 = get_field_str(buf, 0, 3);

    TEST_ASSERT_EQUAL_STRING_MESSAGE("first",  f0, "Entry 0 should be 'first'");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("middle", f1, "Entry 1 should be 'middle'");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("second", f2, "Entry 2 should be 'second'");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("third",  f3, "Entry 3 should be 'third'");

    free(f0); free(f1); free(f2); free(f3);
}

/* =======================================================================
 * Test 4: Insert at the last valid entry position — shifts only last field
 * ======================================================================= */
void test_insert_field_at_last_entry_shifts_last(void)
{
    /* Set up row 0: [alpha, beta] */
    csv_set_field(buf, 0, 0, "alpha");
    csv_set_field(buf, 0, 1, "beta");

    /* Insert "gamma" at entry 1 (last valid index) — expected: [alpha, gamma, beta] */
    int ret = csv_insert_field(buf, 0, 1, "gamma");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "csv_insert_field should return 0");

    TEST_ASSERT_EQUAL_INT_MESSAGE(3, csv_get_width(buf, 0),
        "Row 0 should have width 3 after insert");

    char *f0 = get_field_str(buf, 0, 0);
    char *f1 = get_field_str(buf, 0, 1);
    char *f2 = get_field_str(buf, 0, 2);

    TEST_ASSERT_EQUAL_STRING_MESSAGE("alpha", f0, "Entry 0 should be 'alpha'");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("gamma", f1, "Entry 1 should be 'gamma'");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("beta",  f2, "Entry 2 should be 'beta'");

    free(f0); free(f1); free(f2);
}

/* =======================================================================
 * Test 5: Insert into a non-existent row index — falls through to set,
 *         does not disturb an existing row
 * ======================================================================= */
void test_insert_field_nonexistent_row_does_not_disturb_existing(void)
{
    /* Set up row 0: [one, two] */
    csv_set_field(buf, 0, 0, "one");
    csv_set_field(buf, 0, 1, "two");

    /* Insert into row 5 (does not exist) — should act like csv_set_field */
    int ret = csv_insert_field(buf, 5, 0, "new");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "csv_insert_field should return 0");

    /* Row 0 must be untouched */
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, csv_get_width(buf, 0),
        "Row 0 width should still be 2");

    char *f0 = get_field_str(buf, 0, 0);
    char *f1 = get_field_str(buf, 0, 1);

    TEST_ASSERT_EQUAL_STRING_MESSAGE("one", f0, "Row 0 entry 0 should still be 'one'");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("two", f1, "Row 0 entry 1 should still be 'two'");

    free(f0); free(f1);

    /* The newly inserted field must be accessible */
    char *fn = get_field_str(buf, 5, 0);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("new", fn, "Row 5 entry 0 should be 'new'");
    free(fn);
}

/* =======================================================================
 * main
 * ======================================================================= */
int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_insert_field_into_empty_buffer);
    RUN_TEST(test_insert_field_at_beginning_shifts_existing_fields);
    RUN_TEST(test_insert_field_in_middle_shifts_tail);
    RUN_TEST(test_insert_field_at_last_entry_shifts_last);
    RUN_TEST(test_insert_field_nonexistent_row_does_not_disturb_existing);
    return UNITY_END();
}