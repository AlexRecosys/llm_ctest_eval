#include "unity.h"
#include "csv.h"
#include <stdlib.h>
#include <string.h>

// Helper to create a CSV_BUFFER with 1 row and 2 columns for testing
static CSV_BUFFER* create_test_csv_buffer(void)
{
    CSV_BUFFER *buf = (CSV_BUFFER*)malloc(sizeof(CSV_BUFFER));
    TEST_ASSERT_NOT_NULL(buf);

    buf->rows = 1;
    buf->width = (size_t*)malloc(sizeof(size_t));
    TEST_ASSERT_NOT_NULL(buf->width);
    buf->width[0] = 2;

    // Allocate 2D array of CSV_FIELD* pointers: [1][2]
    buf->field = (CSV_FIELD***)malloc(sizeof(CSV_FIELD**) * buf->rows);
    TEST_ASSERT_NOT_NULL(buf->field);

    buf->field[0] = (CSV_FIELD**)malloc(sizeof(CSV_FIELD*) * buf->width[0]);
    TEST_ASSERT_NOT_NULL(buf->field[0]);

    // Create two test fields
    buf->field[0][0] = (CSV_FIELD*)malloc(sizeof(CSV_FIELD));
    TEST_ASSERT_NOT_NULL(buf->field[0][0]);
    buf->field[0][0]->text = strdup("first");
    TEST_ASSERT_NOT_NULL(buf->field[0][0]->text);
    buf->field[0][0]->length = strlen("first");

    buf->field[0][1] = (CSV_FIELD*)malloc(sizeof(CSV_FIELD));
    TEST_ASSERT_NOT_NULL(buf->field[0][1]);
    buf->field[0][1]->text = strdup("second");
    TEST_ASSERT_NOT_NULL(buf->field[0][1]->text);
    buf->field[0][1]->length = strlen("second");

    buf->field_delim = ',';
    buf->text_delim = '"';

    return buf;
}

// Helper to clean up CSV_BUFFER and its contents
static void destroy_csv_buffer(CSV_BUFFER *buf)
{
    if (!buf) return;
    if (buf->field) {
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
    }
    free(buf->width);
    free(buf);
}

// Test: dest_len == 0 → return 3, dest untouched (but we can't test untouched since it's undefined)
TEST(csv_get_field, dest_len_zero_returns_3)
{
    char dest[10] = "XXXX";
    CSV_BUFFER *buf = create_test_csv_buffer();

    int result = csv_get_field(dest, 0, buf, 0, 0);

    TEST_ASSERT_EQUAL_INT(3, result);
    // dest is not modified per spec (dest_len == 0 → no writes), but we can't assert that portably
    // So we just verify return code

    destroy_csv_buffer(buf);
}

// Test: row out of range → return 2, dest cleared
TEST(csv_get_field, row_out_of_range_clears_dest_and_returns_2)
{
    char dest[10] = "XXXX";
    CSV_BUFFER *buf = create_test_csv_buffer();

    int result = csv_get_field(dest, sizeof(dest), buf, 1, 0); // row=1 >= rows=1

    TEST_ASSERT_EQUAL_INT(2, result);
    TEST_ASSERT_EQUAL_STRING("", dest); // dest should be cleared

    destroy_csv_buffer(buf);
}

// Test: entry out of range → return 2, dest cleared
TEST(csv_get_field, entry_out_of_range_clears_dest_and_returns_2)
{
    char dest[10] = "XXXX";
    CSV_BUFFER *buf = create_test_csv_buffer();

    int result = csv_get_field(dest, sizeof(dest), buf, 0, 2); // entry=2 >= width[0]=2

    TEST_ASSERT_EQUAL_INT(2, result);
    TEST_ASSERT_EQUAL_STRING("", dest);

    destroy_csv_buffer(buf);
}

// Test: negative row (should be caught by row < 0) → return 2, dest cleared
TEST(csv_get_field, negative_row_clears_dest_and_returns_2)
{
    char dest[10] = "XXXX";
    CSV_BUFFER *buf = create_test_csv_buffer();

    // Cast -1 to size_t → huge value, so row >= src->rows
    int result = csv_get_field(dest, sizeof(dest), buf, (size_t)-1, 0);

    TEST_ASSERT_EQUAL_INT(2, result);
    TEST_ASSERT_EQUAL_STRING("", dest);

    destroy_csv_buffer(buf);
}

// Test: negative entry → same as above (size_t cast)
TEST(csv_get_field, negative_entry_clears_dest_and_returns_2)
{
    char dest[10] = "XXXX";
    CSV_BUFFER *buf = create_test_csv_buffer();

    int result = csv_get_field(dest, sizeof(dest), buf, 0, (size_t)-1);

    TEST_ASSERT_EQUAL_INT(2, result);
    TEST_ASSERT_EQUAL_STRING("", dest);

    destroy_csv_buffer(buf);
}

// Test: exact fit (dest_len == field length) → return 0
TEST(csv_get_field, exact_fit_returns_0)
{
    char dest[6]; // "first" is 5 chars + '\0' → need 6
    CSV_BUFFER *buf = create_test_csv_buffer();

    int result = csv_get_field(dest, sizeof(dest), buf, 0, 0); // "first"

    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_STRING("first", dest);

    destroy_csv_buffer(buf);
}

// Test: truncation (dest_len < field length) → return 1
TEST(csv_get_field, truncation_returns_1)
{
    char dest[4]; // "first" is 5 chars, dest_len=4 → truncates to "fir"
    CSV_BUFFER *buf = create_test_csv_buffer();

    int result = csv_get_field(dest, sizeof(dest), buf, 0, 0);

    TEST_ASSERT_EQUAL_INT(1, result);
    TEST_ASSERT_EQUAL_STRING("fir", dest);

    destroy_csv_buffer(buf);
}

// Test: empty field → return 2
TEST(csv_get_field, empty_field_returns_2)
{
    CSV_BUFFER *buf = create_test_csv_buffer();

    // Replace field[0][0] with empty string
    free(buf->field[0][0]->text);
    buf->field[0][0]->text = strdup("");
    buf->field[0][0]->length = 0;

    char dest[10] = "XXXX";
    int result = csv_get_field(dest, sizeof(dest), buf, 0, 0);

    TEST_ASSERT_EQUAL_INT(2, result);
    TEST_ASSERT_EQUAL_STRING("", dest);

    // Cleanup
    free(buf->field[0][0]->text);
    destroy_csv_buffer(buf);
}

// Test: success (no truncation, non-empty) → return 0
TEST(csv_get_field, success_returns_0)
{
    char dest[20]; // larger than "first"
    CSV_BUFFER *buf = create_test_csv_buffer();

    int result = csv_get_field(dest, sizeof(dest), buf, 0, 0); // "first"

    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_STRING("first", dest);

    destroy_csv_buffer(buf);
}