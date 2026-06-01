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

    /* Populate buffer with a 2-row CSV:
     * Row 0: "hello", "world", ""
     * Row 1: "foo"
     */
    /* csv_set_field will create rows/entries as needed */
    csv_set_field(buf, 0, 0, "hello");
    csv_set_field(buf, 0, 1, "world");
    csv_set_field(buf, 0, 2, "");
    csv_set_field(buf, 1, 0, "foo");
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
void test_csv_get_field_zero_dest_len_returns_3(void)
{
    char dest[16] = "unchanged";
    int ret = csv_get_field(dest, 0, buf, 0, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(3, ret,
        "Expected return 3 when dest_len is 0");
    /* dest should be untouched because the function returns early */
    TEST_ASSERT_EQUAL_STRING_MESSAGE("unchanged", dest,
        "dest should not be modified when dest_len is 0");
}

/* ------------------------------------------------------------------ */
/* Test 2: row out of range — should clear dest and return 2           */
/* ------------------------------------------------------------------ */
void test_csv_get_field_row_out_of_range_returns_2(void)
{
    char dest[16];
    memset(dest, 0xAB, sizeof(dest));   /* fill with non-zero sentinel */

    int ret = csv_get_field(dest, sizeof(dest) - 1, buf, 99, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, ret,
        "Expected return 2 when row does not exist");
    /* The function sets dest[0] = '\0' in a loop */
    TEST_ASSERT_EQUAL_HEX8_MESSAGE(0x00, (unsigned char)dest[0],
        "dest[0] should be NUL when row is out of range");
}

/* ------------------------------------------------------------------ */
/* Test 3: entry out of range — should clear dest and return 2         */
/* ------------------------------------------------------------------ */
void test_csv_get_field_entry_out_of_range_returns_2(void)
{
    char dest[16];
    memset(dest, 0xCD, sizeof(dest));

    /* Row 1 has only 1 entry (index 0); request entry 5 */
    int ret = csv_get_field(dest, sizeof(dest) - 1, buf, 1, 5);
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, ret,
        "Expected return 2 when entry does not exist");
    TEST_ASSERT_EQUAL_HEX8_MESSAGE(0x00, (unsigned char)dest[0],
        "dest[0] should be NUL when entry is out of range");
}

/* ------------------------------------------------------------------ */
/* Test 4: normal copy — whole entry fits, return 0                    */
/* ------------------------------------------------------------------ */
void test_csv_get_field_normal_copy_returns_0(void)
{
    char dest[32];
    memset(dest, 0, sizeof(dest));

    /* Row 0, entry 0 = "hello" */
    int ret = csv_get_field(dest, sizeof(dest) - 1, buf, 0, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret,
        "Expected return 0 when whole entry fits in dest");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("hello", dest,
        "dest should contain 'hello'");
}

/* ------------------------------------------------------------------ */
/* Test 5: dest too small — entry truncated, return 1                  */
/* ------------------------------------------------------------------ */
void test_csv_get_field_truncated_returns_1(void)
{
    /* "world" is 5 chars; give only 3 chars of usable space */
    char dest[4];   /* dest_len = 3, dest[3] = '\0' sentinel */
    memset(dest, 0, sizeof(dest));

    /* Row 0, entry 1 = "world" (length 5) */
    int ret = csv_get_field(dest, 3, buf, 0, 1);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, ret,
        "Expected return 1 when entry is truncated");
    /* strncpy copies at most dest_len bytes, then we set dest[dest_len]='\0' */
    TEST_ASSERT_EQUAL_STRING_LEN_MESSAGE("wor", dest, 3,
        "dest should contain the first 3 chars of 'world'");
    TEST_ASSERT_EQUAL_HEX8_MESSAGE(0x00, (unsigned char)dest[3],
        "dest[dest_len] must be NUL terminator");
}

/* ------------------------------------------------------------------ */
/* main                                                                 */
/* ------------------------------------------------------------------ */
int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_csv_get_field_zero_dest_len_returns_3);
    RUN_TEST(test_csv_get_field_row_out_of_range_returns_2);
    RUN_TEST(test_csv_get_field_entry_out_of_range_returns_2);
    RUN_TEST(test_csv_get_field_normal_copy_returns_0);
    RUN_TEST(test_csv_get_field_truncated_returns_1);
    return UNITY_END();
}