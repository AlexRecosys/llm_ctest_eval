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

/* Helper: read a field into a stack buffer and return via strdup-like static buffer */
static char field_result[256];
static const char *get_field(CSV_BUFFER *b, size_t row, size_t entry)
{
    memset(field_result, 0, sizeof(field_result));
    csv_get_field(field_result, sizeof(field_result), b, row, entry);
    return field_result;
}

/* -------------------------------------------------------------------------
 * Test 1: Insert into a non-existent row/entry — should behave like set_field
 * ------------------------------------------------------------------------- */
void test_insert_field_into_empty_buffer_acts_like_set(void)
{
    /* Buffer is empty; row 0 and entry 0 do not exist */
    int ret = csv_insert_field(buf, 0, 0, "hello");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "csv_insert_field should return 0");

    /* The field should now be set */
    TEST_ASSERT_EQUAL_STRING_MESSAGE("hello", get_field(buf, 0, 0),
        "Field at (0,0) should be 'hello' after insert into empty buffer");

    /* Height should be 1 */
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, csv_get_height(buf),
        "Buffer height should be 1 after insert");
}

/* -------------------------------------------------------------------------
 * Test 2: Insert at the beginning of an existing row shifts fields right
 * ------------------------------------------------------------------------- */
void test_insert_field_at_beginning_shifts_existing_fields(void)
{
    /* Set up row 0 with two fields: "A", "B" */
    csv_set_field(buf, 0, 0, "A");
    csv_set_field(buf, 0, 1, "B");

    TEST_ASSERT_EQUAL_INT_MESSAGE(2, csv_get_width(buf, 0),
        "Row 0 should have width 2 before insert");

    /* Insert "X" at position 0 — should shift "A" to 1, "B" to 2 */
    int ret = csv_insert_field(buf, 0, 0, "X");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "csv_insert_field should return 0");

    /* Width should now be 3 */
    TEST_ASSERT_EQUAL_INT_MESSAGE(3, csv_get_width(buf, 0),
        "Row 0 width should be 3 after insert at beginning");

    /* Check field values */
    TEST_ASSERT_EQUAL_STRING_MESSAGE("X", get_field(buf, 0, 0),
        "Field at (0,0) should be 'X'");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("A", get_field(buf, 0, 1),
        "Field at (0,1) should be 'A' (shifted right)");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("B", get_field(buf, 0, 2),
        "Field at (0,2) should be 'B' (shifted right)");
}

/* -------------------------------------------------------------------------
 * Test 3: Insert in the middle of a row shifts only fields to the right
 * ------------------------------------------------------------------------- */
void test_insert_field_in_middle_shifts_right_fields_only(void)
{
    /* Set up row 0 with three fields: "first", "second", "third" */
    csv_set_field(buf, 0, 0, "first");
    csv_set_field(buf, 0, 1, "second");
    csv_set_field(buf, 0, 2, "third");

    TEST_ASSERT_EQUAL_INT_MESSAGE(3, csv_get_width(buf, 0),
        "Row 0 should have width 3 before insert");

    /* Insert "middle" at position 1 */
    int ret = csv_insert_field(buf, 0, 1, "middle");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "csv_insert_field should return 0");

    /* Width should now be 4 */
    TEST_ASSERT_EQUAL_INT_MESSAGE(4, csv_get_width(buf, 0),
        "Row 0 width should be 4 after insert in middle");

    /* Check all field values */
    TEST_ASSERT_EQUAL_STRING_MESSAGE("first", get_field(buf, 0, 0),
        "Field at (0,0) should still be 'first'");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("middle", get_field(buf, 0, 1),
        "Field at (0,1) should be 'middle' (newly inserted)");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("second", get_field(buf, 0, 2),
        "Field at (0,2) should be 'second' (shifted right)");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("third", get_field(buf, 0, 3),
        "Field at (0,3) should be 'third' (shifted right)");
}

/* -------------------------------------------------------------------------
 * Test 4: Insert beyond the last entry in an existing row acts like set_field
 * ------------------------------------------------------------------------- */
void test_insert_field_beyond_last_entry_acts_like_set(void)
{
    /* Set up row 0 with one field */
    csv_set_field(buf, 0, 0, "only");

    TEST_ASSERT_EQUAL_INT_MESSAGE(1, csv_get_width(buf, 0),
        "Row 0 should have width 1 before insert");

    /* Insert at entry 5 (beyond current width) — should act like set_field */
    int ret = csv_insert_field(buf, 0, 5, "far");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "csv_insert_field should return 0");

    /* "only" at position 0 should be unchanged */
    TEST_ASSERT_EQUAL_STRING_MESSAGE("only", get_field(buf, 0, 0),
        "Field at (0,0) should still be 'only'");

    /* "far" should be accessible at position 5 */
    TEST_ASSERT_EQUAL_STRING_MESSAGE("far", get_field(buf, 0, 5),
        "Field at (0,5) should be 'far' after out-of-bounds insert");
}

/* -------------------------------------------------------------------------
 * Test 5: Insert into a row that does not exist yet (row > rows-1)
 *         should act like set_field and not disturb other rows
 * ------------------------------------------------------------------------- */
void test_insert_field_into_nonexistent_row_acts_like_set(void)
{
    /* Set up row 0 */
    csv_set_field(buf, 0, 0, "row0field0");
    csv_set_field(buf, 0, 1, "row0field1");

    /* Buffer currently has 1 row; insert into row 3 (does not exist) */
    int ret = csv_insert_field(buf, 3, 0, "newrow");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "csv_insert_field should return 0");

    /* Row 0 should be untouched */
    TEST_ASSERT_EQUAL_STRING_MESSAGE("row0field0", get_field(buf, 0, 0),
        "Field at (0,0) should be unchanged");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("row0field1", get_field(buf, 0, 1),
        "Field at (0,1) should be unchanged");

    /* The new field should be accessible */
    TEST_ASSERT_EQUAL_STRING_MESSAGE("newrow", get_field(buf, 3, 0),
        "Field at (3,0) should be 'newrow' after insert into non-existent row");

    /* Buffer height should have grown */
    TEST_ASSERT_GREATER_THAN(1, csv_get_height(buf));
}

/* -------------------------------------------------------------------------
 * main
 * ------------------------------------------------------------------------- */
int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_insert_field_into_empty_buffer_acts_like_set);
    RUN_TEST(test_insert_field_at_beginning_shifts_existing_fields);
    RUN_TEST(test_insert_field_in_middle_shifts_right_fields_only);
    RUN_TEST(test_insert_field_beyond_last_entry_acts_like_set);
    RUN_TEST(test_insert_field_into_nonexistent_row_acts_like_set);
    return UNITY_END();
}