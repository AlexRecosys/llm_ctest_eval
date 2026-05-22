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
static void populate_buffer_basic(void)
{
    /* Creates a 2-row buffer:
     * Row 0: "alpha", "beta", "gamma"
     * Row 1: "one",   "two"
     */
    csv_set_field(buffer, 0, 0, "alpha");
    csv_set_field(buffer, 0, 1, "beta");
    csv_set_field(buffer, 0, 2, "gamma");
    csv_set_field(buffer, 1, 0, "one");
    csv_set_field(buffer, 1, 1, "two");
}

/* ------------------------------------------------------------------ */
/* Test 1: Insert into a non-existent row falls back to csv_set_field  */
/* ------------------------------------------------------------------ */
void test_csv_insert_field_nonexistent_row_sets_field(void)
{
    /* Buffer is empty; row 5 does not exist */
    int ret = csv_insert_field(buffer, 5, 0, "hello");

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret,
        "csv_insert_field should return 0 even when row does not exist");

    char dest[64];
    int get_ret = csv_get_field(dest, sizeof(dest), buffer, 5, 0);
    /* get_ret == 0 means the whole entry was copied */
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, get_ret,
        "Field should be retrievable after insert into non-existent row");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("hello", dest,
        "Field text should match what was inserted into non-existent row");
}

/* ------------------------------------------------------------------ */
/* Test 2: Insert beyond the last entry in an existing row             */
/* ------------------------------------------------------------------ */
void test_csv_insert_field_beyond_last_entry_sets_field(void)
{
    /* Set up row 0 with two fields */
    csv_set_field(buffer, 0, 0, "first");
    csv_set_field(buffer, 0, 1, "second");

    /* Insert at entry 10 — beyond current width */
    int ret = csv_insert_field(buffer, 0, 10, "far");

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret,
        "csv_insert_field should return 0 when entry is beyond row width");

    char dest[64];
    int get_ret = csv_get_field(dest, sizeof(dest), buffer, 0, 10);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, get_ret,
        "Field should be retrievable after insert beyond last entry");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("far", dest,
        "Field text should match what was inserted beyond last entry");
}

/* ------------------------------------------------------------------ */
/* Test 3: Insert at position 0 shifts all existing fields right       */
/* ------------------------------------------------------------------ */
void test_csv_insert_field_at_beginning_shifts_fields_right(void)
{
    csv_set_field(buffer, 0, 0, "A");
    csv_set_field(buffer, 0, 1, "B");
    csv_set_field(buffer, 0, 2, "C");

    int ret = csv_insert_field(buffer, 0, 0, "NEW");

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret,
        "csv_insert_field should return 0 on successful insert at position 0");

    /* Width should now be 4 */
    int width = csv_get_width(buffer, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(4, width,
        "Row width should increase by 1 after insert");

    char dest[64];

    csv_get_field(dest, sizeof(dest), buffer, 0, 0);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("NEW", dest,
        "Inserted field should be at position 0");

    csv_get_field(dest, sizeof(dest), buffer, 0, 1);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("A", dest,
        "Original field 0 should shift to position 1");

    csv_get_field(dest, sizeof(dest), buffer, 0, 2);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("B", dest,
        "Original field 1 should shift to position 2");

    csv_get_field(dest, sizeof(dest), buffer, 0, 3);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("C", dest,
        "Original field 2 should shift to position 3");
}

/* ------------------------------------------------------------------ */
/* Test 4: Insert in the middle shifts only the fields after it        */
/* ------------------------------------------------------------------ */
void test_csv_insert_field_in_middle_shifts_trailing_fields(void)
{
    populate_buffer_basic();
    /* Row 0 before: "alpha"[0], "beta"[1], "gamma"[2] */

    int ret = csv_insert_field(buffer, 0, 1, "INSERTED");

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret,
        "csv_insert_field should return 0 on successful middle insert");

    int width = csv_get_width(buffer, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(4, width,
        "Row 0 width should be 4 after inserting one field");

    char dest[64];

    csv_get_field(dest, sizeof(dest), buffer, 0, 0);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("alpha", dest,
        "Field at position 0 should remain 'alpha'");

    csv_get_field(dest, sizeof(dest), buffer, 0, 1);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("INSERTED", dest,
        "Newly inserted field should be at position 1");

    csv_get_field(dest, sizeof(dest), buffer, 0, 2);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("beta", dest,
        "Original field 1 ('beta') should shift to position 2");

    csv_get_field(dest, sizeof(dest), buffer, 0, 3);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("gamma", dest,
        "Original field 2 ('gamma') should shift to position 3");

    /* Row 1 should be untouched */
    csv_get_field(dest, sizeof(dest), buffer, 1, 0);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("one", dest,
        "Row 1 field 0 should be unaffected by insert in row 0");

    csv_get_field(dest, sizeof(dest), buffer, 1, 1);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("two", dest,
        "Row 1 field 1 should be unaffected by insert in row 0");
}

/* ------------------------------------------------------------------ */
/* Test 5: Insert at the last valid position shifts only the last field*/
/* ------------------------------------------------------------------ */
void test_csv_insert_field_at_last_valid_position(void)
{
    csv_set_field(buffer, 0, 0, "X");
    csv_set_field(buffer, 0, 1, "Y");
    csv_set_field(buffer, 0, 2, "Z");
    /* Row 0: "X"[0], "Y"[1], "Z"[2]  — last valid index is 2 */

    int ret = csv_insert_field(buffer, 0, 2, "MID");

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret,
        "csv_insert_field should return 0 when inserting at last valid index");

    int width = csv_get_width(buffer, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(4, width,
        "Row width should be 4 after inserting at last valid position");

    char dest[64];

    csv_get_field(dest, sizeof(dest), buffer, 0, 0);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("X", dest,
        "Field 0 should remain 'X'");

    csv_get_field(dest, sizeof(dest), buffer, 0, 1);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("Y", dest,
        "Field 1 should remain 'Y'");

    csv_get_field(dest, sizeof(dest), buffer, 0, 2);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("MID", dest,
        "Inserted field should be at position 2");

    csv_get_field(dest, sizeof(dest), buffer, 0, 3);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("Z", dest,
        "Original field 2 ('Z') should shift to position 3");
}

/* ------------------------------------------------------------------ */
/* main                                                                 */
/* ------------------------------------------------------------------ */
int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_csv_insert_field_nonexistent_row_sets_field);
    RUN_TEST(test_csv_insert_field_beyond_last_entry_sets_field);
    RUN_TEST(test_csv_insert_field_at_beginning_shifts_fields_right);
    RUN_TEST(test_csv_insert_field_in_middle_shifts_trailing_fields);
    RUN_TEST(test_csv_insert_field_at_last_valid_position);

    return UNITY_END();
}