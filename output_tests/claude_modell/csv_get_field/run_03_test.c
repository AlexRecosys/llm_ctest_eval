#include "unity.h"
#include "csv.h"
#include <string.h>
#include <stdlib.h>

/* File-scope fixtures */
static CSV_BUFFER *buffer;

/* Helper: build a minimal CSV_BUFFER with one row and one field manually */
static CSV_FIELD *make_field(const char *text)
{
    CSV_FIELD *f = (CSV_FIELD *)malloc(sizeof(CSV_FIELD));
    if (!f) return NULL;
    size_t len = strlen(text);
    f->text = (char *)malloc(len + 1);
    if (!f->text) { free(f); return NULL; }
    strcpy(f->text, text);
    f->length = len;
    return f;
}

static void free_field(CSV_FIELD *f)
{
    if (!f) return;
    free(f->text);
    free(f);
}

/*
 * Build a CSV_BUFFER from scratch without using any internal static helpers.
 * Layout: rows x cols, all cells set to the provided text strings.
 *
 * texts[r][c] must be a valid C string.
 */
static CSV_BUFFER *build_buffer(size_t rows, size_t *widths, const char ***texts)
{
    CSV_BUFFER *buf = (CSV_BUFFER *)malloc(sizeof(CSV_BUFFER));
    if (!buf) return NULL;

    buf->rows = rows;
    buf->field_delim = ',';
    buf->text_delim  = '"';

    buf->width = (size_t *)malloc(sizeof(size_t) * rows);
    if (!buf->width) { free(buf); return NULL; }

    buf->field = (CSV_FIELD ***)malloc(sizeof(CSV_FIELD **) * rows);
    if (!buf->field) { free(buf->width); free(buf); return NULL; }

    for (size_t r = 0; r < rows; r++) {
        buf->width[r] = widths[r];
        buf->field[r] = (CSV_FIELD **)malloc(sizeof(CSV_FIELD *) * widths[r]);
        if (!buf->field[r]) {
            /* cleanup already allocated rows */
            for (size_t k = 0; k < r; k++) {
                for (size_t c = 0; c < widths[k]; c++)
                    free_field(buf->field[k][c]);
                free(buf->field[k]);
            }
            free(buf->field);
            free(buf->width);
            free(buf);
            return NULL;
        }
        for (size_t c = 0; c < widths[r]; c++) {
            buf->field[r][c] = make_field(texts[r][c]);
        }
    }
    return buf;
}

static void destroy_buffer(CSV_BUFFER *buf)
{
    if (!buf) return;
    for (size_t r = 0; r < buf->rows; r++) {
        for (size_t c = 0; c < buf->width[r]; c++)
            free_field(buf->field[r][c]);
        free(buf->field[r]);
    }
    free(buf->field);
    free(buf->width);
    free(buf);
}

/* ------------------------------------------------------------------ */
/* setUp / tearDown                                                     */
/* ------------------------------------------------------------------ */

void setUp(void)
{
    buffer = NULL;
}

void tearDown(void)
{
    if (buffer) {
        destroy_buffer(buffer);
        buffer = NULL;
    }
}

/* ------------------------------------------------------------------ */
/* Test cases                                                           */
/* ------------------------------------------------------------------ */

/*
 * Test 1: dest_len == 0 should return 3 immediately without touching dest.
 */
void test_csv_get_field_returns_3_when_dest_len_is_zero(void)
{
    size_t widths[1] = {1};
    const char *row0[1] = {"hello"};
    const char **texts[1] = {row0};

    buffer = build_buffer(1, widths, texts);
    TEST_ASSERT_NOT_NULL_MESSAGE(buffer, "Failed to build test buffer");

    char dest[16] = "UNCHANGED";
    int ret = csv_get_field(dest, 0, buffer, 0, 0);

    TEST_ASSERT_EQUAL_INT_MESSAGE(3, ret,
        "Expected return 3 when dest_len is 0");
    /* dest must not have been modified */
    TEST_ASSERT_EQUAL_STRING_MESSAGE("UNCHANGED", dest,
        "dest should be untouched when dest_len is 0");
}

/*
 * Test 2: Requesting a row that does not exist should return 2 and
 *         clear dest.
 */
