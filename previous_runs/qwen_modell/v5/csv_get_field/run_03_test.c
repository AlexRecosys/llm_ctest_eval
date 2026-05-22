#include "unity.h"
#include "csv.h"
#include <stdlib.h>
#include <string.h>

// Helper to create a CSV_FIELD with given text (null-terminated)
CSV_FIELD* create_field(const char* text) {
    CSV_FIELD* field = (CSV_FIELD*)malloc(sizeof(CSV_FIELD));
    field->length = text ? strlen(text) : 0;
    field->text = (char*)malloc(field->length + 1);
    if (field->text) {
        strcpy(field->text, text);
    }
    return field;
}

// Helper to create a CSV_BUFFER with given rows and columns
// Each row has `cols` fields, all initialized to `text`
CSV_BUFFER* create_csv_buffer(size_t rows, size_t cols, const char* text) {
    CSV_BUFFER* buf = (CSV_BUFFER*)malloc(sizeof(CSV_BUFFER));
    buf->rows = rows;
    buf->width = (size_t*)malloc(rows * sizeof(size_t));
    buf->field = (CSV_FIELD***)malloc(rows * sizeof(CSV_FIELD**));
    buf->field_delim = ',';
    buf->text_delim = '"';

    for (size_t r = 0; r < rows; ++r) {
        buf->width[r] = cols;
        buf->field[r] = (CSV_FIELD**)malloc(cols * sizeof(CSV_FIELD*));
        for (size_t c = 0; c < cols; ++c) {
            buf->field[r][c] = create_field(text);
        }
    }
    return buf;
}

