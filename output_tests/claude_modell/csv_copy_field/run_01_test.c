#include "csv.c"
#include "unity.h"
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
    set_field(buf->field[0][0], "alpha");
    set_field(buf->field[0][1], "beta");

    /* Row 1: two fields */
    append_row(buf);
    append_field(buf, 1);
    append_field(buf, 1);
    set_field(buf->field[1][0], "gamma");
    set_field(buf->field[1][1], "delta");

    return buf;
}

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
/* Test cases                                                          */
/* ------------------------------------------------------------------ */

/* Basic copy: simple ASCII string */
void test_csv_copy_field_basic_string(void)
{
    src_buf = make_buffer_with_field("hello");
    TEST_ASSERT_NOT_NULL(src_buf);

    dst_buf = make_buffer_with_field("placeholder");
    TEST_ASSERT_NOT_NULL(dst_buf);

    int ret = csv_copy_field(dst_buf, 0, 0, src_buf, 0, 0);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING("hello", dst_buf->field[0][0]->text);
}

/* Copy returns 0 on success */
void test_csv_copy_field_return_value_success(void)
{
    src_buf = make_buffer_with_field("test");
    dst_buf = make_buffer_with_field("old");
    TEST_ASSERT_NOT_NULL(src_buf);
    TEST_ASSERT_NOT_NULL(dst_buf);

    int ret = csv_copy_field(dst_buf, 0, 0, src_buf, 0, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);
}

/* Copy an empty string */
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

/* Copy a long string */
void test_csv_copy_field_long_string(void)
{
    char long_str[512];
    memset(long_str, 'A', sizeof(long_str) - 1);
    long_str[511] = '\0';

    src_buf = make_buffer_with_field(long_str);
    dst_buf = make_buffer_with_field("short");
    TEST_ASSERT_NOT_NULL(src_buf);
    TEST_ASSERT_NOT_NULL(dst_buf);

    int ret = csv_copy_field(dst_buf, 0, 0, src_buf, 0, 0);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING(long_str, dst_buf->field[0][0]->text);
}

/* Copy a string with special CSV characters */
void test_csv_copy_field_special_characters(void)
{
    src_buf = make_buffer_with_field("one,two,\"three\"");
    dst_buf = make_buffer_with_field("placeholder");
    TEST_ASSERT_NOT_NULL(src_buf);
    TEST_ASSERT_NOT_NULL(dst_buf);

    int ret = csv_copy_field(dst_buf, 0, 0, src_buf, 0, 0);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING("one,two,\"three\"", dst_buf->field[0][0]->text);
}

/* Copy does not modify source field */
void test_csv_copy_field_source_unchanged(void)
{
    src_buf = make_buffer_with_field("original");
    dst_buf = make_buffer_with_field("destination");
    TEST_ASSERT_NOT_NULL(src_buf);
    TEST_ASSERT_NOT_NULL(dst_buf);

    csv_copy_field(dst_buf, 0, 0, src_buf, 0, 0);

    TEST_ASSERT_EQUAL_STRING("original", src_buf->field[0][0]->text);
}

/* Copy is a deep copy: modifying source after copy does not affect dest */
void test_csv_copy_field_deep_copy(void)
{
    src_buf = make_buffer_with_field("initial");
    dst_buf = make_buffer_with_field("placeholder");
    TEST_ASSERT_NOT_NULL(src_buf);
    TEST_ASSERT_NOT_NULL(dst_buf);

    csv_copy_field(dst_buf, 0, 0, src_buf, 0, 0);

    /* Overwrite source field with a different value */
    set_field(src_buf->field[0][0], "changed");

    /* Destination should still hold the original copied value */
    TEST_ASSERT_EQUAL_STRING("initial", dst_buf->field[0][0]->text);
}

/* Copy from second field in a row */
void test_csv_copy_field_second_source_field(void)
{
    src_buf = make_buffer_multi();
    TEST_ASSERT_NOT_NULL(src_buf);

    dst_buf = make_buffer_with_field("placeholder");
    TEST_ASSERT_NOT_NULL(dst_buf);

    int ret = csv_copy_field(dst_buf, 0, 0, src_buf, 0, 1);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING("beta", dst_buf->field[0][0]->text);
}

/* Copy from second row */
void test_csv_copy_field_second_source_row(void)
{
    src_buf = make_buffer_multi();
    TEST_ASSERT_NOT_NULL(src_buf);

    dst_buf = make_buffer_with_field("placeholder");
    TEST_ASSERT_NOT_NULL(dst_buf);

    int ret = csv_copy_field(dst_buf, 0, 0, src_buf, 1, 0);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING("gamma", dst_buf->field[0][0]->text);
}

