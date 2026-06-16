#include "csv.c"
#include "unity.h"
#include "csv.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* File-scope variables */
static CSV_BUFFER *g_buffer;
static const char *TEST_FILE = "test_csv_save_output.csv";

/* Helper: read entire file into a malloc'd string. Caller must free. */
static char *read_file_contents(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) return NULL;
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *buf = (char *)malloc(size + 1);
    if (buf == NULL) { fclose(fp); return NULL; }
    size_t read = fread(buf, 1, size, fp);
    buf[read] = '\0';
    fclose(fp);
    return buf;
}

/* Helper: build a simple CSV_BUFFER with given rows/cols of plain text */
static CSV_BUFFER *make_buffer_with_data(const char *data[][10], int rows, int *cols) {
    CSV_BUFFER *buf = csv_create_buffer();
    if (buf == NULL) return NULL;
    for (int i = 0; i < rows; i++) {
        append_row(buf);
        for (int j = 0; j < cols[i]; j++) {
            append_field(buf, i);
            set_field(buf->field[i][j], (char *)data[i][j]);
        }
    }
    return buf;
}

void setUp(void) {
    g_buffer = csv_create_buffer();
}

void tearDown(void) {
    if (g_buffer != NULL) {
        csv_destroy_buffer(g_buffer);
        g_buffer = NULL;
    }
    remove(TEST_FILE);
}

/* Test: csv_save returns 1 when given an invalid file path */
void test_csv_save_returns_1_on_invalid_path(void) {
    append_row(g_buffer);
    append_field(g_buffer, 0);
    set_field(g_buffer->field[0][0], "hello");

    int result = csv_save("/nonexistent_dir/no_access/file.csv", g_buffer);
    TEST_ASSERT_EQUAL_INT(1, result);
}

/* Test: csv_save returns 0 on success */
void test_csv_save_returns_0_on_success(void) {
    append_row(g_buffer);
    append_field(g_buffer, 0);
    set_field(g_buffer->field[0][0], "hello");

    int result = csv_save((char *)TEST_FILE, g_buffer);
    TEST_ASSERT_EQUAL_INT(0, result);
}

/* Test: csv_save creates the file */
void test_csv_save_creates_file(void) {
    append_row(g_buffer);
    append_field(g_buffer, 0);
    set_field(g_buffer->field[0][0], "test");

    csv_save((char *)TEST_FILE, g_buffer);

    FILE *fp = fopen(TEST_FILE, "r");
    TEST_ASSERT_NOT_NULL(fp);
    if (fp) fclose(fp);
}

/* Test: single cell, no special characters */
void test_csv_save_single_cell_plain(void) {
    append_row(g_buffer);
    append_field(g_buffer, 0);
    set_field(g_buffer->field[0][0], "hello");

    csv_save((char *)TEST_FILE, g_buffer);

    char *contents = read_file_contents(TEST_FILE);
    TEST_ASSERT_NOT_NULL(contents);
    TEST_ASSERT_EQUAL_STRING("hello", contents);
    free(contents);
}

/* Test: single row, multiple columns */
void test_csv_save_single_row_multiple_cols(void) {
    append_row(g_buffer);
    append_field(g_buffer, 0);
    set_field(g_buffer->field[0][0], "one");
    append_field(g_buffer, 0);
    set_field(g_buffer->field[0][1], "two");
    append_field(g_buffer, 0);
    set_field(g_buffer->field[0][2], "three");

    csv_save((char *)TEST_FILE, g_buffer);

    char *contents = read_file_contents(TEST_FILE);
    TEST_ASSERT_NOT_NULL(contents);
    TEST_ASSERT_EQUAL_STRING("one,two,three", contents);
    free(contents);
}

/* Test: multiple rows */
void test_csv_save_multiple_rows(void) {
    append_row(g_buffer);
    append_field(g_buffer, 0);
    set_field(g_buffer->field[0][0], "a");
    append_field(g_buffer, 0);
    set_field(g_buffer->field[0][1], "b");

    append_row(g_buffer);
    append_field(g_buffer, 1);
    set_field(g_buffer->field[1][0], "c");
    append_field(g_buffer, 1);
    set_field(g_buffer->field[1][1], "d");

    csv_save((char *)TEST_FILE, g_buffer);

    char *contents = read_file_contents(TEST_FILE);
    TEST_ASSERT_NOT_NULL(contents);
    TEST_ASSERT_EQUAL_STRING("a,b\nc,d", contents);
    free(contents);
}

