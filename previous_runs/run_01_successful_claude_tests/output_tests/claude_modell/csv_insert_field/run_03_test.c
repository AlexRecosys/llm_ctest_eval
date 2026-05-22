#include "unity.h"
#include "csv.h"

static CSV_BUFFER *buffer;

void setUp(void)
{
    buffer = csv_create_buffer();
}

void tearDown(void)
{
    if (buffer != NULL) {
        csv_destroy_buffer(buffer);
        buffer = NULL;
    }
}

/* Test 1: Insert into a non-existent row/entry falls through to csv_set_field */
void test_csv_insert_field_into_empty_buffer(void)
{
    int ret = csv_insert_field(buffer, 0, 0, "hello");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "csv_insert_field should return 0");
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, (int)buffer->rows,
        "Buffer should have 1 row after insert into empty buffer");
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, (int)buffer->width[0],
        "Row 0 should have 1 field after insert");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("hello", buffer->field[0][0]->text,
        "Field text should be 'hello'");
}

/* Test 2: Insert into a row that exists but entry index is beyond current width */
void test_csv_insert_field_beyond_existing_width(void)
{
    csv_set_field(buffer, 0, 0, "first");
    csv_set_field(buffer, 0, 1, "second");

    /* entry 5 is beyond width[0]-1 == 1, so csv_set_field path is taken */
    int ret = csv_insert_field(buffer, 0, 5, "far");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "csv_insert_field should return 0");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("first",  buffer->field[0][0]->text,
        "Field 0 should still be 'first'");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("second", buffer->field[0][1]->text,
        "Field 1 should still be 'second'");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("far", buffer->field[0][5]->text,
        "Field 5 should be 'far'");
}

/* Test 3: Insert at the beginning of an existing row shifts all fields right */
void test_csv_insert_field_at_beginning_shifts_fields(void)
{
    csv_set_field(buffer, 0, 0, "A");
    csv_set_field(buffer, 0, 1, "B");
    csv_set_field(buffer, 0, 2, "C");

    int ret = csv_insert_field(buffer, 0, 0, "NEW");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "csv_insert_field should return 0");
    TEST_ASSERT_EQUAL_INT_MESSAGE(4, (int)buffer->width[0],
        "Row 0 should now have 4 fields after insert");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("NEW", buffer->field[0][0]->text,
        "Field 0 should be 'NEW'");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("A", buffer->field[0][1]->text,
        "Field 1 should be 'A' after shift");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("B", buffer->field[0][2]->text,
        "Field 2 should be 'B' after shift");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("C", buffer->field[0][3]->text,
        "Field 3 should be 'C' after shift");
}

/* Test 4: Insert in the middle of an existing row shifts only fields to the right */
void test_csv_insert_field_in_middle_shifts_right_fields(void)
{
    csv_set_field(buffer, 0, 0, "X");
    csv_set_field(buffer, 0, 1, "Y");
    csv_set_field(buffer, 0, 2, "Z");

    int ret = csv_insert_field(buffer, 0, 1, "MID");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "csv_insert_field should return 0");
    TEST_ASSERT_EQUAL_INT_MESSAGE(4, (int)buffer->width[0],
        "Row 0 should now have 4 fields");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("X",   buffer->field[0][0]->text,
        "Field 0 should remain 'X'");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("MID", buffer->field[0][1]->text,
        "Field 1 should be 'MID'");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("Y",   buffer->field[0][2]->text,
        "Field 2 should be 'Y' after shift");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("Z",   buffer->field[0][3]->text,
        "Field 3 should be 'Z' after shift");
}

/* Test 5: Insert into a second row does not disturb the first row */
void test_csv_insert_field_in_second_row_does_not_affect_first_row(void)
{
    csv_set_field(buffer, 0, 0, "row0col0");
    csv_set_field(buffer, 0, 1, "row0col1");
    csv_set_field(buffer, 1, 0, "row1col0");
    csv_set_field(buffer, 1, 1, "row1col1");

    int ret = csv_insert_field(buffer, 1, 0, "INSERTED");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "csv_insert_field should return 0");

    /* Row 0 must be untouched */
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, (int)buffer->width[0],
        "Row 0 width should remain 2");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("row0col0", buffer->field[0][0]->text,
        "Row 0, field 0 should be unchanged");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("row0col1", buffer->field[0][1]->text,
        "Row 0, field 1 should be unchanged");

    /* Row 1 should have the inserted field and shifted fields */
    TEST_ASSERT_EQUAL_INT_MESSAGE(3, (int)buffer->width[1],
        "Row 1 should now have 3 fields");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("INSERTED", buffer->field[1][0]->text,
        "Row 1, field 0 should be 'INSERTED'");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("row1col0", buffer->field[1][1]->text,
        "Row 1, field 1 should be 'row1col0' after shift");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("row1col1", buffer->field[1][2]->text,
        "Row 1, field 2 should be 'row1col1' after shift");
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_csv_insert_field_into_empty_buffer);
    RUN_TEST(test_csv_insert_field_beyond_existing_width);
    RUN_TEST(test_csv_insert_field_at_beginning_shifts_fields);
    RUN_TEST(test_csv_insert_field_in_middle_shifts_right_fields);
    RUN_TEST(test_csv_insert_field_in_second_row_does_not_affect_first_row);
    return UNITY_END();
}