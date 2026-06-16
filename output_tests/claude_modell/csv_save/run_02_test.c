#include "csv.c"
#include "unity.h"
#include "csv.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* File-scope variables */
static CSV_BUFFER *g_buffer = NULL;
static const char *TEST_FILE = "test_csv_save_output.csv";

/* Helper: read entire file into a heap-allocated string. Caller must free. */
static char *read_file_contents(const char *filename)
{
    FILE *fp = fopen(filename, "r");
    if (fp == NULL)
        return NULL;
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    rewind(fp);
    char *buf = (char *)malloc((size_t)(size + 1));
    if (buf == NULL) {
        fclose(fp);
        return NULL;
    }
    size_t read = fread(buf, 1, (size_t)size, fp);
    buf[read] = '\0';
    fclose(fp);
    return buf;
}

/* Helper: build a simple buffer with given rows/cols of text */
static CSV_BUFFER *make_buffer_with_data(const char *data[][8],
                                          int rows,
                                          const int *widths)
{
    CSV_BUFFER *buf = csv_create_buffer();
    if (buf == NULL)
        return NULL;
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < widths[i]; j++) {
            csv_set_field(buf, (size_t)i, (size_t)j, (char *)data[i][j]);
        }
    }
    return buf;
}

void setUp(void)
{
    g_buffer = csv_create_buffer();
}

void tearDown(void)
{
    if (g_buffer != NULL) {
        csv_destroy_buffer(g_buffer);
        g_buffer = NULL;
    }
    remove(TEST_FILE);
}

/* -----------------------------------------------------------------------
 * Test: csv_save returns 1 when given an invalid file path
 * --------------------------------------------------------------------- */
void test_csv_save_returns_1_on_invalid_path(void)
{
    int ret = csv_save("/nonexistent_dir/no_such_file.csv", g_buffer);
    TEST_ASSERT_EQUAL_INT(1, ret);
}

/* -----------------------------------------------------------------------
 * Test: csv_save returns 0 on success with an empty buffer
 * --------------------------------------------------------------------- */
void test_csv_save_returns_0_on_empty_buffer(void)
{
    int ret = csv_save((char *)TEST_FILE, g_buffer);
    TEST_ASSERT_EQUAL_INT(0, ret);
}

/* -----------------------------------------------------------------------
 * Test: csv_save creates the file when it does not exist
 * --------------------------------------------------------------------- */
void test_csv_save_creates_file(void)
{
    remove(TEST_FILE);
    csv_save((char *)TEST_FILE, g_buffer);
    FILE *fp = fopen(TEST_FILE, "r");
    TEST_ASSERT_NOT_NULL(fp);
    if (fp)
        fclose(fp);
}

/* -----------------------------------------------------------------------
 * Test: csv_save writes a single field correctly
 * --------------------------------------------------------------------- */
void test_csv_save_single_field(void)
{
    csv_set_field(g_buffer, 0, 0, "hello");
    int ret = csv_save((char *)TEST_FILE, g_buffer);
    TEST_ASSERT_EQUAL_INT(0, ret);

    char *contents = read_file_contents(TEST_FILE);
    TEST_ASSERT_NOT_NULL(contents);
    TEST_ASSERT_EQUAL_STRING("hello", contents);
    free(contents);
}

/* -----------------------------------------------------------------------
 * Test: csv_save writes multiple fields in one row separated by field_delim
 * --------------------------------------------------------------------- */
void test_csv_save_single_row_multiple_fields(void)
{
    csv_set_field(g_buffer, 0, 0, "alpha");
    csv_set_field(g_buffer, 0, 1, "beta");
    csv_set_field(g_buffer, 0, 2, "gamma");

    int ret = csv_save((char *)TEST_FILE, g_buffer);
    TEST_ASSERT_EQUAL_INT(0, ret);

    char *contents = read_file_contents(TEST_FILE);
    TEST_ASSERT_NOT_NULL(contents);
    TEST_ASSERT_EQUAL_STRING("alpha,beta,gamma", contents);
    free(contents);
}

/* -----------------------------------------------------------------------
 * Test: csv_save writes multiple rows separated by newlines
 * --------------------------------------------------------------------- */
void test_csv_save_multiple_rows(void)
{
    csv_set_field(g_buffer, 0, 0, "row0col0");
    csv_set_field(g_buffer, 0, 1, "row0col1");
    csv_set_field(g_buffer, 1, 0, "row1col0");
    csv_set_field(g_buffer, 1, 1, "row1col1");

    int ret = csv_save((char *)TEST_FILE, g_buffer);
    TEST_ASSERT_EQUAL_INT(0, ret);

    char *contents = read_file_contents(TEST_FILE);
    TEST_ASSERT_NOT_NULL(contents);
    TEST_ASSERT_EQUAL_STRING("row0col0,row0col1\nrow1col0,row1col1", contents);
    free(contents);
}

