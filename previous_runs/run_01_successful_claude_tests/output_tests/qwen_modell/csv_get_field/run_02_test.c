#include "unity.h"
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#include "csv_parser.h"  // assumed header containing CSV_FIELD, CSV_BUFFER definitions

// File-scope static variables / fixtures
static CSV_BUFFER *csv_buf = NULL;
static CSV_FIELD ***field_2d = NULL;
static size_t *width_arr = NULL;
static CSV_FIELD **row0_fields = NULL;
static CSV_FIELD **row1_fields = NULL;
static CSV_FIELD *field00 = NULL;
static CSV_FIELD *field01 = NULL;
static CSV_FIELD *field02 = NULL;
static CSV_FIELD *field10 = NULL;
static CSV_FIELD *field11 = NULL;

// Helper functions
static void init_csv_buffer(size_t rows, size_t *widths);
static void cleanup_csv_buffer(void);
static CSV_FIELD *create_field(const char *text);
static void destroy_field(CSV_FIELD *f);

static CSV_FIELD *create_field(const char *text) {
    CSV_FIELD *f = malloc(sizeof(CSV_FIELD));
    TEST_ASSERT_NOT_NULL_MESSAGE(f, "Failed to allocate CSV_FIELD");
    f->length = strlen(text);
    f->text = malloc(f->length + 1);
    TEST_ASSERT_NOT_NULL_MESSAGE(f->text, "Failed to allocate field text");
    strcpy(f->text, text);
    return f;
}

static void destroy_field(CSV_FIELD *f) {
    if (f != NULL) {
        free(f->text);
        free(f);
    }
}

static void init_csv_buffer(size_t rows, size_t *widths) {
    size_t i, j;

    csv_buf = malloc(sizeof(CSV_BUFFER));
    TEST_ASSERT_NOT_NULL_MESSAGE(csv_buf, "Failed to allocate CSV_BUFFER");

    csv_buf->field = malloc(rows * sizeof(CSV_FIELD **));
    TEST_ASSERT_NOT_NULL_MESSAGE(csv_buf->field, "Failed to allocate field array");

    csv_buf->rows = rows;
    csv_buf->width = malloc(rows * sizeof(size_t));
    TEST_ASSERT_NOT_NULL_MESSAGE(csv_buf->width, "Failed to allocate width array");

    for (i = 0; i < rows; i++) {
        csv_buf->width[i] = widths[i];
        csv_buf->field[i] = malloc(widths[i] * sizeof(CSV_FIELD *));
        TEST_ASSERT_NOT_NULL_MESSAGE(csv_buf->field[i], "Failed to allocate row %zu", i);

        for (j = 0; j < widths[i]; j++) {
            csv_buf->field[i][j] = NULL;  // placeholder
        }
    }

    csv_buf->field_delim = ',';
    csv_buf->text_delim = '"';
}

static void cleanup_csv_buffer(void) {
    if (csv_buf == NULL) return;

    if (csv_buf->field != NULL) {
        for (size_t i = 0; i < csv_buf->rows; i++) {
            if (csv_buf->field[i] != NULL) {
                for (size_t j = 0; j < csv_buf->width[i]; j++) {
                    destroy_field(csv_buf->field[i][j]);
                }
                free(csv_buf->field[i]);
            }
        }
        free(csv_buf->field);
    }
    free(csv_buf->width);
    free(csv_buf);
    csv_buf = NULL;
}

void setUp(void) {
    // Initialize test data: 2 rows, row0 has 3 fields, row1 has 2 fields
    size_t widths[2] = {3, 2};

    init_csv_buffer(2, widths);

    // Allocate and populate row0 fields
    row0_fields = csv_buf->field[0];
    field00 = create_field("name");
    field01 = create_field("John Doe");
    field02 = create_field("123 Main St");
    row0_fields[0] = field00;
    row0_fields[1] = field01;
    row0_fields[2] = field02;

    // Allocate and populate row1 fields
    row1_fields = csv_buf->field[1];
    field10 = create_field("age");
    field11 = create_field("30");
    row1_fields[0] = field10;
    row1_fields[1] = field11;
}

void tearDown(void) {
    cleanup_csv_buffer();
}

void test_csv_get_field_success_full_copy(void) {
    char dest[64];
    int ret;

    ret = csv_get_field(dest, sizeof(dest), csv_buf, 0, 1);

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "Expected return 0 for full copy");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("John Doe", dest, "Expected exact field content");
}

void test_csv_get_field_truncation(void) {
    char dest[5];  // too small for "John Doe" (length 8)
    int ret;

    ret = csv_get_field(dest, sizeof(dest), csv_buf, 0, 1);

    TEST_ASSERT_EQUAL_INT_MESSAGE(1, ret, "Expected return 1 for truncation");
    TEST_ASSERT_EQUAL_STRING_LEN_MESSAGE("John", dest, 4, "Expected truncated content");
    TEST_ASSERT_EQUAL_INT_MESSAGE('\0', dest[4], "Expected null terminator at end of buffer");
}

void test_csv_get_field_empty_field(void) {
    CSV_FIELD *empty_field = create_field("");
    csv_buf->field[1][1] = empty_field;  // replace field11 with empty

    char dest[64];
    int ret;

    ret = csv_get_field(dest, sizeof(dest), csv_buf, 1, 1);

    TEST_ASSERT_EQUAL_INT_MESSAGE(2, ret, "Expected return 2 for empty field");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("", dest, "Expected empty string");
}

void test_csv_get_field_invalid_row(void) {
    char dest[64];
    int ret;

    ret = csv_get_field(dest, sizeof(dest), csv_buf, 5, 0);  // row 5 does not exist

    TEST_ASSERT_EQUAL_INT_MESSAGE(2, ret, "Expected return 2 for invalid row");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("", dest, "Expected cleared buffer");
}

void test_csv_get_field_invalid_entry(void) {
    char dest[64];
    int ret;

    ret = csv_get_field(dest, sizeof(dest), csv_buf, 0, 5);  // entry 5 does not exist in row 0

    TEST_ASSERT_EQUAL_INT_MESSAGE(2, ret, "Expected return 2 for invalid entry");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("", dest, "Expected cleared buffer");
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_csv_get_field_success_full_copy);
    RUN_TEST(test_csv_get_field_truncation);
    RUN_TEST(test_csv_get_field_empty_field);
    RUN_TEST(test_csv_get_field_invalid_row);
    RUN_TEST(test_csv_get_field_invalid_entry);

    return UNITY_END();
}