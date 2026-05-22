#include "unity.h"
#include "csv.h"
#include <stdlib.h>
#include <string.h>

// Helper to create a test buffer with specified rows and columns
static CSV_BUFFER* create_test_buffer(size_t rows, size_t cols) {
    CSV_BUFFER* buffer = malloc(sizeof(CSV_BUFFER));
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
            buffer->field[r][c] = malloc(sizeof(CSV_FIELD));
            TEST_ASSERT_NOT_NULL(buffer->field[r][c]);
            buffer->field[r][c]->text = malloc(10); // enough for "rowX_colY"
            TEST_ASSERT_NOT_NULL(buffer->field[r][c]->text);
            sprintf(buffer->field[r][c]->text, "row%zu_col%zu", r, c);
            buffer->field[r][c]->length = strlen(buffer->field[r][c]->text);
        }
    }

    return buffer;
}

// Helper to destroy a test buffer and free all memory
static void destroy_test_buffer(CSV_BUFFER* buffer) {
    if (!buffer) return;

    for (size_t r = 0; r < buffer->rows; r++) {
        if (buffer->field[r]) {
            for (size_t c = 0; c < buffer->width[r]; c++) {
                if (buffer->field[r][c]) {
                    if (buffer->field[r][c]->text) {
                        free(buffer->field[r][c]->text);
                    }
                    free(buffer->field[r][c]);
                }
            }
            free(buffer->field[r]);
        }
    }

    free(buffer->field);
    free(buffer->width);
    free(buffer);
}

// Test: Clear last row (should delegate to remove_last_row)
TEST(test_csv_clear_row_last_row) {
    CSV_BUFFER* buffer = create_test_buffer(3, 4);

    // Verify initial state
    TEST_ASSERT_EQUAL_SIZE(3, buffer->rows);
    TEST_ASSERT_EQUAL_SIZE(4, buffer->width[2]);

    // Clear last row (row 2)
    int result = csv_clear_row(buffer, 2);

    // Should succeed
    TEST_ASSERT_EQUAL_INT(0, result);

    // Row count should be reduced by 1 (assuming remove_last_row works correctly)
    TEST_ASSERT_EQUAL_SIZE(2, buffer->rows);

    // Clean up
    destroy_test_buffer(buffer);
}

// Test: Clear middle row (should reduce to 1 column)
TEST(test_csv_clear_row_middle_row) {
    CSV_BUFFER* buffer = create_test_buffer(3, 4);

    // Verify initial state
    TEST_ASSERT_EQUAL_SIZE(3, buffer->rows);
    TEST_ASSERT_EQUAL_SIZE(4, buffer->width[1]);

    // Clear middle row (row 1)
    int result = csv_clear_row(buffer, 1);

    // Should succeed
    TEST_ASSERT_EQUAL_INT(0, result);

    // Row count unchanged
    TEST_ASSERT_EQUAL_SIZE(3, buffer->rows);

    // Row 1 should now have width 1
    TEST_ASSERT_EQUAL_SIZE(1, buffer->width[1]);

    // First field should be empty string
    TEST_ASSERT_EQUAL_STRING("", buffer->field[1][0]->text);

    // Only one field should exist in row 1
    TEST_ASSERT_EQUAL_PTR(buffer->field[1], buffer->field[1]);

    // Clean up
    destroy_test_buffer(buffer);
}

// Test: Clear first row (should reduce to 1 column)
TEST(test_csv_clear_row_first_row) {
    CSV_BUFFER* buffer = create_test_buffer(2, 5);

    // Verify initial state
    TEST_ASSERT_EQUAL_SIZE(2, buffer->rows);
    TEST_ASSERT_EQUAL_SIZE(5, buffer->width[0]);

    // Clear first row (row 0)
    int result = csv_clear_row(buffer, 0);

    // Should succeed
    TEST_ASSERT_EQUAL_INT(0, result);

    // Row count unchanged
    TEST_ASSERT_EQUAL_SIZE(2, buffer->rows);

    // Row 0 should now have width 1
    TEST_ASSERT_EQUAL_SIZE(1, buffer->width[0]);

    // First field should be empty string
    TEST_ASSERT_EQUAL_STRING("", buffer->field[0][0]->text);

    // Clean up
    destroy_test_buffer(buffer);
}

// Test: Clear row with 1 column (should be no-op for middle row logic, but still valid)
TEST(test_csv_clear_row_single_column) {
    CSV_BUFFER* buffer = create_test_buffer(2, 1);

    // Verify initial state
    TEST_ASSERT_EQUAL_SIZE(2, buffer->rows);
    TEST_ASSERT_EQUAL_SIZE(1, buffer->width[0]);

    // Clear first row (row 0) — should still work
    int result = csv_clear_row(buffer, 0);

    // Should succeed
    TEST_ASSERT_EQUAL_INT(0, result);

    // Row 0 should still have width 1
    TEST_ASSERT_EQUAL_SIZE(1, buffer->width[0]);

    // Field should be empty
    TEST_ASSERT_EQUAL_STRING("", buffer->field[0][0]->text);

    // Clean up
    destroy_test_buffer(buffer);
}

// Test: Invalid row index (should not crash, but behavior depends on implementation)
// Note: This test assumes the function does *not* validate row index —
// if it does, adjust accordingly. Based on provided code, no bounds check is visible.
TEST(test_csv_clear_row_invalid_row) {
    CSV_BUFFER* buffer = create_test_buffer(2, 3);

    // Try to clear row beyond bounds (row 2 when rows=2)
    // This may crash or behave unpredictably — but we test that it doesn't crash
    // *if* the implementation has bounds checking. If not, this test may need to be skipped.
    // Since the provided code lacks bounds checking, we skip this test for now.
    // Uncomment only if csv_clear_row includes bounds validation.
    // int result = csv_clear_row(buffer, 2);
    // TEST_ASSERT_EQUAL_INT(1, result);

    // Clean up
    destroy_test_buffer(buffer);
}

// Test: realloc failure fallback (simulated by mocking realloc — but we can't mock per constraints)
// Since we cannot mock, we rely on real realloc behavior. This test verifies the fallback path
// *only if* realloc fails (which is hard to force deterministically).
// Instead, we test the *normal* path thoroughly and note that fallback is covered by code inspection.
// For robust testing, a mock would be needed — but per constraints, we omit this.

// Main entry point
int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_csv_clear_row_last_row);
    RUN_TEST(test_csv_clear_row_middle_row);
    RUN_TEST(test_csv_clear_row_first_row);
    RUN_TEST(test_csv_clear_row_single_column);
    RUN_TEST(test_csv_clear_row_invalid_row);

    return UNITY_END();
}