/* Test: field containing field delimiter gets quoted */
void test_csv_save_field_with_field_delim_gets_quoted(void) {
    append_row(g_buffer);
    append_field(g_buffer, 0);
    set_field(g_buffer->field[0][0], "hel,lo");

    csv_save((char *)TEST_FILE, g_buffer);

    char *contents = read_file_contents(TEST_FILE);
    TEST_ASSERT_NOT_NULL(contents);
    /* Should be "hel,lo" (with quotes) */
    TEST_ASSERT_EQUAL_STRING("\"hel,lo\"", contents);
    free(contents);
}

/* Test: field containing text delimiter gets quoted and escaped */
void test_csv_save_field_with_text_delim_gets_escaped(void) {
    append_row(g_buffer);
    append_field(g_buffer, 0);
    /* field text contains a double-quote */
    set_field(g_buffer->field[0][0], "say\"hi");

    csv_save((char *)TEST_FILE, g_buffer);

    char *contents = read_file_contents(TEST_FILE);
    TEST_ASSERT_NOT_NULL(contents);
    /* Expected: "say""hi" */
    TEST_ASSERT_EQUAL_STRING("\"say\"\"hi\"", contents);
    free(contents);
}

/* Test: field containing newline gets quoted */
void test_csv_save_field_with_newline_gets_quoted(void) {
    append_row(g_buffer);
    append_field(g_buffer, 0);
    set_field(g_buffer->field[0][0], "hel\nlo");

    csv_save((char *)TEST_FILE, g_buffer);

    char *contents = read_file_contents(TEST_FILE);
    TEST_ASSERT_NOT_NULL(contents);
    /* Should be "hel\nlo" (with surrounding quotes) */
    TEST_ASSERT_EQUAL_STRING("\"hel\nlo\"", contents);
    free(contents);
}

/* Test: empty field */
void test_csv_save_empty_field(void) {
    append_row(g_buffer);
    append_field(g_buffer, 0);
    set_field(g_buffer->field[0][0], "");

    csv_save((char *)TEST_FILE, g_buffer);

    char *contents = read_file_contents(TEST_FILE);
    TEST_ASSERT_NOT_NULL(contents);
    TEST_ASSERT_EQUAL_STRING("", contents);
    free(contents);
}

/* Test: two rows, second row has quoted field */
void test_csv_save_mixed_rows_with_quoted_field(void) {
    append_row(g_buffer);
    append_field(g_buffer, 0);
    set_field(g_buffer->field[0][0], "name");
    append_field(g_buffer, 0);
    set_field(g_buffer->field[0][1], "value");

    append_row(g_buffer);
    append_field(g_buffer, 1);
    set_field(g_buffer->field[1][0], "foo,bar");
    append_field(g_buffer, 1);
    set_field(g_buffer->field[1][1], "baz");

    csv_save((char *)TEST_FILE, g_buffer);

    char *contents = read_file_contents(TEST_FILE);
    TEST_ASSERT_NOT_NULL(contents);
    TEST_ASSERT_EQUAL_STRING("name,value\n\"foo,bar\",baz", contents);
    free(contents);
}

/* Test: last row does not end with newline */
void test_csv_save_no_trailing_newline(void) {
    append_row(g_buffer);
    append_field(g_buffer, 0);
    set_field(g_buffer->field[0][0], "row1");

    append_row(g_buffer);
    append_field(g_buffer, 1);
    set_field(g_buffer->field[1][0], "row2");

    csv_save((char *)TEST_FILE, g_buffer);

    char *contents = read_file_contents(TEST_FILE);
    TEST_ASSERT_NOT_NULL(contents);
    /* Last character should NOT be newline */
    size_t len = strlen(contents);
    TEST_ASSERT_TRUE(len > 0);
    TEST_ASSERT_FALSE(contents[len - 1] == '\n');
    free(contents);
}

