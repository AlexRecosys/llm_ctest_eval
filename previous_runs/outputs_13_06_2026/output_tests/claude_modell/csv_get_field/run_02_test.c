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

/* setUp and tearDown */
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

/* Helper: populate the buffer with a single row and one field */
static void populate_single_field(const char *text)
{
    int ret = csv_set_field(buf, 0, 0, (char *)text);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "csv_set_field failed in helper");
}

/* ------------------------------------------------------------------ */
/* Test 1: dest_len == 0 should return 3 immediately                   */
/* ------------------------------------------------------------------ */
void test_csv_get_field_zero_dest_len_returns_3(void)
{
    char dest[16] = "unchanged";
    populate_single_field("hello");

    int ret = csv_get_field(dest, 0, buf, 0, 0);

    TEST_ASSERT_EQUAL_INT_MESSAGE(3, ret,
        "Expected return 3 when dest_len is 0");
    /* dest should be untouched because we returned early */
    TEST_ASSERT_EQUAL_STRING_MESSAGE("unchanged", dest,
        "dest should not be modified when dest_len is 0");
}

/* ------------------------------------------------------------------ */
/* Test 2: row out of range clears dest and returns 2                  */
/* ------------------------------------------------------------------ */
void test_csv_get_field_invalid_row_returns_2(void)
{
    char dest[16];
    memset(dest, 'X', sizeof(dest));
    populate_single_field("hello");

    /* row 99 does not exist */
    int ret = csv_get_field(dest, sizeof(dest) - 1, buf, 99, 0);

    TEST_ASSERT_EQUAL_INT_MESSAGE(2, ret,
        "Expected return 2 for out-of-range row");
    /* The function sets dest[0] = '\0' in a loop */
    TEST_ASSERT_EQUAL_INT_MESSAGE('\0', (int)dest[0],
        "dest[0] should be NUL when row is invalid");
}

/* ------------------------------------------------------------------ */
/* Test 3: entry out of range clears dest and returns 2                */
/* ------------------------------------------------------------------ */
void test_csv_get_field_invalid_entry_returns_2(void)
{
    char dest[16];
    memset(dest, 'X', sizeof(dest));
    populate_single_field("hello");

    /* entry 99 does not exist in row 0 */
    int ret = csv_get_field(dest, sizeof(dest) - 1, buf, 0, 99);

    TEST_ASSERT_EQUAL_INT_MESSAGE(2, ret,
        "Expected return 2 for out-of-range entry");
    TEST_ASSERT_EQUAL_INT_MESSAGE('\0', (int)dest[0],
        "dest[0] should be NUL when entry is invalid");
}

/* ------------------------------------------------------------------ */
/* Test 4: normal copy — whole entry fits, returns 0                   */
/* ------------------------------------------------------------------ */
void test_csv_get_field_normal_copy_returns_0(void)
{
    char dest[64];
    memset(dest, 0, sizeof(dest));
    populate_single_field("hello world");

    /* dest_len must be at least length+1 so nothing is truncated.
     * The function writes dest[dest_len] = '\0', so we pass
     * sizeof(dest)-1 to keep the write in bounds.
     */
    int ret = csv_get_field(dest, sizeof(dest) - 1, buf, 0, 0);

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret,
        "Expected return 0 when whole entry fits in dest");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("hello world", dest,
        "dest should contain the full field text");
}

/* ------------------------------------------------------------------ */
/* Test 5: dest too small — entry is truncated, returns 1              */
/* ------------------------------------------------------------------ */
void test_csv_get_field_truncated_returns_1(void)
{
    /* Provide a dest that is smaller than the field text */
    char dest[4]; /* can hold 3 chars + NUL */
    memset(dest, 0, sizeof(dest));
    populate_single_field("hello world");

    /* dest_len = 3: strncpy copies 3 chars, then dest[3] = '\0' */
    int ret = csv_get_field(dest, 3, buf, 0, 0);

    TEST_ASSERT_EQUAL_INT_MESSAGE(1, ret,
        "Expected return 1 when entry is truncated");
    /* First 3 characters of "hello world" are "hel" */
    TEST_ASSERT_EQUAL_STRING_LEN_MESSAGE("hel", dest, 3,
        "dest should contain the first 3 characters of the field");
    TEST_ASSERT_EQUAL_INT_MESSAGE('\0', (int)dest[3],
        "dest[dest_len] should be NUL-terminated");
}

/* ------------------------------------------------------------------ */
/* main                                                                 */
/* ------------------------------------------------------------------ */
int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_csv_get_field_zero_dest_len_returns_3);
    RUN_TEST(test_csv_get_field_invalid_row_returns_2);
    RUN_TEST(test_csv_get_field_invalid_entry_returns_2);
    RUN_TEST(test_csv_get_field_normal_copy_returns_0);
    RUN_TEST(test_csv_get_field_truncated_returns_1);
    return UNITY_END();
}