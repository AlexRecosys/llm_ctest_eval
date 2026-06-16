#include "csv.c"
#include "unity.h"
#include "csv.h"
#include <stdlib.h>
#include <string.h>

/* File-scope fixtures */
static CSV_BUFFER *src_buf;
static CSV_BUFFER *dest_buf;

/* Helper: build a buffer with one row and one field set to the given text */
static CSV_BUFFER *make_buffer_with_field(const char *text)
{
    CSV_BUFFER *buf = csv_create_buffer();
    if (!buf) return NULL;
    append_row(buf);
    append_field(buf, 0);
    set_field(buf->field[0][0], (char *)text);
    return buf;
}

/* Helper: build a buffer with multiple rows and fields */
static CSV_BUFFER *make_buffer_grid(int rows, int cols)
{
    CSV_BUFFER *buf = csv_create_buffer();
    if (!buf) return NULL;
    for (int r = 0; r < rows; r++) {
        append_row(buf);
        for (int c = 0; c < cols; c++) {
            append_field(buf, r);
        }
    }
    return buf;
}

void setUp(void)
{
    src_buf  = NULL;
    dest_buf = NULL;
}

void tearDown(void)
{
    if (src_buf)  { csv_destroy_buffer(src_buf);  src_buf  = NULL; }
    if (dest_buf) { csv_destroy_buffer(dest_buf); dest_buf = NULL; }
}

/* ------------------------------------------------------------------ */
/* Test: basic copy of a simple string field                           */
/* ------------------------------------------------------------------ */
void test_csv_copy_field_basic_string(void)
{
    src_buf  = make_buffer_with_field("hello");
    dest_buf = make_buffer_with_field("world");

    int ret = csv_copy_field(dest_buf, 0, 0, src_buf, 0, 0);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING("hello", dest_buf->field[0][0]->text);
}

/* ------------------------------------------------------------------ */
/* Test: copy overwrites existing content in destination               */
/* ------------------------------------------------------------------ */
void test_csv_copy_field_overwrites_dest(void)
{
    src_buf  = make_buffer_with_field("new_value");
    dest_buf = make_buffer_with_field("old_value");

    int ret = csv_copy_field(dest_buf, 0, 0, src_buf, 0, 0);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING("new_value", dest_buf->field[0][0]->text);
}

/* ------------------------------------------------------------------ */
/* Test: copy an empty string field                                    */
/* ------------------------------------------------------------------ */
void test_csv_copy_field_empty_string(void)
{
    src_buf  = make_buffer_with_field("");
    dest_buf = make_buffer_with_field("something");

    int ret = csv_copy_field(dest_buf, 0, 0, src_buf, 0, 0);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING("", dest_buf->field[0][0]->text);
}

/* ------------------------------------------------------------------ */
/* Test: copy a field with special CSV characters                      */
/* ------------------------------------------------------------------ */
void test_csv_copy_field_special_characters(void)
{
    src_buf  = make_buffer_with_field("hello,\"world\"\nnewline");
    dest_buf = make_buffer_with_field("placeholder");

    int ret = csv_copy_field(dest_buf, 0, 0, src_buf, 0, 0);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING("hello,\"world\"\nnewline",
                             dest_buf->field[0][0]->text);
}

/* ------------------------------------------------------------------ */
/* Test: copy from a non-zero column index                             */
/* ------------------------------------------------------------------ */
void test_csv_copy_field_non_zero_source_entry(void)
{
    src_buf  = make_buffer_grid(1, 3);
    dest_buf = make_buffer_with_field("dest_field");

    set_field(src_buf->field[0][0], "col0");
    set_field(src_buf->field[0][1], "col1");
    set_field(src_buf->field[0][2], "col2");

    int ret = csv_copy_field(dest_buf, 0, 0, src_buf, 0, 2);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING("col2", dest_buf->field[0][0]->text);
}

