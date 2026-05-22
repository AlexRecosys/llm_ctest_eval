#include "unity.h"
#include "csv.h"

#include <stdlib.h>
#include <string.h>

/* File-scope fixture */
static CSV_BUFFER *buffer;

/* ------------------------------------------------------------------ */
/* Helper functions                                                     */
/* ------------------------------------------------------------------ */

static const char *get_field_text(CSV_BUFFER *buf, size_t row, size_t col)
{
    if (buf == NULL) return NULL;
    if (row >= buf->rows) return NULL;
    if (col >= buf->width[row]) return NULL;
    if (buf->field[row][col] == NULL) return NULL;
    return buf->field[row][col]->text;
}

static size_t get_row_width(CSV_BUFFER *buf, size_t row)
{
    if (buf == NULL || row >= buf->rows) return 0;
    return buf->width[row];
}

/* ------------------------------------------------------------------ */
/* setUp / tearDown                                                     */
/* ------------------------------------------------------------------ */

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

/* ------------------------------------------------------------------ */
/* Test cases                                                           */
/* ------------------------------------------------------------------ */

/*
 * Test 1: Insert into a non-existent row/entry (row > rows-1).
 * The function should fall through to csv_set_field and simply create
 * the field at the requested position.
 */
void test_insert_field_into_empty_buffer_creates_field(void)
{
    int ret = csv_insert_field(buffer, 0, 0, "hello");

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret,
        "csv_insert_field should return 0");
    TEST_ASSERT_GREATER_THAN_MESSAGE(0, (int)buffer->rows,
        "Buffer should have at least one row after insert");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("hello", get_field_text(buffer, 0, 0),
        "Field text should be 'hello'");
}

/*
 * Test 2: Insert at the beginning of an existing row shifts existing
 * fields to the right.
 * Pre-populate row 0 with ["A", "B", "C"], then insert "X" at entry 0.
 * Expected result: ["X", "A", "B", "C"].
 */
void test_insert_field_at_beginning_shifts_fields_right(void)
{
    csv_set_field(buffer, 0, 0, "A");
    csv_set_field(buffer, 0, 1, "B");
    csv_set_field(buffer, 0, 2, "C");

    int ret = csv_insert_field(buffer, 0, 0, "X");

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret,
        "csv_insert_field should return 0");
    TEST_ASSERT_EQUAL_UINT_MESSAGE(4, get_row_width(buffer, 0),
        "Row width should be 4 after insert");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("X", get_field_text(buffer, 0, 0),
        "Entry 0 should be 'X'");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("A", get_field_text(buffer, 0, 1),
        "Entry 1 should be 'A'");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("B", get_field_text(buffer, 0, 2),
        "Entry 2 should be 'B'");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("C", get_field_text(buffer, 0, 3),
        "Entry 3 should be 'C'");
}

/*
 * Test 3: Insert in the middle of an existing row.
 * Pre-populate row 0 with ["A", "B", "C"], then insert "M" at entry 1.
 * Expected result: ["A", "M", "B", "C"].
 */
void test_insert_field_in_middle_shifts_tail_right(void)
{
    csv_set_field(buffer, 0, 0, "A");
    csv_set_field(buffer, 0, 1, "B");
    csv_set_field(buffer, 0, 2, "C");

    int ret = csv_insert_field(buffer, 0, 1, "M");

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret,
        "csv_insert_field should return 0");
    TEST_ASSERT_EQUAL_UINT_MESSAGE(4, get_row_width(buffer, 0),
        "Row width should be 4 after insert");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("A", get_field_text(buffer, 0, 0),
        "Entry 0 should remain 'A'");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("M", get_field_text(buffer, 0, 1),
        "Entry 1 should be 'M'");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("B", get_field_text(buffer, 0, 2),
        "Entry 2 should be 'B'");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("C", get_field_text(buffer, 0, 3),
        "Entry 3 should be 'C'");
}

/*
 * Test 4: Insert at the last valid index of an existing row.
 * Pre-populate row 0 with ["A", "B", "C"], then insert "Z" at entry 2
 * (the last existing index).
 * Expected result: ["A", "B", "Z", "C"].
 */
void test_insert_field_at_last_index_shifts_last_element(void)
{
    csv_set_field(buffer, 0, 0, "A");
    csv_set_field(buffer, 0, 1, "B");
    csv_set_field(buffer, 0, 2, "C");

    int ret = csv_insert_field(buffer, 0, 2, "Z");

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret,
        "csv_insert_field should return 0");
    TEST_ASSERT_EQUAL_UINT_MESSAGE(4, get_row_width(buffer, 0),
        "Row width should be 4 after insert");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("A", get_field_text(buffer, 0, 0),
        "Entry 0 should remain 'A'");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("B", get_field_text(buffer, 0, 1),
        "Entry 1 should remain 'B'");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("Z", get_field_text(buffer, 0, 2),
        "Entry 2 should be 'Z'");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("C", get_field_text(buffer, 0, 3),
        "Entry 3 should be 'C'");
}

/*
 * Test 5: Insert into a row that does not yet exist (row index beyond
 * current rows). The function should delegate to csv_set_field which
 * will grow the buffer as needed.
 * Insert "NEW" at row 2, entry 0 when the buffer is empty.
 * Expected: buffer has at least 3 rows, field[2][0] == "NEW".
 */
void test_insert_field_into_nonexistent_row_creates_row(void)
{
    int ret = csv_insert_field(buffer, 2, 0, "NEW");

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret,
        "csv_insert_field should return 0");
    TEST_ASSERT_GREATER_THAN_MESSAGE(2, (int)buffer->rows,
        "Buffer should have more than 2 rows after insert at row 2");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("NEW", get_field_text(buffer, 2, 0),
        "Field at row 2, entry 0 should be 'NEW'");
}

/* ------------------------------------------------------------------ */
/* main                                                                 */
/* ------------------------------------------------------------------ */

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_insert_field_into_empty_buffer_creates_field);
    RUN_TEST(test_insert_field_at_beginning_shifts_fields_right);
    RUN_TEST(test_insert_field_in_middle_shifts_tail_right);
    RUN_TEST(test_insert_field_at_last_index_shifts_last_element);
    RUN_TEST(test_insert_field_into_nonexistent_row_creates_row);

    return UNITY_END();
}