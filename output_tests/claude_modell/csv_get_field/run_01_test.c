#include "csv.c"
#include "unity.h"
#include "csv.h"
#include <stdlib.h>
#include <string.h>
#include <signal.h>

/* File-scope fixtures */
static CSV_BUFFER *buf = NULL;

/* Signal handler for SIGSEGV */
static void sigsegv_handler(int sig)
{
    (void)sig;
    TEST_FAIL_MESSAGE("Segmentation fault (SIGSEGV) caught during test execution");
}

/* setUp and tearDown — non-static, visible to Unity */
void setUp(void)
{
    signal(SIGSEGV, sigsegv_handler);
    buf = csv_create_buffer();
    TEST_ASSERT_NOT_NULL_MESSAGE(buf, "csv_create_buffer() returned NULL in setUp");
}

void tearDown(void)
{
    if (buf != NULL) {
        csv_destroy_buffer(buf);
        buf = NULL;
    }
    signal(SIGSEGV, SIG_DFL);
}

/* Helper: populate a single cell in the buffer at row/entry with given text.
 * Uses only externally-linked csv_set_field which internally calls append_row
 * and append_field as needed.
 */
static void populate_cell(CSV_BUFFER *b, size_t row, size_t entry, char *text)
{
    int rc = csv_set_field(b, row, entry, text);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, rc, "csv_set_field failed in helper populate_cell");
}

/* -------------------------------------------------------------------------
 * Test 1: dest_len == 0 should return 3 immediately
 * ---------------------------------------------------------------------- */
void test_csv_get_field_dest_len_zero_returns_3(void)
{
    char dest[64] = "unchanged";

    /* Populate a valid cell so the only reason for failure is dest_len == 0 */
    populate_cell(buf, 0, 0, "hello");

    int rc = csv_get_field(dest, 0, buf, 0, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(3, rc, "Expected return 3 when dest_len is 0");
    /* dest should be untouched because the function returns immediately */
    TEST_ASSERT_EQUAL_STRING_MESSAGE("unchanged", dest,
        "dest buffer should not be modified when dest_len is 0");
}

/* -------------------------------------------------------------------------
 * Test 2: row out of range — should clear dest and return 2
 * ---------------------------------------------------------------------- */
void test_csv_get_field_row_out_of_range_returns_2(void)
{
    char dest[64];
    memset(dest, 'X', sizeof(dest));

    /* Buffer has 1 row (row 0); request row 5 which does not exist */
    populate_cell(buf, 0, 0, "data");

    int rc = csv_get_field(dest, sizeof(dest) - 1, buf, 5, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, rc,
        "Expected return 2 when requested row does not exist");
    /* The function sets dest[0] = '\0' in a loop — first char must be NUL */
    TEST_ASSERT_EQUAL_INT_MESSAGE('\0', (int)dest[0],
        "dest[0] should be NUL when row is out of range");
}

/* -------------------------------------------------------------------------
 * Test 3: entry out of range — should clear dest and return 2
 * ---------------------------------------------------------------------- */
void test_csv_get_field_entry_out_of_range_returns_2(void)
{
    char dest[64];
    memset(dest, 'A', sizeof(dest));

    /* Row 0 has only entry 0; request entry 99 */
    populate_cell(buf, 0, 0, "value");

    int rc = csv_get_field(dest, sizeof(dest) - 1, buf, 0, 99);
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, rc,
        "Expected return 2 when requested entry does not exist");
    TEST_ASSERT_EQUAL_INT_MESSAGE('\0', (int)dest[0],
        "dest[0] should be NUL when entry is out of range");
}

/* -------------------------------------------------------------------------
 * Test 4: valid cell, dest large enough — should copy text and return 0
 * ---------------------------------------------------------------------- */
void test_csv_get_field_valid_cell_full_copy_returns_0(void)
{
    char dest[128];
    memset(dest, 0, sizeof(dest));

    const char *expected = "Hello, World!";
    populate_cell(buf, 0, 0, (char *)expected);

    /* dest_len must be at least length of string; pass sizeof(dest)-1 */
    int rc = csv_get_field(dest, sizeof(dest) - 1, buf, 0, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, rc,
        "Expected return 0 when dest is large enough to hold the full entry");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(expected, dest,
        "dest should contain the exact field text after a successful copy");
}

/* -------------------------------------------------------------------------
 * Test 5: valid cell but dest too small — text truncated, return 1
 * ---------------------------------------------------------------------- */
void test_csv_get_field_truncation_returns_1(void)
{
    char dest[5]; /* intentionally small */
    memset(dest, 0, sizeof(dest));

    /* Store a string longer than dest can hold */
    const char *long_text = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    populate_cell(buf, 0, 0, (char *)long_text);

    /*
     * dest_len = 4 (we keep one byte for the NUL that the function writes
     * at dest[dest_len]).  The function does:
     *   strncpy(dest, text, dest_len);   -- copies 4 chars
     *   dest[dest_len] = '\0';           -- NUL at index 4
     * Then checks field->length > dest_len + 1 => 26 > 5 => true => return 1
     */
    int rc = csv_get_field(dest, 4, buf, 0, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, rc,
        "Expected return 1 when the field is truncated to fit dest");
    /* First 4 characters should match the beginning of long_text */
    TEST_ASSERT_EQUAL_STRING_LEN_MESSAGE("ABCD", dest, 4,
        "dest should contain the first dest_len characters of the field");
    /* NUL terminator must be present at position 4 */
    TEST_ASSERT_EQUAL_INT_MESSAGE('\0', (int)dest[4],
        "dest[dest_len] must be NUL after truncation");
}

/* -------------------------------------------------------------------------
 * main
 * ---------------------------------------------------------------------- */
int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_csv_get_field_dest_len_zero_returns_3);
    RUN_TEST(test_csv_get_field_row_out_of_range_returns_2);
    RUN_TEST(test_csv_get_field_entry_out_of_range_returns_2);
    RUN_TEST(test_csv_get_field_valid_cell_full_copy_returns_0);
    RUN_TEST(test_csv_get_field_truncation_returns_1);
    return UNITY_END();
}