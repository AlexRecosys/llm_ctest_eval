#include "unity.h"
#include <stdlib.h>
#include <string.h>

/* ── Project types (inlined so the file is self-contained) ─────────────── */
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

/* ── Function under test (declaration) ─────────────────────────────────── */
int csv_get_field(char *dest, size_t dest_len,
                  CSV_BUFFER *src, size_t row, size_t entry);

/* ── File-scope fixtures ────────────────────────────────────────────────── */
static CSV_BUFFER *g_buf;

/* ── Helper functions ───────────────────────────────────────────────────── */

/* Allocate a single CSV_FIELD with the given text string. */
static CSV_FIELD *make_field(const char *text)
{
    CSV_FIELD *f = (CSV_FIELD *)malloc(sizeof(CSV_FIELD));
    TEST_ASSERT_NOT_NULL(f);
    f->text   = strdup(text);
    TEST_ASSERT_NOT_NULL(f->text);
    f->length = strlen(text);
    return f;
}

/* Free a single CSV_FIELD. */
static void free_field(CSV_FIELD *f)
{
    if (f) {
        free(f->text);
        free(f);
    }
}

/*
 * Build a CSV_BUFFER with `nrows` rows.
 * texts[r] is a NULL-terminated array of C-strings for row r.
 *
 * Example:
 *   const char *row0[] = {"hello", "world", NULL};
 *   const char *rows[] = {row0, NULL};
 *   build_buffer(rows, 1);
 */
static CSV_BUFFER *build_buffer(const char ***texts, size_t nrows)
{
    CSV_BUFFER *buf = (CSV_BUFFER *)malloc(sizeof(CSV_BUFFER));
    TEST_ASSERT_NOT_NULL(buf);

    buf->rows        = nrows;
    buf->field_delim = ',';
    buf->text_delim  = '"';
    buf->width       = (size_t *)malloc(nrows * sizeof(size_t));
    TEST_ASSERT_NOT_NULL(buf->width);
    buf->field       = (CSV_FIELD ***)malloc(nrows * sizeof(CSV_FIELD **));
    TEST_ASSERT_NOT_NULL(buf->field);

    for (size_t r = 0; r < nrows; r++) {
        /* Count columns for this row */
        size_t ncols = 0;
        while (texts[r][ncols] != NULL)
            ncols++;
        buf->width[r] = ncols;

        buf->field[r] = (CSV_FIELD **)malloc(ncols * sizeof(CSV_FIELD *));
        TEST_ASSERT_NOT_NULL(buf->field[r]);

        for (size_t c = 0; c < ncols; c++)
            buf->field[r][c] = make_field(texts[r][c]);
    }
    return buf;
}

/* Free a CSV_BUFFER created by build_buffer(). */
static void free_buffer(CSV_BUFFER *buf)
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

/* ── setUp / tearDown ───────────────────────────────────────────────────── */

void setUp(void)
{
    /*
     * Build a small 2-row buffer:
     *   row 0: "hello", "world", ""
     *   row 1: "foo"
     */
    static const char *r0[] = { "hello", "world", "", NULL };
    static const char *r1[] = { "foo",             NULL };
    static const char **rows[] = { r0, r1 };

    g_buf = build_buffer(rows, 2);
}

void tearDown(void)
{
    free_buffer(g_buf);
    g_buf = NULL;
}

/* ── Test cases ─────────────────────────────────────────────────────────── */

/*
 * Test 1 – dest_len == 0 must return 3 immediately without touching dest.
 */
void test_csv_get_field_returns_3_when_dest_len_is_zero(void)
{
    char dest[16] = "unchanged";
    int  rc = csv_get_field(dest, 0, g_buf, 0, 0);

    TEST_ASSERT_EQUAL_INT_MESSAGE(3, rc,
        "csv_get_field should return 3 when dest_len is 0");
    /* dest must not have been modified */
    TEST_ASSERT_EQUAL_STRING_MESSAGE("unchanged", dest,
        "dest buffer must not be modified when dest_len is 0");
}