/* Test: field delimiter between columns but not after last column */
void test_csv_save_field_delim_placement(void) {
    append_row(g_buffer);
    append_field(g_buffer, 0);
    set_field(g_buffer->field[0][0], "A");
    append_field(g_buffer, 0);
    set_field(g_buffer->field[0][1], "B");
    append_field(g_buffer, 0);
    set_field(g_buffer->field[0][2], "C");

    csv_save((char *)TEST_FILE, g_buffer);

    char *contents = read_file_contents(TEST_FILE);
    TEST_ASSERT_NOT_NULL(contents);
    TEST_ASSERT_EQUAL_STRING("A,B,C", contents);
    free(contents);
}

/* Test: custom field delimiter (semicolon) */
void test_csv_save_custom_field_delim(void) {
    csv_set_field_delim(g_buffer, ';');

    append_row(g_buffer);
    append_field(g_buffer, 0);
    set_field(g_buffer->field[0][0], "x");
    append_field(g_buffer, 0);
    set_field(g_buffer->field[0][1], "y");

    csv_save((char *)TEST_FILE, g_buffer);

    char *contents = read_file_contents(TEST_FILE);
    TEST_ASSERT_NOT_NULL(contents);
    TEST_ASSERT_EQUAL_STRING("x;y", contents);
    free(contents);
}

/* Test: custom text delimiter (single quote) */
void test_csv_save_custom_text_delim(void) {
    csv_set_text_delim(g_buffer, '\'');

    append_row(g_buffer);
    append_field(g_buffer, 0);
    /* field contains a comma, so it must be quoted with single quote */
    set_field(g_buffer->field[0][0], "a,b");

    csv_save((char *)TEST_FILE, g_buffer);

    char *contents = read_file_contents(TEST_FILE);
    TEST_ASSERT_NOT_NULL(contents);
    TEST_ASSERT_EQUAL_STRING("'a,b'", contents);
    free(contents);
}

/* Test: save and reload produces same data */
void test_csv_save_and_reload_roundtrip(void) {
    append_row(g_buffer);
    append_field(g_buffer, 0);
    set_field(g_buffer->field[0][0], "alpha");
    append_field(g_buffer, 0);
    set_field(g_buffer->field[0][1], "beta");

    append_row(g_buffer);
    append_field(g_buffer, 1);
    set_field(g_buffer->field[1][0], "gamma");
    append_field(g_buffer, 1);
    set_field(g_buffer->field[1][1], "delta");

    int save_result = csv_save((char *)TEST_FILE, g_buffer);
    TEST_ASSERT_EQUAL_INT(0, save_result);

    CSV_BUFFER *loaded = csv_create_buffer();
    TEST_ASSERT_NOT_NULL(loaded);

    int load_result = csv_load(loaded, (char *)TEST_FILE);
    TEST_ASSERT_EQUAL_INT(0, load_result);

    TEST_ASSERT_EQUAL_INT(2, csv_get_height(loaded));
    TEST_ASSERT_EQUAL_INT(2, csv_get_width(loaded, 0));
    TEST_ASSERT_EQUAL_INT(2, csv_get_width(loaded, 1));

    char dest[64];
    csv_get_field(dest, sizeof(dest), loaded, 0, 0);
    TEST_ASSERT_EQUAL_STRING("alpha", dest);

    csv_get_field(dest, sizeof(dest), loaded, 0, 1);
    TEST_ASSERT_EQUAL_STRING("beta", dest);

    csv_get_field(dest, sizeof(dest), loaded, 1, 0);
    TEST_ASSERT_EQUAL_STRING("gamma", dest);

    csv_get_field(dest, sizeof(dest), loaded, 1, 1);
    TEST_ASSERT_EQUAL_STRING("delta", dest);

    csv_destroy_buffer(loaded);
}

/* Test: save with quoted field and reload roundtrip */
void test_csv_save_quoted_field_roundtrip(void) {
    append_row(g_buffer);
    append_field(g_buffer, 0);
    set_field(g_buffer->field[0][0], "has,comma");
    append_field(g_buffer, 0);
    set_field(g_buffer->field[0][1], "normal");

    int save_result = csv_save((char *)TEST_FILE, g_buffer);
    TEST_ASSERT_EQUAL_INT(0, save_result);

    CSV_BUFFER *loaded = csv_create_buffer();
    TEST_ASSERT_NOT_NULL(loaded);

    int load_result = csv_load(loaded, (char *)TEST_FILE);
    TEST_ASSERT_EQUAL_INT(0, load_result);

    char dest[64];
    csv_get_field(dest, sizeof(dest), loaded, 0, 0);
    TEST_ASSERT_EQUAL_STRING("has,comma", dest);

    csv_get_field(dest, sizeof(dest), loaded, 0, 1);
    TEST_ASSERT_EQUAL_STRING("normal", dest);

    csv_destroy_buffer(loaded);
}

