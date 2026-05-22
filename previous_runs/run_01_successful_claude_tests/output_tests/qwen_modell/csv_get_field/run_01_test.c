#include "unity.h"
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#include "csv_parser.h"  // assumed header containing CSV_FIELD, CSV_BUFFER, csv_get_field declarations

// File-scope static variables / fixtures
static CSV_BUFFER *csv_buf = NULL;
static CSV_FIELD ***field_array = NULL;
static size_t *width_array = NULL;
static char *text_buffer = NULL;
static size_t buffer_size = 0;

// Helper functions
static void allocate_csv_buffer(size_t rows, size_t max_cols);
static void fill_csv_field(size_t row, size_t col, const char *text);
static void free_csv_resources(void);

static void allocate_csv_buffer(size_t rows, size_t max_cols)
{
    size_t i, j;

    // Free existing resources first
    free_csv_resources();

    // Allocate CSV_BUFFER
    csv_buf = (CSV_BUFFER *)malloc(sizeof(CSV_BUFFER));
    TEST_ASSERT_NOT_NULL_MESSAGE(csv_buf, "Failed to allocate CSV_BUFFER");

    // Allocate width array
    width_array = (size_t *)calloc(rows, sizeof(size_t));
    TEST_ASSERT_NOT_NULL_MESSAGE(width_array, "Failed to allocate width_array");
    for (i = 0; i < rows; ++i) {
        width_array[i] = max_cols;
    }

    // Allocate field array: 2D array of CSV_FIELD* pointers
    field_array = (CSV_FIELD ***)malloc(rows * sizeof(CSV_FIELD **));
    TEST_ASSERT_NOT_NULL_MESSAGE(field_array, "Failed to allocate field_array");

    for (i = 0; i < rows; ++i) {
        field_array[i] = (CSV_FIELD **)calloc(max_cols, sizeof(CSV_FIELD *));
        TEST_ASSERT_NOT_NULL_MESSAGE(field_array[i], "Failed to allocate field_array[row]");
        for (j = 0; j < max_cols; ++j) {
            field_array[i][j] = NULL;
        }
    }

    csv_buf->field = field_array;
    csv_buf->rows = rows;
    csv_buf->width = width_array;
    csv_buf->field_delim = ',';
    csv_buf->text_delim = '"';
}

static void fill_csv_field(size_t row, size_t col, const char *text)
{
    size_t len = strlen(text);
    CSV_FIELD *field = (CSV_FIELD *)malloc(sizeof(CSV_FIELD));
    TEST_ASSERT_NOT_NULL_MESSAGE(field, "Failed to allocate CSV_FIELD");

    field->text = (char *)malloc(len + 1);
    TEST_ASSERT_NOT_NULL_MESSAGE(field->text, "Failed to allocate field->text");
    strcpy(field->text, text);
    field->length = len;

    csv_buf->field[row][col] = field;
}

static void free_csv_resources(void)
{
    if (csv_buf != NULL) {
        if (csv_buf->field != NULL) {
            for (size_t i = 0; i < csv_buf->rows; ++i) {
                if (csv_buf->field[i] != NULL) {
                    for (size_t j = 0; j < csv_buf->width[i]; ++j) {
                        if (csv_buf->field[i][j] != NULL) {
                            free(csv_buf->field[i][j]->text);
                            free(csv_buf->field[i][j]);
                        }
                    }
                    free(csv_buf->field[i]);
                }
            }
            free(csv_buf->field);
        }
        free(csv_buf->width);
        free(csv_buf);
        csv_buf = NULL;
        field_array = NULL;
        width_array = NULL;
    }
}

void setUp(void)
{
    csv_buf = NULL;
    field_array = NULL;
    width_array = NULL;
    text_buffer = NULL;
    buffer_size = 0;
}

void tearDown(void)
{
    free_csv_resources();
}

// Test cases

void test_csv_get_field_null_dest_with_zero_dest_len_returns_3(void)
{
    char *dest = NULL;
    size_t dest_len = 0;
    CSV_BUFFER src;

    int result = csv_get_field(dest, dest_len, &src, 0, 0);

    TEST_ASSERT_EQUAL_INT_MESSAGE(3, result, "Expected return code 3 when dest_len == 0");
}

void test_csv_get_field_invalid_row_returns_2_and_clears_dest(void)
{
    char dest[64] = "NOT_EMPTY";
    size_t dest_len = sizeof(dest);

    allocate_csv_buffer(2, 3);
    fill_csv_field(0, 0, "valid");
    fill_csv_field(1, 1, "also_valid");

    int result = csv_get_field(dest, dest_len, csv_buf, 5, 0);  // row 5 does not exist

    TEST_ASSERT_EQUAL_INT_MESSAGE(2, result, "Expected return code 2 for invalid row");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("", dest, "Destination should be cleared on error");
}

void test_csv_get_field_invalid_entry_returns_2_and_clears_dest(void)
{
    char dest[64] = "NOT_EMPTY";
    size_t dest_len = sizeof(dest);

    allocate_csv_buffer(2, 3);
    fill_csv_field(0, 0, "valid");
    fill_csv_field(0, 1, "also_valid");

    int result = csv_get_field(dest, dest_len, csv_buf, 0, 5);  // entry 5 does not exist in row 0

    TEST_ASSERT_EQUAL_INT_MESSAGE(2, result, "Expected return code 2 for invalid entry");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("", dest, "Destination should be cleared on error");
}

void test_csv_get_field_valid_entry_returns_0_and_copies_full_string(void)
{
    char dest[64];
    size_t dest_len = sizeof(dest);

    allocate_csv_buffer(1, 2);
    fill_csv_field(0, 0, "hello");

    int result = csv_get_field(dest, dest_len, csv_buf, 0, 0);

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "Expected return code 0 for full copy");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("hello", dest, "Destination should contain full field text");
}

void test_csv_get_field_truncates_and_returns_1_when_dest_too_small(void)
{
    char dest[6];
    size_t dest_len = sizeof(dest) - 1;  // 5 bytes available (plus null terminator)

    allocate_csv_buffer(1, 2);
    fill_csv_field(0, 0, "hello_world");  // length = 11

    int result = csv_get_field(dest, dest_len, csv_buf, 0, 0);

    TEST_ASSERT_EQUAL_INT_MESSAGE(1, result, "Expected return code 1 for truncation");
    TEST_ASSERT_EQUAL_STRING_LEN_MESSAGE("hello", dest, dest_len, "Destination should be truncated");
    TEST_ASSERT_EQUAL_INT_MESSAGE('\0', dest[dest_len], "Destination should be null-terminated");
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_csv_get_field_null_dest_with_zero_dest_len_returns_3);
    RUN_TEST(test_csv_get_field_invalid_row_returns_2_and_clears_dest);
    RUN_TEST(test_csv_get_field_invalid_entry_returns_2_and_clears_dest);
    RUN_TEST(test_csv_get_field_valid_entry_returns_0_and_copies_full_string);
    RUN_TEST(test_csv_get_field_truncates_and_returns_1_when_dest_too_small);

    return UNITY_END();
}