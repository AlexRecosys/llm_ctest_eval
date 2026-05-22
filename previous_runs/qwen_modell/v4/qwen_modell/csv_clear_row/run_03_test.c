#include "unity.h"
#include "csv.h"
#include <stdlib.h>
#include <string.h>

/* Helper to create a test buffer with specified rows and columns */
static CSV_BUFFER* create_test_buffer(size_t rows, size_t cols) {
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
static void destroy_test_buffer(CSV_BUFFER *buffer) {
    if (buffer == NULL) return;

    for (size_t r = 0; r < buffer->rows; r++) {
        if (buffer->field[r] != NULL) {
            for (size_t c = 0; c < buffer->width[r]; c++) {
                destroy_field(buffer->field[r][c]);
            }
            free(buffer->field[r]);
        }
    }

    free(buffer->field);
    free(buffer->width);
    free(buffer);
}

/* Test: Clear last row (should delegate to remove_last_row) */
void test_csv_clear_row_last_row(void) {
    CSV_BUFFER *buffer = create_test_buffer(3, 4);
    size_t original_rows = buffer->rows;

    /* Clear the last row (row 2) */
    int result = csv_clear_row(buffer, 2);

    /* Should succeed */
    TEST_ASSERT_EQUAL_INT(0, result);

    /* Row count should decrease by 1 */
    TEST_ASSERT_EQUAL_SIZE(original_rows - 1, buffer->rows);

    /* Verify the buffer still has valid data in remaining rows */
    TEST_ASSERT_EQUAL_SIZE(1, buffer->width[0]);
    TEST_ASSERT_EQUAL_STRING("", buffer->field[0][0]->text);

    destroy_test_buffer(buffer);
}

/* Test: Clear non-last row (should reduce to 1 column) */
void test_csv_clear_row_non_last_row(void) {
    CSV_BUFFER *buffer = create_test_buffer(3, 4);

    /* Clear row 1 (non-last) */
    int result = csv_clear_row(buffer, 1);

    /* Should succeed */
    TEST_ASSERT_EQUAL_INT(0, result);

    /* Row count unchanged */
    TEST_ASSERT_EQUAL_SIZE(3, buffer->rows);

    /* Row 1 should now have width 1 */
    TEST_ASSERT_EQUAL_SIZE(1, buffer->width[1]);

    /* The remaining field should be empty */
    TEST_ASSERT_EQUAL_STRING("", buffer->field[1][0]->text);

    /* Other rows unchanged */
    TEST_ASSERT_EQUAL_SIZE(4, buffer->width[0]);
    TEST_ASSERT_EQUAL_STRING("test", buffer->field[0][0]->text);

    destroy_test_buffer(buffer);
}

/* Test: Clear row with 1 column (edge case) */
void test_csv_clear_row_single_column(void) {
    CSV_BUFFER *buffer = create_test_buffer(2, 1);

    /* Clear row 0 (which has only 1 column) */
    int result = csv_clear_row(buffer, 0);

    /* Should succeed */
    TEST_ASSERT_EQUAL_INT(0, result);

    /* Row 0 should still have width 1 */
    TEST_ASSERT_EQUAL_SIZE(1, buffer->width[0]);

    /* Field should be empty */
    TEST_ASSERT_EQUAL_STRING("", buffer->field[0][0]->text);

    destroy_test_buffer(buffer);
}

/* Test: Clear last row when remove_last_row fails (simulated via mock or error path) */
/* Note: Since we cannot mock, we rely on actual error behavior if possible.
   If remove_last_row() returns non-zero, csv_clear_row should return 1. */
void test_csv_clear_row_last_row_failure(void) {
    /* This test assumes remove_last_row() may fail under certain conditions.
       Since we cannot force failure without mocking, this test may be skipped
       if the implementation doesn't provide a way to trigger failure. */

    /* For now, we test the success path only. */
    /* If your project has a way to make remove_last_row() fail (e.g., disk full),
       you could add a test here. Otherwise, this test is a placeholder. */
    TEST_IGNORE_MESSAGE("Skipping: cannot force remove_last_row() failure without mocking.");
}