#include "unity.h"
#include "csv.h"

static CSV_BUFFER *buffer = NULL;

void setUp(void)
{
    buffer = csv_create_buffer();
    TEST_ASSERT_NOT_NULL(buffer);
}

void tearDown(void)
{
    if (buffer != NULL) {
        csv_destroy_buffer(buffer);
        buffer = NULL;
    }
}

static void setup_test_row_with_field(const char *field_text)
{
    int ret;
    ret = csv_set_field(buffer, 0, 0, (char *)field_text);
    TEST_ASSERT_EQUAL_INT(0, ret);
}

static void setup_multi_field_row(size_t row, const char *field1, const char *field2)
{
    int ret;
    ret = csv_set_field(buffer, row, 0, (char *)field1);
    TEST_ASSERT_EQUAL_INT(0, ret);
    ret = csv_set_field(buffer, row, 1, (char *)field2);
    TEST_ASSERT_EQUAL_INT(0, ret);
}

static void setup_two_rows(void)
{
    int ret;
    ret = csv_set_field(buffer, 0, 0, (char *)"row0col0");
    TEST_ASSERT_EQUAL_INT(0, ret);
    ret = csv_set_field(buffer, 0, 1, (char *)"row0col1");
    TEST_ASSERT_EQUAL_INT(0, ret);
    ret = csv_set_field(buffer, 1, 0, (char *)"row1col0");
    TEST_ASSERT_EQUAL_INT(0, ret);
}

static void fill_dest_with_pattern(char *dest, size_t len)
{
    for (size_t i = 0; i < len; ++i) {
        dest[i] = 'X';
    }
}

static int compare_dest_with_expected(const char *dest, size_t dest_len, const char *expected)
{
    size_t expected_len = strlen(expected);
    size_t copy_len = (dest_len <= expected_len) ? dest_len : expected_len;
    if (copy_len > 0) {
        if (strncmp(dest, expected, copy_len) != 0) {
            return 1;
        }
    }
    if (dest_len > 0 && dest[dest_len - 1] != '\0') {
        return 2;
    }
    return 0;
}

void test_csv_get_field_success_full_copy(void)
{
    char dest[32];
    fill_dest_with_pattern(dest, sizeof(dest));
    setup_test_row_with_field("hello");

    int ret = csv_get_field(dest, sizeof(dest), buffer, 0, 0);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("hello", dest, "Full copy should match original text");
}

void test_csv_get_field_truncation_returns_1(void)
{
    char dest[6];
    fill_dest_with_pattern(dest, sizeof(dest));
    setup_test_row_with_field("hello");

    int ret = csv_get_field(dest, sizeof(dest), buffer, 0, 0);

    TEST_ASSERT_EQUAL_INT(1, ret);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("hello", dest, "Truncated string should contain full text up to dest_len-1");
    TEST_ASSERT_EQUAL_INT('\0', dest[5], "String must be null-terminated");
}

void test_csv_get_field_empty_field_returns_2(void)
{
    char dest[32];
    fill_dest_with_pattern(dest, sizeof(dest));
    setup_test_row_with_field("");

    int ret = csv_get_field(dest, sizeof(dest), buffer, 0, 0);

    TEST_ASSERT_EQUAL_INT(2, ret);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("", dest, "Empty field should result in empty string");
}

void test_csv_get_field_invalid_row_or_entry_returns_2(void)
{
    char dest[32];
    fill_dest_with_pattern(dest, sizeof(dest));
    setup_test_row_with_field("hello");

    int ret1 = csv_get_field(dest, sizeof(dest), buffer, 999, 0);
    TEST_ASSERT_EQUAL_INT(2, ret1);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("", dest, "Invalid row should clear dest");

    int ret2 = csv_get_field(dest, sizeof(dest), buffer, 0, 999);
    TEST_ASSERT_EQUAL_INT(2, ret2);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("", dest, "Invalid entry should clear dest");
}

void test_csv_get_field_zero_dest_len_returns_3(void)
{
    char dest[32];
    fill_dest_with_pattern(dest, sizeof(dest));

    int ret = csv_get_field(dest, 0, buffer, 0, 0);

    TEST_ASSERT_EQUAL_INT(3, ret);
    // dest_len == 0 means no write allowed; spec says return 3, no requirement on dest content
}