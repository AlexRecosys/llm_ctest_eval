#include "unity.h"
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#include "csv_parser.h"  // assumed header containing CSV_FIELD, CSV_BUFFER, csv_get_field declarations

// File-scope static variables / fixtures
static CSV_BUFFER *csv_buf = NULL;
static CSV_FIELD ***field_array = NULL;
static size_t *width_array = NULL;
static char *field_text = NULL;

// Helper functions
static void setup_csv_buffer(size_t rows, size_t cols, const char *text, size_t text_len)
{
    size_t i, j;

    // Clean up previous allocation if any
    tearDown();

    // Allocate CSV_BUFFER
    csv_buf = (CSV_BUFFER *)malloc(sizeof(CSV_BUFFER));
    TEST_ASSERT_NOT_NULL_MESSAGE(csv_buf, "Failed to allocate CSV_BUFFER");

    // Allocate width array
    width_array = (size_t *)malloc(rows * sizeof(size_t));
    TEST_ASSERT_NOT_NULL_MESSAGE(width_array, "Failed to allocate width array");
    for (i = 0; i < rows; i++) {
        width_array[i] = cols;
    }
    csv_buf->width = width_array;

    // Allocate field array (3D: rows × cols × 1)
    field_array = (CSV_FIELD ***)malloc(rows * sizeof(CSV_FIELD **));
    TEST_ASSERT_NOT_NULL_MESSAGE(field_array, "Failed to allocate field_array rows");

    for (i = 0; i < rows; i++) {
        field_array[i] = (CSV_FIELD **)malloc(cols * sizeof(CSV_FIELD *));
        TEST_ASSERT_NOT_NULL_MESSAGE(field_array[i], "Failed to allocate field_array[row]");

        for (j = 0; j < cols; j++) {
            field_array[i][j] = (CSV_FIELD *)malloc(sizeof(CSV_FIELD));
            TEST_ASSERT_NOT_NULL_MESSAGE(field_array[i][j], "Failed to allocate CSV_FIELD");

            // Allocate and copy text
            field_array[i][j]->text = (char *)malloc(text_len + 1);
            TEST_ASSERT_NOT_NULL_MESSAGE(field_array[i][j]->text, "Failed to allocate field text");
            memcpy(field_array[i][j]->text, text, text_len);
            field_array[i][j]->text[text_len] = '\0';
            field_array[i][j]->length = text_len;
        }
    }

    csv_buf->field = field_array;
    csv_buf->rows = rows;
    csv_buf->field_delim = ',';
    csv_buf->text_delim = '"';
}

static void cleanup_csv_buffer(void)
{
    if (csv_buf != NULL && csv_buf->field != NULL) {
        size_t i, j;
        for (i = 0; i < csv_buf->rows; i++) {
            if (csv_buf->field[i] != NULL) {
                for (j = 0; j < csv_buf->width[i]; j++) {
                    if (csv_buf->field[i][j] != NULL) {
                        if (csv_buf->field[i][j]->text != NULL) {
                            free(csv_buf->field[i][j]->text);
                        }
                        free(csv_buf->field[i][j]);
                    }
                }
                free(csv_buf->field[i]);
            }
        }
        free(csv_buf->field);
    }
    if (csv_buf != NULL) {
        if (csv_buf->width != NULL) {
            free(csv_buf->width);
        }
        free(csv_buf);
    }
    csv_buf = NULL;
    field_array = NULL;
    width_array = NULL;
}

void setUp(void)
{
    // Nothing else needed; setUp is called before each test
}

void tearDown(void)
{
    cleanup_csv_buffer();
}

// Test 1: Valid row and entry, exact buffer size
void test_csv_get_field_exact_buffer(void)
{
    const char *test_text = "hello";
    size_t text_len = strlen(test_text);
    char dest[6];  // exactly 5 chars + null terminator

    setup_csv_buffer(2, 3, test_text, text_len);

    int result = csv_get_field(dest, sizeof(dest), csv_buf, 0, 1);

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "Expected return 0 for exact fit");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(test_text, dest, "Expected exact text match");
}

// Test 2: Valid row and entry, destination buffer too small (truncation)
void test_csv_get_field_truncation(void)
{
    const char *test_text = "hello";
    size_t text_len = strlen(test_text);
    char dest[4];  // only 3 chars + null terminator

    setup_csv_buffer(1, 2, test_text, text_len);

    int result = csv_get_field(dest, sizeof(dest), csv_buf, 0, 0);

    TEST_ASSERT_EQUAL_INT_MESSAGE(1, result, "Expected return 1 for truncation");
    TEST_ASSERT_EQUAL_STRING_LEN_MESSAGE("hel", dest, 3, "Expected truncated string");
    TEST_ASSERT_EQUAL_INT_MESSAGE('\0', dest[3], "Expected null terminator at end");
}

// Test 3: Invalid row (out of bounds)
void test_csv_get_field_invalid_row(void)
{
    const char *test_text = "data";
    char dest[16];

    setup_csv_buffer(2, 3, test_text, strlen(test_text));

    int result = csv_get_field(dest, sizeof(dest), csv_buf, 5, 1);

    TEST_ASSERT_EQUAL_INT_MESSAGE(2, result, "Expected return 2 for invalid row");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("", dest, "Expected empty string for invalid access");
}

// Test 4: Invalid entry (column out of bounds)
void test_csv_get_field_invalid_entry(void)
{
    const char *test_text = "data";
    char dest[16];

    setup_csv_buffer(2, 3, test_text, strlen(test_text));

    int result = csv_get_field(dest, sizeof(dest), csv_buf, 1, 5);

    TEST_ASSERT_EQUAL_INT_MESSAGE(2, result, "Expected return 2 for invalid entry");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("", dest, "Expected empty string for invalid entry");
}

// Test 5: Empty field (length == 0)
void test_csv_get_field_empty_field(void)
{
    const char *test_text = "";
    char dest[16];

    setup_csv_buffer(1, 1, test_text, 0);

    int result = csv_get_field(dest, sizeof(dest), csv_buf, 0, 0);

    TEST_ASSERT_EQUAL_INT_MESSAGE(2, result, "Expected return 2 for empty field");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("", dest, "Expected empty string for empty field");
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_csv_get_field_exact_buffer);
    RUN_TEST(test_csv_get_field_truncation);
    RUN_TEST(test_csv_get_field_invalid_row);
    RUN_TEST(test_csv_get_field_invalid_entry);
    RUN_TEST(test_csv_get_field_empty_field);

    return UNITY_END();
}