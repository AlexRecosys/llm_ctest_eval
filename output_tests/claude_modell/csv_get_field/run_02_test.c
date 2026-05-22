#include "unity.h"
#include "csv.h"
#include <string.h>
#include <stdlib.h>

/* File-scope fixtures */
static CSV_BUFFER *buffer;

/* ---------------------------------------------------------------------------
 * Helper: build a small CSV_BUFFER manually so we do not depend on csv_load.
 * Layout:
 *   row 0: ["hello", "world"]
 *   row 1: [""]            (one empty field)
 *   row 2: ["truncation_test_long_value"]
 * --------------------------------------------------------------------------*/
static CSV_FIELD *make_field(const char *text)
{
    CSV_FIELD *f = (CSV_FIELD *)malloc(sizeof(CSV_FIELD));
    if (!f) return NULL;
    if (text && text[0] != '\0') {
        f->length = strlen(text);
        f->text   = (char *)malloc(f->length + 1);
        if (!f->text) { free(f); return NULL; }
        strcpy(f->text, text);
    } else {
        f->length = 0;
        f->text   = (char *)malloc(1);
        if (!f->text) { free(f); return NULL; }
        f->text[0] = '\0';
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
    buf->width[0] = 2;
    buf->width[1] = 1;
    buf->width[2] = 1;

    /* field array: rows x entries */
    buf->field = (CSV_FIELD ***)malloc(3 * sizeof(CSV_FIELD **));
    if (!buf->field) { free(buf->width); free(buf); return NULL; }

    /* row 0: "hello", "world" */
    buf->field[0] = (CSV_FIELD **)malloc(2 * sizeof(CSV_FIELD *));
    buf->field[0][0] = make_field("hello");
    buf->field[0][1] = make_field("world");

    /* row 1: "" (empty field) */
    buf->field[1] = (CSV_FIELD **)malloc(1 * sizeof(CSV_FIELD *));
    buf->field[1][0] = make_field("");

    /* row 2: long value for truncation test */
    buf->field[2] = (CSV_FIELD **)malloc(1 * sizeof(CSV_FIELD *));
    buf->field[2][0] = make_field("truncation_test_long_value");

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
    buffer = build_test_buffer();
}

void tearDown(void)
{
    destroy_test_buffer(buffer);
    buffer = NULL;
}

/* ===========================================================================
 * Test cases
 * ===========================================================================*/

/* 1. dest_len == 0 must return 3 immediately without touching dest */
void test_csv_get_field_returns_3_when_dest_len_is_zero(void)
{
    char dest[16];
    memset(dest, 0xAB, sizeof(dest)); /* sentinel fill */
    int ret = csv_get_field(dest, 0, buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(3, ret,
        "csv_get_field should return 3 when dest_len is 0");
    /* dest must be untouched */
    TEST_ASSERT_EQUAL_HEX(0xAB, (unsigned char)dest[0]);
}

/* 2. Row out of range: return 2 and dest cleared */
void test_csv_get_field_returns_2_when_row_out_of_range(void)
{
    char dest[16];
    memset(dest, 0xFF, sizeof(dest));
    int ret = csv_get_field(dest, sizeof(dest) - 1, buffer, 99, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, ret,
        "csv_get_field should return 2 for a non-existent row");
    /* The function sets dest[0] = '\0' in a loop; verify first byte */
    TEST_ASSERT_EQUAL_HEX_MESSAGE(0x00, (unsigned char)dest[0],
        "dest[0] should be NUL when row does not exist");
}

/* 3. Entry out of range: return 2 and dest cleared */
void test_csv_get_field_returns_2_when_entry_out_of_range(void)
{
    char dest[16];
    memset(dest, 0xFF, sizeof(dest));
    /* row 0 has width 2, so entry 5 is out of range */
    int ret = csv_get_field(dest, sizeof(dest) - 1, buffer, 0, 5);
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, ret,
        "csv_get_field should return 2 for a non-existent entry");
    TEST_ASSERT_EQUAL_HEX_MESSAGE(0x00, (unsigned char)dest[0],
        "dest[0] should be NUL when entry does not exist");
}

/* 4. Normal successful copy: return 0 and correct string */
void test_csv_get_field_returns_0_and_copies_string_correctly(void)
{
    char dest[32];
    memset(dest, 0, sizeof(dest));
    /* row 0, entry 0 => "hello" (length 5, dest_len 31 > 5) */
    int ret = csv_get_field(dest, sizeof(dest) - 1, buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret,
        "csv_get_field should return 0 when full copy succeeds");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("hello", dest,
        "dest should contain 'hello' after successful copy");
}

/* 5. Truncation: dest_len smaller than field length => return 1 */
void test_csv_get_field_returns_1_when_truncation_occurs(void)
{
    /* row 2, entry 0 => "truncation_test_long_value" (26 chars)
     * We provide a dest of only 5 usable bytes (dest_len = 5).
     * The function writes at most dest_len chars then NUL-terminates
     * at dest[dest_len], so dest must be at least dest_len+1 bytes. */
    char dest[8];   /* 7 usable + 1 for the forced NUL at dest[dest_len] */
    memset(dest, 0, sizeof(dest));
    int ret = csv_get_field(dest, 5, buffer, 2, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, ret,
        "csv_get_field should return 1 when the field is truncated");
    /* First 5 chars of "truncation_test_long_value" are "trunc" */
    TEST_ASSERT_EQUAL_STRING_LEN_MESSAGE("trunc", dest, 5,
        "dest should hold the first 5 characters of the truncated field");
}

/* ===========================================================================
 * main
 * ===========================================================================*/
int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_csv_get_field_returns_3_when_dest_len_is_zero);
    RUN_TEST(test_csv_get_field_returns_2_when_row_out_of_range);
    RUN_TEST(test_csv_get_field_returns_2_when_entry_out_of_range);
    RUN_TEST(test_csv_get_field_returns_0_and_copies_string_correctly);
    RUN_TEST(test_csv_get_field_returns_1_when_truncation_occurs);
    return UNITY_END();
}