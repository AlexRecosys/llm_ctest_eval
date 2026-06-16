#include "csv.c"
#include "unity.h"
#include "csv.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* File-scope fixtures                                                  */
/* ------------------------------------------------------------------ */

static CSV_BUFFER *g_buf = NULL;

/* Temporary CSV file names used across tests */
#define TMP_SIMPLE      "/tmp/test_csv_simple.csv"
#define TMP_MULTIROW    "/tmp/test_csv_multirow.csv"
#define TMP_EMPTY       "/tmp/test_csv_empty.csv"
#define TMP_QUOTED      "/tmp/test_csv_quoted.csv"
#define TMP_TRAILING    "/tmp/test_csv_trailing.csv"
#define TMP_SINGLECOL   "/tmp/test_csv_singlecol.csv"
#define TMP_ONECELL     "/tmp/test_csv_onecell.csv"
#define TMP_EMPTYFIELDS "/tmp/test_csv_emptyfields.csv"
#define TMP_NEWLINEQUOT "/tmp/test_csv_newlinequot.csv"

/* ------------------------------------------------------------------ */
/* Helper functions                                                     */
/* ------------------------------------------------------------------ */

static void write_file(const char *path, const char *content)
{
    FILE *fp = fopen(path, "w");
    if (fp == NULL) {
        TEST_FAIL_MESSAGE("Helper: could not create temporary test file");
        return;
    }
    fputs(content, fp);
    fclose(fp);
}

static void remove_file(const char *path)
{
    remove(path);
}

/* ------------------------------------------------------------------ */
/* setUp / tearDown                                                     */
/* ------------------------------------------------------------------ */

void setUp(void)
{
    g_buf = csv_create_buffer();
    TEST_ASSERT_NOT_NULL_MESSAGE(g_buf, "csv_create_buffer returned NULL in setUp");
}

void tearDown(void)
{
    if (g_buf != NULL) {
        csv_destroy_buffer(g_buf);
        g_buf = NULL;
    }
}

/* ------------------------------------------------------------------ */
/* Test cases                                                           */
/* ------------------------------------------------------------------ */

/* csv_load returns 1 when the file does not exist */
void test_csv_load_returns_1_for_missing_file(void)
{
    int ret = csv_load(g_buf, "/tmp/this_file_should_not_exist_xyz123.csv");
    TEST_ASSERT_EQUAL_INT(1, ret);
}

/* csv_load returns 0 for a simple single-row, single-field file */
void test_csv_load_returns_0_for_valid_file(void)
{
    write_file(TMP_ONECELL, "hello\n");
    int ret = csv_load(g_buf, TMP_ONECELL);
    TEST_ASSERT_EQUAL_INT(0, ret);
    remove_file(TMP_ONECELL);
}

/* Buffer has exactly 1 row after loading a single-line file */
void test_csv_load_single_row_height(void)
{
    write_file(TMP_ONECELL, "hello\n");
    csv_load(g_buf, TMP_ONECELL);
    TEST_ASSERT_EQUAL_INT(1, csv_get_height(g_buf));
    remove_file(TMP_ONECELL);
}

/* Buffer has exactly 1 field in row 0 after loading a single-cell file */
void test_csv_load_single_row_width(void)
{
    write_file(TMP_ONECELL, "hello\n");
    csv_load(g_buf, TMP_ONECELL);
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(g_buf, 0));
    remove_file(TMP_ONECELL);
}

/* Field text is correct after loading a single-cell file */
void test_csv_load_single_cell_content(void)
{
    write_file(TMP_ONECELL, "hello\n");
    csv_load(g_buf, TMP_ONECELL);

    char dest[64];
    memset(dest, 0, sizeof(dest));
    int ret = csv_get_field(dest, sizeof(dest), g_buf, 0, 0);
    TEST_ASSERT_TRUE_MESSAGE(ret == 0 || ret == 1, "csv_get_field unexpected return");
    TEST_ASSERT_EQUAL_STRING("hello", dest);
    remove_file(TMP_ONECELL);
}

/* Simple two-field, single-row CSV */
void test_csv_load_simple_two_fields(void)
{
    write_file(TMP_SIMPLE, "foo,bar\n");
    csv_load(g_buf, TMP_SIMPLE);

    TEST_ASSERT_EQUAL_INT(1, csv_get_height(g_buf));
    TEST_ASSERT_EQUAL_INT(2, csv_get_width(g_buf, 0));

    char dest[64];
    memset(dest, 0, sizeof(dest));
    csv_get_field(dest, sizeof(dest), g_buf, 0, 0);
    TEST_ASSERT_EQUAL_STRING("foo", dest);

    memset(dest, 0, sizeof(dest));
    csv_get_field(dest, sizeof(dest), g_buf, 0, 1);
    TEST_ASSERT_EQUAL_STRING("bar", dest);

    remove_file(TMP_SIMPLE);
}

