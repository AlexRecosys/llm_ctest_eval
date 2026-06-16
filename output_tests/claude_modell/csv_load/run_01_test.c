#include "csv.c"
#include "unity.h"
#include "csv.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* File-scope fixtures */
static CSV_BUFFER *g_buffer = NULL;
static const char *TEMP_FILE = "test_csv_load_temp.csv";

/* Helper: write a string to a temp file */
static void write_temp_file(const char *content)
{
    FILE *fp = fopen(TEMP_FILE, "w");
    if (fp == NULL) {
        TEST_FAIL_MESSAGE("Could not create temporary test CSV file");
        return;
    }
    fputs(content, fp);
    fclose(fp);
}

/* Helper: remove temp file */
static void remove_temp_file(void)
{
    remove(TEMP_FILE);
}

void setUp(void)
{
    g_buffer = csv_create_buffer();
    if (g_buffer == NULL) {
        TEST_FAIL_MESSAGE("csv_create_buffer returned NULL in setUp");
    }
}

void tearDown(void)
{
    if (g_buffer != NULL) {
        csv_destroy_buffer(g_buffer);
        g_buffer = NULL;
    }
    remove_temp_file();
}

/* ------------------------------------------------------------------ */
/* Test cases                                                           */
/* ------------------------------------------------------------------ */

/* csv_load returns 1 when the file does not exist */
void test_csv_load_returns_1_for_missing_file(void)
{
    remove("nonexistent_file_xyz.csv");
    int result = csv_load(g_buffer, "nonexistent_file_xyz.csv");
    TEST_ASSERT_EQUAL_INT(1, result);
}

/* csv_load returns 0 for an empty file */
void test_csv_load_returns_0_for_empty_file(void)
{
    write_temp_file("");
    int result = csv_load(g_buffer, (char *)TEMP_FILE);
    TEST_ASSERT_EQUAL_INT(0, result);
}

/* csv_load on an empty file leaves the buffer with 0 rows */
void test_csv_load_empty_file_zero_rows(void)
{
    write_temp_file("");
    csv_load(g_buffer, (char *)TEMP_FILE);
    TEST_ASSERT_EQUAL_INT(0, csv_get_height(g_buffer));
}

/* csv_load a single field, single row */
void test_csv_load_single_field(void)
{
    write_temp_file("hello");
    int result = csv_load(g_buffer, (char *)TEMP_FILE);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_INT(1, csv_get_height(g_buffer));
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(g_buffer, 0));

    char buf[64];
    csv_get_field(buf, sizeof(buf), g_buffer, 0, 0);
    TEST_ASSERT_EQUAL_STRING("hello", buf);
}

/* csv_load a single row with multiple fields */
void test_csv_load_single_row_multiple_fields(void)
{
    write_temp_file("a,b,c");
    int result = csv_load(g_buffer, (char *)TEMP_FILE);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_INT(1, csv_get_height(g_buffer));
    TEST_ASSERT_EQUAL_INT(3, csv_get_width(g_buffer, 0));

    char buf[64];
    csv_get_field(buf, sizeof(buf), g_buffer, 0, 0);
    TEST_ASSERT_EQUAL_STRING("a", buf);
    csv_get_field(buf, sizeof(buf), g_buffer, 0, 1);
    TEST_ASSERT_EQUAL_STRING("b", buf);
    csv_get_field(buf, sizeof(buf), g_buffer, 0, 2);
    TEST_ASSERT_EQUAL_STRING("c", buf);
}

/* csv_load multiple rows */
void test_csv_load_multiple_rows(void)
{
    write_temp_file("row1col1,row1col2\nrow2col1,row2col2\n");
    int result = csv_load(g_buffer, (char *)TEMP_FILE);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_INT(2, csv_get_height(g_buffer));
}

/* csv_load multiple rows — correct field values */
void test_csv_load_multiple_rows_field_values(void)
{
    write_temp_file("alpha,beta\ngamma,delta\n");
    csv_load(g_buffer, (char *)TEMP_FILE);

    char buf[64];
    csv_get_field(buf, sizeof(buf), g_buffer, 0, 0);
    TEST_ASSERT_EQUAL_STRING("alpha", buf);
    csv_get_field(buf, sizeof(buf), g_buffer, 0, 1);
    TEST_ASSERT_EQUAL_STRING("beta", buf);
    csv_get_field(buf, sizeof(buf), g_buffer, 1, 0);
    TEST_ASSERT_EQUAL_STRING("gamma", buf);
    csv_get_field(buf, sizeof(buf), g_buffer, 1, 1);
    TEST_ASSERT_EQUAL_STRING("delta", buf);
}

