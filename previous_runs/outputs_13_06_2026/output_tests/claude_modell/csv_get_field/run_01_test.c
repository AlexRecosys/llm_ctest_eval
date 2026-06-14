#include "csv.c"
#include "unity.h"
#include "csv.h"
#include <stdlib.h>
#include <string.h>
#include <signal.h>

/* File-scope fixture */
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

    /* Populate a small 2-row CSV buffer:
     * Row 0: "hello", "world"
     * Row 1: ""  (empty field)
     */
    int rc;
    rc = csv_set_field(buf, 0, 0, "hello");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, rc, "setUp: csv_set_field row0 entry0 failed");
    rc = csv_set_field(buf, 0, 1, "world");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, rc, "setUp: csv_set_field row0 entry1 failed");
    /* Row 1 with an empty string */
    rc = csv_set_field(buf, 1, 0, "");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, rc, "setUp: csv_set_field row1 entry0 failed");
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
/* Test 1: dest_len == 0 should return 3                               */
/* ------------------------------------------------------------------ */
void test_csv_get_field_zero_dest_len_returns_3(void)
{
    char dest[16] = "unchanged";
    int rc = csv_get_field(dest, 0, buf, 0, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(3, rc,
        "Expected return 3 when dest_len is 0");
}

/* ------------------------------------------------------------------ */
/* Test 2: row out of range should return 2 and clear dest             */
/* ------------------------------------------------------------------ */
void test_csv_get_field_invalid_row_returns_2(void)
{
    char dest[16];
    memset(dest, 'X', sizeof(dest));

    int rc = csv_get_field(dest, sizeof(dest) - 1, buf, 999, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, rc,
        "Expected return 2 for out-of-range row");
    /* dest[0] should have been cleared to '\0' */
    TEST_ASSERT_EQUAL_INT_MESSAGE('\0', dest[0],
        "Expected dest[0] == '\\0' after invalid row request");
}

/* ------------------------------------------------------------------ */
/* Test 3: entry out of range should return 2 and clear dest           */
/* ------------------------------------------------------------------ */
void test_csv_get_field_invalid_entry_returns_2(void)
{
    char dest[16];
    memset(dest, 'A', sizeof(dest));

    int rc = csv_get_field(dest, sizeof(dest) - 1, buf, 0, 999);
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, rc,
        "Expected return 2 for out-of-range entry");
    TEST_ASSERT_EQUAL_INT_MESSAGE('\0', dest[0],
        "Expected dest[0] == '\\0' after invalid entry request");
}

/* ------------------------------------------------------------------ */
/* Test 4: successful full copy should return 0 and copy the string    */
/* ------------------------------------------------------------------ */
void test_csv_get_field_success_returns_0_and_copies_string(void)
{
    char dest[32];
    memset(dest, 0, sizeof(dest));

    int rc = csv_get_field(dest, sizeof(dest) - 1, buf, 0, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, rc,
        "Expected return 0 for successful full copy");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("hello", dest,
        "Expected dest to contain 'hello'");
}

/* ------------------------------------------------------------------ */
/* Test 5: dest_len smaller than field length should return 1          */
/*         (truncation case)                                           */
/* ------------------------------------------------------------------ */
void test_csv_get_field_truncation_returns_1(void)
{
    /* "hello" is 5 chars; provide dest_len = 3 so it gets truncated */
    char dest[8];
    memset(dest, 0, sizeof(dest));

    int rc = csv_get_field(dest, 3, buf, 0, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, rc,
        "Expected return 1 when field is truncated");
    /* strncpy copies 3 chars, then dest[3] = '\0' */
    TEST_ASSERT_EQUAL_STRING_LEN_MESSAGE("hel", dest, 3,
        "Expected first 3 chars of 'hello' in dest");
    TEST_ASSERT_EQUAL_INT_MESSAGE('\0', dest[3],
        "Expected null terminator at dest[dest_len]");
}

/* ------------------------------------------------------------------ */
/* main                                                                */
/* ------------------------------------------------------------------ */
int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_csv_get_field_zero_dest_len_returns_3);
    RUN_TEST(test_csv_get_field_invalid_row_returns_2);
    RUN_TEST(test_csv_get_field_invalid_entry_returns_2);
    RUN_TEST(test_csv_get_field_success_returns_0_and_copies_string);
    RUN_TEST(test_csv_get_field_truncation_returns_1);
    return UNITY_END();
}