/* ------------------------------------------------------------------ */
/* Test: copy to a non-zero column index in destination                */
/* ------------------------------------------------------------------ */
void test_csv_copy_field_non_zero_dest_entry(void)
{
    src_buf  = make_buffer_with_field("source_text");
    dest_buf = make_buffer_grid(1, 3);

    set_field(dest_buf->field[0][0], "d0");
    set_field(dest_buf->field[0][1], "d1");
    set_field(dest_buf->field[0][2], "d2");

    int ret = csv_copy_field(dest_buf, 0, 1, src_buf, 0, 0);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING("source_text", dest_buf->field[0][1]->text);
    /* Other fields must be untouched */
    TEST_ASSERT_EQUAL_STRING("d0", dest_buf->field[0][0]->text);
    TEST_ASSERT_EQUAL_STRING("d2", dest_buf->field[0][2]->text);
}

/* ------------------------------------------------------------------ */
/* Test: copy from a non-zero row index                                */
/* ------------------------------------------------------------------ */
void test_csv_copy_field_non_zero_source_row(void)
{
    src_buf  = make_buffer_grid(3, 1);
    dest_buf = make_buffer_with_field("placeholder");

    set_field(src_buf->field[0][0], "row0");
    set_field(src_buf->field[1][0], "row1");
    set_field(src_buf->field[2][0], "row2");

    int ret = csv_copy_field(dest_buf, 0, 0, src_buf, 2, 0);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING("row2", dest_buf->field[0][0]->text);
}

/* ------------------------------------------------------------------ */
/* Test: copy to a non-zero row index in destination                   */
/* ------------------------------------------------------------------ */
void test_csv_copy_field_non_zero_dest_row(void)
{
    src_buf  = make_buffer_with_field("src_text");
    dest_buf = make_buffer_grid(3, 1);

    set_field(dest_buf->field[0][0], "r0");
    set_field(dest_buf->field[1][0], "r1");
    set_field(dest_buf->field[2][0], "r2");

    int ret = csv_copy_field(dest_buf, 2, 0, src_buf, 0, 0);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING("src_text", dest_buf->field[2][0]->text);
    /* Other rows must be untouched */
    TEST_ASSERT_EQUAL_STRING("r0", dest_buf->field[0][0]->text);
    TEST_ASSERT_EQUAL_STRING("r1", dest_buf->field[1][0]->text);
}

/* ------------------------------------------------------------------ */
/* Test: copy within the same buffer (src == dest, different cells)    */
/* ------------------------------------------------------------------ */
void test_csv_copy_field_same_buffer_different_cells(void)
{
    src_buf = make_buffer_grid(1, 2);
    set_field(src_buf->field[0][0], "original");
    set_field(src_buf->field[0][1], "target");

    int ret = csv_copy_field(src_buf, 0, 1, src_buf, 0, 0);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING("original", src_buf->field[0][1]->text);
    /* Source cell must be unchanged */
    TEST_ASSERT_EQUAL_STRING("original", src_buf->field[0][0]->text);
}

/* ------------------------------------------------------------------ */
/* Test: copy within the same buffer, same cell (self-copy)            */
/* ------------------------------------------------------------------ */
void test_csv_copy_field_same_buffer_same_cell(void)
{
    src_buf = make_buffer_with_field("self");

    int ret = csv_copy_field(src_buf, 0, 0, src_buf, 0, 0);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING("self", src_buf->field[0][0]->text);
}

/* ------------------------------------------------------------------ */
/* Test: field length is updated after copy                            */
/* ------------------------------------------------------------------ */
void test_csv_copy_field_length_updated(void)
{
    const char *text = "length_test";
    src_buf  = make_buffer_with_field(text);
    dest_buf = make_buffer_with_field("short");

    int ret = csv_copy_field(dest_buf, 0, 0, src_buf, 0, 0);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING(text, dest_buf->field[0][0]->text);
    /* Length stored in field should match strlen of copied text */
    TEST_ASSERT_EQUAL_INT((int)strlen(text),
                          (int)dest_buf->field[0][0]->length);
}

