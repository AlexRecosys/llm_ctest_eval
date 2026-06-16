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
#define TMP_NONEXIST    "/tmp/this_file_does_not_exist_xyz123.csv"

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
/* setUp / tearDown                                                      */
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
void test_csv_load_returns_1_for_nonexistent_file(void)
{
    int result = csv_load(g_buf, TMP_NONEXIST);
    TEST_ASSERT_EQUAL_INT(1, result);
}

/* csv_load returns 0 for a simple single-row CSV */
void test_csv_load_returns_0_for_valid_file(void)
{
    write_file(TMP_SIMPLE, "hello,world\n");
    int result = csv_load(g_buf, TMP_SIMPLE);
    TEST_ASSERT_EQUAL_INT(0, result);
    remove_file(TMP_SIMPLE);
}

/* After loading a single-row file the buffer has exactly 1 row */
void test_csv_load_single_row_height(void)
{
    write_file(TMP_SIMPLE, "a,b,c\n");
    csv_load(g_buf, TMP_SIMPLE);
    int height = csv_get_height(g_buf);
    TEST_ASSERT_EQUAL_INT(1, height);
    remove_file(TMP_SIMPLE);
}

/* After loading a single-row file the row has the correct width */
void test_csv_load_single_row_width(void)
{
    write_file(TMP_SIMPLE, "a,b,c\n");
    csv_load(g_buf, TMP_SIMPLE);
    int width = csv_get_width(g_buf, 0);
    TEST_ASSERT_EQUAL_INT(3, width);
    remove_file(TMP_SIMPLE);
}

/* Field content is correctly read for a simple row */
void test_csv_load_single_row_field_content(void)
{
    write_file(TMP_SIMPLE, "hello,world,foo\n");
    csv_load(g_buf, TMP_SIMPLE);

    char buf[64];

    csv_get_field(buf, sizeof(buf), g_buf, 0, 0);
    TEST_ASSERT_EQUAL_STRING("hello", buf);

    csv_get_field(buf, sizeof(buf), g_buf, 0, 1);
    TEST_ASSERT_EQUAL_STRING("world", buf);

    csv_get_field(buf, sizeof(buf), g_buf, 0, 2);
    TEST_ASSERT_EQUAL_STRING("foo", buf);

    remove_file(TMP_SIMPLE);
}

/* Multiple rows are loaded correctly */
void test_csv_load_multirow_height(void)
{
    write_file(TMP_MULTIROW, "a,b\nc,d\ne,f\n");
    csv_load(g_buf, TMP_MULTIROW);
    int height = csv_get_height(g_buf);
    TEST_ASSERT_EQUAL_INT(3, height);
    remove_file(TMP_MULTIROW);
}

/* Each row in a multi-row file has the correct width */
void test_csv_load_multirow_widths(void)
{
    write_file(TMP_MULTIROW, "a,b,c\nd,e\nf,g,h,i\n");
    csv_load(g_buf, TMP_MULTIROW);

    TEST_ASSERT_EQUAL_INT(3, csv_get_width(g_buf, 0));
    TEST_ASSERT_EQUAL_INT(2, csv_get_width(g_buf, 1));
    TEST_ASSERT_EQUAL_INT(4, csv_get_width(g_buf, 2));

    remove_file(TMP_MULTIROW);
}

/* Field content is correct across multiple rows */
void test_csv_load_multirow_field_content(void)
{
    write_file(TMP_MULTIROW, "row0col0,row0col1\nrow1col0,row1col1\n");
    csv_load(g_buf, TMP_MULTIROW);

    char buf[64];

    csv_get_field(buf, sizeof(buf), g_buf, 0, 0);
    TEST_ASSERT_EQUAL_STRING("row0col0", buf);

    csv_get_field(buf, sizeof(buf), g_buf, 0, 1);
    TEST_ASSERT_EQUAL_STRING("row0col1", buf);

    csv_get_field(buf, sizeof(buf), g_buf, 1, 0);
    TEST_ASSERT_EQUAL_STRING("row1col0", buf);

    csv_get_field(buf, sizeof(buf), g_buf, 1, 1);
    TEST_ASSERT_EQUAL_STRING("row1col1", buf);

    remove_file(TMP_MULTIROW);
}