/* Test: three rows, varying widths */
void test_csv_save_three_rows_varying_widths(void) {
    append_row(g_buffer);
    append_field(g_buffer, 0);
    set_field(g_buffer->field[0][0], "1");

    append_row(g_buffer);
    append_field(g_buffer, 1);
    set_field(g_buffer->field[1][0], "2");
    append_field(g_buffer, 1);
    set_field(g_buffer->field[1][1], "3");

    append_row(g_buffer);
    append_field(g_buffer, 2);
    set_field(g_buffer->field[2][0], "4");
    append_field(g_buffer, 2);
    set_field(g_buffer->field[2][1], "5");
    append_field(g_buffer, 2);
    set_field(g_buffer->field[2][2], "6");

    csv_save((char *)TEST_FILE, g_buffer);

    char *contents = read_file_contents(TEST_FILE);
    TEST_ASSERT_NOT_NULL(contents);
    TEST_ASSERT_EQUAL_STRING("1\n2,3\n4,5,6", contents);
    free(contents);
}

/* Test: field with only a text delimiter character */
void test_csv_save_field_only_text_delim(void) {
    append_row(g_buffer);
    append_field(g_buffer, 0);
    /* field is just a double-quote character */
    set_field(g_buffer->field[0][0], "\"");

    csv_save((char *)TEST_FILE, g_buffer);

    char *contents = read_file_contents(TEST_FILE);
    TEST_ASSERT_NOT_NULL(contents);
    /* Expected: """" (open quote, escaped quote, close quote) */
    TEST_ASSERT_EQUAL_STRING("\"\"\"\"", contents);
    free(contents);
}

/* Test: overwrite existing file */
void test_csv_save_overwrites_existing_file(void) {
    /* Write initial content */
    FILE *fp = fopen(TEST_FILE, "w");
    TEST_ASSERT_NOT_NULL(fp);
    fputs("old content here", fp);
    fclose(fp);

    append_row(g_buffer);
    append_field(g_buffer, 0);
    set_field(g_buffer->field[0][0], "new");

    csv_save((char *)TEST_FILE, g_buffer);

    char *contents = read_file_contents(TEST_FILE);
    TEST_ASSERT_NOT_NULL(contents);
    TEST_ASSERT_EQUAL_STRING("new", contents);
    free(contents);
}

int main(void) {
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_csv_save_returns_1_on_invalid_path);
    RUN_TEST(test_csv_save_returns_0_on_success);
    RUN_TEST(test_csv_save_creates_file);
    RUN_TEST(test_csv_save_single_cell_plain);
    RUN_TEST(test_csv_save_single_row_multiple_cols);
    RUN_TEST(test_csv_save_multiple_rows);
    RUN_TEST(test_csv_save_field_with_field_delim_gets_quoted);
    RUN_TEST(test_csv_save_field_with_text_delim_gets_escaped);
    RUN_TEST(test_csv_save_field_with_newline_gets_quoted);
    RUN_TEST(test_csv_save_empty_field);
    RUN_TEST(test_csv_save_mixed_rows_with_quoted_field);
    RUN_TEST(test_csv_save_no_trailing_newline);
    RUN_TEST(test_csv_save_field_delim_placement);
    RUN_TEST(test_csv_save_custom_field_delim);
    RUN_TEST(test_csv_save_custom_text_delim);
    RUN_TEST(test_csv_save_and_reload_roundtrip);
    RUN_TEST(test_csv_save_quoted_field_roundtrip);
    RUN_TEST(test_csv_save_three_rows_varying_widths);
    RUN_TEST(test_csv_save_field_only_text_delim);
    RUN_TEST(test_csv_save_overwrites_existing_file);
    return UNITY_END();
}