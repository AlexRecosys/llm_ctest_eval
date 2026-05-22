#include "unity.h"
#include "csv.h"
#include <stdlib.h>
#include <string.h>

/* File-scope static fixture */
static CSV_BUFFER *buffer;

/* ---------------------------------------------------------------------------
 * Helper functions
 * --------------------------------------------------------------------------- */

/**
 * Create a fresh CSV_BUFFER with the given number of rows, each row having
 * `cols` fields pre-populated with the supplied text values.
 *
 * text_matrix is a flat array of (rows * cols) C-strings, row-major order.
 * Pass NULL for a cell to leave it as an empty string.
 */
static CSV_BUFFER *create_buffer(size_t rows, size_t cols,
                                  const char **text_matrix)
{
    CSV_BUFFER *buf = (CSV_BUFFER *)calloc(1, sizeof(CSV_BUFFER));
    if (!buf) return NULL;

    buf->field_delim = ',';
    buf->text_delim  = '"';
    buf->rows        = rows;
    buf->width       = (size_t *)calloc(rows, sizeof(size_t));
    buf->field       = (CSV_FIELD ***)calloc(rows, sizeof(CSV_FIELD **));

    for (size_t r = 0; r < rows; r++) {
        buf->width[r] = cols;
        buf->field[r] = (CSV_FIELD **)calloc(cols, sizeof(CSV_FIELD *));
        for (size_t c = 0; c < cols; c++) {
            buf->field[r][c] = (CSV_FIELD *)calloc(1, sizeof(CSV_FIELD));
            const char *src = (text_matrix && text_matrix[r * cols + c])
                              ? text_matrix[r * cols + c]
                              : "";
            size_t len = strlen(src);
            buf->field[r][c]->length = len;
            buf->field[r][c]->text   = (char *)calloc(len + 1, 1);
            if (len > 0)
                memcpy(buf->field[r][c]->text, src, len);
        }
    }
    return buf;
}

/**
 * Free a CSV_BUFFER and all its contents.
 */
static void free_buffer(CSV_BUFFER *buf)
{
    if (!buf) return;
    for (size_t r = 0; r < buf->rows; r++) {
        if (buf->field[r]) {
            for (size_t c = 0; c < buf->width[r]; c++) {
                if (buf->field[r][c]) {
                    free(buf->field[r][c]->text);
                    free(buf->field[r][c]);
                }
            }
            free(buf->field[r]);
        }
    }
    free(buf->field);
    free(buf->width);
    free(buf);
}

/* ---------------------------------------------------------------------------
 * setUp / tearDown
 * --------------------------------------------------------------------------- */

void setUp(void)
{
    buffer = NULL;
}

void tearDown(void)
{
    free_buffer(buffer);
    buffer = NULL;
}

/* ---------------------------------------------------------------------------
 * Test cases
 * --------------------------------------------------------------------------- */

/**
 * test_csv_clear_row_last_row_removed
 *
 * When the target row is the last row (row == buffer->rows - 1), the function
 * should call remove_last_row() and return 0.  After the call the buffer must
 * have one fewer row.
 */
void test_csv_clear_row_last_row_removed(void)
{
    const char *texts[] = {
        "alpha", "beta",   /* row 0 */
        "gamma", "delta"   /* row 1  <- last row */
    };
    buffer = create_buffer(2, 2, texts);
    TEST_ASSERT_NOT_NULL_MESSAGE(buffer, "Buffer allocation failed");

    size_t original_rows = buffer->rows;
    int result = csv_clear_row(buffer, 1); /* last row */

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result,
        "csv_clear_row should return 0 when removing the last row");
    TEST_ASSERT_EQUAL_UINT_MESSAGE(original_rows - 1, buffer->rows,
        "Row count should decrease by 1 after clearing the last row");
}

/**
 * test_csv_clear_row_non_last_row_returns_zero
 *
 * Clearing a non-last row with multiple fields should succeed (return 0) and
 * reduce the row's width to 1.
 */
void test_csv_clear_row_non_last_row_returns_zero(void)
{
    const char *texts[] = {
        "one", "two", "three",  /* row 0 */
        "four", "five", "six"   /* row 1 */
    };
    buffer = create_buffer(2, 3, texts);
    TEST_ASSERT_NOT_NULL_MESSAGE(buffer, "Buffer allocation failed");

    int result = csv_clear_row(buffer, 0); /* non-last row */

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result,
        "csv_clear_row should return 0 on success for a non-last row");
}

