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

/* ------------------------------------------------------------------ */
/* Test 1: dest_len == 0 should return 3 immediately                   */
/* ------------------------------------------------------------------ */
void test_csv_get_field_dest_len_zero_returns_3(void)
{
    char dest[64] = "unchanged";

    /* Populate a field so the buffer is valid */
    int set_ret = csv_set_field(buf, 0, 0, "hello");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, set_ret, "csv_set_field failed in setup");

    int ret = csv_get_field(dest, 0, buf, 0, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(3, ret,
        "Expected return 3 when dest_len is 0");
}

/* ------------------------------------------------------------------ */
/* Test 2: row out of range — should clear dest and return 2           */
/* ------------------------------------------------------------------ */
void test_csv_get_field_row_out_of_range_returns_2(void)
{
    char dest[64];
    memset(dest, 'X', sizeof(dest));

    /* Buffer has 1 row (row 0) after set_field */
    int set_ret = csv_set_field(buf, 0, 0, "data");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, set_ret, "csv_set_field failed in setup");

    /* Request row 99 which does not exist */
    int ret = csv_get_field(dest, sizeof(dest) - 1, buf, 99, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, ret,
        "Expected return 2 when row is out of range");
    /* dest[0] should have been cleared to '\0' */
    TEST_ASSERT_EQUAL_INT_MESSAGE('\0', dest[0],
        "Expected dest[0] to be NUL when row is out of range");
}

/* ------------------------------------------------------------------ */
/* Test 3: entry out of range — should clear dest and return 2         */
/* ------------------------------------------------------------------ */
void test_csv_get_field_entry_out_of_range_returns_2(void)
{
    char dest[64];
    memset(dest, 'A', sizeof(dest));

    /* Row 0 has only entry 0 */
    int set_ret = csv_set_field(buf, 0, 0, "value");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, set_ret, "csv_set_field failed in setup");

    /* Request entry 99 which does not exist in row 0 */
    int ret = csv_get_field(dest, sizeof(dest) - 1, buf, 0, 99);
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, ret,
        "Expected return 2 when entry is out of range");
    TEST_ASSERT_EQUAL_INT_MESSAGE('\0', dest[0],
        "Expected dest[0] to be NUL when entry is out of range");
}

/* ------------------------------------------------------------------ */
/* Test 4: normal copy — whole entry fits, return 0                    */
/* ------------------------------------------------------------------ */
void test_csv_get_field_normal_copy_returns_0(void)
{
    char dest[64];
    memset(dest, '\0', sizeof(dest));

    const char *expected = "hello world";
    int set_ret = csv_set_field(buf, 0, 0, (char *)expected);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, set_ret, "csv_set_field failed in setup");

    /* dest_len must be large enough; note the function writes dest[dest_len]='\0'
     * so we pass sizeof(dest)-1 to keep the write in bounds */
    int ret = csv_get_field(dest, sizeof(dest) - 1, buf, 0, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret,
        "Expected return 0 when whole entry fits in dest");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(expected, dest,
        "Copied string does not match expected value");
}

/* ------------------------------------------------------------------ */
/* Test 5: truncation — dest_len smaller than field, return 1          */
/* ------------------------------------------------------------------ */
void test_csv_get_field_truncation_returns_1(void)
{
    /* Use a field longer than the destination buffer */
    const char *long_str = "abcdefghijklmnopqrstuvwxyz"; /* 26 chars */
    int set_ret = csv_set_field(buf, 0, 0, (char *)long_str);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, set_ret, "csv_set_field failed in setup");

    /* dest_len = 5, so only 5 chars fit before the forced NUL at dest[5] */
    char dest[8];
    memset(dest, '\0', sizeof(dest));
    /* Pass dest_len = 5; the function writes dest[5] = '\0', which is within
     * our 8-byte buffer */
    int ret = csv_get_field(dest, 5, buf, 0, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, ret,
        "Expected return 1 when entry is truncated");
    /* First 5 chars should match the beginning of long_str */
    TEST_ASSERT_EQUAL_STRING_LEN_MESSAGE("abcde", dest, 5,
        "Truncated string prefix does not match");
    /* The character at position 5 must be NUL (forced terminator) */
    TEST_ASSERT_EQUAL_INT_MESSAGE('\0', dest[5],
        "Expected NUL terminator at dest[dest_len]");
}

/* ------------------------------------------------------------------ */
/* main                                                                 */
/* ------------------------------------------------------------------ */
int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_csv_get_field_dest_len_zero_returns_3);
    RUN_TEST(test_csv_get_field_row_out_of_range_returns_2);
    RUN_TEST(test_csv_get_field_entry_out_of_range_returns_2);
    RUN_TEST(test_csv_get_field_normal_copy_returns_0);
    RUN_TEST(test_csv_get_field_truncation_returns_1);
    return UNITY_END();
}