/* -----------------------------------------------------------------------
 * Test: csv_save wraps field containing field_delim in text delimiters
 * --------------------------------------------------------------------- */
void test_csv_save_field_with_field_delim_is_quoted(void)
{
    /* Default field_delim=',' text_delim='"' */
    csv_set_field(g_buffer, 0, 0, "a,b");

    int ret = csv_save((char *)TEST_FILE, g_buffer);
    TEST_ASSERT_EQUAL_INT(0, ret);

    char *contents = read_file_contents(TEST_FILE);
    TEST_ASSERT_NOT_NULL(contents);
    /* Expected: "a,b"  (the comma triggers quoting) */
    TEST_ASSERT_EQUAL_STRING("\"a,b\"", contents);
    free(contents);
}

/* -----------------------------------------------------------------------
 * Test: csv_save wraps field containing newline in text delimiters
 * --------------------------------------------------------------------- */
void test_csv_save_field_with_newline_is_quoted(void)
{
    csv_set_field(g_buffer, 0, 0, "line1\nline2");

    int ret = csv_save((char *)TEST_FILE, g_buffer);
    TEST_ASSERT_EQUAL_INT(0, ret);

    char *contents = read_file_contents(TEST_FILE);
    TEST_ASSERT_NOT_NULL(contents);
    /* The field contains \n so it must be quoted */
    TEST_ASSERT_TRUE(contents[0] == '"');
    free(contents);
}

/* -----------------------------------------------------------------------
 * Test: csv_save wraps field containing text_delim and escapes it
 * --------------------------------------------------------------------- */
void test_csv_save_field_with_text_delim_is_escaped(void)
{
    /* Field: say "hello"  -> should become: "say ""hello""" */
    csv_set_field(g_buffer, 0, 0, "say \"hello\"");

    int ret = csv_save((char *)TEST_FILE, g_buffer);
    TEST_ASSERT_EQUAL_INT(0, ret);

    char *contents = read_file_contents(TEST_FILE);
    TEST_ASSERT_NOT_NULL(contents);
    /* The outer quotes plus escaped inner quotes */
    TEST_ASSERT_TRUE(contents[0] == '"');
    /* Verify the double-quote escape is present */
    TEST_ASSERT_NOT_NULL(strstr(contents, "\"\""));
    free(contents);
}

/* -----------------------------------------------------------------------
 * Test: csv_save does NOT quote a plain field (no special chars)
 * --------------------------------------------------------------------- */
void test_csv_save_plain_field_not_quoted(void)
{
    csv_set_field(g_buffer, 0, 0, "plain");

    int ret = csv_save((char *)TEST_FILE, g_buffer);
    TEST_ASSERT_EQUAL_INT(0, ret);

    char *contents = read_file_contents(TEST_FILE);
    TEST_ASSERT_NOT_NULL(contents);
    TEST_ASSERT_EQUAL_STRING("plain", contents);
    free(contents);
}

/* -----------------------------------------------------------------------
 * Test: csv_save last row has no trailing newline
 * --------------------------------------------------------------------- */
void test_csv_save_no_trailing_newline(void)
{
    csv_set_field(g_buffer, 0, 0, "A");
    csv_set_field(g_buffer, 1, 0, "B");

    int ret = csv_save((char *)TEST_FILE, g_buffer);
    TEST_ASSERT_EQUAL_INT(0, ret);

    char *contents = read_file_contents(TEST_FILE);
    TEST_ASSERT_NOT_NULL(contents);
    size_t len = strlen(contents);
    TEST_ASSERT_TRUE(len > 0);
    TEST_ASSERT_TRUE(contents[len - 1] != '\n');
    free(contents);
}

/* -----------------------------------------------------------------------
 * Test: csv_save with custom field delimiter (semicolon)
 * --------------------------------------------------------------------- */
void test_csv_save_custom_field_delim(void)
{
    csv_set_field_delim(g_buffer, ';');
    csv_set_field(g_buffer, 0, 0, "one");
    csv_set_field(g_buffer, 0, 1, "two");
    csv_set_field(g_buffer, 0, 2, "three");

    int ret = csv_save((char *)TEST_FILE, g_buffer);
    TEST_ASSERT_EQUAL_INT(0, ret);

    char *contents = read_file_contents(TEST_FILE);
    TEST_ASSERT_NOT_NULL(contents);
    TEST_ASSERT_EQUAL_STRING("one;two;three", contents);
    free(contents);
}