/* Multi-row CSV: correct height */
void test_csv_load_multirow_height(void)
{
    write_file(TMP_MULTIROW, "a,b,c\n1,2,3\nx,y,z\n");
    csv_load(g_buf, TMP_MULTIROW);
    TEST_ASSERT_EQUAL_INT(3, csv_get_height(g_buf));
    remove_file(TMP_MULTIROW);
}

/* Multi-row CSV: correct width per row */
void test_csv_load_multirow_width(void)
{
    write_file(TMP_MULTIROW, "a,b,c\n1,2,3\nx,y,z\n");
    csv_load(g_buf, TMP_MULTIROW);

    TEST_ASSERT_EQUAL_INT(3, csv_get_width(g_buf, 0));
    TEST_ASSERT_EQUAL_INT(3, csv_get_width(g_buf, 1));
    TEST_ASSERT_EQUAL_INT(3, csv_get_width(g_buf, 2));
    remove_file(TMP_MULTIROW);
}

/* Multi-row CSV: spot-check field content */
void test_csv_load_multirow_field_content(void)
{
    write_file(TMP_MULTIROW, "a,b,c\n1,2,3\nx,y,z\n");
    csv_load(g_buf, TMP_MULTIROW);

    char dest[64];

    memset(dest, 0, sizeof(dest));
    csv_get_field(dest, sizeof(dest), g_buf, 0, 0);
    TEST_ASSERT_EQUAL_STRING("a", dest);

    memset(dest, 0, sizeof(dest));
    csv_get_field(dest, sizeof(dest), g_buf, 1, 1);
    TEST_ASSERT_EQUAL_STRING("2", dest);

    memset(dest, 0, sizeof(dest));
    csv_get_field(dest, sizeof(dest), g_buf, 2, 2);
    TEST_ASSERT_EQUAL_STRING("z", dest);

    remove_file(TMP_MULTIROW);
}

/* Empty file: height should be 0 */
void test_csv_load_empty_file_height(void)
{
    write_file(TMP_EMPTY, "");
    int ret = csv_load(g_buf, TMP_EMPTY);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(0, csv_get_height(g_buf));
    remove_file(TMP_EMPTY);
}

/* Quoted field containing a comma */
void test_csv_load_quoted_field_with_comma(void)
{
    write_file(TMP_QUOTED, "\"hello,world\",end\n");
    csv_load(g_buf, TMP_QUOTED);

    TEST_ASSERT_EQUAL_INT(1, csv_get_height(g_buf));
    TEST_ASSERT_EQUAL_INT(2, csv_get_width(g_buf, 0));

    char dest[64];
    memset(dest, 0, sizeof(dest));
    csv_get_field(dest, sizeof(dest), g_buf, 0, 0);
    TEST_ASSERT_EQUAL_STRING("hello,world", dest);

    memset(dest, 0, sizeof(dest));
    csv_get_field(dest, sizeof(dest), g_buf, 0, 1);
    TEST_ASSERT_EQUAL_STRING("end", dest);

    remove_file(TMP_QUOTED);
}

/* Quoted field containing a newline — should be treated as one field */
void test_csv_load_quoted_field_with_newline(void)
{
    write_file(TMP_NEWLINEQUOT, "\"line1\nline2\",after\n");
    csv_load(g_buf, TMP_NEWLINEQUOT);

    /* The quoted field spans a newline, so there should be 1 row */
    TEST_ASSERT_EQUAL_INT(1, csv_get_height(g_buf));
    TEST_ASSERT_EQUAL_INT(2, csv_get_width(g_buf, 0));

    char dest[128];
    memset(dest, 0, sizeof(dest));
    csv_get_field(dest, sizeof(dest), g_buf, 0, 0);
    TEST_ASSERT_EQUAL_STRING("line1\nline2", dest);

    remove_file(TMP_NEWLINEQUOT);
}

/* Empty fields (consecutive delimiters) */
void test_csv_load_empty_fields(void)
{
    write_file(TMP_EMPTYFIELDS, "a,,c\n");
    csv_load(g_buf, TMP_EMPTYFIELDS);

    TEST_ASSERT_EQUAL_INT(1, csv_get_height(g_buf));
    TEST_ASSERT_EQUAL_INT(3, csv_get_width(g_buf, 0));

    char dest[64];

    memset(dest, 0, sizeof(dest));
    csv_get_field(dest, sizeof(dest), g_buf, 0, 0);
    TEST_ASSERT_EQUAL_STRING("a", dest);

    /* Middle field is empty */
    int ret = csv_get_field(dest, sizeof(dest), g_buf, 0, 1);
    TEST_ASSERT_EQUAL_INT(2, ret); /* 2 = empty/does not exist */

    memset(dest, 0, sizeof(dest));
    csv_get_field(dest, sizeof(dest), g_buf, 0, 2);
    TEST_ASSERT_EQUAL_STRING("c", dest);

    remove_file(TMP_EMPTYFIELDS);
}

