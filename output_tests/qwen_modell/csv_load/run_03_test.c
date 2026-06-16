#include "csv.c"
#include "unity.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

static CSV_BUFFER *buffer = NULL;
static char test_file_name[] = "test_csv_load.csv";

static void create_test_file(const char *content) {
    FILE *fp = fopen(test_file_name, "w");
    TEST_ASSERT_NOT_NULL_MESSAGE(fp, "Failed to create test file");
    fprintf(fp, "%s", content);
    fclose(fp);
}

static void cleanup_test_file(void) {
    remove(test_file_name);
}

void setUp(void) {
    buffer = csv_create_buffer();
    TEST_ASSERT_NOT_NULL_MESSAGE(buffer, "Failed to create buffer");
    cleanup_test_file();
}

void tearDown(void) {
    csv_destroy_buffer(buffer);
    cleanup_test_file();
}

static void test_csv_load_success_simple_csv(void) {
    create_test_file("a,b,c\nd,e,f\n");
    int result = csv_load(buffer, test_file_name);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_INT(2, csv_get_height(buffer));
    TEST_ASSERT_EQUAL_INT(3, csv_get_width(buffer, 0));
    TEST_ASSERT_EQUAL_INT(3, csv_get_width(buffer, 1));
    
    char field[256];
    int ret = csv_get_field(field, sizeof(field), buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING("a", field);
    
    ret = csv_get_field(field, sizeof(field), buffer, 1, 2);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING("f", field);
}

static void test_csv_load_success_with_quoted_fields(void) {
    create_test_file("\"hello, world\",\"foo\nbar\",baz\n");
    int result = csv_load(buffer, test_file_name);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_INT(1, csv_get_height(buffer));
    TEST_ASSERT_EQUAL_INT(3, csv_get_width(buffer, 0));
    
    char field[256];
    int ret = csv_get_field(field, sizeof(field), buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING("hello, world", field);
    
    ret = csv_get_field(field, sizeof(field), buffer, 0, 1);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING("foo\nbar", field);
    
    ret = csv_get_field(field, sizeof(field), buffer, 0, 2);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING("baz", field);
}

static void test_csv_load_success_empty_cells(void) {
    create_test_file("a,,c\n,d,\n");
    int result = csv_load(buffer, test_file_name);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_INT(2, csv_get_height(buffer));
    TEST_ASSERT_EQUAL_INT(3, csv_get_width(buffer, 0));
    TEST_ASSERT_EQUAL_INT(3, csv_get_width(buffer, 1));
    
    char field[256];
    int ret = csv_get_field(field, sizeof(field), buffer, 0, 1);
    TEST_ASSERT_EQUAL_INT(2, ret);  // empty cell
    
    ret = csv_get_field(field, sizeof(field), buffer, 1, 0);
    TEST_ASSERT_EQUAL_INT(2, ret);  // empty cell
}

static void test_csv_load_success_trailing_delimiter(void) {
    create_test_file("a,b,c,\n");
    int result = csv_load(buffer, test_file_name);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_INT(1, csv_get_height(buffer));
    TEST_ASSERT_EQUAL_INT(4, csv_get_width(buffer, 0));
    
    char field[256];
    int ret = csv_get_field(field, sizeof(field), buffer, 0, 3);
    TEST_ASSERT_EQUAL_INT(2, ret);  // trailing empty cell
}

static void test_csv_load_file_not_found(void) {
    int result = csv_load(buffer, "nonexistent_file.csv");
    TEST_ASSERT_EQUAL_INT(1, result);
}

static void test_csv_load_success_single_cell(void) {
    create_test_file("hello");
    int result = csv_load(buffer, test_file_name);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_INT(1, csv_get_height(buffer));
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buffer, 0));
    
    char field[256];
    int ret = csv_get_field(field, sizeof(field), buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING("hello", field);
}

static void test_csv_load_success_single_row_multiple_cells(void) {
    create_test_file("one,two,three,four");
    int result = csv_load(buffer, test_file_name);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_INT(1, csv_get_height(buffer));
    TEST_ASSERT_EQUAL_INT(4, csv_get_width(buffer, 0));
    
    char field[256];
    int ret = csv_get_field(field, sizeof(field), buffer, 0, 3);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING("four", field);
}

static void test_csv_load_success_single_column(void) {
    create_test_file("row1\nrow2\nrow3\n");
    int result = csv_load(buffer, test_file_name);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_INT(3, csv_get_height(buffer));
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buffer, 0));
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buffer, 1));
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buffer, 2));
    
    char field[256];
    int ret = csv_get_field(field, sizeof(field), buffer, 2, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING("row3", field);
}

static void test_csv_load_success_mixed_quoted_and_unquoted(void) {
    create_test_file("normal,\"quoted,field\",\"quoted\nnewline\",last\n");
    int result = csv_load(buffer, test_file_name);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_INT(1, csv_get_height(buffer));
    TEST_ASSERT_EQUAL_INT(4, csv_get_width(buffer, 0));
    
    char field[256];
    int ret = csv_get_field(field, sizeof(field), buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING("normal", field);
    
    ret = csv_get_field(field, sizeof(field), buffer, 0, 1);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING("quoted,field", field);
    
    ret = csv_get_field(field, sizeof(field), buffer, 0, 2);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING("quoted\nnewline", field);
    
    ret = csv_get_field(field, sizeof(field), buffer, 0, 3);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING("last", field);
}

static void test_csv_load_success_empty_file(void) {
    create_test_file("");
    int result = csv_load(buffer, test_file_name);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_INT(0, csv_get_height(buffer));
}

int main(void) {
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_csv_load_success_simple_csv);
    RUN_TEST(test_csv_load_success_with_quoted_fields);
    RUN_TEST(test_csv_load_success_empty_cells);
    RUN_TEST(test_csv_load_success_trailing_delimiter);
    RUN_TEST(test_csv_load_file_not_found);
    RUN_TEST(test_csv_load_success_single_cell);
    RUN_TEST(test_csv_load_success_single_row_multiple_cells);
    RUN_TEST(test_csv_load_success_single_column);
    RUN_TEST(test_csv_load_success_mixed_quoted_and_unquoted);
    RUN_TEST(test_csv_load_success_empty_file);
    return UNITY_END();
}