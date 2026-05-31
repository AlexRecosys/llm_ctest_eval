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
    const char *row0_fields[] = {"A1", "A2"};
    const char *row1_fields[] = {"B1", "B2", "B3"};
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
    const char *test_field = "Hello";
    char dest[64] = {0};

    setup_test_row_with_field(test_field);

    int ret = csv_get_field(dest, sizeof(dest), buffer, 0, 0);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING_MESSAGE(test_field, dest, "Field content mismatch");
}

void test_csv_get_field_truncation_returns_1(void)
{
    const char *test_field = "HelloWorld";
    char dest[6] = {0};  // only 5 chars + null

    setup_test_row_with_field(test_field);

    int ret = csv_get_field(dest, sizeof(dest), buffer, 0, 0);

    TEST_ASSERT_EQUAL_INT(1, ret);
    TEST_ASSERT_EQUAL_STRING_LEN_MESSAGE("Hello", dest, 5, "Truncated content mismatch");
}

void test_csv_get_field_empty_or_missing_returns_2(void)
{
    char dest[64] = {0};

    // First test: empty field (row exists, entry 0 exists but is empty)
    int ret = csv_set_field(buffer, 0, 0, "");
    TEST_ASSERT_EQUAL_INT(0, ret);
    ret = csv_get_field(dest, sizeof(dest), buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT(2, ret);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("", dest, "Empty field should yield empty string");

    // Second test: non-existent row
    ret = csv_get_field(dest, sizeof(dest), buffer, 99, 0);
    TEST_ASSERT_EQUAL_INT(2, ret);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("", dest, "Non-existent row should yield empty string");

    // Third test: non-existent entry in existing row
    ret = csv_get_field(dest, sizeof(dest), buffer, 0, 99);
    TEST_ASSERT_EQUAL_INT(2, ret);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("", dest, "Non-existent entry should yield empty string");
}

void test_csv_get_field_zero_dest_len_returns_3(void)
{
    char dest[64] = {0};

    setup_test_row_with_field("test");

    int ret = csv_get_field(dest, 0, buffer, 0, 0);

    TEST_ASSERT_EQUAL_INT(3, ret);
    // Per spec: dest[0] = '\0' is set for invalid cases
    TEST_ASSERT_EQUAL_STRING_MESSAGE("", dest, "Zero-length dest should be cleared");
}

void test_csv_get_field_negative_row_entry_handling(void)
{
    char dest[64] = {0};

    setup_test_row_with_field("test");

    // Test negative row (cast to size_t makes it huge, so >= rows)
    int ret = csv_get_field(dest, sizeof(dest), buffer, (size_t)-1, 0);
    TEST_ASSERT_EQUAL_INT(2, ret);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("", dest, "Negative row should be treated as invalid");

    // Test negative entry (cast to size_t makes it huge)
    ret = csv_get_field(dest, sizeof(dest), buffer, 0, (size_t)-1);
    TEST_ASSERT_EQUAL_INT(2, ret);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("", dest, "Negative entry should be treated as invalid");
}