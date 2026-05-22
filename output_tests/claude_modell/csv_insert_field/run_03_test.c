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
    /* Set up row 0 with one field */
    csv_set_field(buffer, 0, 0, "first");

    /* Insert at entry 10, which is beyond width[0]-1 */
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
/* Test 3: Insert at the beginning of an existing row shifts fields    */
/* ------------------------------------------------------------------ */
void test_csv_insert_field_at_beginning_shifts_all_fields(void)
{
    populate_buffer_basic();

    /* Row 0 before: "alpha"[0], "beta"[1], "gamma"[2] */
    int ret = csv_insert_field(buffer, 0, 0, "NEW");

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret,
        "csv_insert_field should return 0 on successful insert at beginning");

    /* Row 0 after: "NEW"[0], "alpha"[1], "beta"[2], "gamma"[3] */
    int new_width = csv_get_width(buffer, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(4, new_width,
        "Row width should increase by 1 after insert at beginning");

    char dest[64];

    csv_get_field(dest, sizeof(dest), buffer, 0, 0);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("NEW", dest,
        "Inserted field should be at index 0");

    csv_get_field(dest, sizeof(dest), buffer, 0, 1);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("alpha", dest,
        "Original field[0] should shift to index 1");

    csv_get_field(dest, sizeof(dest), buffer, 0, 2);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("beta", dest,
        "Original field[1] should shift to index 2");

    csv_get_field(dest, sizeof(dest), buffer, 0, 3);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("gamma", dest,
        "Original field[2] should shift to index 3");
}

/* ------------------------------------------------------------------ */
/* Test 4: Insert in the middle of an existing row shifts later fields */
/* ------------------------------------------------------------------ */
void test_csv_insert_field_in_middle_shifts_later_fields(void)
{
    populate_buffer_basic();

    /* Row 0 before: "alpha"[0], "beta"[1], "gamma"[2] */
    int ret = csv_insert_field(buffer, 0, 1, "MIDDLE");

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret,
        "csv_insert_field should return 0 on successful insert in middle");

    /* Row 0 after: "alpha"[0], "MIDDLE"[1], "beta"[2], "gamma"[3] */
    int new_width = csv_get_width(buffer, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(4, new_width,
        "Row width should increase by 1 after insert in middle");

    char dest[64];

    csv_get_field(dest, sizeof(dest), buffer, 0, 0);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("alpha", dest,
        "Field before insertion point should be unchanged");

    csv_get_field(dest, sizeof(dest), buffer, 0, 1);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("MIDDLE", dest,
        "Inserted field should appear at the specified index");

    csv_get_field(dest, sizeof(dest), buffer, 0, 2);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("beta", dest,
        "Original field[1] should shift to index 2");

    csv_get_field(dest, sizeof(dest), buffer, 0, 3);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("gamma", dest,
        "Original field[2] should shift to index 3");
}

/* ------------------------------------------------------------------ */
/* Test 5: Insert does not disturb other rows                          */
/* ------------------------------------------------------------------ */
void test_csv_insert_field_does_not_disturb_other_rows(void)
{
    populate_buffer_basic();

    /* Row 1 before: "one"[0], "two"[1] */
    /* Insert into row 0 only */
    csv_insert_field(buffer, 0, 0, "INSERTED");

    /* Row 1 should be completely unchanged */
    int row1_width = csv_get_width(buffer, 1);
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, row1_width,
        "Row 1 width should remain 2 after insert into row 0");

    char dest[64];

    csv_get_field(dest, sizeof(dest), buffer, 1, 0);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("one", dest,
        "Row 1, field 0 should be unchanged after insert into row 0");

    csv_get_field(dest, sizeof(dest), buffer, 1, 1);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("two", dest,
        "Row 1, field 1 should be unchanged after insert into row 0");
}

/* ------------------------------------------------------------------ */
/* main                                                                 */
/* ------------------------------------------------------------------ */
int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_csv_insert_field_nonexistent_row_sets_field);
    RUN_TEST(test_csv_insert_field_beyond_last_entry_sets_field);
    RUN_TEST(test_csv_insert_field_at_beginning_shifts_all_fields);
    RUN_TEST(test_csv_insert_field_in_middle_shifts_later_fields);
    RUN_TEST(test_csv_insert_field_does_not_disturb_other_rows);

    return UNITY_END();
}