#include "csv.c"
#include "unity.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

static char test_file_path[] = "test_csv_save_output.csv";

static void cleanup_test_file(void) {
    if (access(test_file_path, F_OK) == 0) {
        remove(test_file_path);
    }
}

static void setup_csv_buffer_with_data(CSV_BUFFER *buffer, size_t rows, size_t *widths, char ***data) {
    buffer->rows = rows;
    buffer->width = widths;
    buffer->field = malloc(rows * sizeof(CSV_FIELD **));
    TEST_ASSERT_NOT_NULL(buffer->field);

    for (size_t i = 0; i < rows; i++) {
        buffer->field[i] = malloc(widths[i] * sizeof(CSV_FIELD *));
        TEST_ASSERT_NOT_NULL(buffer->field[i]);
        for (size_t j = 0; j < widths[i]; j++) {
            buffer->field[i][j] = create_field();
            TEST_ASSERT_NOT_NULL(buffer->field[i][j]);
            set_field(buffer->field[i][j], data[i][j]);
        }
    }
}

static void teardown_csv_buffer(CSV_BUFFER *buffer) {
    for (size_t i = 0; i < buffer->rows; i++) {
        for (size_t j = 0; j < buffer->width[i]; j++) {
            destroy_field(buffer->field[i][j]);
        }
        free(buffer->field[i]);
    }
    free(buffer->field);
    free(buffer->width);
}

void setUp(void) {
    cleanup_test_file();
}

void tearDown(void) {
    cleanup_test_file();
}

void test_csv_save_basic_csv(void) {
    CSV_BUFFER buffer;
    buffer.field_delim = ',';
    buffer.text_delim = '"';
    buffer.rows = 2;
    size_t widths[] = {2, 2};
    buffer.width = widths;

    char *data[2][2] = {
        {"name", "age"},
        {"Alice", "30"}
    };

    buffer.field = malloc(buffer.rows * sizeof(CSV_FIELD **));
    TEST_ASSERT_NOT_NULL(buffer.field);
    for (size_t i = 0; i < buffer.rows; i++) {
        buffer.field[i] = malloc(widths[i] * sizeof(CSV_FIELD *));
        TEST_ASSERT_NOT_NULL(buffer.field[i]);
        for (size_t j = 0; j < widths[i]; j++) {
            buffer.field[i][j] = create_field();
            TEST_ASSERT_NOT_NULL(buffer.field[i][j]);
            set_field(buffer.field[i][j], data[i][j]);
        }
    }

    int result = csv_save(test_file_path, &buffer);
    TEST_ASSERT_EQUAL_INT(0, result);

    FILE *fp = fopen(test_file_path, "r");
    TEST_ASSERT_NOT_NULL(fp);

    char expected[] = "name,age\nAlice,30\n";
    char actual[256] = {0};
    size_t bytes_read = fread(actual, 1, sizeof(actual) - 1, fp);
    fclose(fp);

    TEST_ASSERT_EQUAL_INT(strlen(expected), bytes_read);
    TEST_ASSERT_EQUAL_STRING_LEN(expected, actual, bytes_read);

    teardown_csv_buffer(&buffer);
}

void test_csv_save_with_text_delimiter_in_field(void) {
    CSV_BUFFER buffer;
    buffer.field_delim = ',';
    buffer.text_delim = '"';
    buffer.rows = 1;
    size_t widths[] = {2};
    buffer.width = widths;

    buffer.field = malloc(buffer.rows * sizeof(CSV_FIELD **));
    TEST_ASSERT_NOT_NULL(buffer.field);
    buffer.field[0] = malloc(widths[0] * sizeof(CSV_FIELD *));
    TEST_ASSERT_NOT_NULL(buffer.field[0]);

    buffer.field[0][0] = create_field();
    TEST_ASSERT_NOT_NULL(buffer.field[0][0]);
    set_field(buffer.field[0][0], "Hello, \"World\"");

    int result = csv_save(test_file_path, &buffer);
    TEST_ASSERT_EQUAL_INT(0, result);

    FILE *fp = fopen(test_file_path, "r");
    TEST_ASSERT_NOT_NULL(fp);

    char expected[] = "\"Hello, \"\"World\"\"\"\n";
    char actual[256] = {0};
    size_t bytes_read = fread(actual, 1, sizeof(actual) - 1, fp);
    fclose(fp);

    TEST_ASSERT_EQUAL_INT(strlen(expected), bytes_read);
    TEST_ASSERT_EQUAL_STRING_LEN(expected, actual, bytes_read);

    teardown_csv_buffer(&buffer);
}