// Helper to free a CSV_BUFFER and all its contents
static void free_csv_buffer(CSV_BUFFER* buf) {
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

// Test: dest_len == 0 → return 3, dest unchanged (but not dereferenced)
static void test_csv_get_field_dest_len_zero(void) {
    CSV_BUFFER* buf = create_csv_buffer(1, 2, "hello");
    char dest[10] = "XXXXX";
    int result = csv_get_field(dest, 0, buf, 0, 0);
    TEST_ASSERT_EQUAL_INT(3, result);
    // dest is not modified per spec (dest_len == 0 → loop does nothing)
    TEST_ASSERT_EQUAL_STRING("XXXXX", dest);
    free_csv_buffer(buf);
}

// Test: row out of range (row >= rows) → return 2, dest cleared
static void test_csv_get_field_row_out_of_range(void) {
    CSV_BUFFER* buf = create_csv_buffer(2, 3, "data");
    char dest[10] = "XXXXX";
    int result = csv_get_field(dest, sizeof(dest), buf, 5, 1);
    TEST_ASSERT_EQUAL_INT(2, result);
    // dest should be cleared (first char set to '\0')
    TEST_ASSERT_EQUAL_INT(0, dest[0]);
    free_csv_buffer(buf);
}

// Test: entry out of range (entry >= width[row]) → return 2, dest cleared
static void test_csv_get_field_entry_out_of_range(void) {
    CSV_BUFFER* buf = create_csv_buffer(1, 2, "data");
    char dest[10] = "XXXXX";
    int result = csv_get_field(dest, sizeof(dest), buf, 0, 5);
    TEST_ASSERT_EQUAL_INT(2, result);
    TEST_ASSERT_EQUAL_INT(0, dest[0]);
    free_csv_buffer(buf);
}

// Test: valid row/entry, field fits in dest → return 0
static void test_csv_get_field_exact_fit(void) {
    CSV_BUFFER* buf = create_csv_buffer(1, 1, "abc");
    char dest[10];
    int result = csv_get_field(dest, sizeof(dest), buf, 0, 0);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_STRING("abc", dest);
    free_csv_buffer(buf);
}

// Test: valid row/entry, field truncated to dest_len → return 1
static void test_csv_get_field_truncated(void) {
    CSV_BUFFER* buf = create_csv_buffer(1, 1, "abcdef");
    char dest[5]; // will hold "abcd" + '\0'
    int result = csv_get_field(dest, sizeof(dest), buf, 0, 0);
    TEST_ASSERT_EQUAL_INT(1, result); // truncated
    TEST_ASSERT_EQUAL_STRING("abcd", dest);
    free_csv_buffer(buf);
}

// Test: valid row/entry, empty field → return 2
static void test_csv_get_field_empty(void) {
    CSV_BUFFER* buf = create_csv_buffer(1, 1, "");
    char dest[10] = "XXXXX";
    int result = csv_get_field(dest, sizeof(dest), buf, 0, 0);
    TEST_ASSERT_EQUAL_INT(2, result);
    TEST_ASSERT_EQUAL_INT(0, dest[0]); // cleared
    free_csv_buffer(buf);
}

// Test: negative row (should be caught by row < 0 check)
static void test_csv_get_field_negative_row(void) {
    CSV_BUFFER* buf = create_csv_buffer(1, 1, "data");
    char dest[10] = "XXXXX";
    // Cast -1 to size_t → huge value, so row >= rows
    int result = csv_get_field(dest, sizeof(dest), buf, (size_t)-1, 0);
    TEST_ASSERT_EQUAL_INT(2, result);
    TEST_ASSERT_EQUAL_INT(0, dest[0]);
    free_csv_buffer(buf);
}

// Test: negative entry (same as above)
static void test_csv_get_field_negative_entry(void) {
    CSV_BUFFER* buf = create_csv_buffer(1, 1, "data");
    char dest[10] = "XXXXX";
    int result = csv_get_field(dest, sizeof(dest), buf, 0, (size_t)-1);
    TEST_ASSERT_EQUAL_INT(2, result);
    TEST_ASSERT_EQUAL_INT(0, dest[0]);
    free_csv_buffer(buf);
}

// Test: dest buffer exactly matches field length (no truncation, no extra space)
static void test_csv_get_field_exact_dest_size(void) {
    CSV_BUFFER* buf = create_csv_buffer(1, 1, "abc");
    char dest[4]; // "abc" + '\0' = 4 bytes
    int result = csv_get_field(dest, sizeof(dest), buf, 0, 0);
    TEST_ASSERT_EQUAL_INT(0, result); // no truncation
    TEST_ASSERT_EQUAL_STRING("abc", dest);
    free_csv_buffer(buf);
}

// Test: multi-row buffer, access different rows/cols
static void test_csv_get_field_multi_row(void) {
    CSV_BUFFER* buf = create_csv_buffer(3, 2, "X");
    // Change row 1, col 0 to "Y"
    free(buf->field[1][0]->text);
    buf->field[1][0]->text = strdup("Y");
    buf->field[1][0]->length = 1;

    char dest[10];
    int result;

    // Row 0, col 0 → "X"
    result = csv_get_field(dest, sizeof(dest), buf, 0, 0);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_STRING("X", dest);

    // Row 1, col 0 → "Y"
    result = csv_get_field(dest, sizeof(dest), buf, 1, 0);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_STRING("Y", dest);

    // Row 2, col 1 → "X"
    result = csv_get_field(dest, sizeof(dest), buf, 2, 1);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_STRING("X", dest);

    free_csv_buffer(buf);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_csv_get_field_dest_len_zero);
    RUN_TEST(test_csv_get_field_row_out_of_range);
    RUN_TEST(test_csv_get_field_entry_out_of_range);
    RUN_TEST(test_csv_get_field_exact_fit);
    RUN_TEST(test_csv_get_field_truncated);
    RUN_TEST(test_csv_get_field_empty);
    RUN_TEST(test_csv_get_field_negative_row);
    RUN_TEST(test_csv_get_field_negative_entry);
    RUN_TEST(test_csv_get_field_exact_dest_size);
    RUN_TEST(test_csv_get_field_multi_row);
    return UNITY_END();
}