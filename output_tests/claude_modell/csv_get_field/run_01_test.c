#include "unity.h"
#include "csv.h"
#include <string.h>
#include <stdlib.h>

/* File-scope fixtures */
static CSV_BUFFER *test_buffer;

/* ---------------------------------------------------------------------------
 * Helper: build a small CSV_BUFFER manually so we do not depend on csv_load.
 * Layout:
 *   row 0: ["hello", "world"]
 *   row 1: [""]          (one empty field, length == 0)
 *   row 2: ["abcdefghij"] (10-char field for truncation test)
 * --------------------------------------------------------------------------*/
static CSV_FIELD *make_field(const char *text)
{
    CSV_FIELD *f = (CSV_FIELD *)malloc(sizeof(CSV_FIELD));
    if (!f) return NULL;
    if (text && text[0] != '\0') {
        size_t len = strlen(text);
        f->text = (char *)malloc(len + 1);
        if (!f->text) { free(f); return NULL; }
        memcpy(f->text, text, len + 1);
        f->length = len;
    } else {
        /* empty field */
        f->text = (char *)malloc(1);
        if (!f->text) { free(f); return NULL; }
        f->text[0] = '\0';
        f->length = 0;
    }
    return f;
}

static void free_field(CSV_FIELD *f)
{
    if (!f) return;
    free(f->text);
    free(f);
}

static CSV_BUFFER *build_test_buffer(void)
{
    CSV_BUFFER *buf = (CSV_BUFFER *)malloc(sizeof(CSV_BUFFER));
    if (!buf) return NULL;

    buf->rows        = 3;
    buf->field_delim = ',';
    buf->text_delim  = '"';

    /* width array */
    buf->width = (size_t *)malloc(3 * sizeof(size_t));
    if (!buf->width) { free(buf); return NULL; }
    buf->width[0] = 2;   /* row 0: 2 fields */
    buf->width[1] = 1;   /* row 1: 1 field  */
    buf->width[2] = 1;   /* row 2: 1 field  */

    /* field array: rows */
    buf->field = (CSV_FIELD ***)malloc(3 * sizeof(CSV_FIELD **));
    if (!buf->field) { free(buf->width); free(buf); return NULL; }

    /* row 0 */
    buf->field[0] = (CSV_FIELD **)malloc(2 * sizeof(CSV_FIELD *));
    buf->field[0][0] = make_field("hello");
    buf->field[0][1] = make_field("world");

    /* row 1 – empty field */
    buf->field[1] = (CSV_FIELD **)malloc(1 * sizeof(CSV_FIELD *));
    buf->field[1][0] = make_field("");

    /* row 2 – long field */
    buf->field[2] = (CSV_FIELD **)malloc(1 * sizeof(CSV_FIELD *));
    buf->field[2][0] = make_field("abcdefghij");

    return buf;
}

static void destroy_test_buffer(CSV_BUFFER *buf)
{
    size_t r, e;
    if (!buf) return;
    for (r = 0; r < buf->rows; r++) {
        for (e = 0; e < buf->width[r]; e++) {
            free_field(buf->field[r][e]);
        }
        free(buf->field[r]);
    }
    free(buf->field);
    free(buf->width);
    free(buf);
}

/* ---------------------------------------------------------------------------
 * setUp / tearDown
 * --------------------------------------------------------------------------*/
void setUp(void)
{
    test_buffer = build_test_buffer();
}

void tearDown(void)
{
    destroy_test_buffer(test_buffer);
    test_buffer = NULL;
}

/* ===========================================================================
 * Test cases
 * ==========================================================================*/

/* 1. dest_len == 0 must return 3 immediately, dest untouched */
void test_csv_get_field_returns_3_when_dest_len_is_zero(void)
{
    char dest[8] = "XXXXXXX";
    int ret = csv_get_field(dest, 0, test_buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(3, ret,
        "csv_get_field should return 3 when dest_len is 0");
    /* dest must not have been modified */
    TEST_ASSERT_EQUAL_STRING_MESSAGE("XXXXXXX", dest,
        "dest buffer must be untouched when dest_len is 0");
}

/* 2. Valid row/entry, dest large enough → return 0, correct string copied */
void test_csv_get_field_returns_0_and_copies_string_when_valid(void)
{
    char dest[32];
    memset(dest, 0xAB, sizeof(dest));
    int ret = csv_get_field(dest, sizeof(dest) - 1, test_buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret,
        "csv_get_field should return 0 for a successful full copy");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("hello", dest,
        "dest should contain the field text 'hello'");
}

/* 3. Row out of range → return 2, dest cleared to '\0' */
void test_csv_get_field_returns_2_when_row_out_of_range(void)
{
    char dest[16];
    memset(dest, 0xFF, sizeof(dest));
    /* row 99 does not exist */
    int ret = csv_get_field(dest, sizeof(dest) - 1, test_buffer, 99, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, ret,
        "csv_get_field should return 2 when the requested row does not exist");
    TEST_ASSERT_EQUAL_INT_MESSAGE('\0', dest[0],
        "dest[0] should be '\\0' when row is out of range");
}

/* 4. Empty field (length == 0) → return 2 */
void test_csv_get_field_returns_2_for_empty_field(void)
{
    char dest[16];
    memset(dest, 0xAB, sizeof(dest));
    /* row 1, entry 0 is an empty field */
    int ret = csv_get_field(dest, sizeof(dest) - 1, test_buffer, 1, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, ret,
        "csv_get_field should return 2 when the field is empty");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("", dest,
        "dest should be an empty string for an empty field");
}

/* 5. dest_len smaller than field length → return 1, string truncated */
void test_csv_get_field_returns_1_and_truncates_when_dest_too_small(void)
{
    /* row 2, entry 0 is "abcdefghij" (10 chars).
     * Provide dest_len = 4 so only "abcd" fits (plus NUL at dest[4]). */
    char dest[8];
    memset(dest, 0xAB, sizeof(dest));
    int ret = csv_get_field(dest, 4, test_buffer, 2, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, ret,
        "csv_get_field should return 1 when the field is truncated");
    TEST_ASSERT_EQUAL_STRING_LEN_MESSAGE("abcd", dest, 4,
        "dest should contain the first 4 characters of the field");
    TEST_ASSERT_EQUAL_INT_MESSAGE('\0', dest[4],
        "dest[dest_len] must be NUL-terminated after truncation");
}

/* ===========================================================================
 * main
 * ==========================================================================*/
int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_csv_get_field_returns_3_when_dest_len_is_zero);
    RUN_TEST(test_csv_get_field_returns_0_and_copies_string_when_valid);
    RUN_TEST(test_csv_get_field_returns_2_when_row_out_of_range);
    RUN_TEST(test_csv_get_field_returns_2_for_empty_field);
    RUN_TEST(test_csv_get_field_returns_1_and_truncates_when_dest_too_small);
    return UNITY_END();
}