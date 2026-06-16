#include "csv.c"
#include "unity.h"
#include "csv.h"
#include <stdlib.h>
#include <string.h>

/* File-scope fixtures */
static CSV_BUFFER *src_buf;
static CSV_BUFFER *dst_buf;

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
static CSV_BUFFER *make_buffer_multi(void)
{
    CSV_BUFFER *buf = csv_create_buffer();
    if (!buf) return NULL;

    /* Row 0: two fields */
    append_row(buf);
    append_field(buf, 0);
    append_field(buf, 0);
    set_field(buf->field[0][0], "hello");
    set_field(buf->field[0][1], "world");

    /* Row 1: two fields */
    append_row(buf);
    append_field(buf, 1);
    append_field(buf, 1);
    set_field(buf->field[1][0], "foo");
    set_field(buf->field[1][1], "bar");

    return buf;
}

/* setUp / tearDown */
void setUp(void)
{
    src_buf = NULL;
    dst_buf = NULL;
}

void tearDown(void)
{
    if (src_buf) {
        csv_destroy_buffer(src_buf);
        src_buf = NULL;
    }
    if (dst_buf) {
        csv_destroy_buffer(dst_buf);
        dst_buf = NULL;
    }
}

/* ------------------------------------------------------------------ */
/* Test cases                                                           */
/* ------------------------------------------------------------------ */

/* Basic copy: text is transferred correctly */
void test_csv_copy_field_basic_text(void)
{
    src_buf = make_buffer_with_field("hello");
    dst_buf = make_buffer_with_field("old_value");

    TEST_ASSERT_NOT_NULL(src_buf);
    TEST_ASSERT_NOT_NULL(dst_buf);

    int ret = csv_copy_field(dst_buf, 0, 0, src_buf, 0, 0);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING("hello", dst_buf->field[0][0]->text);
}

/* Source text is empty string */
void test_csv_copy_field_empty_string(void)
{
    src_buf = make_buffer_with_field("");
    dst_buf = make_buffer_with_field("something");

    TEST_ASSERT_NOT_NULL(src_buf);
    TEST_ASSERT_NOT_NULL(dst_buf);

    int ret = csv_copy_field(dst_buf, 0, 0, src_buf, 0, 0);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING("", dst_buf->field[0][0]->text);
}

/* Source text contains spaces */
void test_csv_copy_field_text_with_spaces(void)
{
    src_buf = make_buffer_with_field("hello world");
    dst_buf = make_buffer_with_field("placeholder");

    TEST_ASSERT_NOT_NULL(src_buf);
    TEST_ASSERT_NOT_NULL(dst_buf);

    int ret = csv_copy_field(dst_buf, 0, 0, src_buf, 0, 0);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING("hello world", dst_buf->field[0][0]->text);
}

/* Source text contains special CSV characters */
void test_csv_copy_field_special_characters(void)
{
    src_buf = make_buffer_with_field("a,b,\"c\"");
    dst_buf = make_buffer_with_field("old");

    TEST_ASSERT_NOT_NULL(src_buf);
    TEST_ASSERT_NOT_NULL(dst_buf);

    int ret = csv_copy_field(dst_buf, 0, 0, src_buf, 0, 0);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING("a,b,\"c\"", dst_buf->field[0][0]->text);
}

/* Copy does not modify source field */
void test_csv_copy_field_source_unchanged(void)
{
    src_buf = make_buffer_with_field("original");
    dst_buf = make_buffer_with_field("dest_old");

    TEST_ASSERT_NOT_NULL(src_buf);
    TEST_ASSERT_NOT_NULL(dst_buf);

    csv_copy_field(dst_buf, 0, 0, src_buf, 0, 0);

    TEST_ASSERT_EQUAL_STRING("original", src_buf->field[0][0]->text);
}

/* Copy between different rows in multi-row buffers */
void test_csv_copy_field_different_rows(void)
{
    src_buf = make_buffer_multi();
    dst_buf = make_buffer_multi();

    TEST_ASSERT_NOT_NULL(src_buf);
    TEST_ASSERT_NOT_NULL(dst_buf);

    /* Copy src row1, field0 ("foo") into dst row0, field1 */
    int ret = csv_copy_field(dst_buf, 0, 1, src_buf, 1, 0);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING("foo", dst_buf->field[0][1]->text);
}