void test_csv_get_field_returns_2_for_invalid_row(void)
{
    size_t widths[1] = {1};
    const char *row0[1] = {"data"};
    const char **texts[1] = {row0};

    buffer = build_buffer(1, widths, texts);
    TEST_ASSERT_NOT_NULL_MESSAGE(buffer, "Failed to build test buffer");

    char dest[16];
    memset(dest, 0xAB, sizeof(dest));   /* fill with non-zero sentinel */

    int ret = csv_get_field(dest, sizeof(dest) - 1, buffer, 99, 0);

    TEST_ASSERT_EQUAL_INT_MESSAGE(2, ret,
        "Expected return 2 for out-of-range row");
    /* The function clears dest[0] in a loop; at minimum dest[0] must be NUL */
    TEST_ASSERT_EQUAL_HEX(0x00, (unsigned char)dest[0]);
}

/*
 * Test 3: Requesting an entry index beyond the row width should return 2
 *         and clear dest.
 */
void test_csv_get_field_returns_2_for_invalid_entry(void)
{
    size_t widths[1] = {2};
    const char *row0[2] = {"alpha", "beta"};
    const char **texts[1] = {row0};

    buffer = build_buffer(1, widths, texts);
    TEST_ASSERT_NOT_NULL_MESSAGE(buffer, "Failed to build test buffer");

    char dest[32];
    memset(dest, 0xFF, sizeof(dest));

    int ret = csv_get_field(dest, sizeof(dest) - 1, buffer, 0, 5);

    TEST_ASSERT_EQUAL_INT_MESSAGE(2, ret,
        "Expected return 2 for out-of-range entry index");
    TEST_ASSERT_EQUAL_HEX(0x00, (unsigned char)dest[0]);
}

/*
 * Test 4: A valid, non-empty field that fits entirely in dest should
 *         return 0 and copy the text correctly.
 */
void test_csv_get_field_returns_0_and_copies_full_string(void)
{
    size_t widths[2] = {3, 2};
    const char *row0[3] = {"one", "two", "three"};
    const char *row1[2] = {"four", "five"};
    const char **texts[2] = {row0, row1};

    buffer = build_buffer(2, widths, texts);
    TEST_ASSERT_NOT_NULL_MESSAGE(buffer, "Failed to build test buffer");

    char dest[64];
    memset(dest, 0, sizeof(dest));

    /* Retrieve row 1, entry 1 => "five" */
    int ret = csv_get_field(dest, sizeof(dest) - 1, buffer, 1, 1);

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret,
        "Expected return 0 for a fully copied field");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("five", dest,
        "dest should contain the exact field text");
}

/*
 * Test 5: When dest_len is smaller than the field length, the string
 *         should be truncated and the function should return 1.
 */
void test_csv_get_field_returns_1_when_truncated(void)
{
    size_t widths[1] = {1};
    const char *row0[1] = {"Hello, World!"};   /* length 13 */
    const char **texts[1] = {row0};

    buffer = build_buffer(1, widths, texts);
    TEST_ASSERT_NOT_NULL_MESSAGE(buffer, "Failed to build test buffer");

    /* dest_len = 5, so only "Hello" fits (5 chars + NUL written at dest[5]) */
    char dest[8];
    memset(dest, 0xCC, sizeof(dest));

    int ret = csv_get_field(dest, 5, buffer, 0, 0);

    TEST_ASSERT_EQUAL_INT_MESSAGE(1, ret,
        "Expected return 1 when field is truncated");
    /* strncpy copies 5 bytes, then dest[5] = '\0' */
    TEST_ASSERT_EQUAL_STRING_LEN_MESSAGE("Hello", dest, 5,
        "dest should contain the first 5 characters of the field");
    TEST_ASSERT_EQUAL_HEX(0x00, (unsigned char)dest[5]);
}

/* ------------------------------------------------------------------ */
/* main                                                                 */
/* ------------------------------------------------------------------ */

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_csv_get_field_returns_3_when_dest_len_is_zero);
    RUN_TEST(test_csv_get_field_returns_2_for_invalid_row);
    RUN_TEST(test_csv_get_field_returns_2_for_invalid_entry);
    RUN_TEST(test_csv_get_field_returns_0_and_copies_full_string);
    RUN_TEST(test_csv_get_field_returns_1_when_truncated);

    return UNITY_END();
}