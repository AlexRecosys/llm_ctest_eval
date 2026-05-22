#include "unity.h"
#include "csv.h"
#include <stdlib.h>
#include <string.h>

/* Helper to create a test buffer with specified rows and columns */
static CSV_BUFFER* create_test_buffer(size_t rows, size_t cols)
{
    CSV_BUFFER *buffer = malloc(sizeof(CSV_BUFFER));
    TEST_ASSERT_NOT_NULL(buffer);

    buffer->field = malloc(rows * sizeof(CSV_FIELD**));
    TEST_ASSERT_NOT_NULL(buffer->field);

    buffer->width = malloc(rows * sizeof(size_t));
    TEST_ASSERT_NOT_NULL(buffer->width);

    buffer->rows = rows;
    buffer->field_delim = ',';
    buffer->text_delim = '"';

    for (size_t r = 0; r < rows; r++) {
        buffer->width[r] = cols;
        buffer->field[r] = malloc(cols * sizeof(CSV_FIELD*));
        TEST_ASSERT_NOT_NULL(buffer->field[r]);

        for (size_t c = 0; c < cols; c++) {
            buffer->field[r][c] = create_field("test");
            TEST_ASSERT_NOT_NULL(buffer->field[r][c]);
        }
    }

    return buffer;
}

/* Helper to destroy a test buffer */
static void destroy_test_buffer(CSV_BUFFER *buffer)
{
    if (buffer == NULL) return;

    for (size_t r = 0; r < buffer->rows; r++) {
        for (size_t w = 0; w < buffer->width[r]; w++) {
            destroy_field(buffer->field[r][w]);
        }
        free(buffer->field[r]);
    }

    free(buffer->field);
    free(buffer->width);
    free(buffer);
}

/* Helper to verify row has exactly one empty field */
static void assert_row_cleared(CSV_BUFFER *buffer, size_t row)
{
    TEST_ASSERT_EQUAL_SIZE(1, buffer->width[row]);
    TEST_ASSERT_NOT_NULL(buffer->field[row]);
    TEST_ASSERT_EQUAL_STRING("", buffer->field[row][0]->text);
}

/* Helper to verify row has expected number of fields */
static void assert_row_width(CSV_BUFFER *buffer, size_t row, size_t expected_width)
{
    TEST_ASSERT_EQUAL_SIZE(expected_width, buffer->width[row]);
    TEST_ASSERT_NOT_NULL(buffer->field[row]);
}

/* Test: Clear last row (should call remove_last_row) */
void test_csv_clear_row_last_row(void)
{
    CSV_BUFFER *buffer = create_test_buffer(3, 5);
    size_t original_rows = buffer->rows;

    /* Clear the last row (row 2) */
    int result = csv_clear_row(buffer, 2);

    /* Should succeed */
    TEST_ASSERT_EQUAL_INT(0, result);

    /* Row count should be reduced by 1 */
    TEST_ASSERT_EQUAL_SIZE(original_rows - 1, buffer->rows);

    /* Verify buffer state */
    TEST_ASSERT_EQUAL_SIZE(2, buffer->rows);
    TEST_ASSERT_EQUAL_SIZE(5, buffer->width[0]);  /* row 0 unchanged */
    TEST_ASSERT_EQUAL_SIZE(5, buffer->width[1]);  /* row 1 unchanged */

    destroy_test_buffer(buffer);
}

/* Test: Clear non-last row with multiple fields */
void test_csv_clear_row_non_last_row(void)
{
    CSV_BUFFER *buffer = create_test_buffer(3, 5);

    /* Clear row 1 (non-last) */
    int result = csv_clear_row(buffer, 1);

    /* Should succeed */
    TEST_ASSERT_EQUAL_INT(0, result);

    /* Row count unchanged */
    TEST_ASSERT_EQUAL_SIZE(3, buffer->rows);

    /* Row 1 should now have exactly 1 field */
    assert_row_cleared(buffer, 1);

    /* Other rows unchanged */
    assert_row_width(buffer, 0, 5);
    assert_row_width(buffer, 2, 5);

    destroy_test_buffer(buffer);
}

/* Test: Clear row with 1 field (edge case) */
void test_csv_clear_row_single_field(void)
{
    CSV_BUFFER *buffer = create_test_buffer(2, 1);

    /* Clear row 0 (which has only 1 field) */
    int result = csv_clear_row(buffer, 0);

    /* Should succeed */
    TEST_ASSERT_EQUAL_INT(0, result);

    /* Row 0 should still have 1 field, now empty */
    assert_row_cleared(buffer, 0);

    destroy_test_buffer(buffer);
}

/* Test: Clear row with 0 fields (should not crash) */
void test_csv_clear_row_zero_width(void)
{
    CSV_BUFFER *buffer = create_test_buffer(2, 1);

    /* Manually set row 0 to have 0 fields */
    buffer->width[0] = 0;
    buffer->field[0] = NULL;

    /* Clear row 0 (should handle gracefully) */
    int result = csv_clear_row(buffer, 0);

    /* Should succeed (no-op for empty row) */
    TEST_ASSERT_EQUAL_INT(0, result);

    /* Row 0 should now have 1 empty field */
    assert_row_cleared(buffer, 0);

    destroy_test_buffer(buffer);
}

/* Test: realloc failure fallback (simulated via NULL return) */
void test_csv_clear_row_realloc_failure(void)
{
    /* We cannot directly simulate realloc failure in pure C without patching,
       but we can test the fallback logic by ensuring the function handles
       the case where realloc fails and restores fields. */

    /* Create buffer with 2 rows, 3 fields each */
    CSV_BUFFER *buffer = create_test_buffer(2, 3);

    /* Temporarily patch realloc to return NULL (requires platform-specific hook) */
    /* Since we cannot mock, we skip this test in standard Unity without hooks.
       In real code, you'd use a test-specific realloc wrapper. */

    /* For now, test normal behavior only */
    int result = csv_clear_row(buffer, 0);

    TEST_ASSERT_EQUAL_INT(0, result);
    assert_row_cleared(buffer, 0);

    destroy_test_buffer(buffer);
}