/* A file with a single cell (no delimiter, no newline) loads correctly */
void test_csv_load_single_cell(void)
{
    write_file(TMP_ONECELL, "onlyone");
    csv_load(g_buf, TMP_ONECELL);

    TEST_ASSERT_EQUAL_INT(1, csv_get_height(g_buf));
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(g_buf, 0));

    char buf[64];
    csv_get_field(buf, sizeof(buf), g_buf, 0, 0);
    TEST_ASSERT_EQUAL_STRING("onlyone", buf);

    remove_file(TMP_ONECELL);
}

/* Quoted fields containing commas are handled correctly */
void test_csv_load_quoted_field_with_comma(void)
{
    write_file(TMP_QUOTED, "\"hello,world\",plain\n");
    csv_load(g_buf, TMP_QUOTED);

    TEST_ASSERT_EQUAL_INT(1, csv_get_height(g_buf));
    TEST_ASSERT_EQUAL_INT(2, csv_get_width(g_buf, 0));

    char buf[64];
    csv_get_field(buf, sizeof(buf), g_buf, 0, 0);
    TEST_ASSERT_EQUAL_STRING("hello,world", buf);

    csv_get_field(buf, sizeof(buf), g_buf, 0, 1);
    TEST_ASSERT_EQUAL_STRING("plain", buf);

    remove_file(TMP_QUOTED);
}

/* Quoted fields containing newlines span only one logical row */
void test_csv_load_quoted_field_with_newline(void)
{
    write_file(TMP_QUOTED, "\"line1\nline2\",after\n");
    csv_load(g_buf, TMP_QUOTED);

    /* The quoted field with embedded newline should still be one row */
    TEST_ASSERT_EQUAL_INT(1, csv_get_height(g_buf));

    char buf[128];
    csv_get_field(buf, sizeof(buf), g_buf, 0, 0);
    TEST_ASSERT_EQUAL_STRING("line1\nline2", buf);

    remove_file(TMP_QUOTED);
}

/* Empty fields (consecutive delimiters) are loaded as empty strings */
void test_csv_load_empty_fields(void)
{
    write_file(TMP_EMPTYFIELDS, "a,,c\n");
    csv_load(g_buf, TMP_EMPTYFIELDS);

    TEST_ASSERT_EQUAL_INT(1, csv_get_height(g_buf));
    TEST_ASSERT_EQUAL_INT(3, csv_get_width(g_buf, 0));

    char buf[64];

    csv_get_field(buf, sizeof(buf), g_buf, 0, 0);
    TEST_ASSERT_EQUAL_STRING("a", buf);

    /* Middle field is empty */
    int rc = csv_get_field(buf, sizeof(buf), g_buf, 0, 1);
    TEST_ASSERT_TRUE(rc == 2 || strcmp(buf, "") == 0);

    csv_get_field(buf, sizeof(buf), g_buf, 0, 2);
    TEST_ASSERT_EQUAL_STRING("c", buf);

    remove_file(TMP_EMPTYFIELDS);
}

/* A single-column multi-row file loads correctly */
void test_csv_load_single_column_multirow(void)
{
    write_file(TMP_SINGLECOL, "alpha\nbeta\ngamma\n");
    csv_load(g_buf, TMP_SINGLECOL);

    TEST_ASSERT_EQUAL_INT(3, csv_get_height(g_buf));

    char buf[64];

    csv_get_field(buf, sizeof(buf), g_buf, 0, 0);
    TEST_ASSERT_EQUAL_STRING("alpha", buf);

    csv_get_field(buf, sizeof(buf), g_buf, 1, 0);
    TEST_ASSERT_EQUAL_STRING("beta", buf);

    csv_get_field(buf, sizeof(buf), g_buf, 2, 0);
    TEST_ASSERT_EQUAL_STRING("gamma", buf);

    remove_file(TMP_SINGLECOL);
}

/* A trailing delimiter at end of row creates an extra empty field */
void test_csv_load_trailing_delimiter_creates_empty_field(void)
{
    write_file(TMP_TRAILING, "a,b,\n");
    csv_load(g_buf, TMP_TRAILING);

    TEST_ASSERT_EQUAL_INT(1, csv_get_height(g_buf));
    /* Trailing comma means 3 fields: "a", "b", "" */
    int width = csv_get_width(g_buf, 0);
    TEST_ASSERT_EQUAL_INT(3, width);

    remove_file(TMP_TRAILING);
}

