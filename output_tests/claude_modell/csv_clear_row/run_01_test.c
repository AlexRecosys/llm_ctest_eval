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
 * number of fields set to a provided text value. */
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
 * Test 1: Clear a row that is the last (and only) row in the buffer.
 *         The function should call remove_last_row internally and return 0.
 *         After the call the buffer should have 0 rows.
 * ---------------------------------------------------------------------- */
void test_csv_clear_row_last_row_is_removed(void)
{
        /* Build a single-row buffer with one field */
        csv_set_field(buffer, 0, 0, "hello");

        TEST_ASSERT_EQUAL_INT_MESSAGE(1, csv_get_height(buffer),
                "Buffer should have exactly 1 row before clear");

        int result = csv_clear_row(buffer, 0);

        TEST_ASSERT_EQUAL_INT_MESSAGE(0, result,
                "csv_clear_row should return 0 when removing the last row");
        TEST_ASSERT_EQUAL_INT_MESSAGE(0, csv_get_height(buffer),
                "Buffer height should be 0 after clearing the only row");
}

/* -------------------------------------------------------------------------
 * Test 2: Clear a non-last row that has multiple fields.
 *         The row should be reduced to width 1 and the remaining field
 *         should be empty (set to "\0").
 * ---------------------------------------------------------------------- */
void test_csv_clear_row_non_last_row_width_becomes_one(void)
{
        /* Row 0: three fields; Row 1: one field (so row 0 is not the last) */
        csv_set_field(buffer, 0, 0, "alpha");
        csv_set_field(buffer, 0, 1, "beta");
        csv_set_field(buffer, 0, 2, "gamma");
        csv_set_field(buffer, 1, 0, "anchor");

        TEST_ASSERT_EQUAL_INT_MESSAGE(3, csv_get_width(buffer, 0),
                "Row 0 should have width 3 before clear");

        int result = csv_clear_row(buffer, 0);

        TEST_ASSERT_EQUAL_INT_MESSAGE(0, result,
                "csv_clear_row should return 0 on success");
        TEST_ASSERT_EQUAL_INT_MESSAGE(1, csv_get_width(buffer, 0),
                "Row 0 width should be 1 after csv_clear_row");
}

/* -------------------------------------------------------------------------
 * Test 3: After clearing a non-last row the remaining field should be empty.
 * ---------------------------------------------------------------------- */
void test_csv_clear_row_non_last_row_field_is_empty(void)
{
        /* Row 0: two fields; Row 1: sentinel row */
        csv_set_field(buffer, 0, 0, "data1");
        csv_set_field(buffer, 0, 1, "data2");
        csv_set_field(buffer, 1, 0, "sentinel");

        int result = csv_clear_row(buffer, 0);

        TEST_ASSERT_EQUAL_INT_MESSAGE(0, result,
                "csv_clear_row should return 0 on success");

        char dest[64];
        int get_result = csv_get_field(dest, sizeof(dest), buffer, 0, 0);

        /* csv_get_field returns 2 when the cell is empty */
        TEST_ASSERT_EQUAL_INT_MESSAGE(2, get_result,
                "The remaining field in the cleared row should be empty");
}

/* -------------------------------------------------------------------------
 * Test 4: Clearing a non-last row should not affect other rows.
 *         Row 1 content and width must remain intact.
 * ---------------------------------------------------------------------- */
void test_csv_clear_row_does_not_affect_other_rows(void)
{
        /* Row 0: two fields; Row 1: two fields; Row 2: one field */
        csv_set_field(buffer, 0, 0, "r0f0");
        csv_set_field(buffer, 0, 1, "r0f1");
        csv_set_field(buffer, 1, 0, "r1f0");
        csv_set_field(buffer, 1, 1, "r1f1");
        csv_set_field(buffer, 2, 0, "r2f0");

        int result = csv_clear_row(buffer, 0);

        TEST_ASSERT_EQUAL_INT_MESSAGE(0, result,
                "csv_clear_row should return 0 on success");

        /* Row 1 should still have width 2 */
        TEST_ASSERT_EQUAL_INT_MESSAGE(2, csv_get_width(buffer, 1),
                "Row 1 width should be unchanged after clearing row 0");

        /* Row 1 field 0 should still contain "r1f0" */
        char dest[64];
        csv_get_field(dest, sizeof(dest), buffer, 1, 0);
        TEST_ASSERT_EQUAL_STRING_MESSAGE("r1f0", dest,
                "Row 1 field 0 content should be unchanged");

        /* Row 1 field 1 should still contain "r1f1" */
        csv_get_field(dest, sizeof(dest), buffer, 1, 1);
        TEST_ASSERT_EQUAL_STRING_MESSAGE("r1f1", dest,
                "Row 1 field 1 content should be unchanged");
}

/* -------------------------------------------------------------------------
 * Test 5: Clear a row that already has exactly one field.
 *         The row is not the last row, so it should stay in the buffer
 *         with width 1 and an empty field.
 * ---------------------------------------------------------------------- */
void test_csv_clear_row_single_field_non_last_row(void)
{
        /* Row 0: one field; Row 1: sentinel */
        csv_set_field(buffer, 0, 0, "only_field");
        csv_set_field(buffer, 1, 0, "sentinel");

        TEST_ASSERT_EQUAL_INT_MESSAGE(1, csv_get_width(buffer, 0),
                "Row 0 should have width 1 before clear");

        int result = csv_clear_row(buffer, 0);

        TEST_ASSERT_EQUAL_INT_MESSAGE(0, result,
                "csv_clear_row should return 0 for a single-field non-last row");

        /* Width must still be 1 */
        TEST_ASSERT_EQUAL_INT_MESSAGE(1, csv_get_width(buffer, 0),
                "Row 0 width should remain 1 after clearing a single-field row");

        /* The field should now be empty */
        char dest[64];
        int get_result = csv_get_field(dest, sizeof(dest), buffer, 0, 0);
        TEST_ASSERT_EQUAL_INT_MESSAGE(2, get_result,
                "The single field in the cleared row should be empty");

        /* Total row count must be unchanged */
        TEST_ASSERT_EQUAL_INT_MESSAGE(2, csv_get_height(buffer),
                "Buffer height should remain 2 after clearing a non-last row");
}

int main(void)
{
        UNITY_BEGIN();
        RUN_TEST(test_csv_clear_row_last_row_is_removed);
        RUN_TEST(test_csv_clear_row_non_last_row_width_becomes_one);
        RUN_TEST(test_csv_clear_row_non_last_row_field_is_empty);
        RUN_TEST(test_csv_clear_row_does_not_affect_other_rows);
        RUN_TEST(test_csv_clear_row_single_field_non_last_row);
        return UNITY_END();
}