void test_csv_save_with_field_delimiter_in_field(void) {
    CSV_BUFFER buffer;
    buffer.field_delim = ',';
    buffer.text_delim = '"';
    buffer.rows = 1;
    size_t widths[] = {1};
    buffer.width = widths;

    buffer.field = malloc(buffer.rows * sizeof(CSV_FIELD **));
    TEST_ASSERT_NOT_NULL(buffer.field);
    buffer.field[0] = malloc(widths[0] * sizeof(CSV_FIELD *));
    TEST_ASSERT_NOT_NULL(buffer.field[0]);

    buffer.field[0][0] = create_field();
    TEST_ASSERT_NOT_NULL(buffer.field[0][0]);
    set_field(buffer.field[0][0], "A,B");

    int result = csv_save(test_file_path, &buffer);
    TEST_ASSERT_EQUAL_INT(0, result);

    FILE *fp = fopen(test_file_path, "r");
    TEST_ASSERT_NOT_NULL(fp);

    char expected[] = "\"A,B\"\n";
    char actual[256] = {0};
    size_t bytes_read = fread(actual, 1, sizeof(actual) - 1, fp);
    fclose(fp);

    TEST_ASSERT_EQUAL_INT(strlen(expected), bytes_read);
    TEST_ASSERT_EQUAL_STRING_LEN(expected, actual, bytes_read);

    teardown_csv_buffer(&buffer);
}

void test_csv_save_with_newline_in_field(void) {
    CSV_BUFFER buffer;
    buffer.field_delim = ',';
    buffer.text_delim = '"';
    buffer.rows = 1;
    size_t widths[] = {1};
    buffer.width = widths;

    buffer.field = malloc(buffer.rows * sizeof(CSV_FIELD **));
    TEST_ASSERT_NOT_NULL(buffer.field);
    buffer.field[0] = malloc(widths[0] * sizeof(CSV_FIELD *));
    TEST_ASSERT_NOT_NULL(buffer.field[0]);

    buffer.field[0][0] = create_field();
    TEST_ASSERT_NOT_NULL(buffer.field[0][0]);
    char *text_with_newline = malloc(10);
    strcpy(text_with_newline, "Line1\nLine2");
    set_field(buffer.field[0][0], text_with_newline);
    free(text_with_newline);

    int result = csv_save(test_file_path, &buffer);
    TEST_ASSERT_EQUAL_INT(0, result);

    FILE *fp = fopen(test_file_path, "r");
    TEST_ASSERT_NOT_NULL(fp);

    char expected[] = "\"Line1\nLine2\"\n";
    char actual[256] = {0};
    size_t bytes_read = fread(actual, 1, sizeof(actual) - 1, fp);
    fclose(fp);

    TEST_ASSERT_EQUAL_INT(strlen(expected), bytes_read);
    TEST_ASSERT_EQUAL_STRING_LEN(expected, actual, bytes_read);

    teardown_csv_buffer(&buffer);
}

void test_csv_save_empty_buffer(void) {
    CSV_BUFFER buffer;
    buffer.field_delim = ',';
    buffer.text_delim = '"';
    buffer.rows = 0;
    buffer.width = NULL;
    buffer.field = NULL;

    int result = csv_save(test_file_path, &buffer);
    TEST_ASSERT_EQUAL_INT(0, result);

    FILE *fp = fopen(test_file_path, "r");
    TEST_ASSERT_NOT_NULL(fp);

    char actual[256] = {0};
    size_t bytes_read = fread(actual, 1, sizeof(actual) - 1, fp);
    fclose(fp);

    TEST_ASSERT_EQUAL_INT(0, bytes_read);

    // buffer is empty, nothing to teardown
}

void test_csv_save_invalid_file_path(void) {
    CSV_BUFFER buffer;
    buffer.field_delim = ',';
    buffer.text_delim = '"';
    buffer.rows = 0;
    buffer.width = NULL;
    buffer.field = NULL;

    int result = csv_save("/nonexistent/path/to/file.csv", &buffer);
    TEST_ASSERT_EQUAL_INT(1, result);
}

void test_csv_save_with_custom_delimiters(void) {
    CSV_BUFFER buffer;
    buffer.field_delim = ';';
    buffer.text_delim = '\'';
    buffer.rows = 1;
    size_t widths[] = {2};
    buffer.width = widths;

    buffer.field = malloc(buffer.rows * sizeof(CSV_FIELD **));
    TEST_ASSERT_NOT_NULL(buffer.field);
    buffer.field[0] = malloc(widths[0] * sizeof(CSV_FIELD *));
    TEST_ASSERT_NOT_NULL(buffer.field[0]);

    buffer.field[0][0] = create_field();
    TEST_ASSERT_NOT_NULL(buffer.field[0][0]);
    set_field(buffer.field[0][0], "Hello;World");

    buffer.field[0][1] = create_field();
    TEST_ASSERT_NOT_NULL(buffer.field[0][1]);
    set_field(buffer.field[0][1], "A'B");

    int result = csv_save(test_file_path, &buffer);
    TEST_ASSERT_EQUAL_INT(0, result);

    FILE *fp = fopen(test_file_path, "r");
    TEST_ASSERT_NOT_NULL(fp);

    char expected[] = "'Hello;World';'A''B'\n";
    char actual[256] = {0};
    size_t bytes_read = fread(actual, 1, sizeof(actual) - 1, fp);
    fclose(fp);

    TEST_ASSERT_EQUAL_INT(strlen(expected), bytes_read);
    TEST_ASSERT_EQUAL_STRING_LEN(expected, actual, bytes_read);

    teardown_csv_buffer(&buffer);
}

