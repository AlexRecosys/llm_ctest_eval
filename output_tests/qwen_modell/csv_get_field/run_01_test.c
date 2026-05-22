#include "unity.h"
#include "csv.h"

static CSV_BUFFER *test_buffer = NULL;

void setUp(void)
{
    test_buffer = csv_create_buffer();
    TEST_ASSERT_NOT_NULL(test_buffer);
}

void tearDown(void)
{
    if (test_buffer != NULL) {
        csv_destroy_buffer(test_buffer);
        test_buffer = NULL;
    }
}

static void setup_test_csv(CSV_BUFFER *buf, size_t rows, size_t cols, const char * const * const data)
{
    for (size_t r = 0; r < rows; r++) {
        append_row(buf);
        for (size_t c = 0; c < cols; c++) {
            if (data && data[r * cols + c]) {
                append_field(buf, r);
                set_field(&buf->field[r][c][0], (char *)data[r * cols + c]);
            }
        }
    }
}

static void setup_single_field(CSV_BUFFER *buf, const char *text)
{
    append_row(buf);
    append_field(buf, 0);
    set_field(&buf->field[0][0][0], (char *)text);
}

static void setup_multi_field_row(CSV_BUFFER *buf, const char * const *fields, size_t count)
{
    append_row(buf);
    for (size_t i = 0; i < count; i++) {
        append_field(buf, 0);
        set_field(&buf->field[0][i][0], (char *)fields[i]);
    }
}

void test_csv_get_field_success_full_copy(void)
{
    const char *text = "Hello, World!";
    setup_single_field(test_buffer, text);

    char dest[64] = {0};
    int result = csv_get_field(dest, sizeof(dest), test_buffer, 0, 0);

    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_STRING(text, dest);
}

void test_csv_get_field_truncation_returns_1(void)
{
    const char *text = "This is a long string";
    setup_single_field(test_buffer, text);

    char dest[10] = {0};
    int result = csv_get_field(dest, sizeof(dest), test_buffer, 0, 0);

    TEST_ASSERT_EQUAL_INT(1, result);
    TEST_ASSERT_EQUAL_STRING_LEN("This is a ", dest, 10);
    TEST_ASSERT_EQUAL_INT('\0', dest[9]);
}

void test_csv_get_field_empty_field_returns_2(void)
{
    setup_single_field(test_buffer, "");

    char dest[64] = {0};
    int result = csv_get_field(dest, sizeof(dest), test_buffer, 0, 0);

    TEST_ASSERT_EQUAL_INT(2, result);
    TEST_ASSERT_EQUAL_STRING("", dest);
}

void test_csv_get_field_invalid_row_or_entry_returns_2(void)
{
    setup_single_field(test_buffer, "data");

    char dest[64] = {0};
    int result;

    // Test invalid row (row >= rows)
    result = csv_get_field(dest, sizeof(dest), test_buffer, 1, 0);
    TEST_ASSERT_EQUAL_INT(2, result);
    TEST_ASSERT_EQUAL_STRING("", dest);

    // Test invalid entry (entry >= width[row])
    result = csv_get_field(dest, sizeof(dest), test_buffer, 0, 1);
    TEST_ASSERT_EQUAL_INT(2, result);
    TEST_ASSERT_EQUAL_STRING("", dest);
}

void test_csv_get_field_zero_dest_len_returns_3(void)
{
    setup_single_field(test_buffer, "data");

    char dest[64] = {0};
    int result = csv_get_field(dest, 0, test_buffer, 0, 0);

    TEST_ASSERT_EQUAL_INT(3, result);
    // dest should remain unchanged (not written to)
    TEST_ASSERT_EQUAL_STRING("", dest);
}