/*
 * Test 2 – Valid row/entry with a non-empty field that fits in dest.
 *           Expects return 0 and the correct string in dest.
 */
void test_csv_get_field_returns_0_and_copies_text_for_valid_entry(void)
{
    char dest[32] = {0};
    int  rc = csv_get_field(dest, sizeof(dest) - 1, g_buf, 0, 0);

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, rc,
        "csv_get_field should return 0 for a normal, fitting field");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("hello", dest,
        "dest should contain the field text 'hello'");
}

/*
 * Test 3 – Row index out of range: row >= src->rows.
 *           Expects return 2 and dest cleared to '\0'.
 */
void test_csv_get_field_returns_2_when_row_out_of_range(void)
{
    char dest[16];
    memset(dest, 0xAB, sizeof(dest));   /* fill with non-zero sentinel */

    int rc = csv_get_field(dest, sizeof(dest) - 1, g_buf, 99, 0);

    TEST_ASSERT_EQUAL_INT_MESSAGE(2, rc,
        "csv_get_field should return 2 when row index is out of range");
    /* The function clears dest[0] in a loop; at minimum dest[0] must be NUL */
    TEST_ASSERT_EQUAL_HEX_MESSAGE(0x00, (unsigned char)dest[0],
        "dest[0] must be NUL when row is out of range");
}

/*
 * Test 4 – Entry index out of range: entry >= src->width[row].
 *           Expects return 2 and dest cleared.
 */
void test_csv_get_field_returns_2_when_entry_out_of_range(void)
{
    char dest[16];
    memset(dest, 0xCD, sizeof(dest));

    int rc = csv_get_field(dest, sizeof(dest) - 1, g_buf, 0, 99);

    TEST_ASSERT_EQUAL_INT_MESSAGE(2, rc,
        "csv_get_field should return 2 when entry index is out of range");
    TEST_ASSERT_EQUAL_HEX_MESSAGE(0x00, (unsigned char)dest[0],
        "dest[0] must be NUL when entry is out of range");
}

/*
 * Test 5 – dest_len smaller than the field length causes truncation.
 *           Expects return 1 (truncated) and a NUL-terminated prefix in dest.
 *
 *   Field "hello" has length 5.  We provide dest_len = 3, so only "hel"
 *   should be copied and the function should return 1.
 */
void test_csv_get_field_returns_1_and_truncates_when_dest_too_small(void)
{
    /* dest must be at least dest_len + 1 bytes to hold the NUL written by
     * dest[dest_len] = '\0' inside the function. */
    char dest[8] = {0};
    size_t dest_len = 3;   /* "hello" (5 chars) won't fit */

    int rc = csv_get_field(dest, dest_len, g_buf, 0, 0);

    TEST_ASSERT_EQUAL_INT_MESSAGE(1, rc,
        "csv_get_field should return 1 when field is truncated");
    /* Only the first dest_len characters should be present */
    TEST_ASSERT_EQUAL_STRING_LEN_MESSAGE("hel", dest, dest_len,
        "dest should contain the first 3 characters of 'hello'");
    /* Ensure NUL termination at position dest_len */
    TEST_ASSERT_EQUAL_HEX_MESSAGE(0x00, (unsigned char)dest[dest_len],
        "dest[dest_len] must be NUL after truncation");
}

/* ── main ───────────────────────────────────────────────────────────────── */

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_csv_get_field_returns_3_when_dest_len_is_zero);
    RUN_TEST(test_csv_get_field_returns_0_and_copies_text_for_valid_entry);
    RUN_TEST(test_csv_get_field_returns_2_when_row_out_of_range);
    RUN_TEST(test_csv_get_field_returns_2_when_entry_out_of_range);
    RUN_TEST(test_csv_get_field_returns_1_and_truncates_when_dest_too_small);

    return UNITY_END();
}