void test_csv_save_single_field_no_delimiters(void) {
    CSV_BUFFER buffer;
    buffer.field_delim = ',';
    buffer.text_delim = '"';
    buffer.rows = 1;
    size_t widths[] = {1};
    buffer.width = widths;

    buffer.field = malloc(buffer.rows * sizeof(CSV_FIELD **));
    TEST_ASSERT_NOT_NULL(buffer.field);
    buffer.field[0] = malloc(widths[0] * sizeof(CSV_FIELD *));
    TEST_ASSERT_NOT_NULL(buffer.field[0]);

    buffer.field[0][0] = create_field();
    TEST_ASSERT_NOT_NULL(buffer.field[0][0]);
    set_field(buffer.field[0][0], "simple");

    int result = csv_save(test_file_path, &buffer);
    TEST_ASSERT_EQUAL_INT(0, result);

    FILE *fp = fopen(test_file_path, "r");
    TEST_ASSERT_NOT_NULL(fp);

    char expected[] = "simple\n";
    char actual[256] = {0};
    size_t bytes_read = fread(actual, 1, sizeof(actual) - 1, fp);
    fclose(fp);

    TEST_ASSERT_EQUAL_INT(strlen(expected), bytes_read);
    TEST_ASSERT_EQUAL_STRING_LEN(expected, actual, bytes_read);

    teardown_csv_buffer(&buffer);
}

void test_csv_save_multiple_rows_with_mixed_content(void) {
    CSV_BUFFER buffer;
    buffer.field_delim = ',';
    buffer.text_delim = '"';
    buffer.rows = 3;
    size_t widths[] = {2, 3, 1};
    buffer.width = widths;

    buffer.field = malloc(buffer.rows * sizeof(CSV_FIELD **));
    TEST_ASSERT_NOT_NULL(buffer.field);

    buffer.field[0] = malloc(widths[0] * sizeof(CSV_FIELD *));
    buffer.field[0][0] = create_field();
    buffer.field[0][1] = create_field();
    set_field(buffer.field[0][0], "id");
    set_field(buffer.field[0][1], "name");

    buffer.field[1] = malloc(widths[1] * sizeof(CSV_FIELD *));
    buffer.field[1][0] = create_field();
    buffer.field[1][1] = create_field();
    buffer.field[1][2] = create_field();
    set_field(buffer.field[1][0], "1");
    set_field(buffer.field[1][1], "John, Jr.");
    set_field(buffer.field[1][2], "Doe");

    buffer.field[2] = malloc(widths[2] * sizeof(CSV_FIELD *));
    buffer.field[2][0] = create_field();
    set_field(buffer.field[2][0], "End");

    int result = csv_save(test_file_path, &buffer);
    TEST_ASSERT_EQUAL_INT(0, result);

    FILE *fp = fopen(test_file_path, "r");
    TEST_ASSERT_NOT_NULL(fp);

    char expected[] = "id,name\n1,\"John, Jr.\",Doe\nEnd\n";
    char actual[512] = {0};
    size_t bytes_read = fread(actual, 1, sizeof(actual) - 1, fp);
    fclose(fp);

    TEST_ASSERT_EQUAL_INT(strlen(expected), bytes_read);
    TEST_ASSERT_EQUAL_STRING_LEN(expected, actual, bytes_read);

    teardown_csv_buffer(&buffer);
}

int main(void) {
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_csv_save_basic_csv);
    RUN_TEST(test_csv_save_with_text_delimiter_in_field);
    RUN_TEST(test_csv_save_with_field_delimiter_in_field);
    RUN_TEST(test_csv_save_with_newline_in_field);
    RUN_TEST(test_csv_save_empty_buffer);
    RUN_TEST(test_csv_save_invalid_file_path);
    RUN_TEST(test_csv_save_with_custom_delimiters);
    RUN_TEST(test_csv_save_single_field_no_delimiters);
    RUN_TEST(test_csv_save_multiple_rows_with_mixed_content);
    return UNITY_END();
}