/* Loading a file with no newline at EOF still produces a row */
void test_csv_load_no_trailing_newline(void)
{
    write_file(TMP_SIMPLE, "x,y,z");
    csv_load(g_buf, TMP_SIMPLE);

    TEST_ASSERT_EQUAL_INT(1, csv_get_height(g_buf));
    TEST_ASSERT_EQUAL_INT(3, csv_get_width(g_buf, 0));

    char buf[64];
    csv_get_field(buf, sizeof(buf), g_buf, 0, 2);
    TEST_ASSERT_EQUAL_STRING("z", buf);

    remove_file(TMP_SIMPLE);
}

/* Buffer state is unchanged (still 0 rows) after a failed load */
void test_csv_load_buffer_unchanged_on_file_not_found(void)
{
    int result = csv_load(g_buf, TMP_NONEXIST);
    TEST_ASSERT_EQUAL_INT(1, result);
    TEST_ASSERT_EQUAL_INT(0, csv_get_height(g_buf));
}

/* Loading a file with numeric strings preserves them as text */
void test_csv_load_numeric_strings(void)
{
    write_file(TMP_SIMPLE, "1,2,3\n4,5,6\n");
    csv_load(g_buf, TMP_SIMPLE);

    char buf[32];

    csv_get_field(buf, sizeof(buf), g_buf, 0, 0);
    TEST_ASSERT_EQUAL_STRING("1", buf);

    csv_get_field(buf, sizeof(buf), g_buf, 1, 2);
    TEST_ASSERT_EQUAL_STRING("6", buf);

    remove_file(TMP_SIMPLE);
}

/* A file with many rows loads the correct number of rows */
void test_csv_load_many_rows(void)
{
    FILE *fp = fopen(TMP_MULTIROW, "w");
    TEST_ASSERT_NOT_NULL(fp);
    for (int r = 0; r < 50; r++) {
        fprintf(fp, "cell%d_0,cell%d_1,cell%d_2\n", r, r, r);
    }
    fclose(fp);

    csv_load(g_buf, TMP_MULTIROW);
    TEST_ASSERT_EQUAL_INT(50, csv_get_height(g_buf));

    remove_file(TMP_MULTIROW);
}

/* After loading, the buffer's delimiters are unchanged */
void test_csv_load_does_not_alter_delimiters(void)
{
    write_file(TMP_SIMPLE, "a,b\n");
    char orig_field = g_buf->field_delim;
    char orig_text  = g_buf->text_delim;

    csv_load(g_buf, TMP_SIMPLE);

    TEST_ASSERT_EQUAL_INT((int)orig_field, (int)g_buf->field_delim);
    TEST_ASSERT_EQUAL_INT((int)orig_text,  (int)g_buf->text_delim);

    remove_file(TMP_SIMPLE);
}

/* ------------------------------------------------------------------ */
/* main                                                                 */
/* ------------------------------------------------------------------ */

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_csv_load_returns_1_for_nonexistent_file);
    RUN_TEST(test_csv_load_returns_0_for_valid_file);
    RUN_TEST(test_csv_load_single_row_height);
    RUN_TEST(test_csv_load_single_row_width);
    RUN_TEST(test_csv_load_single_row_field_content);
    RUN_TEST(test_csv_load_multirow_height);
    RUN_TEST(test_csv_load_multirow_widths);
    RUN_TEST(test_csv_load_multirow_field_content);
    RUN_TEST(test_csv_load_single_cell);
    RUN_TEST(test_csv_load_quoted_field_with_comma);
    RUN_TEST(test_csv_load_quoted_field_with_newline);
    RUN_TEST(test_csv_load_empty_fields);
    RUN_TEST(test_csv_load_single_column_multirow);
    RUN_TEST(test_csv_load_trailing_delimiter_creates_empty_field);
    RUN_TEST(test_csv_load_no_trailing_newline);
    RUN_TEST(test_csv_load_buffer_unchanged_on_file_not_found);
    RUN_TEST(test_csv_load_numeric_strings);
    RUN_TEST(test_csv_load_many_rows);
    RUN_TEST(test_csv_load_does_not_alter_delimiters);
    return UNITY_END();
}