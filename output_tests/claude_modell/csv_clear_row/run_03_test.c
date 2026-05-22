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

/* Helper: populate a buffer with a given number of rows, each with a given
 * number of fields set to the provided text. */
static void populate_buffer(CSV_BUFFER *buf, size_t num_rows,
                             size_t fields_per_row, const char *text)
{
        size_t r, f;
        for (r = 0; r < num_rows; r++) {
                csv_set_field(buf, (int)r, 0, (char *)text);
                for (f = 1; f < fields_per_row; f++) {
                        csv_set_field(buf, (int)r, (int)f, (char *)text);
                }
        }
}

/* -------------------------------------------------------------------------
 * Test 1: Clear the last row in a buffer that has only one row.
 * Expected: returns 0, buffer now has 0 rows (last row removed).
 * ------------------------------------------------------------------------- */
void test_csv_clear_row_last_row_single_row_buffer(void)
{
        /* Set up a buffer with exactly one row and one field */
        int rc = csv_set_field(buffer, 0, 0, "hello");
        TEST_ASSERT_EQUAL_INT_MESSAGE(0, rc,
                "csv_set_field should succeed for row 0, entry 0");

        TEST_ASSERT_EQUAL_UINT_MESSAGE((size_t)1, (size_t)csv_get_height(buffer),
                "Buffer should have 1 row before clearing");

        /* Row 0 is the last row, so csv_clear_row should call remove_last_row */
        int result = csv_clear_row(buffer, 0);
        TEST_ASSERT_EQUAL_INT_MESSAGE(0, result,
                "csv_clear_row on the only row should return 0");

        TEST_ASSERT_EQUAL_INT_MESSAGE(0, csv_get_height(buffer),
                "Buffer should have 0 rows after clearing the only row");
}

/* -------------------------------------------------------------------------
 * Test 2: Clear the last row in a multi-row buffer.
 * Expected: returns 0, buffer height decreases by 1, other rows intact.
 * ------------------------------------------------------------------------- */
void test_csv_clear_row_last_row_multi_row_buffer(void)
{
        /* Populate 3 rows, each with 2 fields */
        populate_buffer(buffer, 3, 2, "data");

        TEST_ASSERT_EQUAL_INT_MESSAGE(3, csv_get_height(buffer),
                "Buffer should have 3 rows before clearing");

        /* Clear the last row (row index 2) */
        int result = csv_clear_row(buffer, 2);
        TEST_ASSERT_EQUAL_INT_MESSAGE(0, result,
                "csv_clear_row on last row of multi-row buffer should return 0");

        TEST_ASSERT_EQUAL_INT_MESSAGE(2, csv_get_height(buffer),
                "Buffer should have 2 rows after removing the last row");

        /* Verify the remaining rows still have their data */
        char dest[64];
        csv_get_field(dest, sizeof(dest), buffer, 0, 0);
        TEST_ASSERT_EQUAL_STRING_MESSAGE("data", dest,
                "Row 0 field 0 should still contain 'data'");

        csv_get_field(dest, sizeof(dest), buffer, 1, 0);
        TEST_ASSERT_EQUAL_STRING_MESSAGE("data", dest,
                "Row 1 field 0 should still contain 'data'");
}

/* -------------------------------------------------------------------------
 * Test 3: Clear a non-last row that has multiple fields.
 * Expected: returns 0, row width becomes 1, field is cleared (empty).
 * ------------------------------------------------------------------------- */
void test_csv_clear_row_non_last_row_multiple_fields(void)
{
        /* Populate 3 rows; row 0 gets 4 fields, others get 1 */
        populate_buffer(buffer, 3, 4, "value");

        TEST_ASSERT_EQUAL_INT_MESSAGE(4, csv_get_width(buffer, 0),
                "Row 0 should have 4 fields before clearing");

        /* Clear row 0 (not the last row) */
        int result = csv_clear_row(buffer, 0);
        TEST_ASSERT_EQUAL_INT_MESSAGE(0, result,
                "csv_clear_row on non-last row with multiple fields should return 0");

        /* Width should now be 1 */
        TEST_ASSERT_EQUAL_INT_MESSAGE(1, csv_get_width(buffer, 0),
                "Row 0 width should be 1 after csv_clear_row");

        /* The single remaining field should be empty */
        char dest[64];
        int get_rc = csv_get_field(dest, sizeof(dest), buffer, 0, 0);
        TEST_ASSERT_EQUAL_INT_MESSAGE(2, get_rc,
                "csv_get_field should return 2 (empty cell) for cleared field");

        /* Buffer height should be unchanged */
        TEST_ASSERT_EQUAL_INT_MESSAGE(3, csv_get_height(buffer),
                "Buffer height should remain 3 after clearing a non-last row");
}

/* -------------------------------------------------------------------------
 * Test 4: Clear a non-last row that already has exactly one field.
 * Expected: returns 0, row width stays 1, field content is cleared.
 * ------------------------------------------------------------------------- */
