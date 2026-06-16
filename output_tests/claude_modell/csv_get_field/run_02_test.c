#include "csv.c"
#include "unity.h"
#include <stdlib.h>
#include <string.h>

/* File-scope fixtures */
static CSV_BUFFER *buf;

/* Helper: build a minimal CSV_BUFFER with one row and one field */
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

    /* Row 2: one empty field */
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
/* Test: dest_len == 0 returns 3                                        */
/* ------------------------------------------------------------------ */
void test_csv_get_field_dest_len_zero_returns_3(void)
{
    buf = make_buffer_with_field("hello");
    TEST_ASSERT_NOT_NULL(buf);

    char dest[16] = {0};
    int ret = csv_get_field(dest, 0, buf, 0, 0);
    TEST_ASSERT_EQUAL_INT(3, ret);
}

/* ------------------------------------------------------------------ */
/* Test: row out of range returns 2 and clears dest                    */
/* ------------------------------------------------------------------ */
void test_csv_get_field_row_out_of_range_returns_2(void)
{
    buf = make_buffer_with_field("hello");
    TEST_ASSERT_NOT_NULL(buf);

    char dest[16];
    memset(dest, 'X', sizeof(dest));

    int ret = csv_get_field(dest, sizeof(dest) - 1, buf, 99, 0);
    TEST_ASSERT_EQUAL_INT(2, ret);
    /* dest[0] must be '\0' after clearing */
    TEST_ASSERT_EQUAL_INT('\0', dest[0]);
}

/* ------------------------------------------------------------------ */
/* Test: entry out of range returns 2 and clears dest                  */
/* ------------------------------------------------------------------ */
void test_csv_get_field_entry_out_of_range_returns_2(void)
{
    buf = make_buffer_with_field("hello");
    TEST_ASSERT_NOT_NULL(buf);

    char dest[16];
    memset(dest, 'X', sizeof(dest));

    int ret = csv_get_field(dest, sizeof(dest) - 1, buf, 0, 99);
    TEST_ASSERT_EQUAL_INT(2, ret);
    TEST_ASSERT_EQUAL_INT('\0', dest[0]);
}

