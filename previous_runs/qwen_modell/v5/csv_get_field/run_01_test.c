#include "unity.h"
#include "csv.h"
#include <stdlib.h>
#include <string.h>

// Helper to create a CSV_FIELD with given text (null-terminated)
CSV_FIELD* create_csv_field(const char* text) {
    CSV_FIELD* field = (CSV_FIELD*)malloc(sizeof(CSV_FIELD));
    field->length = strlen(text);
    field->text = (char*)malloc(field->length + 1);
    strcpy(field->text, text);
    return field;
}

// Helper to create a CSV_BUFFER with given rows and columns
CSV_BUFFER* create_csv_buffer(size_t num_rows, size_t* widths, char** data) {
    CSV_BUFFER* buf = (CSV_BUFFER*)malloc(sizeof(CSV_BUFFER));
    buf->rows = num_rows;
    buf->width = (size_t*)malloc(num_rows * sizeof(size_t));
    buf->field = (CSV_FIELD***)malloc(num_rows * sizeof(CSV_FIELD**));

    for (size_t r = 0; r < num_rows; ++r) {
        buf->width[r] = widths[r];
        buf->field[r] = (CSV_FIELD**)malloc(widths[r] * sizeof(CSV_FIELD*));
        for (size_t c = 0; c < widths[r]; ++c) {
            buf->field[r][c] = create_csv_field(data[r * 100 + c]); // assume max 100 cols per row for indexing
        }
    }

    buf->field_delim = ',';
    buf->text_delim = '"';

    return buf;
}

// Helper to free CSV_BUFFER and all its contents
void free_csv_buffer(CSV_BUFFER* buf) {
    if (!buf) return;
    for (size_t r = 0; r < buf->rows; ++r) {
        if (!buf->field || !buf->field[r]) continue;
        for (size_t c = 0; c < buf->width[r]; ++c) {
            if (buf->field[r][c]) {
                free(buf->field[r][c]->text);
                free(buf->field[r][c]);
            }
        }
        free(buf->field[r]);
    }
    free(buf->field);
    free(buf->width);
    free(buf);
}

// Test: Valid field, fits in buffer exactly
static void test_csv_get_field_valid_exact_fit(void) {
    size_t widths[1] = {2};
    char* data[2] = {"foo", "bar"};
    CSV_BUFFER* buf = create_csv_buffer(1, widths, data);

    char dest[10];
    int result = csv_get_field(dest, sizeof(dest), buf, 0, 0);

    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_STRING("foo", dest);

    free_csv_buffer(buf);
}

// Test: Valid field, truncated due to small buffer
static void test_csv_get_field_truncation(void) {
    size_t widths[1] = {1};
    char* data[1] = {"hello"};
    CSV_BUFFER* buf = create_csv_buffer(1, widths, data);

    char dest[4]; // only 4 bytes: can hold "hel" + '\0'
    int result = csv_get_field(dest, sizeof(dest), buf, 0, 0);

    TEST_ASSERT_EQUAL_INT(1, result); // truncated
    TEST_ASSERT_EQUAL_STRING_LEN("hel", dest, 3);
    TEST_ASSERT_EQUAL_INT('\0', dest[3]);

    free_csv_buffer(buf);
}

// Test: Empty field (length == 0)
static void test_csv_get_field_empty(void) {
    size_t widths[1] = {1};
    char* data[1] = {""};
    CSV_BUFFER* buf = create_csv_buffer(1, widths, data);

    char dest[10];
    int result = csv_get_field(dest, sizeof(dest), buf, 0, 0);

    TEST_ASSERT_EQUAL_INT(2, result);
    TEST_ASSERT_EQUAL_STRING("", dest);

    free_csv_buffer(buf);
}

// Test: Invalid row (row >= rows)
static void test_csv_get_field_invalid_row(void) {
    size_t widths[1] = {1};
    char* data[1] = {"foo"};
    CSV_BUFFER* buf = create_csv_buffer(1, widths, data);

    char dest[10];
    int result = csv_get_field(dest, sizeof(dest), buf, 1, 0); // row 1 doesn't exist

    TEST_ASSERT_EQUAL_INT(2, result);
    TEST_ASSERT_EQUAL_STRING("", dest);

    free_csv_buffer(buf);
}

// Test: Invalid entry (entry >= width[row])
static void test_csv_get_field_invalid_entry(void) {
    size_t widths[1] = {1};
    char* data[1] = {"foo"};
    CSV_BUFFER* buf = create_csv_buffer(1, widths, data);

    char dest[10];
    int result = csv_get_field(dest, sizeof(dest), buf, 0, 1); // entry 1 doesn't exist

    TEST_ASSERT_EQUAL_INT(2, result);
    TEST_ASSERT_EQUAL_STRING("", dest);

    free_csv_buffer(buf);
}

// Test: dest_len == 0 → return 3, no write (but dest is untouched)
static void test_csv_get_field_zero_dest_len(void) {
    size_t widths[1] = {1};
    char* data[1] = {"foo"};
    CSV_BUFFER* buf = create_csv_buffer(1, widths, data);

    char dest[10] = "XXXX";
    int result = csv_get_field(dest, 0, buf, 0, 0);

    TEST_ASSERT_EQUAL_INT(3, result);
    // dest should remain unchanged (no write attempted)
    TEST_ASSERT_EQUAL_STRING("XXXX", dest);

    free_csv_buffer(buf);
}

// Test: Negative row (should be caught by row < 0 check)
static void test_csv_get_field_negative_row(void) {
    size_t widths[1] = {1};
    char* data[1] = {"foo"};
    CSV_BUFFER* buf = create_csv_buffer(1, widths, data);

    char dest[10];
    int result = csv_get_field(dest, sizeof(dest), buf, (size_t)-1, 0); // cast -1 to size_t

    TEST_ASSERT_EQUAL_INT(2, result);
    TEST_ASSERT_EQUAL_STRING("", dest);

    free_csv_buffer(buf);
}

// Test: Negative entry (cast to size_t)
static void test_csv_get_field_negative_entry(void) {
    size_t widths[1] = {1};
    char* data[1] = {"foo"};
    CSV_BUFFER* buf = create_csv_buffer(1, widths, data);

    char dest[10];
    int result = csv_get_field(dest, sizeof(dest), buf, 0, (size_t)-1);

    TEST_ASSERT_EQUAL_INT(2, result);
    TEST_ASSERT_EQUAL_STRING("", dest);

    free_csv_buffer(buf);
}

// Test: Multi-row buffer, access second row
static void test_csv_get_field_multi_row(void) {
    size_t widths[2] = {2, 1};
    char* data[3] = {"a", "b", "c"}; // row0: [a, b], row1: [c]
    CSV_BUFFER* buf = create_csv_buffer(2, widths, data);

    char dest[10];
    int result = csv_get_field(dest, sizeof(dest), buf, 1, 0);

    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_STRING("c", dest);

    free_csv_buffer(buf);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_csv_get_field_valid_exact_fit);
    RUN_TEST(test_csv_get_field_truncation);
    RUN_TEST(test_csv_get_field_empty);
    RUN_TEST(test_csv_get_field_invalid_row);
    RUN_TEST(test_csv_get_field_invalid_entry);
    RUN_TEST(test_csv_get_field_zero_dest_len);
    RUN_TEST(test_csv_get_field_negative_row);
    RUN_TEST(test_csv_get_field_negative_entry);
    RUN_TEST(test_csv_get_field_multi_row);

    return UNITY_END();
}