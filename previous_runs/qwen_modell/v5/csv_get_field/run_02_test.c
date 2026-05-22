#include "unity.h"
#include "csv.h"
#include <stdlib.h>
#include <string.h>

// Helper to create a CSV_FIELD with given text
static CSV_FIELD* create_field(const char* text) {
    CSV_FIELD* field = (CSV_FIELD*)malloc(sizeof(CSV_FIELD));
    TEST_ASSERT_NOT_NULL(field);
    field->length = strlen(text);
    field->text = (char*)malloc(field->length + 1);
    TEST_ASSERT_NOT_NULL(field->text);
    strcpy(field->text, text);
    return field;
}

// Helper to create a CSV_BUFFER with given rows and columns
static CSV_BUFFER* create_buffer(size_t rows, size_t cols_per_row, ...) {
    CSV_BUFFER* buf = (CSV_BUFFER*)malloc(sizeof(CSV_BUFFER));
    TEST_ASSERT_NOT_NULL(buf);

    buf->rows = rows;
    buf->width = (size_t*)malloc(rows * sizeof(size_t));
    TEST_ASSERT_NOT_NULL(buf->width);

    buf->field = (CSV_FIELD***)malloc(rows * sizeof(CSV_FIELD**));
    TEST_ASSERT_NOT_NULL(buf->field);

    va_list args;
    va_start(args, cols_per_row);

    for (size_t r = 0; r < rows; ++r) {
        size_t cols = va_arg(args, size_t);
        buf->width[r] = cols;

        buf->field[r] = (CSV_FIELD**)malloc(cols * sizeof(CSV_FIELD*));
        TEST_ASSERT_NOT_NULL(buf->field[r]);

        for (size_t c = 0; c < cols; ++c) {
            const char* text = va_arg(args, const char*);
            buf->field[r][c] = create_field(text);
        }
    }

    va_end(args);

    buf->field_delim = ',';
    buf->text_delim = '"';

    return buf;
}

// Helper to free a CSV_BUFFER and all its contents
static void free_buffer(CSV_BUFFER* buf) {
    if (!buf) return;
    for (size_t r = 0; r < buf->rows; ++r) {
        if (buf->field[r]) {
            for (size_t c = 0; c < buf->width[r]; ++c) {
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

// Global buffer for tests
CSV_BUFFER* test_buffer = NULL;

// Setup: create a 2x3 test buffer
void setUp(void) {
    test_buffer = create_buffer(
        2,  // rows
        3, "A1", "A2", "A3",  // row 0: 3 fields
        2, "B1", "B2"          // row 1: 2 fields
    );
}

// Teardown: free buffer
void tearDown(void) {
    free_buffer(test_buffer);
    test_buffer = NULL;
}

// Test: valid field retrieval (exact fit)
static void test_csv_get_field_valid_exact_fit(void) {
    char dest[10];
    int result = csv_get_field(dest, sizeof(dest), test_buffer, 0, 1);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_STRING("A2", dest);
}

// Test: valid field retrieval (truncation needed)
static void test_csv_get_field_valid_truncated(void) {
    char dest[3];  // too small for "A2" (length=2 + null = 3) → exact fit, but test truncation case
    int result = csv_get_field(dest, sizeof(dest), test_buffer, 0, 1);
    // dest_len=3, field length=2 → dest_len+1 = 3 == field length → no truncation warning
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_STRING("A2", dest);

    // Now test with smaller buffer → truncation
    char dest2[2];
    result = csv_get_field(dest2, sizeof(dest2), test_buffer, 0, 1);
    TEST_ASSERT_EQUAL_INT(1, result);  // truncated
    TEST_ASSERT_EQUAL_STRING("A", dest2);
}

// Test: empty field
static void test_csv_get_field_empty(void) {
    CSV_BUFFER* buf = create_buffer(1, 1, "");
    char dest[10];
    int result = csv_get_field(dest, sizeof(dest), buf, 0, 0);
    TEST_ASSERT_EQUAL_INT(2, result);
    TEST_ASSERT_EQUAL_STRING("", dest);
    free_buffer(buf);
}

// Test: invalid row (too high)
static void test_csv_get_field_invalid_row_high(void) {
    char dest[10];
    int result = csv_get_field(dest, sizeof(dest), test_buffer, 5, 0);
    TEST_ASSERT_EQUAL_INT(2, result);
    TEST_ASSERT_EQUAL_STRING("", dest);
}

// Test: invalid row (negative)
static void test_csv_get_field_invalid_row_negative(void) {
    char dest[10];
    // Cast -1 to size_t → huge positive number (since size_t is unsigned)
    int result = csv_get_field(dest, sizeof(dest), test_buffer, (size_t)-1, 0);
    TEST_ASSERT_EQUAL_INT(2, result);
    TEST_ASSERT_EQUAL_STRING("", dest);
}

// Test: invalid entry (too high for row)
static void test_csv_get_field_invalid_entry_high(void) {
    char dest[10];
    int result = csv_get_field(dest, sizeof(dest), test_buffer, 1, 5);  // row 1 has only 2 cols
    TEST_ASSERT_EQUAL_INT(2, result);
    TEST_ASSERT_EQUAL_STRING("", dest);
}

// Test: invalid entry (negative)
static void test_csv_get_field_invalid_entry_negative(void) {
    char dest[10];
    int result = csv_get_field(dest, sizeof(dest), test_buffer, 0, (size_t)-1);
    TEST_ASSERT_EQUAL_INT(2, result);
    TEST_ASSERT_EQUAL_STRING("", dest);
}

// Test: zero dest_len
static void test_csv_get_field_zero_dest_len(void) {
    char dest[10];
    int result = csv_get_field(dest, 0, test_buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT(3, result);
    // dest should remain untouched (but we can't assert contents safely)
}

// Test: dest buffer exactly fits (no null overflow)
static void test_csv_get_field_exact_dest_len_no_overflow(void) {
    char dest[4];  // "A2" + '\0' = 3 chars → dest_len=4 is safe
    int result = csv_get_field(dest, sizeof(dest), test_buffer, 0, 1);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_STRING("A2", dest);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_csv_get_field_valid_exact_fit);
    RUN_TEST(test_csv_get_field_valid_truncated);
    RUN_TEST(test_csv_get_field_empty);
    RUN_TEST(test_csv_get_field_invalid_row_high);
    RUN_TEST(test_csv_get_field_invalid_row_negative);
    RUN_TEST(test_csv_get_field_invalid_entry_high);
    RUN_TEST(test_csv_get_field_invalid_entry_negative);
    RUN_TEST(test_csv_get_field_zero_dest_len);
    RUN_TEST(test_csv_get_field_exact_dest_len_no_overflow);
    return UNITY_END();
}