/* ------------------------------------------------------------------ */
/* Test: normal copy returns 0 and copies text correctly               */
/* ------------------------------------------------------------------ */
void test_csv_get_field_normal_copy_returns_0(void)
{
    buf = make_buffer_with_field("hello");
    TEST_ASSERT_NOT_NULL(buf);

    char dest[16] = {0};
    int ret = csv_get_field(dest, sizeof(dest) - 1, buf, 0, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING("hello", dest);
}

/* ------------------------------------------------------------------ */
/* Test: empty field returns 2                                          */
/* ------------------------------------------------------------------ */
void test_csv_get_field_empty_field_returns_2(void)
{
    buf = make_buffer_with_field("");
    TEST_ASSERT_NOT_NULL(buf);

    char dest[16] = {0};
    int ret = csv_get_field(dest, sizeof(dest) - 1, buf, 0, 0);
    TEST_ASSERT_EQUAL_INT(2, ret);
}

/* ------------------------------------------------------------------ */
/* Test: truncation returns 1                                           */
/* ------------------------------------------------------------------ */
void test_csv_get_field_truncation_returns_1(void)
{
    buf = make_buffer_with_field("hello_world_long_string");
    TEST_ASSERT_NOT_NULL(buf);

    /* dest_len smaller than the field length */
    char dest[5] = {0};
    int ret = csv_get_field(dest, sizeof(dest) - 1, buf, 0, 0);
    TEST_ASSERT_EQUAL_INT(1, ret);
    /* Verify the copied portion */
    TEST_ASSERT_EQUAL_STRING_LEN("hell", dest, 4);
    /* Ensure null termination */
    TEST_ASSERT_EQUAL_INT('\0', dest[4]);
}

/* ------------------------------------------------------------------ */
/* Test: multi-row buffer, access second row                           */
/* ------------------------------------------------------------------ */
void test_csv_get_field_second_row_first_entry(void)
{
    buf = make_multi_buffer();
    TEST_ASSERT_NOT_NULL(buf);

    char dest[16] = {0};
    int ret = csv_get_field(dest, sizeof(dest) - 1, buf, 1, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING("foo", dest);
}

/* ------------------------------------------------------------------ */
/* Test: multi-row buffer, access first row second entry               */
/* ------------------------------------------------------------------ */
void test_csv_get_field_first_row_second_entry(void)
{
    buf = make_multi_buffer();
    TEST_ASSERT_NOT_NULL(buf);

    char dest[16] = {0};
    int ret = csv_get_field(dest, sizeof(dest) - 1, buf, 0, 1);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING("world", dest);
}

/* ------------------------------------------------------------------ */
/* Test: multi-row buffer, empty field in row 2 returns 2              */
/* ------------------------------------------------------------------ */
void test_csv_get_field_empty_field_in_row2_returns_2(void)
{
    buf = make_multi_buffer();
    TEST_ASSERT_NOT_NULL(buf);

    char dest[16] = {0};
    int ret = csv_get_field(dest, sizeof(dest) - 1, buf, 2, 0);
    TEST_ASSERT_EQUAL_INT(2, ret);
}

/* ------------------------------------------------------------------ */
/* Test: dest_len exactly fits the field (no truncation)               */
/* ------------------------------------------------------------------ */
void test_csv_get_field_exact_fit_returns_0(void)
{
    buf = make_buffer_with_field("abc");
    TEST_ASSERT_NOT_NULL(buf);

    /* dest_len = 3 means we can store 3 chars + null terminator */
    char dest[4] = {0};
    int ret = csv_get_field(dest, 3, buf, 0, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING("abc", dest);
}

/* ------------------------------------------------------------------ */
/* Test: dest_len one less than field length causes truncation          */
/* ------------------------------------------------------------------ */
void test_csv_get_field_one_less_than_field_length_returns_1(void)
{
    buf = make_buffer_with_field("abcd");
    TEST_ASSERT_NOT_NULL(buf);

    /* dest_len = 3, field is "abcd" (length 4) */
    char dest[5] = {0};
    int ret = csv_get_field(dest, 3, buf, 0, 0);
    TEST_ASSERT_EQUAL_INT(1, ret);
    TEST_ASSERT_EQUAL_STRING_LEN("abc", dest, 3);
}

/* ------------------------------------------------------------------ */
/* Test: single character field, dest_len = 1 returns 0                */
/* ------------------------------------------------------------------ */
void test_csv_get_field_single_char_field_returns_0(void)
{
    buf = make_buffer_with_field("z");
    TEST_ASSERT_NOT_NULL(buf);

    char dest[4] = {0};
    int ret = csv_get_field(dest, 3, buf, 0, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING("z", dest);
}

/* ------------------------------------------------------------------ */
/* Test: row == src->rows (boundary) returns 2                         */
/* ------------------------------------------------------------------ */
void test_csv_get_field_row_equals_rows_boundary_returns_2(void)
{
    buf = make_buffer_with_field("test");
    TEST_ASSERT_NOT_NULL(buf);

    /* buf->rows == 1, so row == 1 is out of range */
    char dest[16];
    memset(dest, 'A', sizeof(dest));

    int ret = csv_get_field(dest, sizeof(dest) - 1, buf, buf->rows, 0);
    TEST_ASSERT_EQUAL_INT(2, ret);
    TEST_ASSERT_EQUAL_INT('\0', dest[0]);
}

/* ------------------------------------------------------------------ */
/* Test: entry == src->width[row] (boundary) returns 2                 */
/* ------------------------------------------------------------------ */
void test_csv_get_field_entry_equals_width_boundary_returns_2(void)
{
    buf = make_buffer_with_field("test");
    TEST_ASSERT_NOT_NULL(buf);

    /* buf->width[0] == 1, so entry == 1 is out of range */
    char dest[16];
    memset(dest, 'A', sizeof(dest));

    int ret = csv_get_field(dest, sizeof(dest) - 1, buf, 0, buf->width[0]);
    TEST_ASSERT_EQUAL_INT(2, ret);
    TEST_ASSERT_EQUAL_INT('\0', dest[0]);
}

/* ------------------------------------------------------------------ */
/* main                                                                 */
/* ------------------------------------------------------------------ */
int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_csv_get_field_dest_len_zero_returns_3);
    RUN_TEST(test_csv_get_field_row_out_of_range_returns_2);
    RUN_TEST(test_csv_get_field_entry_out_of_range_returns_2);
    RUN_TEST(test_csv_get_field_normal_copy_returns_0);
    RUN_TEST(test_csv_get_field_empty_field_returns_2);
    RUN_TEST(test_csv_get_field_truncation_returns_1);
    RUN_TEST(test_csv_get_field_second_row_first_entry);
    RUN_TEST(test_csv_get_field_first_row_second_entry);
    RUN_TEST(test_csv_get_field_empty_field_in_row2_returns_2);
    RUN_TEST(test_csv_get_field_exact_fit_returns_0);
    RUN_TEST(test_csv_get_field_one_less_than_field_length_returns_1);
    RUN_TEST(test_csv_get_field_single_char_field_returns_0);
    RUN_TEST(test_csv_get_field_row_equals_rows_boundary_returns_2);
    RUN_TEST(test_csv_get_field_entry_equals_width_boundary_returns_2);
    return UNITY_END();
}