/* csv_load row widths are correct */
void test_csv_load_row_widths(void)
{
    write_temp_file("1,2,3\n4,5\n6\n");
    csv_load(g_buffer, (char *)TEMP_FILE);
    TEST_ASSERT_EQUAL_INT(3, csv_get_width(g_buffer, 0));
    TEST_ASSERT_EQUAL_INT(2, csv_get_width(g_buffer, 1));
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(g_buffer, 2));
}

/* csv_load handles text-delimited fields (quoted strings) */
void test_csv_load_quoted_field(void)
{
    write_temp_file("\"hello world\",plain\n");
    int result = csv_load(g_buffer, (char *)TEMP_FILE);
    TEST_ASSERT_EQUAL_INT(0, result);

    char buf[64];
    csv_get_field(buf, sizeof(buf), g_buffer, 0, 0);
    TEST_ASSERT_EQUAL_STRING("hello world", buf);
    csv_get_field(buf, sizeof(buf), g_buffer, 0, 1);
    TEST_ASSERT_EQUAL_STRING("plain", buf);
}

/* csv_load handles quoted field containing a comma */
void test_csv_load_quoted_field_with_comma(void)
{
    write_temp_file("\"a,b\",c\n");
    csv_load(g_buffer, (char *)TEMP_FILE);
    TEST_ASSERT_EQUAL_INT(1, csv_get_height(g_buffer));
    TEST_ASSERT_EQUAL_INT(2, csv_get_width(g_buffer, 0));

    char buf[64];
    csv_get_field(buf, sizeof(buf), g_buffer, 0, 0);
    TEST_ASSERT_EQUAL_STRING("a,b", buf);
}

/* csv_load handles quoted field containing a newline */
void test_csv_load_quoted_field_with_newline(void)
{
    write_temp_file("\"line1\nline2\",end\n");
    csv_load(g_buffer, (char *)TEMP_FILE);

    char buf[128];
    csv_get_field(buf, sizeof(buf), g_buffer, 0, 0);
    TEST_ASSERT_EQUAL_STRING("line1\nline2", buf);
}

/* csv_load handles empty fields (consecutive delimiters) */
void test_csv_load_empty_fields(void)
{
    write_temp_file("a,,c\n");
    csv_load(g_buffer, (char *)TEMP_FILE);
    TEST_ASSERT_EQUAL_INT(1, csv_get_height(g_buffer));
    TEST_ASSERT_EQUAL_INT(3, csv_get_width(g_buffer, 0));

    char buf[64];
    csv_get_field(buf, sizeof(buf), g_buffer, 0, 0);
    TEST_ASSERT_EQUAL_STRING("a", buf);

    /* middle field is empty */
    int rc = csv_get_field(buf, sizeof(buf), g_buffer, 0, 1);
    TEST_ASSERT_EQUAL_INT(2, rc); /* 2 = empty cell */

    csv_get_field(buf, sizeof(buf), g_buffer, 0, 2);
    TEST_ASSERT_EQUAL_STRING("c", buf);
}

/* csv_load handles a file with no trailing newline */
void test_csv_load_no_trailing_newline(void)
{
    write_temp_file("x,y,z");
    int result = csv_load(g_buffer, (char *)TEMP_FILE);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_INT(1, csv_get_height(g_buffer));
    TEST_ASSERT_EQUAL_INT(3, csv_get_width(g_buffer, 0));

    char buf[64];
    csv_get_field(buf, sizeof(buf), g_buffer, 0, 2);
    TEST_ASSERT_EQUAL_STRING("z", buf);
}

/* csv_load handles a single row with trailing delimiter */
void test_csv_load_trailing_delimiter(void)
{
    write_temp_file("a,b,\n");
    csv_load(g_buffer, (char *)TEMP_FILE);
    TEST_ASSERT_EQUAL_INT(1, csv_get_height(g_buffer));
    /* trailing delimiter implies an empty trailing cell */
    TEST_ASSERT_EQUAL_INT(3, csv_get_width(g_buffer, 0));
}

/* csv_load: buffer height is correct for three rows */
void test_csv_load_three_rows_height(void)
{
    write_temp_file("1\n2\n3\n");
    csv_load(g_buffer, (char *)TEMP_FILE);
    TEST_ASSERT_EQUAL_INT(3, csv_get_height(g_buffer));
}

