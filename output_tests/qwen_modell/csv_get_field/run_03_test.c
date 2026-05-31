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
    ret = csv_set_field(buffer, 0, 0, field_text);
    TEST_ASSERT_EQUAL_INT(0, ret);
}

static void setup_test_row_with_multiple_fields(const char *f0, const char *f1, const char *f2)
{
    int ret;
    ret = csv_set_field(buffer, 0, 0, f0);
    TEST_ASSERT_EQUAL_INT(0, ret);
    ret = csv_set_field(buffer, 0, 1, f1);
    TEST_ASSERT_EQUAL_INT(0, ret);
    ret = csv_set_field(buffer, 0, 2, f2);
    TEST_ASSERT_EQUAL_INT(0, ret);
}

static void setup_two_rows(void)
{
    int ret;
    ret = csv_set_field(buffer, 0, 0, "row0col0");
    TEST_ASSERT_EQUAL_INT(0, ret);
    ret = csv_set_field(buffer, 1, 0, "row1col0");
    TEST_ASSERT_EQUAL_INT(0, ret);
}

static void fill_dest_with_pattern(char *dest, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        dest[i] = 'X';
    }
}

static int compare_dest_with_expected(const char *dest, size_t dest_len, const char *expected)
{
    size_t copy_len = (dest_len > 0) ? dest_len - 1 : 0;
    if (copy_len == 0) {
        return (dest[0] == '\0') ? 0 : -1;
    }
    if (strncmp(dest, expected, copy_len) != 0) {
        return -1;
    }
    if (dest[copy_len] != '\0') {
        return -2;
    }
    return 0;
}

void test_csv_get_field_success_full_copy(void)
{
    char dest[100];
    fill_dest_with_pattern(dest, sizeof(dest));
    setup_test_row_with_field("hello");

    int ret = csv_get_field(dest, sizeof(dest), buffer, 0, 0);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("hello", dest, "Field content mismatch");
}

void test_csv_get_field_truncation_returns_1(void)
{
    char dest[6];
    fill_dest_with_pattern(dest, sizeof(dest));
    setup_test_row_with_field("hello");

    int ret = csv_get_field(dest, sizeof(dest), buffer, 0, 0);

    TEST_ASSERT_EQUAL_INT(1, ret);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("hello", dest, "Truncated content mismatch");
}

void test_csv_get_field_empty_field_returns_2(void)
{
    char dest[100];
    fill_dest_with_pattern(dest, sizeof(dest));
    setup_test_row_with_field("");

    int ret = csv_get_field(dest, sizeof(dest), buffer, 0, 0);

    TEST_ASSERT_EQUAL_INT(2, ret);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("", dest, "Empty field should produce empty string");
}

void test_csv_get_field_invalid_row_or_entry_returns_2(void)
{
    char dest[100];
    fill_dest_with_pattern(dest, sizeof(dest));
    setup_test_row_with_field("hello");

    int ret;

    ret = csv_get_field(dest, sizeof(dest), buffer, 99, 0);
    TEST_ASSERT_EQUAL_INT(2, ret);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("", dest, "Invalid row should clear dest");

    ret = csv_get_field(dest, sizeof(dest), buffer, 0, 99);
    TEST_ASSERT_EQUAL_INT(2, ret);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("", dest, "Invalid entry should clear dest");
}

void test_csv_get_field_zero_dest_len_returns_3(void)
{
    char dest[100];
    fill_dest_with_pattern(dest, sizeof(dest));
    setup_test_row_with_field("hello");

    int ret = csv_get_field(dest, 0, buffer, 0, 0);

    TEST_ASSERT_EQUAL_INT(3, ret);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("", dest, "Zero dest_len should clear dest");
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_csv_get_field_success_full_copy);
    RUN_TEST(test_csv_get_field_truncation_returns_1);
    RUN_TEST(test_csv_get_field_empty_field_returns_2);
    RUN_TEST(test_csv_get_field_invalid_row_or_entry_returns_2);
    RUN_TEST(test_csv_get_field_zero_dest_len_returns_3);
    return UNITY_END();
}