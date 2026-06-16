#include "csv.c"
#include "unity.h"
#include <stdlib.h>
#include <string.h>

static CSV_BUFFER *buffer = NULL;

void setUp(void) {
    buffer = csv_create_buffer();
    TEST_ASSERT_NOT_NULL(buffer);
}

void tearDown(void) {
    csv_destroy_buffer(buffer);
    buffer = NULL;
}

static void init_buffer_with_data(void) {
    // Create 2 rows with 3 fields each
    append_row(buffer);
    append_row(buffer);

    csv_set_field(buffer, 0, 0, "A1");
    csv_set_field(buffer, 0, 1, "B1");
    csv_set_field(buffer, 0, 2, "C1");

    csv_set_field(buffer, 1, 0, "A2");
    csv_set_field(buffer, 1, 1, "B2");
    csv_set_field(buffer, 1, 2, "C2");
}

static void assert_field_equals(CSV_BUFFER *buf, size_t row, size_t col, const char *expected) {
    char actual[256] = {0};
    int result = csv_get_field(actual, sizeof(actual), buf, row, col);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_STRING(expected, actual);
}

static void assert_width_equals(CSV_BUFFER *buf, size_t row, size_t expected_width) {
    int actual_width = csv_get_width(buf, row);
    TEST_ASSERT_EQUAL_INT((int)expected_width, actual_width);
}

void test_csv_insert_field_inserts_at_end_of_row(void) {
    init_buffer_with_data();

    // Insert at position 3 (end of row 0, which has width 3)
    int result = csv_insert_field(buffer, 0, 3, "D1");
    TEST_ASSERT_EQUAL_INT(0, result);

    assert_field_equals(buffer, 0, 3, "D1");
    assert_width_equals(buffer, 0, 4);
}

void test_csv_insert_field_inserts_in_middle_of_row(void) {
    init_buffer_with_data();

    // Insert at position 1 (middle of row 0)
    int result = csv_insert_field(buffer, 0, 1, "X1");
    TEST_ASSERT_EQUAL_INT(0, result);

    assert_field_equals(buffer, 0, 0, "A1");
    assert_field_equals(buffer, 0, 1, "X1");
    assert_field_equals(buffer, 0, 2, "B1");
    assert_field_equals(buffer, 0, 3, "C1");
    assert_width_equals(buffer, 0, 4);
}

void test_csv_insert_field_inserts_at_beginning_of_row(void) {
    init_buffer_with_data();

    // Insert at position 0 (beginning of row 0)
    int result = csv_insert_field(buffer, 0, 0, "ZERO1");
    TEST_ASSERT_EQUAL_INT(0, result);

    assert_field_equals(buffer, 0, 0, "ZERO1");
    assert_field_equals(buffer, 0, 1, "A1");
    assert_field_equals(buffer, 0, 2, "B1");
    assert_field_equals(buffer, 0, 3, "C1");
    assert_width_equals(buffer, 0, 4);
}

void test_csv_insert_field_inserts_at_end_of_second_row(void) {
    init_buffer_with_data();

    // Insert at position 3 (end of row 1)
    int result = csv_insert_field(buffer, 1, 3, "D2");
    TEST_ASSERT_EQUAL_INT(0, result);

    assert_field_equals(buffer, 1, 3, "D2");
    assert_width_equals(buffer, 1, 4);
}

void test_csv_insert_field_inserts_in_second_row_middle(void) {
    init_buffer_with_data();

    // Insert at position 1 in row 1
    int result = csv_insert_field(buffer, 1, 1, "X2");
    TEST_ASSERT_EQUAL_INT(0, result);

    assert_field_equals(buffer, 1, 0, "A2");
    assert_field_equals(buffer, 1, 1, "X2");
    assert_field_equals(buffer, 1, 2, "B2");
    assert_field_equals(buffer, 1, 3, "C2");
    assert_width_equals(buffer, 1, 4);
}

void test_csv_insert_field_inserts_beyond_existing_width_creates_new_field(void) {
    init_buffer_with_data();

    // Insert at position 5 (beyond existing width=3) -> should call csv_set_field
    int result = csv_insert_field(buffer, 0, 5, "FAR1");
    TEST_ASSERT_EQUAL_INT(0, result);

    // csv_set_field should have created fields up to index 5
    assert_field_equals(buffer, 0, 5, "FAR1");
    assert_width_equals(buffer, 0, 6);
}

void test_csv_insert_field_inserts_into_new_row(void) {
    // Start with empty buffer
    append_row(buffer);

    // Insert into row 0, entry 0 (should create field)
    int result = csv_insert_field(buffer, 0, 0, "FIRST");
    TEST_ASSERT_EQUAL_INT(0, result);

    assert_field_equals(buffer, 0, 0, "FIRST");
    assert_width_equals(buffer, 0, 1);
}

void test_csv_insert_field_inserts_into_new_row_with_gap(void) {
    // Start with empty buffer
    append_row(buffer);

    // Insert into row 0, entry 2 (should create fields 0,1,2)
    int result = csv_insert_field(buffer, 0, 2, "MIDDLE");
    TEST_ASSERT_EQUAL_INT(0, result);

    assert_field_equals(buffer, 0, 2, "MIDDLE");
    assert_width_equals(buffer, 0, 3);
}

void test_csv_insert_field_inserts_empty_string(void) {
    init_buffer_with_data();

    int result = csv_insert_field(buffer, 0, 1, "");
    TEST_ASSERT_EQUAL_INT(0, result);

    assert_field_equals(buffer, 0, 1, "");
}

void test_csv_insert_field_inserts_long_string(void) {
    init_buffer_with_data();

    char long_str[1024];
    memset(long_str, 'X', sizeof(long_str) - 1);
    long_str[sizeof(long_str) - 1] = '\0';

    int result = csv_insert_field(buffer, 0, 1, long_str);
    TEST_ASSERT_EQUAL_INT(0, result);

    assert_field_equals(buffer, 0, 1, long_str);
}

void test_csv_insert_field_inserts_into_nonexistent_row(void) {
    init_buffer_with_data();

    // Row 2 does not exist (only rows 0 and 1 exist)
    int result = csv_insert_field(buffer, 2, 0, "NEW_ROW");
    TEST_ASSERT_EQUAL_INT(0, result);

    // csv_set_field should have created row 2
    assert_field_equals(buffer, 2, 0, "NEW_ROW");
    assert_width_equals(buffer, 2, 1);
}

int main(void) {
    unity_install_sighandler();
    UNITY_BEGIN();

    RUN_TEST(test_csv_insert_field_inserts_at_end_of_row);
    RUN_TEST(test_csv_insert_field_inserts_in_middle_of_row);
    RUN_TEST(test_csv_insert_field_inserts_at_beginning_of_row);
    RUN_TEST(test_csv_insert_field_inserts_at_end_of_second_row);
    RUN_TEST(test_csv_insert_field_inserts_in_second_row_middle);
    RUN_TEST(test_csv_insert_field_inserts_beyond_existing_width_creates_new_field);
    RUN_TEST(test_csv_insert_field_inserts_into_new_row);
    RUN_TEST(test_csv_insert_field_inserts_into_new_row_with_gap);
    RUN_TEST(test_csv_insert_field_inserts_empty_string);
    RUN_TEST(test_csv_insert_field_inserts_long_string);
    RUN_TEST(test_csv_insert_field_inserts_into_nonexistent_row);

    return UNITY_END();
}