/* ------------------------------------------------------------------ */
/* Test: copy a long string field                                      */
/* ------------------------------------------------------------------ */
void test_csv_copy_field_long_string(void)
{
    char long_str[512];
    memset(long_str, 'A', sizeof(long_str) - 1);
    long_str[sizeof(long_str) - 1] = '\0';

    src_buf  = make_buffer_with_field(long_str);
    dest_buf = make_buffer_with_field("short");

    int ret = csv_copy_field(dest_buf, 0, 0, src_buf, 0, 0);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING(long_str, dest_buf->field[0][0]->text);
}

/* ------------------------------------------------------------------ */
/* Test: copy does not alter other fields in destination row           */
/* ------------------------------------------------------------------ */
void test_csv_copy_field_does_not_alter_other_dest_fields(void)
{
    src_buf  = make_buffer_with_field("copied");
    dest_buf = make_buffer_grid(1, 4);

    set_field(dest_buf->field[0][0], "a");
    set_field(dest_buf->field[0][1], "b");
    set_field(dest_buf->field[0][2], "c");
    set_field(dest_buf->field[0][3], "d");

    int ret = csv_copy_field(dest_buf, 0, 2, src_buf, 0, 0);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING("copied", dest_buf->field[0][2]->text);
    TEST_ASSERT_EQUAL_STRING("a", dest_buf->field[0][0]->text);
    TEST_ASSERT_EQUAL_STRING("b", dest_buf->field[0][1]->text);
    TEST_ASSERT_EQUAL_STRING("d", dest_buf->field[0][3]->text);
}

/* ------------------------------------------------------------------ */
/* Test: copy does not alter source field                              */
/* ------------------------------------------------------------------ */
void test_csv_copy_field_source_unchanged(void)
{
    src_buf  = make_buffer_with_field("immutable");
    dest_buf = make_buffer_with_field("mutable");

    csv_copy_field(dest_buf, 0, 0, src_buf, 0, 0);

    TEST_ASSERT_EQUAL_STRING("immutable", src_buf->field[0][0]->text);
}

/* ------------------------------------------------------------------ */
/* Test: copy a field containing only whitespace                       */
/* ------------------------------------------------------------------ */
void test_csv_copy_field_whitespace_string(void)
{
    src_buf  = make_buffer_with_field("   \t  ");
    dest_buf = make_buffer_with_field("nonempty");

    int ret = csv_copy_field(dest_buf, 0, 0, src_buf, 0, 0);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING("   \t  ", dest_buf->field[0][0]->text);
}

/* ------------------------------------------------------------------ */
/* Test: copy a field containing numeric string                        */
/* ------------------------------------------------------------------ */
void test_csv_copy_field_numeric_string(void)
{
    src_buf  = make_buffer_with_field("3.14159");
    dest_buf = make_buffer_with_field("0");

    int ret = csv_copy_field(dest_buf, 0, 0, src_buf, 0, 0);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING("3.14159", dest_buf->field[0][0]->text);
}

/* ------------------------------------------------------------------ */
/* main                                                                */
/* ------------------------------------------------------------------ */
int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_csv_copy_field_basic_string);
    RUN_TEST(test_csv_copy_field_overwrites_dest);
    RUN_TEST(test_csv_copy_field_empty_string);
    RUN_TEST(test_csv_copy_field_special_characters);
    RUN_TEST(test_csv_copy_field_non_zero_source_entry);
    RUN_TEST(test_csv_copy_field_non_zero_dest_entry);
    RUN_TEST(test_csv_copy_field_non_zero_source_row);
    RUN_TEST(test_csv_copy_field_non_zero_dest_row);
    RUN_TEST(test_csv_copy_field_same_buffer_different_cells);
    RUN_TEST(test_csv_copy_field_same_buffer_same_cell);
    RUN_TEST(test_csv_copy_field_length_updated);
    RUN_TEST(test_csv_copy_field_long_string);
    RUN_TEST(test_csv_copy_field_does_not_alter_other_dest_fields);
    RUN_TEST(test_csv_copy_field_source_unchanged);
    RUN_TEST(test_csv_copy_field_whitespace_string);
    RUN_TEST(test_csv_copy_field_numeric_string);
    return UNITY_END();
}