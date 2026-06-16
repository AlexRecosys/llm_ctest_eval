#include "csv.c"
#include "unity.h"
#include <string.h>
#include <stdlib.h>

static CSV_BUFFER *test_buffer = NULL;
static char test_dest[256];

static void setup_test_buffer(void) {
    test_buffer = csv_create_buffer();
    TEST_ASSERT_NOT_NULL(test_buffer);

    // Add 2 rows
    append_row(test_buffer);
    append_row(test_buffer);

    // Row 0: 3 fields
    append_field(test_buffer, 0);
    set_field(test_buffer->field[0][0], "field0_0");
    append_field(test_buffer, 0);
    set_field(test_buffer->field[0][1], "field0_1");
    append_field(test_buffer, 0);
    set_field(test_buffer->field[0][2], "");

    // Row 1: 2 fields
    append_field(test_buffer, 1);
    set_field(test_buffer->field[1][0], "field1_0");
    append_field(test_buffer, 1);
    set_field(test_buffer->field[1][1], "field1_1");
}

static void teardown_test_buffer(void) {
    if (test_buffer != NULL) {
        csv_destroy_buffer(test_buffer);
        test_buffer = NULL;
    }
}

void setUp(void) {
    setup_test_buffer();
}

void tearDown(void) {
    teardown_test_buffer();
}

void test_csv_get_field_success_full_copy(void) {
    int result = csv_get_field(test_dest, sizeof(test_dest), test_buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_STRING("field0_0", test_dest);
}

void test_csv_get_field_success_truncated(void) {
    int result = csv_get_field(test_dest, 5, test_buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT(1, result);
    TEST_ASSERT_EQUAL_STRING_LEN("field0_0", test_dest, 5);
    TEST_ASSERT_EQUAL_STRING("field0_0", test_dest); // Should be truncated to "fiel" + '\0'
    TEST_ASSERT_EQUAL_MEMORY("fiel", test_dest, 4);
    TEST_ASSERT_EQUAL_CHAR('\0', test_dest[4]);
}

void test_csv_get_field_empty_field(void) {
    int result = csv_get_field(test_dest, sizeof(test_dest), test_buffer, 0, 2);
    TEST_ASSERT_EQUAL_INT(2, result);
    TEST_ASSERT_EQUAL_STRING("", test_dest);
}

void test_csv_get_field_invalid_row_negative(void) {
    int result = csv_get_field(test_dest, sizeof(test_dest), test_buffer, (size_t)-1, 0);
    TEST_ASSERT_EQUAL_INT(2, result);
    TEST_ASSERT_EQUAL_STRING("", test_dest);
}

void test_csv_get_field_invalid_row_too_large(void) {
    int result = csv_get_field(test_dest, sizeof(test_dest), test_buffer, 100, 0);
    TEST_ASSERT_EQUAL_INT(2, result);
    TEST_ASSERT_EQUAL_STRING("", test_dest);
}

void test_csv_get_field_invalid_entry_negative(void) {
    int result = csv_get_field(test_dest, sizeof(test_dest), test_buffer, 0, (size_t)-1);
    TEST_ASSERT_EQUAL_INT(2, result);
    TEST_ASSERT_EQUAL_STRING("", test_dest);
}

void test_csv_get_field_invalid_entry_too_large(void) {
    int result = csv_get_field(test_dest, sizeof(test_dest), test_buffer, 0, 100);
    TEST_ASSERT_EQUAL_INT(2, result);
    TEST_ASSERT_EQUAL_STRING("", test_dest);
}

void test_csv_get_field_zero_dest_len(void) {
    int result = csv_get_field(test_dest, 0, test_buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT(3, result);
    // dest_len == 0: function returns 3 and does not modify dest
    // We cannot assert contents because dest is not modified
}

void test_csv_get_field_row1_entry1(void) {
    int result = csv_get_field(test_dest, sizeof(test_dest), test_buffer, 1, 1);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_STRING("field1_1", test_dest);
}

void test_csv_get_field_row1_entry0(void) {
    int result = csv_get_field(test_dest, sizeof(test_dest), test_buffer, 1, 0);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_STRING("field1_0", test_dest);
}

void test_csv_get_field_exact_buffer_size(void) {
    // field0_0 is 8 chars + '\0' = 9 bytes
    int result = csv_get_field(test_dest, 9, test_buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_STRING("field0_0", test_dest);
}

void test_csv_get_field_one_less_than_required(void) {
    // field0_0 is 8 chars + '\0' = 9 bytes
    int result = csv_get_field(test_dest, 8, test_buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT(1, result);
    TEST_ASSERT_EQUAL_STRING_LEN("field0_0", test_dest, 8);
    TEST_ASSERT_EQUAL_STRING("field0_0", test_dest);
    TEST_ASSERT_EQUAL_MEMORY("field0", test_dest, 6);
    TEST_ASSERT_EQUAL_CHAR('\0', test_dest[7]);
}

int main(void) {
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_csv_get_field_success_full_copy);
    RUN_TEST(test_csv_get_field_success_truncated);
    RUN_TEST(test_csv_get_field_empty_field);
    RUN_TEST(test_csv_get_field_invalid_row_negative);
    RUN_TEST(test_csv_get_field_invalid_row_too_large);
    RUN_TEST(test_csv_get_field_invalid_entry_negative);
    RUN_TEST(test_csv_get_field_invalid_entry_too_large);
    RUN_TEST(test_csv_get_field_zero_dest_len);
    RUN_TEST(test_csv_get_field_row1_entry1);
    RUN_TEST(test_csv_get_field_row1_entry0);
    RUN_TEST(test_csv_get_field_exact_buffer_size);
    RUN_TEST(test_csv_get_field_one_less_than_required);
    return UNITY_END();
}