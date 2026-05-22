#include "unity.h"
#include "csv.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// File-scope static variables / fixtures
static CSV_BUFFER *buffer = NULL;

// Helper functions and macros
static void init_buffer_with_rows_and_widths(CSV_BUFFER *buf, size_t num_rows, size_t *widths) {
    buf->rows = num_rows;
    buf->width = (size_t *)malloc(num_rows * sizeof(size_t));
    TEST_ASSERT_NOT_NULL_MESSAGE(buf->width, "Failed to allocate width array");

    buf->field = (CSV_FIELD ***)malloc(num_rows * sizeof(CSV_FIELD **));
    TEST_ASSERT_NOT_NULL_MESSAGE(buf->field, "Failed to allocate field array");

    for (size_t i = 0; i < num_rows; i++) {
        buf->width[i] = widths[i];
        buf->field[i] = (CSV_FIELD **)calloc(widths[i], sizeof(CSV_FIELD *));
        TEST_ASSERT_NOT_NULL_MESSAGE(buf->field[i], "Failed to allocate row fields");

        for (size_t j = 0; j < widths[i]; j++) {
            buf->field[i][j] = create_field();
            TEST_ASSERT_NOT_NULL_MESSAGE(buf->field[i][j], "Failed to create field");
            char text[32];
            snprintf(text, sizeof(text), "R%zuE%zu", i, j);
            set_field(buf->field[i][j], text);
        }
    }
}

static void destroy_buffer_contents(CSV_BUFFER *buf) {
    if (!buf) return;
    for (size_t i = 0; i < buf->rows; i++) {
        if (buf->field && buf->field[i]) {
            for (size_t j = 0; j < buf->width[i]; j++) {
                if (buf->field[i][j]) {
                    destroy_field(buf->field[i][j]);
                }
            }
            free(buf->field[i]);
        }
    }
    if (buf->field) free(buf->field);
    if (buf->width) free(buf->width);
    memset(buf, 0, sizeof(CSV_BUFFER));
}

void setUp(void) {
    buffer = csv_create_buffer();
    TEST_ASSERT_NOT_NULL_MESSAGE(buffer, "Failed to create buffer");
    buffer->field_delim = ',';
    buffer->text_delim = '"';
}

void tearDown(void) {
    csv_destroy_buffer(buffer);
    buffer = NULL;
}

// Test cases
void test_csv_insert_field_inserts_at_end_when_row_or_entry_out_of_bounds(void) {
    // Setup: create buffer with 1 row, width 2
    size_t widths[] = {2};
    init_buffer_with_rows_and_widths(buffer, 1, widths);

    // Insert at row=0, entry=2 (beyond current width)
    int result = csv_insert_field(buffer, 0, 2, "NEW");

    // Should have appended field (via csv_set_field)
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_UINT(3, buffer->width[0]);
    char field_text[64];
    int get_result = csv_get_field(field_text, sizeof(field_text), buffer, 0, 2);
    TEST_ASSERT_EQUAL_INT(0, get_result);
    TEST_ASSERT_EQUAL_STRING("NEW", field_text);
}

void test_csv_insert_field_inserts_at_existing_position_by_shifting(void) {
    // Setup: create buffer with 1 row, width 3
    size_t widths[] = {3};
    init_buffer_with_rows_and_widths(buffer, 1, widths);

    // Insert at row=0, entry=1 (middle)
    int result = csv_insert_field(buffer, 0, 1, "INSERTED");

    // Should shift existing fields right and insert
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_UINT(4, buffer->width[0]);

    char field_text[64];
    csv_get_field(field_text, sizeof(field_text), buffer, 0, 0);
    TEST_ASSERT_EQUAL_STRING("R0E0", field_text);

    csv_get_field(field_text, sizeof(field_text), buffer, 0, 1);
    TEST_ASSERT_EQUAL_STRING("INSERTED", field_text);

    csv_get_field(field_text, sizeof(field_text), buffer, 0, 2);
    TEST_ASSERT_EQUAL_STRING("R0E1", field_text);

    csv_get_field(field_text, sizeof(field_text), buffer, 0, 3);
    TEST_ASSERT_EQUAL_STRING("R0E2", field_text);
}

void test_csv_insert_field_inserts_at_beginning_of_row(void) {
    // Setup: create buffer with 1 row, width 2
    size_t widths[] = {2};
    init_buffer_with_rows_and_widths(buffer, 1, widths);

    // Insert at row=0, entry=0 (beginning)
    int result = csv_insert_field(buffer, 0, 0, "FIRST");

    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_UINT(3, buffer->width[0]);

    char field_text[64];
    csv_get_field(field_text, sizeof(field_text), buffer, 0, 0);
    TEST_ASSERT_EQUAL_STRING("FIRST", field_text);

    csv_get_field(field_text, sizeof(field_text), buffer, 0, 1);
    TEST_ASSERT_EQUAL_STRING("R0E0", field_text);

    csv_get_field(field_text, sizeof(field_text), buffer, 0, 2);
    TEST_ASSERT_EQUAL_STRING("R0E1", field_text);
}

void test_csv_insert_field_inserts_into_new_row_appends_row(void) {
    // Setup: create buffer with 1 row
    size_t widths[] = {1};
    init_buffer_with_rows_and_widths(buffer, 1, widths);

    // Insert at row=1 (nonexistent row), entry=0
    int result = csv_insert_field(buffer, 1, 0, "NEW_ROW_FIELD");

    // Should create new row and insert field
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_UINT(2, buffer->rows);
    TEST_ASSERT_EQUAL_UINT(1, buffer->width[1]);

    char field_text[64];
    csv_get_field(field_text, sizeof(field_text), buffer, 1, 0);
    TEST_ASSERT_EQUAL_STRING("NEW_ROW_FIELD", field_text);
}

void test_csv_insert_field_inserts_into_empty_row_creates_first_field(void) {
    // Setup: create buffer with 1 empty row
    buffer->rows = 1;
    buffer->width = (size_t *)malloc(sizeof(size_t));
    buffer->width[0] = 0;
    buffer->field = (CSV_FIELD ***)malloc(sizeof(CSV_FIELD **));
    buffer->field[0] = NULL;

    // Insert at row=0, entry=0 (first field in empty row)
    int result = csv_insert_field(buffer, 0, 0, "FIRST");

    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_UINT(1, buffer->width[0]);

    char field_text[64];
    csv_get_field(field_text, sizeof(field_text), buffer, 0, 0);
    TEST_ASSERT_EQUAL_STRING("FIRST", field_text);
}