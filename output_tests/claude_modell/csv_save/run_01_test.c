#include "csv.c"
#include "unity.h"
#include "csv.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* -------------------------------------------------------------------------
 * File-scope fixtures
 * ---------------------------------------------------------------------- */
static CSV_BUFFER *g_buf = NULL;
static const char *TEST_FILE = "test_csv_save_output.csv";

/* -------------------------------------------------------------------------
 * Helper functions
 * ---------------------------------------------------------------------- */
static void build_buffer_simple(CSV_BUFFER *buf,
                                 int rows, int cols,
                                 const char *values[])
{
    int idx = 0;
    for (int r = 0; r < rows; r++) {
        append_row(buf);
        for (int c = 0; c < cols; c++) {
            append_field(buf, r);
            set_field(buf->field[r][c], (char *)values[idx++]);
        }
    }
}

/* Read the entire file into a heap-allocated string.
 * Caller must free the returned pointer. */
static char *read_file_contents(const char *path)
{
    FILE *fp = fopen(path, "r");
    if (!fp) return NULL;
    fseek(fp, 0, SEEK_END);
    long sz = ftell(fp);
    rewind(fp);
    char *buf = (char *)malloc((size_t)sz + 1);
    if (!buf) { fclose(fp); return NULL; }
    size_t n = fread(buf, 1, (size_t)sz, fp);
    buf[n] = '\0';
    fclose(fp);
    return buf;
}

static void remove_test_file(void)
{
    remove(TEST_FILE);
}

/* -------------------------------------------------------------------------
 * setUp / tearDown
 * ---------------------------------------------------------------------- */
void setUp(void)
{
    g_buf = csv_create_buffer();
    remove_test_file();
}

void tearDown(void)
{
    if (g_buf) {
        csv_destroy_buffer(g_buf);
        g_buf = NULL;
    }
    remove_test_file();
}

/* -------------------------------------------------------------------------
 * Test cases
 * ---------------------------------------------------------------------- */

/* csv_save returns 1 when the file path is invalid / unwritable */
void test_csv_save_returns_1_on_bad_path(void)
{
    int ret = csv_save("/nonexistent_dir/no_file.csv", g_buf);
    TEST_ASSERT_EQUAL_INT(1, ret);
}

/* csv_save returns 0 on success */
void test_csv_save_returns_0_on_success(void)
{
    const char *vals[] = { "hello", "world" };
    build_buffer_simple(g_buf, 1, 2, vals);
    int ret = csv_save((char *)TEST_FILE, g_buf);
    TEST_ASSERT_EQUAL_INT(0, ret);
}

/* csv_save creates the output file */
void test_csv_save_creates_file(void)
{
    const char *vals[] = { "a" };
    build_buffer_simple(g_buf, 1, 1, vals);
    csv_save((char *)TEST_FILE, g_buf);
    FILE *fp = fopen(TEST_FILE, "r");
    TEST_ASSERT_NOT_NULL(fp);
    if (fp) fclose(fp);
}

/* Single cell, no special characters */
void test_csv_save_single_cell(void)
{
    const char *vals[] = { "hello" };
    build_buffer_simple(g_buf, 1, 1, vals);
    csv_save((char *)TEST_FILE, g_buf);

    char *contents = read_file_contents(TEST_FILE);
    TEST_ASSERT_NOT_NULL(contents);
    TEST_ASSERT_EQUAL_STRING("hello", contents);
    free(contents);
}

/* Single row, multiple columns — fields separated by field_delim */
void test_csv_save_single_row_multiple_cols(void)
{
    const char *vals[] = { "one", "two", "three" };
    build_buffer_simple(g_buf, 1, 3, vals);
    csv_save((char *)TEST_FILE, g_buf);

    char *contents = read_file_contents(TEST_FILE);
    TEST_ASSERT_NOT_NULL(contents);
    TEST_ASSERT_EQUAL_STRING("one,two,three", contents);
    free(contents);
}

/* Multiple rows — rows separated by newline, no trailing newline */
void test_csv_save_multiple_rows(void)
{
    const char *vals[] = { "a", "b", "c", "d" };
    build_buffer_simple(g_buf, 2, 2, vals);
    csv_save((char *)TEST_FILE, g_buf);

    char *contents = read_file_contents(TEST_FILE);
    TEST_ASSERT_NOT_NULL(contents);
    TEST_ASSERT_EQUAL_STRING("a,b\nc,d", contents);
    free(contents);
}

/* Field containing field_delim must be text-delimited */
void test_csv_save_field_with_field_delim_is_quoted(void)
{
    /* default field_delim = ',' */
    const char *vals[] = { "he,llo" };
    build_buffer_simple(g_buf, 1, 1, vals);
    csv_save((char *)TEST_FILE, g_buf);

    char *contents = read_file_contents(TEST_FILE);
    TEST_ASSERT_NOT_NULL(contents);
    /* The field contains a comma so it must be wrapped in text_delim ('"') */
    TEST_ASSERT_TRUE(contents[0] == '"');
    free(contents);
}

