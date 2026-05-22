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
        /* Set up a single row with one field */
        int rc = csv_set_field(buffer, 0, 0, "hello");
        TEST_ASSERT_EQUAL_INT_MESSAGE(0, rc,
                "csv_set_field should succeed for row 0, entry 0");

        TEST_ASSERT_EQUAL_INT_MESSAGE(1, csv_get_height(buffer),
                "Buffer should have 1 row before clear");

        /* Clear the only (last) row */
        int result = csv_clear_row(buffer, 0);
        TEST_ASSERT_EQUAL_INT_MESSAGE(0, result,
                "csv_clear_row should return 0 when clearing the last row");

        /* The row should have been removed entirely */
        TEST_ASSERT_EQUAL_INT_MESSAGE(0, csv_get_height(buffer),
                "Buffer height should be 0 after removing the only row");
}

/* -------------------------------------------------------------------------
 * Test 2: Clear the last row in a multi-row buffer.
 * Expected: returns 0, buffer height decreases by 1, other rows intact.
 * ------------------------------------------------------------------------- */
void test_csv_clear_row_last_row_multi_row_buffer(void)
{
        /* Create 3 rows, each with 2 fields */
        populate_buffer(buffer, 3, 2, "data");

        TEST_ASSERT_EQUAL_INT_MESSAGE(3, csv_get_height(buffer),
                "Buffer should have 3 rows before clear");

        /* Clear the last row (index 2) */
        int result = csv_clear_row(buffer, 2);
        TEST_ASSERT_EQUAL_INT_MESSAGE(0, result,
                "csv_clear_row should return 0 when clearing the last row");

        /* Buffer should now have 2 rows */
        TEST_ASSERT_EQUAL_INT_MESSAGE(2, csv_get_height(buffer),
                "Buffer height should be 2 after removing the last row");

        /* First two rows should still have their data */
        char dest[64];
        csv_get_field(dest, sizeof(dest), buffer, 0, 0);
        TEST_ASSERT_EQUAL_STRING_MESSAGE("data", dest,
                "Row 0, field 0 should still contain 'data'");

        csv_get_field(dest, sizeof(dest), buffer, 1, 0);
        TEST_ASSERT_EQUAL_STRING_MESSAGE("data", dest,
                "Row 1, field 0 should still contain 'data'");
}

/* -------------------------------------------------------------------------
 * Test 3: Clear a non-last row that has multiple fields.
 * Expected: returns 0, row width becomes 1, field is cleared (empty).
 * ------------------------------------------------------------------------- */
void test_csv_clear_row_non_last_row_multiple_fields(void)
{
        /* Create 3 rows; row 0 gets 4 fields, others get 1 */
        csv_set_field(buffer, 0, 0, "alpha");
        csv_set_field(buffer, 0, 1, "beta");
        csv_set_field(buffer, 0, 2, "gamma");
        csv_set_field(buffer, 0, 3, "delta");
        csv_set_field(buffer, 1, 0, "row1");
        csv_set_field(buffer, 2, 0, "row2");

        TEST_ASSERT_EQUAL_INT_MESSAGE(4, csv_get_width(buffer, 0),
                "Row 0 should have width 4 before clear");

        /* Clear row 0 (not the last row) */
        int result = csv_clear_row(buffer, 0);
        TEST_ASSERT_EQUAL_INT_MESSAGE(0, result,
                "csv_clear_row should return 0 for a non-last row");

        /* Row 0 width should now be 1 */
        TEST_ASSERT_EQUAL_INT_MESSAGE(1, csv_get_width(buffer, 0),
                "Row 0 width should be 1 after csv_clear_row");

        /* The single remaining field should be empty */
        char dest[64];
        int get_rc = csv_get_field(dest, sizeof(dest), buffer, 0, 0);
        TEST_ASSERT_EQUAL_INT_MESSAGE(2, get_rc,
                "csv_get_field should return 2 (empty cell) for cleared field");

        /* Other rows should be unaffected */
        csv_get_field(dest, sizeof(dest), buffer, 1, 0);
        TEST_ASSERT_EQUAL_STRING_MESSAGE("row1", dest,
                "Row 1, field 0 should still contain 'row1'");

        csv_get_field(dest, sizeof(dest), buffer, 2, 0);
        TEST_ASSERT_EQUAL_STRING_MESSAGE("row2", dest,
                "Row 2, field 0 should still contain 'row2'");
}