void test_csv_clear_row_non_last_row_single_field(void)
{
        /* Populate 2 rows, each with 1 field */
        int rc0 = csv_set_field(buffer, 0, 0, "keep_me_not");
        int rc1 = csv_set_field(buffer, 1, 0, "other");
        TEST_ASSERT_EQUAL_INT_MESSAGE(0, rc0,
                "csv_set_field for row 0 should succeed");
        TEST_ASSERT_EQUAL_INT_MESSAGE(0, rc1,
                "csv_set_field for row 1 should succeed");

        TEST_ASSERT_EQUAL_INT_MESSAGE(1, csv_get_width(buffer, 0),
                "Row 0 should have 1 field before clearing");

        /* Clear row 0 (not the last row) */
        int result = csv_clear_row(buffer, 0);
        TEST_ASSERT_EQUAL_INT_MESSAGE(0, result,
                "csv_clear_row on non-last single-field row should return 0");

        /* Width should still be 1 */
        TEST_ASSERT_EQUAL_INT_MESSAGE(1, csv_get_width(buffer, 0),
                "Row 0 width should remain 1 after clearing a single-field row");

        /* The field should now be empty */
        char dest[64];
        int get_rc = csv_get_field(dest, sizeof(dest), buffer, 0, 0);
        TEST_ASSERT_EQUAL_INT_MESSAGE(2, get_rc,
                "csv_get_field should return 2 (empty) after clearing single-field row");

        /* Row 1 should be unaffected */
        csv_get_field(dest, sizeof(dest), buffer, 1, 0);
        TEST_ASSERT_EQUAL_STRING_MESSAGE("other", dest,
                "Row 1 field 0 should be unaffected by clearing row 0");
}

/* -------------------------------------------------------------------------
 * Test 5: Clear a middle row in a buffer with many rows and many fields.
 * Expected: returns 0, middle row width becomes 1 and is empty,
 *           surrounding rows are unaffected.
 * ------------------------------------------------------------------------- */
void test_csv_clear_row_middle_row_many_fields(void)
{
        /* Create 4 rows */
        csv_set_field(buffer, 0, 0, "row0_f0");
        csv_set_field(buffer, 0, 1, "row0_f1");

        csv_set_field(buffer, 1, 0, "row1_f0");
        csv_set_field(buffer, 1, 1, "row1_f1");
        csv_set_field(buffer, 1, 2, "row1_f2");
        csv_set_field(buffer, 1, 3, "row1_f3");
        csv_set_field(buffer, 1, 4, "row1_f4");

        csv_set_field(buffer, 2, 0, "row2_f0");
        csv_set_field(buffer, 2, 1, "row2_f1");

        csv_set_field(buffer, 3, 0, "row3_f0");

        TEST_ASSERT_EQUAL_INT_MESSAGE(4, csv_get_height(buffer),
                "Buffer should have 4 rows");
        TEST_ASSERT_EQUAL_INT_MESSAGE(5, csv_get_width(buffer, 1),
                "Row 1 should have 5 fields before clearing");

        /* Clear row 1 (a middle row) */
        int result = csv_clear_row(buffer, 1);
        TEST_ASSERT_EQUAL_INT_MESSAGE(0, result,
                "csv_clear_row on middle row should return 0");

        /* Row 1 width should now be 1 */
        TEST_ASSERT_EQUAL_INT_MESSAGE(1, csv_get_width(buffer, 1),
                "Row 1 width should be 1 after csv_clear_row");

        /* Row 1 field 0 should be empty */
        char dest[64];
        int get_rc = csv_get_field(dest, sizeof(dest), buffer, 1, 0);
        TEST_ASSERT_EQUAL_INT_MESSAGE(2, get_rc,
                "Row 1 field 0 should be empty after csv_clear_row");

        /* Buffer height should be unchanged */
        TEST_ASSERT_EQUAL_INT_MESSAGE(4, csv_get_height(buffer),
                "Buffer height should remain 4 after clearing middle row");

        /* Row 0 should be unaffected */
        csv_get_field(dest, sizeof(dest), buffer, 0, 0);
        TEST_ASSERT_EQUAL_STRING_MESSAGE("row0_f0", dest,
                "Row 0 field 0 should be unaffected");
        csv_get_field(dest, sizeof(dest), buffer, 0, 1);
        TEST_ASSERT_EQUAL_STRING_MESSAGE("row0_f1", dest,
                "Row 0 field 1 should be unaffected");

        /* Row 2 should be unaffected */
        csv_get_field(dest, sizeof(dest), buffer, 2, 0);
        TEST_ASSERT_EQUAL_STRING_MESSAGE("row2_f0", dest,
                "Row 2 field 0 should be unaffected");

        /* Row 3 should be unaffected */
        csv_get_field(dest, sizeof(dest), buffer, 3, 0);
        TEST_ASSERT_EQUAL_STRING_MESSAGE("row3_f0", dest,
                "Row 3 field 0 should be unaffected");
}

int main(void)
{
        UNITY_BEGIN();
        RUN_TEST(test_csv_clear_row_last_row_single_row_buffer);
        RUN_TEST(test_csv_clear_row_last_row_multi_row_buffer);
        RUN_TEST(test_csv_clear_row_non_last_row_multiple_fields);
        RUN_TEST(test_csv_clear_row_non_last_row_single_field);
        RUN_TEST(test_csv_clear_row_middle_row_many_fields);
        return UNITY_END();
}