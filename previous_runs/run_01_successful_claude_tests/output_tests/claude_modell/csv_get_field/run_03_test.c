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

/* ── Forward declaration of the function under test ────────────────────── */
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
 * Build a CSV_BUFFER with a single row containing `ncols` fields whose
 * text values are supplied in the `texts` array.
 */
static CSV_BUFFER *make_buffer_one_row(const char **texts, size_t ncols)
{
    CSV_BUFFER *buf = (CSV_BUFFER *)malloc(sizeof(CSV_BUFFER));
    TEST_ASSERT_NOT_NULL_MESSAGE(buf, "malloc failed for CSV_BUFFER");

    buf->rows        = 1;
    buf->field_delim = ',';
    buf->text_delim  = '"';

    buf->width    = (size_t *)malloc(sizeof(size_t));
    TEST_ASSERT_NOT_NULL_MESSAGE(buf->width, "malloc failed for width");
    buf->width[0] = ncols;

    /* field[0] = array of ncols CSV_FIELD* */
    buf->field = (CSV_FIELD ***)malloc(sizeof(CSV_FIELD **));
    TEST_ASSERT_NOT_NULL_MESSAGE(buf->field, "malloc failed for field rows");

    buf->field[0] = (CSV_FIELD **)malloc(ncols * sizeof(CSV_FIELD *));
    TEST_ASSERT_NOT_NULL_MESSAGE(buf->field[0], "malloc failed for field cols");

    for (size_t c = 0; c < ncols; c++)
        buf->field[0][c] = make_field(texts[c]);

    return buf;
}

/*
 * Free every resource owned by a CSV_BUFFER created with make_buffer_one_row.
 */
static void free_buffer(CSV_BUFFER *buf)
{
    if (!buf) return;
    for (size_t r = 0; r < buf->rows; r++) {
        for (size_t c = 0; c < buf->width[r]; c++) {
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
    /* Default fixture: one row, three normal fields */
    const char *texts[] = { "hello", "world", "foo" };
    g_buf = make_buffer_one_row(texts, 3);
}

void tearDown(void)
{
    free_buffer(g_buf);
    g_buf = NULL;
}

/* ── Test cases ─────────────────────────────────────────────────────────── */

/*
 * TC-1  dest_len == 0  →  must return 3 immediately without touching dest.
 */
void test_csv_get_field_returns_3_when_dest_len_is_zero(void)
{
    char dest[16] = "unchanged";
    int  rc = csv_get_field(dest, 0, g_buf, 0, 0);

    TEST_ASSERT_EQUAL_INT_MESSAGE(3, rc,
        "Expected return code 3 when dest_len is 0");
    /* dest must not have been modified */
    TEST_ASSERT_EQUAL_STRING_MESSAGE("unchanged", dest,
        "dest buffer must not be modified when dest_len is 0");
}

/*
 * TC-2  row index out of range  →  must return 2 and clear dest.
 */
void test_csv_get_field_returns_2_for_invalid_row(void)
{
    char dest[16];
    memset(dest, 0xAB, sizeof(dest));   /* fill with non-zero sentinel */

    int rc = csv_get_field(dest, sizeof(dest) - 1, g_buf,
                           99 /*row does not exist*/, 0);

    TEST_ASSERT_EQUAL_INT_MESSAGE(2, rc,
        "Expected return code 2 for out-of-range row");
    /* The function clears dest[0] in a loop; at minimum dest[0] must be NUL */
    TEST_ASSERT_EQUAL_HEX_MESSAGE(0x00, (unsigned char)dest[0],
        "dest[0] must be NUL after invalid-row call");
}

/*
 * TC-3  entry index out of range  →  must return 2 and clear dest.
 */
void test_csv_get_field_returns_2_for_invalid_entry(void)
{
    char dest[16];
    memset(dest, 0xCD, sizeof(dest));

    int rc = csv_get_field(dest, sizeof(dest) - 1, g_buf,
                           0, 99 /*entry does not exist*/);

    TEST_ASSERT_EQUAL_INT_MESSAGE(2, rc,
        "Expected return code 2 for out-of-range entry");
    TEST_ASSERT_EQUAL_HEX_MESSAGE(0x00, (unsigned char)dest[0],
        "dest[0] must be NUL after invalid-entry call");
}

/*
 * TC-4  Normal retrieval of an existing field  →  must return 0 and copy text.
 */
void test_csv_get_field_returns_0_and_copies_text_for_valid_field(void)
{
    char dest[32];
    memset(dest, 0, sizeof(dest));

    int rc = csv_get_field(dest, sizeof(dest) - 1, g_buf, 0, 1 /*"world"*/);

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, rc,
        "Expected return code 0 for a normal, fully-fitting field");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("world", dest,
        "dest must contain the field text 'world'");
}

/*
 * TC-5  dest_len smaller than the field text  →  must return 1 (truncated)
 *       and the copied portion must match the beginning of the original text.
 */
void test_csv_get_field_returns_1_when_dest_too_small(void)
{
    /* "hello" has length 5; give dest only 3 usable bytes */
    char dest[8];
    memset(dest, 0xFF, sizeof(dest));

    /* dest_len = 3 means strncpy copies 3 chars; dest[3] is set to NUL */
    int rc = csv_get_field(dest, 3, g_buf, 0, 0 /*"hello"*/);

    TEST_ASSERT_EQUAL_INT_MESSAGE(1, rc,
        "Expected return code 1 when dest_len is smaller than field length");
    /* Only the first 3 characters should have been copied */
    TEST_ASSERT_EQUAL_STRING_LEN_MESSAGE("hel", dest, 3,
        "dest must contain the first 3 characters of 'hello'");
    TEST_ASSERT_EQUAL_HEX_MESSAGE(0x00, (unsigned char)dest[3],
        "dest[dest_len] must be NUL-terminated");
}

/* ── main ───────────────────────────────────────────────────────────────── */

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_csv_get_field_returns_3_when_dest_len_is_zero);
    RUN_TEST(test_csv_get_field_returns_2_for_invalid_row);
    RUN_TEST(test_csv_get_field_returns_2_for_invalid_entry);
    RUN_TEST(test_csv_get_field_returns_0_and_copies_text_for_valid_field);
    RUN_TEST(test_csv_get_field_returns_1_when_dest_too_small);

    return UNITY_END();
}