/* -------------------------------------------------------------------------
 * Test 4: Clear a non-last row that already has only one field.
 * Expected: returns 0, row width stays 1, field content is cleared.
 * ------------------------------------------------------------------------- */
void test_csv_clear_row_non_last_row_single_field(void)
{
        /* Create 2 rows, each with 1 field */
        csv_set_field(buffer, 0, 0, "only_field");
        csv_set_field(buffer, 1, 0, "second_row");

        TEST_ASSERT_EQUAL_INT_MESSAGE(2, csv_get_height(buffer),
                "Buffer should have 2 rows");
        TEST_ASSERT_EQUAL_INT_MESSAGE(1, csv_get_width(buffer, 0),
                "Row 0 should have width 1 before clear");

        /* Clear row 0 (not the last row) */
        int result = csv_clear_row(buffer, 0);
        TEST_ASSERT_EQUAL_INT_MESSAGE(0, result,
                "csv_clear_row should return 0 for a single-field non-last row");

        /* Row 0 width should still be 1 */
        TEST_ASSERT_EQUAL_INT_MESSAGE(1, csv_get_width(buffer, 0),
                "Row 0 width should remain 1 after clearing a single-field row");

        /* The field should now be empty */
        char dest[64];
        int get_rc = csv_get_field(dest, sizeof(dest), buffer, 0, 0);
        TEST_ASSERT_EQUAL_INT_MESSAGE(2, get_rc,
                "csv_get_field should return 2 (empty) after clearing the field");

        /* Row 1 should be unaffected */
        csv_get_field(dest, sizeof(dest), buffer, 1, 0);
        TEST_ASSERT_EQUAL_STRING_MESSAGE("second_row", dest,
                "Row 1, field 0 should still contain 'second_row'");
}

/* -------------------------------------------------------------------------
 * Test 5: Clear a middle row in a buffer with many rows and many fields.
 * Expected: returns 0, middle row width becomes 1 and is empty,
 *           surrounding rows are unaffected.
 * ------------------------------------------------------------------------- */
void test_csv_clear_row_middle_row_many_fields(void)
{
        /* Row 0: 1 field */
        csv_set_field(buffer, 0, 0, "first");
        /* Row 1: 5 fields */
        csv_set_field(buffer, 1, 0, "f0");
        csv_set_field(buffer, 1, 1, "f1");
        csv_set_field(buffer, 1, 2, "f2");
        csv_set_field(buffer, 1, 3, "f3");
        csv_set_field(buffer, 1, 4, "f4");
        /* Row 2: 1 field */
        csv_set_field(buffer, 2, 0, "last");

        TEST_ASSERT_EQUAL_INT_MESSAGE(3, csv_get_height(buffer),
                "Buffer should have 3 rows");
        TEST_ASSERT_EQUAL_INT_MESSAGE(5, csv_get_width(buffer, 1),
                "Row 1 should have width 5 before clear");

        /* Clear row 1 (middle row, not the last) */
        int result = csv_clear_row(buffer, 1);
        TEST_ASSERT_EQUAL_INT_MESSAGE(0, result,
                "csv_clear_row should return 0 for the middle row");

        /* Row 1 width should now be 1 */
        TEST_ASSERT_EQUAL_INT_MESSAGE(1, csv_get_width(buffer, 1),
                "Row 1 width should be 1 after csv_clear_row");

        /* Row 1 field 0 should be empty */
        char dest[64];
        int get_rc = csv_get_field(dest, sizeof(dest), buffer, 1, 0);
        TEST_ASSERT_EQUAL_INT_MESSAGE(2, get_rc,
                "Row 1, field 0 should be empty after csv_clear_row");

        /* Row 0 should be unaffected */
        csv_get_field(dest, sizeof(dest), buffer, 0, 0);
        TEST_ASSERT_EQUAL_STRING_MESSAGE("first", dest,
                "Row 0, field 0 should still contain 'first'");

        /* Row 2 should be unaffected */
        csv_get_field(dest, sizeof(dest), buffer, 2, 0);
        TEST_ASSERT_EQUAL_STRING_MESSAGE("last", dest,
                "Row 2, field 0 should still contain 'last'");

        /* Buffer height should remain 3 */
        TEST_ASSERT_EQUAL_INT_MESSAGE(3, csv_get_height(buffer),
                "Buffer height should remain 3 after clearing a middle row");
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