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

static void setup_multi_field_row(size_t row, size_t num_fields, const char **fields)
{
    size_t i;
    int ret;

    for (i = 0; i < num_fields; i++) {
        ret = csv_set_field(buffer, row, i, (char *)fields[i]);
        TEST_ASSERT_EQUAL_INT(0, ret);
    }
}

void test_csv_get_field_success_full_copy(void)
{
    const char *test_text = "Hello, World!";
    char dest[64];

    setup_test_row_with_field(test_text);

    int result = csv_get_field(dest, sizeof(dest), buffer, 0, 0);

    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_STRING(test_text, dest);
}

void test_csv_get_field_truncation_returns_1(void)
{
    const char *test_text = "This is a long string";
    char dest[10];

    setup_test_row_with_field(test_text);

    int result = csv_get_field(dest, sizeof(dest), buffer, 0, 0);

    TEST_ASSERT_EQUAL_INT(1, result);
    TEST_ASSERT_EQUAL_STRING_LEN("This is a ", dest, sizeof(dest) - 1);
    TEST_ASSERT_EQUAL_INT('\0', dest[sizeof(dest) - 1]);
}

void test_csv_get_field_empty_field_returns_2(void)
{
    const char *empty_text = "";
    char dest[64];

    setup_test_row_with_field(empty_text);

    int result = csv_get_field(dest, sizeof(dest), buffer, 0, 0);

    TEST_ASSERT_EQUAL_INT(2, result);
    TEST_ASSERT_EQUAL_STRING("", dest);
}

void test_csv_get_field_invalid_row_or_entry_returns_2(void)
{
    const char *test_text = "Some data";
    char dest[64];

    setup_test_row_with_field(test_text);

    int result;

    result = csv_get_field(dest, sizeof(dest), buffer, 99, 0);
    TEST_ASSERT_EQUAL_INT(2, result);
    TEST_ASSERT_EQUAL_STRING("", dest);

    result = csv_get_field(dest, sizeof(dest), buffer, 0, 99);
    TEST_ASSERT_EQUAL_INT(2, result);
    TEST_ASSERT_EQUAL_STRING("", dest);
}

void test_csv_get_field_zero_dest_len_returns_3(void)
{
    char dest[64];

    setup_test_row_with_field("Some data");

    int result = csv_get_field(dest, 0, buffer, 0, 0);

    TEST_ASSERT_EQUAL_INT(3, result);
}