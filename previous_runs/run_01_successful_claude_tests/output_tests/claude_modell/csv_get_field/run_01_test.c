#include "unity.h"
#include <stdlib.h>
#include <string.h>

/* ── Project types (inline, no separate header assumed) ─────────────────── */

typedef struct {
    char   *text;
    size_t  length;
} CSV_FIELD;

typedef struct {
    CSV_FIELD ***field;
    size_t       rows;
    size_t      *width;
    char         field_delim;
    char         text_delim;
} CSV_BUFFER;

/* ── Function under test (forward declaration) ──────────────────────────── */

int csv_get_field(char *dest, size_t dest_len,
                  CSV_BUFFER *src, size_t row, size_t entry);

/* ── File-scope fixtures ────────────────────────────────────────────────── */

static CSV_BUFFER *g_buf;

/* ── Helper functions ───────────────────────────────────────────────────── */

/*
 * Allocate a single CSV_FIELD with the given text.
 * length is set to strlen(text) (0 for an empty string).
 */
static CSV_FIELD *make_field(const char *text)
{
    CSV_FIELD *f = (CSV_FIELD *)malloc(sizeof(CSV_FIELD));
    TEST_ASSERT_NOT_NULL_MESSAGE(f, "malloc failed for CSV_FIELD");
    f->text   = strdup(text);
    f->length = strlen(text);
    TEST_ASSERT_NOT_NULL_MESSAGE(f->text, "strdup failed");
    return f;
}

/*
 * Build a CSV_BUFFER with `rows` rows.
 * widths[]  – number of columns per row
 * texts[][] – NULL-terminated list of strings per row (row-major)
 *
 * Caller must free with free_csv_buffer().
 */
static CSV_BUFFER *make_csv_buffer(size_t rows,
                                   const size_t *widths,
                                   const char * const * const *texts)
{
    size_t r, c;
    CSV_BUFFER *buf = (CSV_BUFFER *)malloc(sizeof(CSV_BUFFER));
    TEST_ASSERT_NOT_NULL_MESSAGE(buf, "malloc failed for CSV_BUFFER");

    buf->rows        = rows;
    buf->field_delim = ',';
    buf->text_delim  = '"';
    buf->width       = (size_t *)malloc(rows * sizeof(size_t));
    TEST_ASSERT_NOT_NULL_MESSAGE(buf->width, "malloc failed for width array");

    buf->field = (CSV_FIELD ***)malloc(rows * sizeof(CSV_FIELD **));
    TEST_ASSERT_NOT_NULL_MESSAGE(buf->field, "malloc failed for field array");

    for (r = 0; r < rows; r++) {
        buf->width[r] = widths[r];
        buf->field[r] = (CSV_FIELD **)malloc(widths[r] * sizeof(CSV_FIELD *));
        TEST_ASSERT_NOT_NULL_MESSAGE(buf->field[r], "malloc failed for row");
        for (c = 0; c < widths[r]; c++) {
            buf->field[r][c] = make_field(texts[r][c]);
        }
    }
    return buf;
}

static void free_csv_buffer(CSV_BUFFER *buf)
{
    size_t r, c;
    if (!buf) return;
    for (r = 0; r < buf->rows; r++) {
        for (c = 0; c < buf->width[r]; c++) {
            free(buf->field[r][c]->text);
            free(buf->field[r][c]);
        }
        free(buf->field[r]);
    }
    free(buf->field);
    free(buf->width);
    free(buf);
}

/* ── setUp / tearDown ───────────────────────────────────────────────────── */

void setUp(void)
{
    /*
     * Build a small 2-row buffer:
     *   row 0: "hello", "world"
     *   row 1: "",      "data"
     */
    static const char *row0[] = { "hello", "world" };
    static const char *row1[] = { "",      "data"  };
    static const char * const *texts[] = { row0, row1 };
    static const size_t widths[]       = { 2, 2 };

    g_buf = make_csv_buffer(2, widths,
                            (const char * const * const *)texts);
}

void tearDown(void)
{
    free_csv_buffer(g_buf);
    g_buf = NULL;
}

/* ── Test cases ─────────────────────────────────────────────────────────── */

