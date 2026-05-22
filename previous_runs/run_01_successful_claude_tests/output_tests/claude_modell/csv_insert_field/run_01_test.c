#include "unity.h"
#include "csv.h"

#include <stdlib.h>
#include <string.h>

/* File-scope fixture */
static CSV_BUFFER *buffer;

/* ---------------------------------------------------------------------------
 * Helper: retrieve the text of a field as a plain C string (may be NULL).
 * We access the internal structure directly because the project context
 * exposes it fully.
 * ------------------------------------------------------------------------- */
static const char *get_field_text(CSV_BUFFER *buf, size_t row, size_t col)
{
    if (buf == NULL) return NULL;
    if (row >= buf->rows) return NULL;
    if (col >= buf->width[row]) return NULL;
    if (buf->field[row][col] == NULL) return NULL;
    return buf->field[row][col]->text;
}

/* ---------------------------------------------------------------------------
 * setUp / tearDown
 * ------------------------------------------------------------------------- */
void setUp(void)
{
    buffer = csv_create_buffer();
    TEST_ASSERT_NOT_NULL_MESSAGE(buffer, "csv_create_buffer() returned NULL");
}

void tearDown(void)
{
    if (buffer != NULL) {
        csv_destroy_buffer(buffer);
        buffer = NULL;
    }
}

/* ---------------------------------------------------------------------------
 * Test 1 – Insert into an empty buffer (row/entry beyond current bounds).
 *
 * When row > buffer->rows-1 the function falls through to csv_set_field,
 * which must grow the buffer and store the value.
 * ------------------------------------------------------------------------- */
void test_insert_field_into_empty_buffer(void)
{
    int ret = csv_insert_field(buffer, 0, 0, "hello");

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret,
        "csv_insert_field should return 0");

    TEST_ASSERT_GREATER_THAN_MESSAGE(0, (int)buffer->rows,
        "Buffer should have at least one row after insert");

    TEST_ASSERT_EQUAL_STRING_MESSAGE("hello", get_field_text(buffer, 0, 0),
        "Field (0,0) should contain 'hello'");
}

/* ---------------------------------------------------------------------------
 * Test 2 – Insert at the beginning of an existing row shifts existing fields.
 *
 * Pre-populate row 0 with ["A", "B", "C"], then insert "X" at entry 0.
 * Expected result: ["X", "A", "B", "C"].
 * ------------------------------------------------------------------------- */
void test_insert_field_at_beginning_shifts_existing_fields(void)
{
    /* Pre-populate */
    csv_set_field(buffer, 0, 0, "A");
    csv_set_field(buffer, 0, 1, "B");
    csv_set_field(buffer, 0, 2, "C");

    int ret = csv_insert_field(buffer, 0, 0, "X");

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret,
        "csv_insert_field should return 0");

    TEST_ASSERT_GREATER_THAN_MESSAGE(3, (int)buffer->width[0],
        "Row 0 should have more than 3 fields after insert");

    TEST_ASSERT_EQUAL_STRING_MESSAGE("X", get_field_text(buffer, 0, 0),
        "Field (0,0) should be 'X' after insert at beginning");

    TEST_ASSERT_EQUAL_STRING_MESSAGE("A", get_field_text(buffer, 0, 1),
        "Field (0,1) should be 'A' after shift");

    TEST_ASSERT_EQUAL_STRING_MESSAGE("B", get_field_text(buffer, 0, 2),
        "Field (0,2) should be 'B' after shift");

    TEST_ASSERT_EQUAL_STRING_MESSAGE("C", get_field_text(buffer, 0, 3),
        "Field (0,3) should be 'C' after shift");
}

/* ---------------------------------------------------------------------------
 * Test 3 – Insert in the middle of an existing row.
 *
 * Pre-populate row 0 with ["first", "second", "third"], insert "middle" at
 * entry 1.  Expected: ["first", "middle", "second", "third"].
 * ------------------------------------------------------------------------- */
void test_insert_field_in_middle_shifts_tail(void)
{
    csv_set_field(buffer, 0, 0, "first");
    csv_set_field(buffer, 0, 1, "second");
    csv_set_field(buffer, 0, 2, "third");

    int ret = csv_insert_field(buffer, 0, 1, "middle");

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret,
        "csv_insert_field should return 0");

    TEST_ASSERT_EQUAL_STRING_MESSAGE("first",  get_field_text(buffer, 0, 0),
        "Field (0,0) should remain 'first'");

    TEST_ASSERT_EQUAL_STRING_MESSAGE("middle", get_field_text(buffer, 0, 1),
        "Field (0,1) should be the newly inserted 'middle'");

    TEST_ASSERT_EQUAL_STRING_MESSAGE("second", get_field_text(buffer, 0, 2),
        "Field (0,2) should be the shifted 'second'");

    TEST_ASSERT_EQUAL_STRING_MESSAGE("third",  get_field_text(buffer, 0, 3),
        "Field (0,3) should be the shifted 'third'");
}

/* ---------------------------------------------------------------------------
 * Test 4 – Insert beyond the last entry of an existing row (append path).
 *
 * When entry > buffer->width[row]-1 the function calls csv_set_field, which
 * simply appends.  Existing fields must be untouched.
 * ------------------------------------------------------------------------- */
void test_insert_field_beyond_row_width_appends(void)
{
    csv_set_field(buffer, 0, 0, "alpha");
    csv_set_field(buffer, 0, 1, "beta");

    /* entry 5 is well beyond current width (2) */
    int ret = csv_insert_field(buffer, 0, 5, "gamma");

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret,
        "csv_insert_field should return 0");

    /* Original fields must be intact */
    TEST_ASSERT_EQUAL_STRING_MESSAGE("alpha", get_field_text(buffer, 0, 0),
        "Field (0,0) should still be 'alpha'");

    TEST_ASSERT_EQUAL_STRING_MESSAGE("beta",  get_field_text(buffer, 0, 1),
        "Field (0,1) should still be 'beta'");

    /* The new field must exist somewhere in the row */
    TEST_ASSERT_GREATER_THAN_MESSAGE(1, (int)buffer->width[0],
        "Row 0 should have more than one field");
}

/* ---------------------------------------------------------------------------
 * Test 5 – Insert into a non-existent row (row beyond current rows count).
 *
 * The buffer starts empty; inserting at row 2 must not crash and must store
 * the value so that it can be retrieved.
 * ------------------------------------------------------------------------- */
void test_insert_field_into_nonexistent_row(void)
{
    /* Buffer is empty; row 2 does not exist */
    int ret = csv_insert_field(buffer, 2, 0, "newrow");

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret,
        "csv_insert_field should return 0 even for a non-existent row");

    TEST_ASSERT_GREATER_THAN_MESSAGE(2, (int)buffer->rows,
        "Buffer should have been grown to accommodate row 2");

    TEST_ASSERT_EQUAL_STRING_MESSAGE("newrow", get_field_text(buffer, 2, 0),
        "Field (2,0) should contain 'newrow'");
}

/* ---------------------------------------------------------------------------
 * main
 * ------------------------------------------------------------------------- */
int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_insert_field_into_empty_buffer);
    RUN_TEST(test_insert_field_at_beginning_shifts_existing_fields);
    RUN_TEST(test_insert_field_in_middle_shifts_tail);
    RUN_TEST(test_insert_field_beyond_row_width_appends);
    RUN_TEST(test_insert_field_into_nonexistent_row);

    return UNITY_END();
}