/* Copy between different columns in multi-column buffers */
void test_csv_copy_field_different_columns(void)
{
    src_buf = make_buffer_multi();
    dst_buf = make_buffer_multi();

    TEST_ASSERT_NOT_NULL(src_buf);
    TEST_ASSERT_NOT_NULL(dst_buf);

    /* Copy src row0, field1 ("world") into dst row1, field1 */
    int ret = csv_copy_field(dst_buf, 1, 1, src_buf, 0, 1);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING("world", dst_buf->field[1][1]->text);
}

/* Copy within the same buffer (same buffer, different cells) */
void test_csv_copy_field_same_buffer_different_cells(void)
{
    src_buf = make_buffer_multi();

    TEST_ASSERT_NOT_NULL(src_buf);

    /* Copy row0,field0 ("hello") into row1,field1 (was "bar") */
    int ret = csv_copy_field(src_buf, 1, 1, src_buf, 0, 0);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING("hello", src_buf->field[1][1]->text);
    /* Original cell should be unchanged */
    TEST_ASSERT_EQUAL_STRING("hello", src_buf->field[0][0]->text);
}

/* Copy a long string */
void test_csv_copy_field_long_string(void)
{
    char long_text[512];
    memset(long_text, 'A', sizeof(long_text) - 1);
    long_text[sizeof(long_text) - 1] = '\0';

    src_buf = make_buffer_with_field(long_text);
    dst_buf = make_buffer_with_field("short");

    TEST_ASSERT_NOT_NULL(src_buf);
    TEST_ASSERT_NOT_NULL(dst_buf);

    int ret = csv_copy_field(dst_buf, 0, 0, src_buf, 0, 0);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING(long_text, dst_buf->field[0][0]->text);
}

/* Overwrite destination that already has a value */
void test_csv_copy_field_overwrites_existing_dest(void)
{
    src_buf = make_buffer_with_field("new_value");
    dst_buf = make_buffer_with_field("old_value");

    TEST_ASSERT_NOT_NULL(src_buf);
    TEST_ASSERT_NOT_NULL(dst_buf);

    int ret = csv_copy_field(dst_buf, 0, 0, src_buf, 0, 0);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING("new_value", dst_buf->field[0][0]->text);
}

/* Copy numeric string */
void test_csv_copy_field_numeric_string(void)
{
    src_buf = make_buffer_with_field("12345");
    dst_buf = make_buffer_with_field("00000");

    TEST_ASSERT_NOT_NULL(src_buf);
    TEST_ASSERT_NOT_NULL(dst_buf);

    int ret = csv_copy_field(dst_buf, 0, 0, src_buf, 0, 0);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING("12345", dst_buf->field[0][0]->text);
}

/* Verify field length is updated after copy */
void test_csv_copy_field_length_updated(void)
{
    src_buf = make_buffer_with_field("abc");
    dst_buf = make_buffer_with_field("x");

    TEST_ASSERT_NOT_NULL(src_buf);
    TEST_ASSERT_NOT_NULL(dst_buf);

    csv_copy_field(dst_buf, 0, 0, src_buf, 0, 0);

    /* "abc" has length 3 */
    TEST_ASSERT_EQUAL_INT(3, (int)dst_buf->field[0][0]->length);
}

/* Copy single character */
void test_csv_copy_field_single_char(void)
{
    src_buf = make_buffer_with_field("Z");
    dst_buf = make_buffer_with_field("A");

    TEST_ASSERT_NOT_NULL(src_buf);
    TEST_ASSERT_NOT_NULL(dst_buf);

    int ret = csv_copy_field(dst_buf, 0, 0, src_buf, 0, 0);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING("Z", dst_buf->field[0][0]->text);
}

/* ------------------------------------------------------------------ */
/* main                                                                 */
/* ------------------------------------------------------------------ */
int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_csv_copy_field_basic_text);
    RUN_TEST(test_csv_copy_field_empty_string);
    RUN_TEST(test_csv_copy_field_text_with_spaces);
    RUN_TEST(test_csv_copy_field_special_characters);
    RUN_TEST(test_csv_copy_field_source_unchanged);
    RUN_TEST(test_csv_copy_field_different_rows);
    RUN_TEST(test_csv_copy_field_different_columns);
    RUN_TEST(test_csv_copy_field_same_buffer_different_cells);
    RUN_TEST(test_csv_copy_field_long_string);
    RUN_TEST(test_csv_copy_field_overwrites_existing_dest);
    RUN_TEST(test_csv_copy_field_numeric_string);
    RUN_TEST(test_csv_copy_field_length_updated);
    RUN_TEST(test_csv_copy_field_single_char);
    return UNITY_END();
}