/* -----------------------------------------------------------------------
 * Test: csv_save with custom text delimiter (single quote)
 * --------------------------------------------------------------------- */
void test_csv_save_custom_text_delim(void)
{
    csv_set_text_delim(g_buffer, '\'');
    /* Field contains a comma, so it must be quoted with single quote */
    csv_set_field(g_buffer, 0, 0, "a,b");

    int ret = csv_save((char *)TEST_FILE, g_buffer);
    TEST_ASSERT_EQUAL_INT(0, ret);

    char *contents = read_file_contents(TEST_FILE);
    TEST_ASSERT_NOT_NULL(contents);
    TEST_ASSERT_EQUAL_STRING("'a,b'", contents);
    free(contents);
}

/* -----------------------------------------------------------------------
 * Test: csv_save round-trip — save then load produces same data
 * --------------------------------------------------------------------- */
void test_csv_save_round_trip(void)
{
    csv_set_field(g_buffer, 0, 0, "name");
    csv_set_field(g_buffer, 0, 1, "value");
    csv_set_field(g_buffer, 1, 0, "foo");
    csv_set_field(g_buffer, 1, 1, "42");

    int save_ret = csv_save((char *)TEST_FILE, g_buffer);
    TEST_ASSERT_EQUAL_INT(0, save_ret);

    CSV_BUFFER *loaded = csv_create_buffer();
    TEST_ASSERT_NOT_NULL(loaded);

    int load_ret = csv_load(loaded, (char *)TEST_FILE);
    TEST_ASSERT_EQUAL_INT(0, load_ret);

    TEST_ASSERT_EQUAL_INT(2, csv_get_height(loaded));
    TEST_ASSERT_EQUAL_INT(2, csv_get_width(loaded, 0));
    TEST_ASSERT_EQUAL_INT(2, csv_get_width(loaded, 1));

    char dest[64];
    csv_get_field(dest, sizeof(dest), loaded, 0, 0);
    TEST_ASSERT_EQUAL_STRING("name", dest);

    csv_get_field(dest, sizeof(dest), loaded, 0, 1);
    TEST_ASSERT_EQUAL_STRING("value", dest);

    csv_get_field(dest, sizeof(dest), loaded, 1, 0);
    TEST_ASSERT_EQUAL_STRING("foo", dest);

    csv_get_field(dest, sizeof(dest), loaded, 1, 1);
    TEST_ASSERT_EQUAL_STRING("42", dest);

    csv_destroy_buffer(loaded);
}

/* -----------------------------------------------------------------------
 * Test: csv_save round-trip with quoted field containing comma
 * --------------------------------------------------------------------- */
void test_csv_save_round_trip_quoted_field(void)
{
    csv_set_field(g_buffer, 0, 0, "Smith, John");
    csv_set_field(g_buffer, 0, 1, "Engineer");

    int save_ret = csv_save((char *)TEST_FILE, g_buffer);
    TEST_ASSERT_EQUAL_INT(0, save_ret);

    CSV_BUFFER *loaded = csv_create_buffer();
    TEST_ASSERT_NOT_NULL(loaded);

    int load_ret = csv_load(loaded, (char *)TEST_FILE);
    TEST_ASSERT_EQUAL_INT(0, load_ret);

    char dest[64];
    csv_get_field(dest, sizeof(dest), loaded, 0, 0);
    TEST_ASSERT_EQUAL_STRING("Smith, John", dest);

    csv_get_field(dest, sizeof(dest), loaded, 0, 1);
    TEST_ASSERT_EQUAL_STRING("Engineer", dest);

    csv_destroy_buffer(loaded);
}

/* -----------------------------------------------------------------------
 * Test: csv_save overwrites existing file content
 * --------------------------------------------------------------------- */
void test_csv_save_overwrites_existing_file(void)
{
    /* Write initial content */
    FILE *fp = fopen(TEST_FILE, "w");
    TEST_ASSERT_NOT_NULL(fp);
    fputs("old content that should be gone", fp);
    fclose(fp);

    csv_set_field(g_buffer, 0, 0, "new");

    int ret = csv_save((char *)TEST_FILE, g_buffer);
    TEST_ASSERT_EQUAL_INT(0, ret);

    char *contents = read_file_contents(TEST_FILE);
    TEST_ASSERT_NOT_NULL(contents);
    TEST_ASSERT_EQUAL_STRING("new", contents);
    free(contents);
}

/* -----------------------------------------------------------------------
 * Test: csv_save with three rows of varying widths
 * --------------------------------------------------------------------- */
