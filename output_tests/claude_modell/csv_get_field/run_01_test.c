#include "csv.c"
#include "unity.h"
#include "csv.h"
#include <stdlib.h>
#include <string.h>

/* File-scope fixtures */
static CSV_BUFFER *buf;

/* Helper: build a minimal CSV_BUFFER with one row and one field set to text */
static CSV_BUFFER *make_buffer_with_field(const char *text)
{
    CSV_BUFFER *b = csv_create_buffer();
    if (!b) return NULL;
    append_row(b);
    append_field(b, 0);
    set_field(b->field[0][0], (char *)text);
    return b;
}

/* Helper: build a buffer with multiple rows and fields */
static CSV_BUFFER *make_multi_buffer(void)
{
    CSV_BUFFER *b = csv_create_buffer();
    if (!b) return NULL;

    /* Row 0: two fields */
    append_row(b);
    append_field(b, 0);
    set_field(b->field[0][0], "hello");
    append_field(b, 0);
    set_field(b->field[0][1], "world");

    /* Row 1: one field */
    append_row(b);
    append_field(b, 1);
    set_field(b->field[1][0], "foo");

    /* Row 2: one empty field (length == 0) */
    append_row(b);
    append_field(b, 2);
    set_field(b->field[2][0], "");

    return b;
}

void setUp(void)
{
    buf = NULL;
}

void tearDown(void)
{
    if (buf) {
        csv_destroy_buffer(buf);
        buf = NULL;
    }
}

/* ------------------------------------------------------------------ */
/* Test: dest_len == 0 returns 3 */
/* ------------------------------------------------------------------ */
void test_csv_get_field_dest_len_zero_returns_3(void)
{
    buf = make_buffer_with_field("hello");
    char dest[16] = {0};
    int ret = csv_get_field(dest, 0, buf, 0, 0);
    TEST_ASSERT_EQUAL_INT(3, ret);
}

/* ------------------------------------------------------------------ */
/* Test: row out of range returns 2 and clears dest */
/* ------------------------------------------------------------------ */
void test_csv_get_field_row_out_of_range_returns_2(void)
{
    buf = make_buffer_with_field("hello");
    char dest[16];
    memset(dest, 0xAB, sizeof(dest));
    int ret = csv_get_field(dest, sizeof(dest) - 1, buf, 99, 0);
    TEST_ASSERT_EQUAL_INT(2, ret);
    /* dest[0] should be '\0' after clearing */
    TEST_ASSERT_EQUAL_INT('\0', dest[0]);
}

/* ------------------------------------------------------------------ */
/* Test: entry out of range returns 2 and clears dest */
/* ------------------------------------------------------------------ */
void test_csv_get_field_entry_out_of_range_returns_2(void)
{
    buf = make_buffer_with_field("hello");
    char dest[16];
    memset(dest, 0xAB, sizeof(dest));
    int ret = csv_get_field(dest, sizeof(dest) - 1, buf, 0, 99);
    TEST_ASSERT_EQUAL_INT(2, ret);
    TEST_ASSERT_EQUAL_INT('\0', dest[0]);
}

