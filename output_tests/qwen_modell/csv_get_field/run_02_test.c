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

static void setup_two_rows(void)
{
    const char *row0_fields[] = {"a", "b"};
    const char *row1_fields[] = {"c", "d", "e"};
    int ret;

    ret = csv_set_field(buffer, 0, 0, (char *)row0_fields[0]);
    TEST_ASSERT_EQUAL_INT(0, ret);
    ret = csv_set_field(buffer, 0, 1, (char *)row0_fields[1]);
    TEST_ASSERT_EQUAL_INT(0, ret);

    ret = csv_set_field(buffer, 1, 0, (char *)row1_fields[0]);
    TEST_ASSERT_EQUAL_INT(0, ret);
    ret = csv_set_field(buffer, 1, 1, (char *)row1_fields[1]);
    TEST_ASSERT_EQUAL_INT(0, ret);
    ret = csv_set_field(buffer, 1, 2, (char *)row1_fields[2]);
    TEST_ASSERT_EQUAL_INT(0, ret);
}

void test_csv_get_field_success_full_copy(void)
{
    const char *field_text = "hello";
    char dest[32] = {0};
    int ret;

    setup_test_row_with_field(field_text);

    ret = csv_get_field(dest, sizeof(dest), buffer, 0, 0);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING_MESSAGE(field_text, dest, "Field content mismatch");
}

void test_csv_get_field_truncation_returns_1(void)
{
    const char *field_text = "hello world";
    char dest[6] = {0};  // only 5 chars + null
    int ret;

    setup_test_row_with_field(field_text);

    ret = csv_get_field(dest, sizeof(dest), buffer, 0, 0);

    TEST_ASSERT_EQUAL_INT(1, ret);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("hello", dest, "Truncated content mismatch");
}

void test_csv_get_field_empty_or_missing_returns_2(void)
{
    char dest[32] = {0};
    int ret;

    // First test: empty field (row exists, entry exists but empty)
    ret = csv_set_field(buffer, 0, 0, "");
    TEST_ASSERT_EQUAL_INT(0, ret);
    ret = csv_get_field(dest, sizeof(dest), buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT(2, ret);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("", dest, "Empty field should be null-terminated empty string");

    // Second test: non-existent row
    ret = csv_get_field(dest, sizeof(dest), buffer, 99, 0);
    TEST_ASSERT_EQUAL_INT(2, ret);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("", dest, "Non-existent row should clear dest");

    // Third test: non-existent entry in existing row
    ret = csv_get_field(dest, sizeof(dest), buffer, 0, 99);
    TEST_ASSERT_EQUAL_INT(2, ret);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("", dest, "Non-existent entry should clear dest");
}

void test_csv_get_field_zero_dest_len_returns_3(void)
{
    char dest[32] = {0};
    int ret;

    setup_test_row_with_field("test");

    ret = csv_get_field(dest, 0, buffer, 0, 0);

    TEST_ASSERT_EQUAL_INT(3, ret);
    // Per spec: dest should be cleared (but dest_len=0 means no write possible)
    // Unity doesn't guarantee dest[0] is set when dest_len==0, so skip content check
}

void test_csv_get_field_multi_row_entry(void)
{
    const char *row0_fields[] = {"alpha", "beta"};
    const char *row1_fields[] = {"gamma", "delta", "epsilon"};
    char dest[32] = {0};
    int ret;

    setup_multi_field_row(0, 2, row0_fields);
    setup_multi_field_row(1, 3, row1_fields);

    // Test row 0, entry 1
    ret = csv_get_field(dest, sizeof(dest), buffer, 0, 1);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("beta", dest, "Row 0, entry 1 mismatch");

    // Test row 1, entry 2
    ret = csv_get_field(dest, sizeof(dest), buffer, 1, 2);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("epsilon", dest, "Row 1, entry 2 mismatch");

    // Test row 1, entry 0
    ret = csv_get_field(dest, sizeof(dest), buffer, 1, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("gamma", dest, "Row 1, entry 0 mismatch");
}