/* csv_load: numeric string fields are preserved as strings */
void test_csv_load_numeric_strings(void)
{
    write_temp_file("42,3.14,0\n");
    csv_load(g_buffer, (char *)TEMP_FILE);

    char buf[64];
    csv_get_field(buf, sizeof(buf), g_buffer, 0, 0);
    TEST_ASSERT_EQUAL_STRING("42", buf);
    csv_get_field(buf, sizeof(buf), g_buffer, 0, 1);
    TEST_ASSERT_EQUAL_STRING("3.14", buf);
    csv_get_field(buf, sizeof(buf), g_buffer, 0, 2);
    TEST_ASSERT_EQUAL_STRING("0", buf);
}

/* csv_load: field with only whitespace is preserved */
void test_csv_load_whitespace_field(void)
{
    write_temp_file("   ,b\n");
    csv_load(g_buffer, (char *)TEMP_FILE);

    char buf[64];
    csv_get_field(buf, sizeof(buf), g_buffer, 0, 0);
    TEST_ASSERT_EQUAL_STRING("   ", buf);
}

/* csv_load: loading into a buffer that already has data appends correctly */
void test_csv_load_buffer_starts_empty(void)
{
    write_temp_file("new1,new2\n");
    TEST_ASSERT_EQUAL_INT(0, csv_get_height(g_buffer));
    csv_load(g_buffer, (char *)TEMP_FILE);
    TEST_ASSERT_EQUAL_INT(1, csv_get_height(g_buffer));
}

/* csv_load: single character field */
void test_csv_load_single_char_field(void)
{
    write_temp_file("X\n");
    csv_load(g_buffer, (char *)TEMP_FILE);
    TEST_ASSERT_EQUAL_INT(1, csv_get_height(g_buffer));
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(g_buffer, 0));

    char buf[8];
    csv_get_field(buf, sizeof(buf), g_buffer, 0, 0);
    TEST_ASSERT_EQUAL_STRING("X", buf);
}

/* csv_load: many rows */
void test_csv_load_many_rows(void)
{
    /* Build a file with 50 rows */
    FILE *fp = fopen(TEMP_FILE, "w");
    TEST_ASSERT_NOT_NULL(fp);
    for (int r = 0; r < 50; r++) {
        fprintf(fp, "field%d\n", r);
    }
    fclose(fp);

    int result = csv_load(g_buffer, (char *)TEMP_FILE);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_INT(50, csv_get_height(g_buffer));
}

/* csv_load: many columns in one row */
void test_csv_load_many_columns(void)
{
    FILE *fp = fopen(TEMP_FILE, "w");
    TEST_ASSERT_NOT_NULL(fp);
    for (int c = 0; c < 20; c++) {
        if (c > 0) fputc(',', fp);
        fprintf(fp, "col%d", c);
    }
    fputc('\n', fp);
    fclose(fp);

    int result = csv_load(g_buffer, (char *)TEMP_FILE);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_INT(20, csv_get_width(g_buffer, 0));
}

/* ------------------------------------------------------------------ */
/* main                                                                 */
/* ------------------------------------------------------------------ */

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_csv_load_returns_1_for_missing_file);
    RUN_TEST(test_csv_load_returns_0_for_empty_file);
    RUN_TEST(test_csv_load_empty_file_zero_rows);
    RUN_TEST(test_csv_load_single_field);
    RUN_TEST(test_csv_load_single_row_multiple_fields);
    RUN_TEST(test_csv_load_multiple_rows);
    RUN_TEST(test_csv_load_multiple_rows_field_values);
    RUN_TEST(test_csv_load_row_widths);
    RUN_TEST(test_csv_load_quoted_field);
    RUN_TEST(test_csv_load_quoted_field_with_comma);
    RUN_TEST(test_csv_load_quoted_field_with_newline);
    RUN_TEST(test_csv_load_empty_fields);
    RUN_TEST(test_csv_load_no_trailing_newline);
    RUN_TEST(test_csv_load_trailing_delimiter);
    RUN_TEST(test_csv_load_three_rows_height);
    RUN_TEST(test_csv_load_numeric_strings);
    RUN_TEST(test_csv_load_whitespace_field);
    RUN_TEST(test_csv_load_buffer_starts_empty);
    RUN_TEST(test_csv_load_single_char_field);
    RUN_TEST(test_csv_load_many_rows);
    RUN_TEST(test_csv_load_many_columns);
    return UNITY_END();
}