/* Copy to second field in destination */
void test_csv_copy_field_second_dest_field(void)
{
    src_buf = make_buffer_with_field("source_value");
    TEST_ASSERT_NOT_NULL(src_buf);

    dst_buf = make_buffer_multi();
    TEST_ASSERT_NOT_NULL(dst_buf);

    int ret = csv_copy_field(dst_buf, 0, 1, src_buf, 0, 0);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING("source_value", dst_buf->field[0][1]->text);
    /* First field in dest row 0 should be unchanged */
    TEST_ASSERT_EQUAL_STRING("alpha", dst_buf->field[0][0]->text);
}

/* Copy to second row in destination */
void test_csv_copy_field_second_dest_row(void)
{
    src_buf = make_buffer_with_field("row1_value");
    TEST_ASSERT_NOT_NULL(src_buf);

    dst_buf = make_buffer_multi();
    TEST_ASSERT_NOT_NULL(dst_buf);

    int ret = csv_copy_field(dst_buf, 1, 0, src_buf, 0, 0);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING("row1_value", dst_buf->field[1][0]->text);
    /* Row 0 should be unchanged */
    TEST_ASSERT_EQUAL_STRING("alpha", dst_buf->field[0][0]->text);
}

/* Copy within the same buffer (different fields) */
void test_csv_copy_field_same_buffer_different_fields(void)
{
    src_buf = make_buffer_multi();
    TEST_ASSERT_NOT_NULL(src_buf);

    int ret = csv_copy_field(src_buf, 0, 1, src_buf, 0, 0);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING("alpha", src_buf->field[0][1]->text);
}

/* Copy within the same buffer (different rows) */
void test_csv_copy_field_same_buffer_different_rows(void)
{
    src_buf = make_buffer_multi();
    TEST_ASSERT_NOT_NULL(src_buf);

    int ret = csv_copy_field(src_buf, 1, 0, src_buf, 0, 0);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING("alpha", src_buf->field[1][0]->text);
}

/* Copy a string with whitespace */
void test_csv_copy_field_whitespace_string(void)
{
    src_buf = make_buffer_with_field("  spaces  ");
    dst_buf = make_buffer_with_field("old");
    TEST_ASSERT_NOT_NULL(src_buf);
    TEST_ASSERT_NOT_NULL(dst_buf);

    int ret = csv_copy_field(dst_buf, 0, 0, src_buf, 0, 0);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING("  spaces  ", dst_buf->field[0][0]->text);
}

/* Copy a string with newline characters */
void test_csv_copy_field_newline_in_string(void)
{
    src_buf = make_buffer_with_field("line1\nline2");
    dst_buf = make_buffer_with_field("placeholder");
    TEST_ASSERT_NOT_NULL(src_buf);
    TEST_ASSERT_NOT_NULL(dst_buf);

    int ret = csv_copy_field(dst_buf, 0, 0, src_buf, 0, 0);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING("line1\nline2", dst_buf->field[0][0]->text);
}

/* Verify field length is updated after copy */
void test_csv_copy_field_length_updated(void)
{
    const char *text = "hello";
    src_buf = make_buffer_with_field(text);
    dst_buf = make_buffer_with_field("x");
    TEST_ASSERT_NOT_NULL(src_buf);
    TEST_ASSERT_NOT_NULL(dst_buf);

    csv_copy_field(dst_buf, 0, 0, src_buf, 0, 0);

    /* Length should reflect the copied string length */
    TEST_ASSERT_EQUAL_INT((int)strlen(text), (int)dst_buf->field[0][0]->length);
}

/* ------------------------------------------------------------------ */
/* main                                                                */
/* ------------------------------------------------------------------ */

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_csv_copy_field_basic_string);
    RUN_TEST(test_csv_copy_field_return_value_success);
    RUN_TEST(test_csv_copy_field_empty_string);
    RUN_TEST(test_csv_copy_field_long_string);
    RUN_TEST(test_csv_copy_field_special_characters);
    RUN_TEST(test_csv_copy_field_source_unchanged);
    RUN_TEST(test_csv_copy_field_deep_copy);
    RUN_TEST(test_csv_copy_field_second_source_field);
    RUN_TEST(test_csv_copy_field_second_source_row);
    RUN_TEST(test_csv_copy_field_second_dest_field);
    RUN_TEST(test_csv_copy_field_second_dest_row);
    RUN_TEST(test_csv_copy_field_same_buffer_different_fields);
    RUN_TEST(test_csv_copy_field_same_buffer_different_rows);
    RUN_TEST(test_csv_copy_field_whitespace_string);
    RUN_TEST(test_csv_copy_field_newline_in_string);
    RUN_TEST(test_csv_copy_field_length_updated);
    return UNITY_END();
}