/* Field containing text_delim must be text-delimited and the inner
 * text_delim must be escaped (doubled) */
void test_csv_save_field_with_text_delim_is_escaped(void)
{
    /* default text_delim = '"' */
    /* We set the field text to: say "hi"
     * Expected output: "say ""hi"""
     * The function writes text[0..length-2] (length includes '\0'),
     * so set_field stores "say \"hi\"" with length = strlen+1.
     */
    const char *vals[] = { "say \"hi\"" };
    build_buffer_simple(g_buf, 1, 1, vals);
    csv_save((char *)TEST_FILE, g_buf);

    char *contents = read_file_contents(TEST_FILE);
    TEST_ASSERT_NOT_NULL(contents);
    /* Must start and end with text_delim */
    TEST_ASSERT_TRUE(contents[0] == '"');
    /* Must contain doubled quote for the inner quote */
    TEST_ASSERT_NOT_NULL(strstr(contents, "\"\""));
    free(contents);
}

/* Field containing newline must be text-delimited */
void test_csv_save_field_with_newline_is_quoted(void)
{
    const char *vals[] = { "line1\nline2" };
    build_buffer_simple(g_buf, 1, 1, vals);
    csv_save((char *)TEST_FILE, g_buf);

    char *contents = read_file_contents(TEST_FILE);
    TEST_ASSERT_NOT_NULL(contents);
    TEST_ASSERT_TRUE(contents[0] == '"');
    free(contents);
}

/* Empty field produces empty string between delimiters */
void test_csv_save_empty_field(void)
{
    const char *vals[] = { "a", "", "b" };
    build_buffer_simple(g_buf, 1, 3, vals);
    csv_save((char *)TEST_FILE, g_buf);

    char *contents = read_file_contents(TEST_FILE);
    TEST_ASSERT_NOT_NULL(contents);
    TEST_ASSERT_EQUAL_STRING("a,,b", contents);
    free(contents);
}

/* Verify no trailing newline after the last row */
void test_csv_save_no_trailing_newline(void)
{
    const char *vals[] = { "x", "y", "z", "w" };
    build_buffer_simple(g_buf, 2, 2, vals);
    csv_save((char *)TEST_FILE, g_buf);

    char *contents = read_file_contents(TEST_FILE);
    TEST_ASSERT_NOT_NULL(contents);
    size_t len = strlen(contents);
    TEST_ASSERT_TRUE(len > 0);
    TEST_ASSERT_TRUE(contents[len - 1] != '\n');
    free(contents);
}