/**
 * test_csv_clear_row_width_reduced_to_one
 *
 * After clearing a non-last row that originally had multiple columns, the
 * width of that row must be exactly 1.
 */
void test_csv_clear_row_width_reduced_to_one(void)
{
    const char *texts[] = {
        "a", "b", "c", "d",  /* row 0 */
        "e", "f", "g", "h"   /* row 1 */
    };
    buffer = create_buffer(2, 4, texts);
    TEST_ASSERT_NOT_NULL_MESSAGE(buffer, "Buffer allocation failed");

    int result = csv_clear_row(buffer, 0);

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result,
        "csv_clear_row should return 0");
    TEST_ASSERT_EQUAL_UINT_MESSAGE(1u, buffer->width[0],
        "Width of cleared row should be 1");
}

/**
 * test_csv_clear_row_remaining_field_is_empty
 *
 * After clearing a non-last row, the single remaining field in that row must
 * contain an empty string (i.e. the text starts with '\0').
 */
void test_csv_clear_row_remaining_field_is_empty(void)
{
    const char *texts[] = {
        "hello", "world",  /* row 0 */
        "foo",   "bar"     /* row 1 */
    };
    buffer = create_buffer(2, 2, texts);
    TEST_ASSERT_NOT_NULL_MESSAGE(buffer, "Buffer allocation failed");

    int result = csv_clear_row(buffer, 0);

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result,
        "csv_clear_row should return 0");
    TEST_ASSERT_NOT_NULL_MESSAGE(buffer->field[0],
        "Row pointer must not be NULL after clear");
    TEST_ASSERT_NOT_NULL_MESSAGE(buffer->field[0][0],
        "Field pointer must not be NULL after clear");
    TEST_ASSERT_NOT_NULL_MESSAGE(buffer->field[0][0]->text,
        "Field text pointer must not be NULL after clear");
    /* The field text should be an empty / null-terminated string */
    TEST_ASSERT_EQUAL_INT_MESSAGE('\0', buffer->field[0][0]->text[0],
        "Remaining field text should be empty after csv_clear_row");
}

/**
 * test_csv_clear_row_other_rows_unaffected
 *
 * Clearing row 0 must not alter the contents or width of the other rows.
 */
void test_csv_clear_row_other_rows_unaffected(void)
{
    const char *texts[] = {
        "x", "y",          /* row 0 – will be cleared */
        "keep1", "keep2",  /* row 1 – must stay intact */
        "keep3", "keep4"   /* row 2 – must stay intact */
    };
    buffer = create_buffer(3, 2, texts);
    TEST_ASSERT_NOT_NULL_MESSAGE(buffer, "Buffer allocation failed");

    int result = csv_clear_row(buffer, 0);

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result,
        "csv_clear_row should return 0");

    /* Total row count must be unchanged */
    TEST_ASSERT_EQUAL_UINT_MESSAGE(3u, buffer->rows,
        "Total row count must not change when clearing a non-last row");

    /* Row 1 width and content must be intact */
    TEST_ASSERT_EQUAL_UINT_MESSAGE(2u, buffer->width[1],
        "Width of row 1 must be unchanged");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("keep1", buffer->field[1][0]->text,
        "Field [1][0] must be unchanged after clearing row 0");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("keep2", buffer->field[1][1]->text,
        "Field [1][1] must be unchanged after clearing row 0");

    /* Row 2 width and content must be intact */
    TEST_ASSERT_EQUAL_UINT_MESSAGE(2u, buffer->width[2],
        "Width of row 2 must be unchanged");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("keep3", buffer->field[2][0]->text,
        "Field [2][0] must be unchanged after clearing row 0");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("keep4", buffer->field[2][1]->text,
        "Field [2][1] must be unchanged after clearing row 0");
}

/* ---------------------------------------------------------------------------
 * main
 * --------------------------------------------------------------------------- */

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_csv_clear_row_last_row_removed);
    RUN_TEST(test_csv_clear_row_non_last_row_returns_zero);
    RUN_TEST(test_csv_clear_row_width_reduced_to_one);
    RUN_TEST(test_csv_clear_row_remaining_field_is_empty);
    RUN_TEST(test_csv_clear_row_other_rows_unaffected);

    return UNITY_END();
}