void test_csv_save_three_rows_varying_widths(void)
{
    csv_set_field(g_buffer, 0, 0, "A");
    csv_set_field(g_buffer, 1, 0, "B");
    csv_set_field(g_buffer, 1, 1, "C");
    csv_set_field(g_buffer, 2, 0, "D");
    csv_set_field(g_buffer, 2, 1, "E");
    csv_set_field(g_buffer, 2, 2, "F");

    int ret = csv_save((char *)TEST_FILE, g_buffer);
    TEST_ASSERT_EQUAL_INT(0, ret);

    char *contents = read_file_contents(TEST_FILE);
    TEST_ASSERT_NOT_NULL(contents);
    TEST_ASSERT_EQUAL_STRING("A\nB,C\nD,E,F", contents);
    free(contents);
}

/* -----------------------------------------------------------------------
 * Test: csv_save field with only text_delim character
 * --------------------------------------------------------------------- */
void test_csv_save_field_only_text_delim(void)
{
    /* A field that is just a double-quote character */
    csv_set_field(g_buffer, 0, 0, "\"");

    int ret = csv_save((char *)TEST_FILE, g_buffer);
    TEST_ASSERT_EQUAL_INT(0, ret);

    CSV_BUFFER *loaded = csv_create_buffer();
    TEST_ASSERT_NOT_NULL(loaded);
    int load_ret = csv_load(loaded, (char *)TEST_FILE);
    TEST_ASSERT_EQUAL_INT(0, load_ret);

    char dest[64];
    csv_get_field(dest, sizeof(dest), loaded, 0, 0);
    TEST_ASSERT_EQUAL_STRING("\"", dest);

    csv_destroy_buffer(loaded);
}

/* -----------------------------------------------------------------------
 * Test: csv_save empty string field
 * --------------------------------------------------------------------- */
void test_csv_save_empty_field(void)
{
    csv_set_field(g_buffer, 0, 0, "");
    csv_set_field(g_buffer, 0, 1, "after");

    int ret = csv_save((char *)TEST_FILE, g_buffer);
    TEST_ASSERT_EQUAL_INT(0, ret);

    char *contents = read_file_contents(TEST_FILE);
    TEST_ASSERT_NOT_NULL(contents);
    TEST_ASSERT_EQUAL_STRING(",after", contents);
    free(contents);
}

/* -----------------------------------------------------------------------
 * Test: csv_save field containing both field_delim and text_delim
 * --------------------------------------------------------------------- */
void test_csv_save_field_with_both_delimiters(void)
{
    /* Field: he said "hello, world" */
    csv_set_field(g_buffer, 0, 0, "he said \"hello, world\"");

    int ret = csv_save((char *)TEST_FILE, g_buffer);
    TEST_ASSERT_EQUAL_INT(0, ret);

    CSV_BUFFER *loaded = csv_create_buffer();
    TEST_ASSERT_NOT_NULL(loaded);
    int load_ret = csv_load(loaded, (char *)TEST_FILE);
    TEST_ASSERT_EQUAL_INT(0, load_ret);

    char dest[128];
    csv_get_field(dest, sizeof(dest), loaded, 0, 0);
    TEST_ASSERT_EQUAL_STRING("he said \"hello, world\"", dest);

    csv_destroy_buffer(loaded);
}

/* -----------------------------------------------------------------------
 * main
 * --------------------------------------------------------------------- */
int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_csv_save_returns_1_on_invalid_path);
    RUN_TEST(test_csv_save_returns_0_on_empty_buffer);
    RUN_TEST(test_csv_save_creates_file);
    RUN_TEST(test_csv_save_single_field);
    RUN_TEST(test_csv_save_single_row_multiple_fields);
    RUN_TEST(test_csv_save_multiple_rows);
    RUN_TEST(test_csv_save_field_with_field_delim_is_quoted);
    RUN_TEST(test_csv_save_field_with_newline_is_quoted);
    RUN_TEST(test_csv_save_field_with_text_delim_is_escaped);
    RUN_TEST(test_csv_save_plain_field_not_quoted);
    RUN_TEST(test_csv_save_no_trailing_newline);
    RUN_TEST(test_csv_save_custom_field_delim);
    RUN_TEST(test_csv_save_custom_text_delim);
    RUN_TEST(test_csv_save_round_trip);
    RUN_TEST(test_csv_save_round_trip_quoted_field);
    RUN_TEST(test_csv_save_overwrites_existing_file);
    RUN_TEST(test_csv_save_three_rows_varying_widths);
    RUN_TEST(test_csv_save_field_only_text_delim);
    RUN_TEST(test_csv_save_empty_field);
    RUN_TEST(test_csv_save_field_with_both_delimiters);
    return UNITY_END();
}