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
    /* append_row and append_field are static; use csv_set_field which calls them */
    /* We need at least one row and one field */
    csv_set_field(b, 0, 0, (char *)text);
    return b;
}

/* Helper: build a buffer with multiple rows and fields */
static CSV_BUFFER *make_multi_buffer(void)
{
    CSV_BUFFER *b = csv_create_buffer();
    if (!b) return NULL;
    csv_set_field(b, 0, 0, "hello");
    csv_set_field(b, 0, 1, "world");
    csv_set_field(b, 1, 0, "foo");
    csv_set_field(b, 1, 1, "bar");
    csv_set_field(b, 1, 2, "baz");
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

/* -----------------------------------------------------------------------
 * Test: dest_len == 0 should return 3
 * --------------------------------------------------------------------- */
void test_csv_get_field_returns_3_when_dest_len_zero(void)
{
    buf = make_buffer_with_field("hello");
    TEST_ASSERT_NOT_NULL(buf);

    char dest[16] = {0};
    int ret = csv_get_field(dest, 0, buf, 0, 0);
    TEST_ASSERT_EQUAL_INT(3, ret);
}

/* -----------------------------------------------------------------------
 * Test: row out of range returns 2 and clears dest
 * --------------------------------------------------------------------- */
void test_csv_get_field_returns_2_when_row_out_of_range(void)
{
    buf = make_buffer_with_field("hello");
    TEST_ASSERT_NOT_NULL(buf);

    char dest[16];
    memset(dest, 'X', sizeof(dest));

    int ret = csv_get_field(dest, sizeof(dest) - 1, buf, 99, 0);
    TEST_ASSERT_EQUAL_INT(2, ret);
    /* dest[0] should be '\0' after clearing */
    TEST_ASSERT_EQUAL_INT('\0', dest[0]);
}

/* -----------------------------------------------------------------------
 * Test: entry out of range returns 2 and clears dest
 * --------------------------------------------------------------------- */
void test_csv_get_field_returns_2_when_entry_out_of_range(void)
{
    buf = make_buffer_with_field("hello");
    TEST_ASSERT_NOT_NULL(buf);

    char dest[16];
    memset(dest, 'X', sizeof(dest));

    int ret = csv_get_field(dest, sizeof(dest) - 1, buf, 0, 99);
    TEST_ASSERT_EQUAL_INT(2, ret);
    TEST_ASSERT_EQUAL_INT('\0', dest[0]);
}

/* -----------------------------------------------------------------------
 * Test: valid field copied correctly, returns 0
 * --------------------------------------------------------------------- */
void test_csv_get_field_returns_0_and_copies_field(void)
{
    buf = make_buffer_with_field("hello");
    TEST_ASSERT_NOT_NULL(buf);

    char dest[16] = {0};
    int ret = csv_get_field(dest, sizeof(dest) - 1, buf, 0, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING("hello", dest);
}

/* -----------------------------------------------------------------------
 * Test: dest_len smaller than field length causes truncation, returns 1
 * --------------------------------------------------------------------- */
void test_csv_get_field_returns_1_when_truncated(void)
{
    buf = make_buffer_with_field("hello_world");
    TEST_ASSERT_NOT_NULL(buf);

    char dest[5] = {0};
    /* dest_len = 4, field is "hello_world" (11 chars) */
    int ret = csv_get_field(dest, 4, buf, 0, 0);
    TEST_ASSERT_EQUAL_INT(1, ret);
    /* dest should contain first 4 chars then null */
    TEST_ASSERT_EQUAL_STRING_LEN("hell", dest, 4);
    TEST_ASSERT_EQUAL_INT('\0', dest[4]);
}

/* -----------------------------------------------------------------------
 * Test: empty field returns 2
 * --------------------------------------------------------------------- */
void test_csv_get_field_returns_2_for_empty_field(void)
{
    buf = make_buffer_with_field("");
    TEST_ASSERT_NOT_NULL(buf);

    char dest[16] = {0};
    int ret = csv_get_field(dest, sizeof(dest) - 1, buf, 0, 0);
    TEST_ASSERT_EQUAL_INT(2, ret);
}

/* -----------------------------------------------------------------------
 * Test: multi-row buffer, access second row first field
 * --------------------------------------------------------------------- */
void test_csv_get_field_second_row_first_entry(void)
{
    buf = make_multi_buffer();
    TEST_ASSERT_NOT_NULL(buf);

    char dest[16] = {0};
    int ret = csv_get_field(dest, sizeof(dest) - 1, buf, 1, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING("foo", dest);
}

/* -----------------------------------------------------------------------
 * Test: multi-row buffer, access second row third field
 * --------------------------------------------------------------------- */
void test_csv_get_field_second_row_third_entry(void)
{
    buf = make_multi_buffer();
    TEST_ASSERT_NOT_NULL(buf);

    char dest[16] = {0};
    int ret = csv_get_field(dest, sizeof(dest) - 1, buf, 1, 2);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING("baz", dest);
}

/* -----------------------------------------------------------------------
 * Test: multi-row buffer, access first row second field
 * --------------------------------------------------------------------- */
void test_csv_get_field_first_row_second_entry(void)
{
    buf = make_multi_buffer();
    TEST_ASSERT_NOT_NULL(buf);

    char dest[16] = {0};
    int ret = csv_get_field(dest, sizeof(dest) - 1, buf, 0, 1);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING("world", dest);
}

/* -----------------------------------------------------------------------
 * Test: entry index equal to width (one past last) returns 2
 * --------------------------------------------------------------------- */
void test_csv_get_field_entry_equal_to_width_returns_2(void)
{
    buf = make_multi_buffer();
    TEST_ASSERT_NOT_NULL(buf);

    /* row 0 has 2 entries (index 0 and 1), so entry 2 is out of range */
    char dest[16];
    memset(dest, 'X', sizeof(dest));
    int ret = csv_get_field(dest, sizeof(dest) - 1, buf, 0, 2);
    TEST_ASSERT_EQUAL_INT(2, ret);
    TEST_ASSERT_EQUAL_INT('\0', dest[0]);
}

/* -----------------------------------------------------------------------
 * Test: dest_len exactly equal to field length (no truncation), returns 0
 * --------------------------------------------------------------------- */
void test_csv_get_field_dest_len_exact_fit(void)
{
    /* "hi" has length 2, dest_len = 2 means we copy 2 chars + null at dest[2] */
    buf = make_buffer_with_field("hi");
    TEST_ASSERT_NOT_NULL(buf);

    char dest[4] = {0};
    /* dest_len = 2: strncpy copies 2 chars, dest[2] = '\0' */
    int ret = csv_get_field(dest, 2, buf, 0, 0);
    /* field->length should be 2, dest_len = 2, so length > dest_len+1 is 2 > 3 = false */
    /* length == 0 is false, so returns 0 */
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING("hi", dest);
}

/* -----------------------------------------------------------------------
 * Test: single character field, dest_len = 1
 * --------------------------------------------------------------------- */
void test_csv_get_field_single_char_field(void)
{
    buf = make_buffer_with_field("A");
    TEST_ASSERT_NOT_NULL(buf);

    char dest[4] = {0};
    int ret = csv_get_field(dest, 1, buf, 0, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT('A', dest[0]);
    TEST_ASSERT_EQUAL_INT('\0', dest[1]);
}

/* -----------------------------------------------------------------------
 * Test: row == src->rows (boundary, one past last row) returns 2
 * --------------------------------------------------------------------- */
void test_csv_get_field_row_equals_rows_boundary(void)
{
    buf = make_buffer_with_field("test");
    TEST_ASSERT_NOT_NULL(buf);

    /* buf->rows should be 1 after one row added */
    size_t rows = (size_t)csv_get_height(buf);
    char dest[16];
    memset(dest, 'X', sizeof(dest));

    int ret = csv_get_field(dest, sizeof(dest) - 1, buf, rows, 0);
    TEST_ASSERT_EQUAL_INT(2, ret);
    TEST_ASSERT_EQUAL_INT('\0', dest[0]);
}

/* -----------------------------------------------------------------------
 * Test: field with spaces copies correctly
 * --------------------------------------------------------------------- */
void test_csv_get_field_field_with_spaces(void)
{
    buf = make_buffer_with_field("hello world");
    TEST_ASSERT_NOT_NULL(buf);

    char dest[32] = {0};
    int ret = csv_get_field(dest, sizeof(dest) - 1, buf, 0, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING("hello world", dest);
}

/* -----------------------------------------------------------------------
 * Test: dest_len = 1 with long field truncates to 1 char, returns 1
 * --------------------------------------------------------------------- */
void test_csv_get_field_dest_len_1_truncates(void)
{
    buf = make_buffer_with_field("abcdef");
    TEST_ASSERT_NOT_NULL(buf);

    char dest[4] = {0};
    int ret = csv_get_field(dest, 1, buf, 0, 0);
    TEST_ASSERT_EQUAL_INT(1, ret);
    TEST_ASSERT_EQUAL_INT('a', dest[0]);
    TEST_ASSERT_EQUAL_INT('\0', dest[1]);
}

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_csv_get_field_returns_3_when_dest_len_zero);
    RUN_TEST(test_csv_get_field_returns_2_when_row_out_of_range);
    RUN_TEST(test_csv_get_field_returns_2_when_entry_out_of_range);
    RUN_TEST(test_csv_get_field_returns_0_and_copies_field);
    RUN_TEST(test_csv_get_field_returns_1_when_truncated);
    RUN_TEST(test_csv_get_field_returns_2_for_empty_field);
    RUN_TEST(test_csv_get_field_second_row_first_entry);
    RUN_TEST(test_csv_get_field_second_row_third_entry);
    RUN_TEST(test_csv_get_field_first_row_second_entry);
    RUN_TEST(test_csv_get_field_entry_equal_to_width_returns_2);
    RUN_TEST(test_csv_get_field_dest_len_exact_fit);
    RUN_TEST(test_csv_get_field_single_char_field);
    RUN_TEST(test_csv_get_field_row_equals_rows_boundary);
    RUN_TEST(test_csv_get_field_field_with_spaces);
    RUN_TEST(test_csv_get_field_dest_len_1_truncates);
    return UNITY_END();
}