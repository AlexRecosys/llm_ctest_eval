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

/* Helper: populate a buffer with known data */
static void populate_buffer_3x3(CSV_BUFFER *buf)
{
    csv_set_field(buf, 0, 0, "A0");
    csv_set_field(buf, 0, 1, "B0");
    csv_set_field(buf, 0, 2, "C0");
    csv_set_field(buf, 1, 0, "A1");
    csv_set_field(buf, 1, 1, "B1");
    csv_set_field(buf, 1, 2, "C1");
    csv_set_field(buf, 2, 0, "A2");
    csv_set_field(buf, 2, 1, "B2");
    csv_set_field(buf, 2, 2, "C2");
}

/* Helper: retrieve field text into a local buffer */
static void get_field_text(CSV_BUFFER *buf, size_t row, size_t entry,
                            char *dest, size_t dest_len)
{
    csv_get_field(dest, dest_len, buf, row, entry);
}

/* Test 1: Insert into a non-existent row (beyond current rows).
 * The function should fall through to csv_set_field, creating the row/entry. */
void test_insert_field_beyond_existing_rows(void)
{
    char result[64];

    /* Buffer starts empty (rows == 0), so row 5 does not exist */
    int ret = csv_insert_field(buffer, 5, 0, "NewField");

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret,
        "csv_insert_field should return 0 when inserting beyond existing rows");

    /* The field should now be accessible */
    get_field_text(buffer, 5, 0, result, sizeof(result));
    TEST_ASSERT_EQUAL_STRING_MESSAGE("NewField", result,
        "Field value should be 'NewField' after insert beyond existing rows");
}

/* Test 2: Insert into a non-existent entry (beyond current width of an existing row).
 * The function should fall through to csv_set_field. */
void test_insert_field_beyond_existing_width(void)
{
    char result[64];

    /* Create row 0 with one field */
    csv_set_field(buffer, 0, 0, "Existing");

    /* Insert at entry 10, which is beyond current width */
    int ret = csv_insert_field(buffer, 0, 10, "FarField");

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret,
        "csv_insert_field should return 0 when inserting beyond existing width");

    get_field_text(buffer, 0, 10, result, sizeof(result));
    TEST_ASSERT_EQUAL_STRING_MESSAGE("FarField", result,
        "Field value should be 'FarField' after insert beyond existing width");
}

/* Test 3: Insert at the beginning of an existing row (entry 0).
 * All existing fields should shift right by one. */
void test_insert_field_at_beginning_of_row(void)
{
    char result[64];

    populate_buffer_3x3(buffer);

    /* Row 0 currently: A0, B0, C0 */
    int ret = csv_insert_field(buffer, 0, 0, "INSERTED");

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret,
        "csv_insert_field should return 0 on successful insert at beginning");

    /* Width of row 0 should now be 4 */
    int width = csv_get_width(buffer, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(4, width,
        "Row 0 width should be 4 after inserting one field at the beginning");

    /* New layout: INSERTED, A0, B0, C0 */
    get_field_text(buffer, 0, 0, result, sizeof(result));
    TEST_ASSERT_EQUAL_STRING_MESSAGE("INSERTED", result,
        "Entry 0 should be 'INSERTED' after insert at beginning");

    get_field_text(buffer, 0, 1, result, sizeof(result));
    TEST_ASSERT_EQUAL_STRING_MESSAGE("A0", result,
        "Entry 1 should be 'A0' after insert at beginning");

    get_field_text(buffer, 0, 2, result, sizeof(result));
    TEST_ASSERT_EQUAL_STRING_MESSAGE("B0", result,
        "Entry 2 should be 'B0' after insert at beginning");

    get_field_text(buffer, 0, 3, result, sizeof(result));
    TEST_ASSERT_EQUAL_STRING_MESSAGE("C0", result,
        "Entry 3 should be 'C0' after insert at beginning");
}

/* Test 4: Insert in the middle of an existing row.
 * Fields at and after the insertion point should shift right. */
void test_insert_field_in_middle_of_row(void)
{
    char result[64];

    populate_buffer_3x3(buffer);

    /* Row 1 currently: A1, B1, C1 — insert at entry 1 */
    int ret = csv_insert_field(buffer, 1, 1, "MID");

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret,
        "csv_insert_field should return 0 on successful insert in middle");

    int width = csv_get_width(buffer, 1);
    TEST_ASSERT_EQUAL_INT_MESSAGE(4, width,
        "Row 1 width should be 4 after inserting one field in the middle");

    /* New layout: A1, MID, B1, C1 */
    get_field_text(buffer, 1, 0, result, sizeof(result));
    TEST_ASSERT_EQUAL_STRING_MESSAGE("A1", result,
        "Entry 0 should remain 'A1' after middle insert");

    get_field_text(buffer, 1, 1, result, sizeof(result));
    TEST_ASSERT_EQUAL_STRING_MESSAGE("MID", result,
        "Entry 1 should be 'MID' after middle insert");

    get_field_text(buffer, 1, 2, result, sizeof(result));
    TEST_ASSERT_EQUAL_STRING_MESSAGE("B1", result,
        "Entry 2 should be 'B1' after middle insert");

    get_field_text(buffer, 1, 3, result, sizeof(result));
    TEST_ASSERT_EQUAL_STRING_MESSAGE("C1", result,
        "Entry 3 should be 'C1' after middle insert");
}

/* Test 5: Insert at the last valid entry of an existing row.
 * The last existing field should shift right, and the new field occupies
 * the position of the old last field. */
void test_insert_field_at_last_entry_of_row(void)
{
    char result[64];

    populate_buffer_3x3(buffer);

    /* Row 2 currently: A2, B2, C2 — insert at entry 2 (last valid index) */
    int ret = csv_insert_field(buffer, 2, 2, "LAST_INSERT");

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret,
        "csv_insert_field should return 0 on successful insert at last entry");

    int width = csv_get_width(buffer, 2);
    TEST_ASSERT_EQUAL_INT_MESSAGE(4, width,
        "Row 2 width should be 4 after inserting at last entry");

    /* New layout: A2, B2, LAST_INSERT, C2 */
    get_field_text(buffer, 2, 0, result, sizeof(result));
    TEST_ASSERT_EQUAL_STRING_MESSAGE("A2", result,
        "Entry 0 should remain 'A2' after insert at last entry");

    get_field_text(buffer, 2, 1, result, sizeof(result));
    TEST_ASSERT_EQUAL_STRING_MESSAGE("B2", result,
        "Entry 1 should remain 'B2' after insert at last entry");

    get_field_text(buffer, 2, 2, result, sizeof(result));
    TEST_ASSERT_EQUAL_STRING_MESSAGE("LAST_INSERT", result,
        "Entry 2 should be 'LAST_INSERT' after insert at last entry");

    get_field_text(buffer, 2, 3, result, sizeof(result));
    TEST_ASSERT_EQUAL_STRING_MESSAGE("C2", result,
        "Entry 3 should be 'C2' (shifted) after insert at last entry");
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_insert_field_beyond_existing_rows);
    RUN_TEST(test_insert_field_beyond_existing_width);
    RUN_TEST(test_insert_field_at_beginning_of_row);
    RUN_TEST(test_insert_field_in_middle_of_row);
    RUN_TEST(test_insert_field_at_last_entry_of_row);
    return UNITY_END();
}