/* Trailing delimiter creates an extra empty field */
void test_csv_load_trailing_delimiter(void)
{
    write_file(TMP_TRAILING, "a,b,\n");
    csv_load(g_buf, TMP_TRAILING);

    TEST_ASSERT_EQUAL_INT(1, csv_get_height(g_buf));
    /* Trailing comma => 3 fields: "a", "b", "" */
    TEST_ASSERT_EQUAL_INT(3, csv_get_width(g_buf, 0));

    remove_file(TMP_TRAILING);
}

/* Single-column multi-row file */
void test_csv_load_single_column_multirow(void)
{
    write_file(TMP_SINGLECOL, "alpha\nbeta\ngamma\n");
    csv_load(g_buf, TMP_SINGLECOL);

    TEST_ASSERT_EQUAL_INT(3, csv_get_height(g_buf));
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(g_buf, 0));
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(g_buf, 1));
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(g_buf, 2));

    char dest[64];

    memset(dest, 0, sizeof(dest));
    csv_get_field(dest, sizeof(dest), g_buf, 0, 0);
    TEST_ASSERT_EQUAL_STRING("alpha", dest);

    memset(dest, 0, sizeof(dest));
    csv_get_field(dest, sizeof(dest), g_buf, 1, 0);
    TEST_ASSERT_EQUAL_STRING("beta", dest);

    memset(dest, 0, sizeof(dest));
    csv_get_field(dest, sizeof(dest), g_buf, 2, 0);
    TEST_ASSERT_EQUAL_STRING("gamma", dest);

    remove_file(TMP_SINGLECOL);
}

/* Buffer is not modified when file is not found */
void test_csv_load_buffer_unchanged_on_missing_file(void)
{
    int ret = csv_load(g_buf, "/tmp/nonexistent_file_abc987.csv");
    TEST_ASSERT_EQUAL_INT(1, ret);
    TEST_ASSERT_EQUAL_INT(0, csv_get_height(g_buf));
}

/* Custom field delimiter (semicolon) */
void test_csv_load_custom_field_delim(void)
{
    write_file(TMP_SIMPLE, "one;two;three\n");
    csv_set_field_delim(g_buf, ';');
    csv_load(g_buf, TMP_SIMPLE);

    TEST_ASSERT_EQUAL_INT(1, csv_get_height(g_buf));
    TEST_ASSERT_EQUAL_INT(3, csv_get_width(g_buf, 0));

    char dest[64];
    memset(dest, 0, sizeof(dest));
    csv_get_field(dest, sizeof(dest), g_buf, 0, 0);
    TEST_ASSERT_EQUAL_STRING("one", dest);

    memset(dest, 0, sizeof(dest));
    csv_get_field(dest, sizeof(dest), g_buf, 0, 2);
    TEST_ASSERT_EQUAL_STRING("three", dest);

    remove_file(TMP_SIMPLE);
}

/* File without trailing newline */
void test_csv_load_no_trailing_newline(void)
{
    write_file(TMP_ONECELL, "hello");
    int ret = csv_load(g_buf, TMP_ONECELL);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(1, csv_get_height(g_buf));
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(g_buf, 0));

    char dest[64];
    memset(dest, 0, sizeof(dest));
    csv_get_field(dest, sizeof(dest), g_buf, 0, 0);
    TEST_ASSERT_EQUAL_STRING("hello", dest);

    remove_file(TMP_ONECELL);
}

/* ------------------------------------------------------------------ */
/* main                                                                 */
/* ------------------------------------------------------------------ */

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_csv_load_returns_1_for_missing_file);
    RUN_TEST(test_csv_load_returns_0_for_valid_file);
    RUN_TEST(test_csv_load_single_row_height);
    RUN_TEST(test_csv_load_single_row_width);
    RUN_TEST(test_csv_load_single_cell_content);
    RUN_TEST(test_csv_load_simple_two_fields);
    RUN_TEST(test_csv_load_multirow_height);
    RUN_TEST(test_csv_load_multirow_width);
    RUN_TEST(test_csv_load_multirow_field_content);
    RUN_TEST(test_csv_load_empty_file_height);
    RUN_TEST(test_csv_load_quoted_field_with_comma);
    RUN_TEST(test_csv_load_quoted_field_with_newline);
    RUN_TEST(test_csv_load_empty_fields);
    RUN_TEST(test_csv_load_trailing_delimiter);
    RUN_TEST(test_csv_load_single_column_multirow);
    RUN_TEST(test_csv_load_buffer_unchanged_on_missing_file);
    RUN_TEST(test_csv_load_custom_field_delim);
    RUN_TEST(test_csv_load_no_trailing_newline);
    return UNITY_END();
}