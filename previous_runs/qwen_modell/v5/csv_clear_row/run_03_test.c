#include "unity.h"
#include "csv.h"
#include <stdlib.h>
#include <string.h>

// Helper to create a test CSV buffer with specified rows, columns, and content
static CSV_BUFFER* create_test_buffer(size_t rows, size_t cols, const char* content)
{
    CSV_BUFFER* buffer = malloc(sizeof(CSV_BUFFER));
    TEST_ASSERT_NOT_NULL(buffer);

    buffer->field = malloc(rows * sizeof(CSV_FIELD**));
    buffer->width = malloc(rows * sizeof(size_t));
    buffer->rows = rows;
    buffer->field_delim = ',';
    buffer->text_delim = '"';

    for (size_t r = 0; r < rows; r++) {
        buffer->field[r] = malloc(cols * sizeof(CSV_FIELD*));
        buffer->width[r] = cols;
        for (size_t c = 0; c < cols; c++) {
            buffer->field[r][c] = malloc(sizeof(CSV_FIELD));
            buffer->field[r][c]->text = strdup(content ? content : "test");
            buffer->field[r][c]->length = strlen(buffer->field[r][c]->text);
        }
    }

    return buffer;
}

// Helper to destroy a test buffer and free all memory
static void destroy_test_buffer(CSV_BUFFER* buffer)
{
    if (!buffer) return;

    for (size_t r = 0; r < buffer->rows; r++) {
        for (size_t w = 0; w < buffer->width[r]; w++) {
            if (buffer->field[r][w]) {
                free(buffer->field[r][w]->text);
                free(buffer->field[r][w]);
            }
        }
        free(buffer->field[r]);
    }
    free(buffer->field);
    free(buffer->width);
    free(buffer);
}

// Helper to verify a row has exactly 1 field with empty content
static void assert_row_cleared(CSV_BUFFER* buffer, size_t row)
{
    TEST_ASSERT_EQUAL_SIZE(1, buffer->width[row]);
    TEST_ASSERT_NOT_NULL(buffer->field[row]);
    TEST_ASSERT_EQUAL_PTR(NULL, buffer->field[row][0]->text[0]); // empty string
    TEST_ASSERT_EQUAL_SIZE(0, buffer->field[row][0]->length);
}

// Test: clear last row (delegates to remove_last_row)
TEST(csv_clear_row, clear_last_row_delegates_to_remove_last_row)
{
    CSV_BUFFER* buffer = create_test_buffer(3, 4, "data");
    size_t original_rows = buffer->rows;

    // Mock remove_last_row to return 0 (success)
    // Since we can't mock, we rely on actual behavior — but we'll test the path
    // Note: This test assumes remove_last_row() works correctly per project spec
    int result = csv_clear_row(buffer, buffer->rows - 1);

    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_SIZE(original_rows - 1, buffer->rows);

    destroy_test_buffer(buffer);
}

// Test: clear non-last row with multiple fields → shrinks to 1 empty field
TEST(csv_clear_row, clear_non_last_row_with_multiple_fields)
{
    CSV_BUFFER* buffer = create_test_buffer(2, 5, "field");

    size_t row = 0;
    size_t original_width = buffer->width[row];
    TEST_ASSERT_GREATER_THAN(1, original_width);

    int result = csv_clear_row(buffer, row);

    TEST_ASSERT_EQUAL_INT(0, result);
    assert_row_cleared(buffer, row);

    // Verify other rows unchanged
    TEST_ASSERT_EQUAL_SIZE(5, buffer->width[1]);
    TEST_ASSERT_EQUAL_STRING("field", buffer->field[1][0]->text);

    destroy_test_buffer(buffer);
}

// Test: clear row with 1 field (edge case: should still succeed and keep 1 empty field)
TEST(csv_clear_row, clear_row_with_single_field)
{
    CSV_BUFFER* buffer = create_test_buffer(2, 1, "only");

    int result = csv_clear_row(buffer, 0);

    TEST_ASSERT_EQUAL_INT(0, result);
    assert_row_cleared(buffer, 0);

    destroy_test_buffer(buffer);
}

// Test: realloc failure → restore destroyed fields and return 1
// Note: This is hard to trigger reliably in unit tests without modifying realloc behavior.
// We'll test the *normal* path only, and note that full coverage requires injection.
// Since we cannot mock, and Unity doesn't support memory injection easily,
// we skip this path in pure Unity without project-specific hooks.
// Instead, we test that the function doesn't crash on valid input.

// Test: clear row 0 of multi-row buffer
TEST(csv_clear_row, clear_first_row)
{
    CSV_BUFFER* buffer = create_test_buffer(3, 2, "A");

    int result = csv_clear_row(buffer, 0);

    TEST_ASSERT_EQUAL_INT(0, result);
    assert_row_cleared(buffer, 0);

    // Verify other rows intact
    TEST_ASSERT_EQUAL_SIZE(2, buffer->width[1]);
    TEST_ASSERT_EQUAL_STRING("A", buffer->field[1][0]->text);

    destroy_test_buffer(buffer);
}

// Test: clear row with different content per field
TEST(csv_clear_row, clear_row_preserves_other_rows)
{
    CSV_BUFFER* buffer = create_test_buffer(2, 3, "X");
    // Overwrite some fields to have distinct content
    free(buffer->field[0][1]->text);
    buffer->field[0][1]->text = strdup("Y1");
    buffer->field[0][1]->length = strlen("Y1");

    free(buffer->field[0][2]->text);
    buffer->field[0][2]->text = strdup("Z1");
    buffer->field[0][2]->length = strlen("Z1");

    free(buffer->field[1][0]->text);
    buffer->field[1][0]->text = strdup("A2");
    buffer->field[1][0]->length = strlen("A2");

    int result = csv_clear_row(buffer, 0);

    TEST_ASSERT_EQUAL_INT(0, result);
    assert_row_cleared(buffer, 0);

    // Verify row 1 unchanged
    TEST_ASSERT_EQUAL_SIZE(3, buffer->width[1]);
    TEST_ASSERT_EQUAL_STRING("A2", buffer->field[1][0]->text);
    TEST_ASSERT_EQUAL_STRING("X", buffer->field[1][1]->text);
    TEST_ASSERT_EQUAL_STRING("X", buffer->field[1][2]->text);

    destroy_test_buffer(buffer);
}

// Main test runner
int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(csv_clear_row, clear_last_row_delegates_to_remove_last_row);
    RUN_TEST(csv_clear_row, clear_non_last_row_with_multiple_fields);
    RUN_TEST(csv_clear_row, clear_row_with_single_field);
    RUN_TEST(csv_clear_row, clear_first_row);
    RUN_TEST(csv_clear_row, clear_row_preserves_other_rows);
    return UNITY_END();
}