/* ------------------------------------------------------------------ */
/* Test: valid field copied successfully returns 0 */
/* ------------------------------------------------------------------ */
void test_csv_get_field_valid_field_returns_0(void)
{
    buf = make_buffer_with_field("hello");
    char dest[16] = {0};
    int ret = csv_get_field(dest, sizeof(dest) - 1, buf, 0, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING("hello", dest);
}

/* ------------------------------------------------------------------ */
/* Test: field copied correctly with exact content */
/* ------------------------------------------------------------------ */
void test_csv_get_field_copies_correct_text(void)
{
    buf = make_multi_buffer();
    char dest[16] = {0};
    int ret = csv_get_field(dest, sizeof(dest) - 1, buf, 0, 1);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING("world", dest);
}

/* ------------------------------------------------------------------ */
/* Test: second row field copied correctly */
/* ------------------------------------------------------------------ */
void test_csv_get_field_second_row_returns_0(void)
{
    buf = make_multi_buffer();
    char dest[16] = {0};
    int ret = csv_get_field(dest, sizeof(dest) - 1, buf, 1, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING("foo", dest);
}

/* ------------------------------------------------------------------ */
/* Test: empty field returns 2 */
/* ------------------------------------------------------------------ */
void test_csv_get_field_empty_field_returns_2(void)
{
    buf = make_multi_buffer();
    char dest[16];
    memset(dest, 0xAB, sizeof(dest));
    int ret = csv_get_field(dest, sizeof(dest) - 1, buf, 2, 0);
    TEST_ASSERT_EQUAL_INT(2, ret);
}

/* ------------------------------------------------------------------ */
/* Test: truncation returns 1 when dest_len is too small */
/* ------------------------------------------------------------------ */
void test_csv_get_field_truncation_returns_1(void)
{
    buf = make_buffer_with_field("hello");
    /* dest_len = 2, field is "hello" (length 5), so truncation occurs */
    char dest[4] = {0};
    int ret = csv_get_field(dest, 2, buf, 0, 0);
    TEST_ASSERT_EQUAL_INT(1, ret);
    /* dest should contain first 2 chars then null */
    TEST_ASSERT_EQUAL_INT('h', dest[0]);
    TEST_ASSERT_EQUAL_INT('e', dest[1]);
    TEST_ASSERT_EQUAL_INT('\0', dest[2]);
}

/* ------------------------------------------------------------------ */
/* Test: truncation content is correct */
/* ------------------------------------------------------------------ */
void test_csv_get_field_truncation_content_correct(void)
{
    buf = make_buffer_with_field("abcdef");
    char dest[8] = {0};
    /* dest_len = 3, field is "abcdef" */
    int ret = csv_get_field(dest, 3, buf, 0, 0);
    TEST_ASSERT_EQUAL_INT(1, ret);
    TEST_ASSERT_EQUAL_INT('a', dest[0]);
    TEST_ASSERT_EQUAL_INT('b', dest[1]);
    TEST_ASSERT_EQUAL_INT('c', dest[2]);
    TEST_ASSERT_EQUAL_INT('\0', dest[3]);
}

/* ------------------------------------------------------------------ */
/* Test: dest_len == 0 does not modify dest */
/* ------------------------------------------------------------------ */
void test_csv_get_field_dest_len_zero_does_not_modify_dest(void)
{
    buf = make_buffer_with_field("hello");
    char dest[8];
    memset(dest, 0xCC, sizeof(dest));
    csv_get_field(dest, 0, buf, 0, 0);
    /* dest should be untouched */
    TEST_ASSERT_EQUAL_HEX(0xCC, (unsigned char)dest[0]);
}

/* ------------------------------------------------------------------ */
/* Test: row == src->rows (boundary) returns 2 */
/* ------------------------------------------------------------------ */
void test_csv_get_field_row_equals_rows_boundary_returns_2(void)
{
    buf = make_buffer_with_field("hello");
    /* buf->rows == 1, so row == 1 is out of range */
    char dest[16];
    memset(dest, 0xAB, sizeof(dest));
    int ret = csv_get_field(dest, sizeof(dest) - 1, buf, buf->rows, 0);
    TEST_ASSERT_EQUAL_INT(2, ret);
    TEST_ASSERT_EQUAL_INT('\0', dest[0]);
}

/* ------------------------------------------------------------------ */
/* Test: entry == src->width[row] (boundary) returns 2 */
/* ------------------------------------------------------------------ */
void test_csv_get_field_entry_equals_width_boundary_returns_2(void)
{
    buf = make_buffer_with_field("hello");
    /* buf->width[0] == 1, so entry == 1 is out of range */
    char dest[16];
    memset(dest, 0xAB, sizeof(dest));
    int ret = csv_get_field(dest, sizeof(dest) - 1, buf, 0, buf->width[0]);
    TEST_ASSERT_EQUAL_INT(2, ret);
    TEST_ASSERT_EQUAL_INT('\0', dest[0]);
}

/* ------------------------------------------------------------------ */
/* Test: single character field, dest_len == 1 returns 0 */
/* ------------------------------------------------------------------ */
void test_csv_get_field_single_char_field_exact_dest_len(void)
{
    buf = make_buffer_with_field("A");
    char dest[4] = {0};
    /* dest_len = 1, field is "A" (length 1) */
    int ret = csv_get_field(dest, 1, buf, 0, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT('A', dest[0]);
    TEST_ASSERT_EQUAL_INT('\0', dest[1]);
}

/* ------------------------------------------------------------------ */
/* Test: field with spaces copied correctly */
/* ------------------------------------------------------------------ */
void test_csv_get_field_field_with_spaces(void)
{
    buf = make_buffer_with_field("hello world");
    char dest[32] = {0};
    int ret = csv_get_field(dest, sizeof(dest) - 1, buf, 0, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING("hello world", dest);
}

/* ------------------------------------------------------------------ */
/* Test: multiple rows, access first row second field */
/* ------------------------------------------------------------------ */
void test_csv_get_field_multi_row_first_row_second_field(void)
{
    buf = make_multi_buffer();
    char dest[16] = {0};
    int ret = csv_get_field(dest, sizeof(dest) - 1, buf, 0, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING("hello", dest);
}

/* ------------------------------------------------------------------ */
/* Test: dest_len exactly matches field length returns 0 */
/* ------------------------------------------------------------------ */
void test_csv_get_field_dest_len_exact_match(void)
{
    buf = make_buffer_with_field("test");
    /* "test" has length 4, dest_len = 4 */
    char dest[8] = {0};
    int ret = csv_get_field(dest, 4, buf, 0, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING("test", dest);
}

/* ------------------------------------------------------------------ */
/* main */
/* ------------------------------------------------------------------ */
int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_csv_get_field_dest_len_zero_returns_3);
    RUN_TEST(test_csv_get_field_row_out_of_range_returns_2);
    RUN_TEST(test_csv_get_field_entry_out_of_range_returns_2);
    RUN_TEST(test_csv_get_field_valid_field_returns_0);
    RUN_TEST(test_csv_get_field_copies_correct_text);
    RUN_TEST(test_csv_get_field_second_row_returns_0);
    RUN_TEST(test_csv_get_field_empty_field_returns_2);
    RUN_TEST(test_csv_get_field_truncation_returns_1);
    RUN_TEST(test_csv_get_field_truncation_content_correct);
    RUN_TEST(test_csv_get_field_dest_len_zero_does_not_modify_dest);
    RUN_TEST(test_csv_get_field_row_equals_rows_boundary_returns_2);
    RUN_TEST(test_csv_get_field_entry_equals_width_boundary_returns_2);
    RUN_TEST(test_csv_get_field_single_char_field_exact_dest_len);
    RUN_TEST(test_csv_get_field_field_with_spaces);
    RUN_TEST(test_csv_get_field_multi_row_first_row_second_field);
    RUN_TEST(test_csv_get_field_dest_len_exact_match);
    return UNITY_END();
}