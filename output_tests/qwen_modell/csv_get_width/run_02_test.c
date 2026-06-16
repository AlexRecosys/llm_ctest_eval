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

static void setup_buffer_with_rows(size_t num_rows, size_t *widths) {
    for (size_t i = 0; i < num_rows; i++) {
        append_row(buffer);
        buffer->width[i] = widths[i];
    }
}

void test_csv_get_width_valid_row_zero(void) {
    setup_buffer_with_rows(1, (size_t[]){0});
    TEST_ASSERT_EQUAL_INT(0, csv_get_width(buffer, 0));
}

void test_csv_get_width_valid_row_nonzero(void) {
    setup_buffer_with_rows(1, (size_t[]){5});
    TEST_ASSERT_EQUAL_INT(5, csv_get_width(buffer, 0));
}

void test_csv_get_width_multiple_rows_first(void) {
    setup_buffer_with_rows(3, (size_t[]){2, 4, 6});
    TEST_ASSERT_EQUAL_INT(2, csv_get_width(buffer, 0));
}

void test_csv_get_width_multiple_rows_middle(void) {
    setup_buffer_with_rows(3, (size_t[]){2, 4, 6});
    TEST_ASSERT_EQUAL_INT(4, csv_get_width(buffer, 1));
}

void test_csv_get_width_multiple_rows_last(void) {
    setup_buffer_with_rows(3, (size_t[]){2, 4, 6});
    TEST_ASSERT_EQUAL_INT(6, csv_get_width(buffer, 2));
}

void test_csv_get_width_row_out_of_bounds_high(void) {
    setup_buffer_with_rows(2, (size_t[]){1, 3});
    TEST_ASSERT_EQUAL_INT(0, csv_get_width(buffer, 2));
}

void test_csv_get_width_row_out_of_bounds_zero_rows(void) {
    TEST_ASSERT_EQUAL_INT(0, csv_get_width(buffer, 0));
}

void test_csv_get_width_row_out_of_bounds_negative(void) {
    setup_buffer_with_rows(2, (size_t[]){1, 3});
    TEST_ASSERT_EQUAL_INT(0, csv_get_width(buffer, (size_t)-1));
}

int main(void) {
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_csv_get_width_valid_row_zero);
    RUN_TEST(test_csv_get_width_valid_row_nonzero);
    RUN_TEST(test_csv_get_width_multiple_rows_first);
    RUN_TEST(test_csv_get_width_multiple_rows_middle);
    RUN_TEST(test_csv_get_width_multiple_rows_last);
    RUN_TEST(test_csv_get_width_row_out_of_bounds_high);
    RUN_TEST(test_csv_get_width_row_out_of_bounds_zero_rows);
    RUN_TEST(test_csv_get_width_row_out_of_bounds_negative);
    return UNITY_END();
}