/* Verify the file can be re-loaded and matches the original buffer */
void test_csv_save_roundtrip(void)
{
    const char *vals[] = { "alpha", "beta", "gamma", "delta" };
    build_buffer_simple(g_buf, 2, 2, vals);
    int ret = csv_save((char *)TEST_FILE, g_buf);
    TEST_ASSERT_EQUAL_INT(0, ret);

    CSV_BUFFER *loaded = csv_create_buffer();
    TEST_ASSERT_NOT_NULL(loaded);
    int load_ret = csv_load(loaded, (char *)TEST_FILE);
    TEST_ASSERT_EQUAL_INT(0, load_ret);

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

/* Custom field delimiter (semicolon) */
void test_csv_save_custom_field_delim(void)
{
    csv_set_field_delim(g_buf, ';');
    const char *vals[] = { "one", "two", "three" };
    build_buffer_simple(g_buf, 1, 3, vals);
    csv_save((char *)TEST_FILE, g_buf);

    char *contents = read_file_contents(TEST_FILE);
    TEST_ASSERT_NOT_NULL(contents);
    TEST_ASSERT_EQUAL_STRING("one;two;three", contents);
    free(contents);
}

/* Custom text delimiter (single quote) */
void test_csv_save_custom_text_delim(void)
{
    csv_set_text_delim(g_buf, '\'');
    csv_set_field_delim(g_buf, ',');
    /* Field contains a comma so it must be quoted with single quote */
    const char *vals[] = { "a,b" };
    build_buffer_simple(g_buf, 1, 1, vals);
    csv_save((char *)TEST_FILE, g_buf);

    char *contents = read_file_contents(TEST_FILE);
    TEST_ASSERT_NOT_NULL(contents);
    TEST_ASSERT_TRUE(contents[0] == '\'');
    free(contents);
}

/* Three rows, varying widths — check newlines between rows */
void test_csv_save_three_rows(void)
{
    /* Row 0: a,b  Row 1: c  Row 2: d,e,f */
    append_row(g_buf);
    append_field(g_buf, 0); set_field(g_buf->field[0][0], "a");
    append_field(g_buf, 0); set_field(g_buf->field[0][1], "b");

    append_row(g_buf);
    append_field(g_buf, 1); set_field(g_buf->field[1][0], "c");

    append_row(g_buf);
    append_field(g_buf, 2); set_field(g_buf->field[2][0], "d");
    append_field(g_buf, 2); set_field(g_buf->field[2][1], "e");
    append_field(g_buf, 2); set_field(g_buf->field[2][2], "f");

    csv_save((char *)TEST_FILE, g_buf);

    char *contents = read_file_contents(TEST_FILE);
    TEST_ASSERT_NOT_NULL(contents);
    TEST_ASSERT_EQUAL_STRING("a,b\nc\nd,e,f", contents);
    free(contents);
}

/* Field that contains only the text_delim character */
void test_csv_save_field_only_text_delim(void)
{
    /* Field text is just a double-quote character */
    const char *vals[] = { "\"" };
    build_buffer_simple(g_buf, 1, 1, vals);
    csv_save((char *)TEST_FILE, g_buf);

    char *contents = read_file_contents(TEST_FILE);
    TEST_ASSERT_NOT_NULL(contents);
    /* Must start with text_delim */
    TEST_ASSERT_TRUE(contents[0] == '"');
    free(contents);
}

/* Saving an empty buffer (0 rows) produces an empty file */
void test_csv_save_empty_buffer(void)
{
    /* g_buf has 0 rows by default */
    int ret = csv_save((char *)TEST_FILE, g_buf);
    TEST_ASSERT_EQUAL_INT(0, ret);

    char *contents = read_file_contents(TEST_FILE);
    TEST_ASSERT_NOT_NULL(contents);
    TEST_ASSERT_EQUAL_STRING("", contents);
    free(contents);
}

/* Overwrite: saving twice to the same file replaces the content */
void test_csv_save_overwrites_existing_file(void)
{
    const char *vals1[] = { "first" };
    build_buffer_simple(g_buf, 1, 1, vals1);
    csv_save((char *)TEST_FILE, g_buf);

    /* Destroy and rebuild with different content */
    csv_destroy_buffer(g_buf);
    g_buf = csv_create_buffer();
    const char *vals2[] = { "second", "value" };
    build_buffer_simple(g_buf, 1, 2, vals2);
    csv_save((char *)TEST_FILE, g_buf);

    char *contents = read_file_contents(TEST_FILE);
    TEST_ASSERT_NOT_NULL(contents);
    TEST_ASSERT_EQUAL_STRING("second,value", contents);
    free(contents);
}

/* Roundtrip with a field that contains a comma (quoted field) */
void test_csv_save_roundtrip_quoted_field(void)
{
    const char *vals[] = { "hello,world", "plain" };
    build_buffer_simple(g_buf, 1, 2, vals);
    csv_save((char *)TEST_FILE, g_buf);

    CSV_BUFFER *loaded = csv_create_buffer();
    TEST_ASSERT_NOT_NULL(loaded);
    csv_load(loaded, (char *)TEST_FILE);

    char dest[64];
    csv_get_field(dest, sizeof(dest), loaded, 0, 0);
    TEST_ASSERT_EQUAL_STRING("hello,world", dest);
    csv_get_field(dest, sizeof(dest), loaded, 0, 1);
    TEST_ASSERT_EQUAL_STRING("plain", dest);

    csv_destroy_buffer(loaded);
}

/* -------------------------------------------------------------------------
 * main
 * ---------------------------------------------------------------------- */
int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_csv_save_returns_1_on_bad_path);
    RUN_TEST(test_csv_save_returns_0_on_success);
    RUN_TEST(test_csv_save_creates_file);
    RUN_TEST(test_csv_save_single_cell);
    RUN_TEST(test_csv_save_single_row_multiple_cols);
    RUN_TEST(test_csv_save_multiple_rows);
    RUN_TEST(test_csv_save_field_with_field_delim_is_quoted);
    RUN_TEST(test_csv_save_field_with_text_delim_is_escaped);
    RUN_TEST(test_csv_save_field_with_newline_is_quoted);
    RUN_TEST(test_csv_save_empty_field);
    RUN_TEST(test_csv_save_no_trailing_newline);
    RUN_TEST(test_csv_save_roundtrip);
    RUN_TEST(test_csv_save_custom_field_delim);
    RUN_TEST(test_csv_save_custom_text_delim);
    RUN_TEST(test_csv_save_three_rows);
    RUN_TEST(test_csv_save_field_only_text_delim);
    RUN_TEST(test_csv_save_empty_buffer);
    RUN_TEST(test_csv_save_overwrites_existing_file);
    RUN_TEST(test_csv_save_roundtrip_quoted_field);
    return UNITY_END();
}