/*
 * Test 1 – dest_len == 0 must return 3 immediately.
 */
void test_csv_get_field_returns_3_when_dest_len_is_zero(void)
{
    char dest[16] = "unchanged";
    int  rc = csv_get_field(dest, 0, g_buf, 0, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(3, rc,
        "Expected return code 3 when dest_len is 0");
    /* dest must not have been touched */
    TEST_ASSERT_EQUAL_STRING_MESSAGE("unchanged", dest,
        "dest buffer must not be modified when dest_len is 0");
}

/*
 * Test 2 – row index out of range must return 2 and clear dest.
 */
void test_csv_get_field_returns_2_for_invalid_row(void)
{
    char dest[16];
    memset(dest, 0xAB, sizeof(dest));   /* fill with non-zero sentinel */

    int rc = csv_get_field(dest, sizeof(dest) - 1, g_buf,
                           99 /*invalid row*/, 0);

    TEST_ASSERT_EQUAL_INT_MESSAGE(2, rc,
        "Expected return code 2 for out-of-range row");
    /* The function clears dest[0] in a loop; at minimum dest[0] must be NUL */
    TEST_ASSERT_EQUAL_HEX8_MESSAGE(0x00, (unsigned char)dest[0],
        "dest[0] must be NUL when row is invalid");
}

/*
 * Test 3 – column index out of range must return 2 and clear dest.
 */
void test_csv_get_field_returns_2_for_invalid_entry(void)
{
    char dest[16];
    memset(dest, 0xCD, sizeof(dest));

    int rc = csv_get_field(dest, sizeof(dest) - 1, g_buf,
                           0, 99 /*invalid column*/);

    TEST_ASSERT_EQUAL_INT_MESSAGE(2, rc,
        "Expected return code 2 for out-of-range entry");
    TEST_ASSERT_EQUAL_HEX8_MESSAGE(0x00, (unsigned char)dest[0],
        "dest[0] must be NUL when entry is invalid");
}

/*
 * Test 4 – valid field with content that fits in dest must return 0
 *           and copy the text correctly.
 */
void test_csv_get_field_returns_0_and_copies_text_for_valid_field(void)
{
    char dest[32];
    memset(dest, 0, sizeof(dest));

    /* row 0, entry 0 → "hello" */
    int rc = csv_get_field(dest, sizeof(dest) - 1, g_buf, 0, 0);

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, rc,
        "Expected return code 0 for a normal, fitting field");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("hello", dest,
        "dest must contain the field text 'hello'");
}

/*
 * Test 5 – when dest_len is smaller than the field length the function
 *           must truncate the text and return 1.
 *
 *   row 0, entry 1 → "world" (length 5).
 *   We provide dest_len = 3, so only "wor" fits and return code must be 1.
 */
void test_csv_get_field_returns_1_and_truncates_when_dest_too_small(void)
{
    /* dest needs at least dest_len+1 bytes so the NUL write is safe */
    char dest[8];
    memset(dest, 0xFF, sizeof(dest));

    /* dest_len = 3: strncpy copies 3 chars, then dest[3] = '\0' */
    int rc = csv_get_field(dest, 3, g_buf, 0, 1 /* "world" */);

    TEST_ASSERT_EQUAL_INT_MESSAGE(1, rc,
        "Expected return code 1 when field is truncated");
    TEST_ASSERT_EQUAL_STRING_LEN_MESSAGE("wor", dest, 3,
        "dest must contain the first 3 characters of 'world'");
    TEST_ASSERT_EQUAL_HEX8_MESSAGE(0x00, (unsigned char)dest[3],
        "dest[dest_len] must be NUL-terminated after truncation");
}

/* ── main ───────────────────────────────────────────────────────────────── */

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_csv_get_field_returns_3_when_dest_len_is_zero);
    RUN_TEST(test_csv_get_field_returns_2_for_invalid_row);
    RUN_TEST(test_csv_get_field_returns_2_for_invalid_entry);
    RUN_TEST(test_csv_get_field_returns_0_and_copies_text_for_valid_field);
    RUN_TEST(test_csv_get_field_returns_1_and_truncates_when_dest_too_small);
    return UNITY_END();
}