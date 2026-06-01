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
    TEST_FAIL_MESSAGE("Segmentation fault (SIGSEGV) caught during test");
}

void setUp(void)
{
    signal(SIGSEGV, sigsegv_handler);
    buf = csv_create_buffer();
    TEST_ASSERT_NOT_NULL_MESSAGE(buf, "csv_create_buffer returned NULL in setUp");
}

void tearDown(void)
{
    if (buf != NULL) {
        csv_destroy_buffer(buf);
        buf = NULL;
    }
    signal(SIGSEGV, SIG_DFL);
}

/* Helper: populate a single row with one field containing the given text */
static void populate_single_field(CSV_BUFFER *b, const char *text)
{
    int ret = csv_set_field(b, 0, 0, (char *)text);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "csv_set_field failed in helper");
}

/* ------------------------------------------------------------------ */
/* Test 1: dest_len == 0 should return 3 immediately                  */
/* ------------------------------------------------------------------ */
void test_csv_get_field_zero_dest_len_returns_3(void)
{
    char dest[64];
    populate_single_field(buf, "hello");

    int ret = csv_get_field(dest, 0, buf, 0, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(3, ret,
        "Expected return 3 when dest_len is 0");
}

/* ------------------------------------------------------------------ */
/* Test 2: row out of range clears dest and returns 2                 */
/* ------------------------------------------------------------------ */
void test_csv_get_field_invalid_row_returns_2_and_clears_dest(void)
{
    char dest[64];
    memset(dest, 'X', sizeof(dest));

    /* buf has 0 rows initially; row 5 does not exist */
    int ret = csv_get_field(dest, sizeof(dest) - 1, buf, 5, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, ret,
        "Expected return 2 for out-of-range row");
    /* The function sets dest[0] = '\0' in a loop over dest_len */
    TEST_ASSERT_EQUAL_INT_MESSAGE('\0', dest[0],
        "Expected dest[0] to be NUL after invalid row");
}

/* ------------------------------------------------------------------ */
/* Test 3: entry out of range clears dest and returns 2               */
/* ------------------------------------------------------------------ */
void test_csv_get_field_invalid_entry_returns_2_and_clears_dest(void)
{
    char dest[64];
    memset(dest, 'A', sizeof(dest));

    /* Populate row 0, entry 0 only; entry 5 does not exist */
    populate_single_field(buf, "data");

    int ret = csv_get_field(dest, sizeof(dest) - 1, buf, 0, 5);
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, ret,
        "Expected return 2 for out-of-range entry");
    TEST_ASSERT_EQUAL_INT_MESSAGE('\0', dest[0],
        "Expected dest[0] to be NUL after invalid entry");
}

/* ------------------------------------------------------------------ */
/* Test 4: normal copy — whole entry fits, returns 0                  */
/* ------------------------------------------------------------------ */
void test_csv_get_field_normal_copy_returns_0(void)
{
    char dest[64];
    const char *expected = "hello world";

    populate_single_field(buf, expected);

    int ret = csv_get_field(dest, sizeof(dest) - 1, buf, 0, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret,
        "Expected return 0 when whole entry fits in dest");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(expected, dest,
        "Copied string does not match original field text");
}

/* ------------------------------------------------------------------ */
/* Test 5: dest too small — entry is truncated, returns 1             */
/* ------------------------------------------------------------------ */
void test_csv_get_field_truncated_copy_returns_1(void)
{
    /* Use a dest buffer smaller than the field text */
    char dest[5]; /* can hold 4 chars + NUL */
    const char *long_text = "abcdefghij"; /* 10 chars */

    populate_single_field(buf, long_text);

    /* dest_len = 4 so strncpy copies 4 chars, then dest[4] = '\0' */
    int ret = csv_get_field(dest, 4, buf, 0, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, ret,
        "Expected return 1 when entry is truncated");
    /* First 4 characters should match */
    TEST_ASSERT_EQUAL_STRING_LEN_MESSAGE("abcd", dest, 4,
        "Truncated content does not match expected prefix");
    /* Ensure NUL termination at position 4 */
    TEST_ASSERT_EQUAL_INT_MESSAGE('\0', dest[4],
        "Expected NUL terminator at dest[dest_len]");
}

/* ------------------------------------------------------------------ */
/* main                                                                */
/* ------------------------------------------------------------------ */
int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_csv_get_field_zero_dest_len_returns_3);
    RUN_TEST(test_csv_get_field_invalid_row_returns_2_and_clears_dest);
    RUN_TEST(test_csv_get_field_invalid_entry_returns_2_and_clears_dest);
    RUN_TEST(test_csv_get_field_normal_copy_returns_0);
    RUN_TEST(test_csv_get_field_truncated_copy_returns_1);
    return UNITY_END();
}