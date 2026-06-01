#include "cJSON_Utils.c"
#include "unity.h"
#include "cJSON.h"
#include <stdlib.h>
#include <string.h>
#include <signal.h>

/* Signal handling for SIGSEGV */
static volatile int segfault_occurred = 0;

static void sigsegv_handler(int sig)
{
    (void)sig;
    segfault_occurred = 1;
    /* We cannot safely longjmp here in all cases, so just mark it */
}

void setUp(void)
{
    segfault_occurred = 0;
    signal(SIGSEGV, sigsegv_handler);
}

void tearDown(void)
{
    signal(SIGSEGV, SIG_DFL);
}

/* Test 1: NULL inputs return false */
void test_compare_pointers_null_name_returns_false(void)
{
    cJSON_bool result = compare_pointers(NULL, (const unsigned char *)"foo", cJSON_True);
    TEST_ASSERT_FALSE_MESSAGE(result, "Expected false when name is NULL");
}

void test_compare_pointers_null_pointer_returns_false(void)
{
    cJSON_bool result = compare_pointers((const unsigned char *)"foo", NULL, cJSON_True);
    TEST_ASSERT_FALSE_MESSAGE(result, "Expected false when pointer is NULL");
}

/* Test 2: Exact match, case sensitive */
void test_compare_pointers_exact_match_case_sensitive(void)
{
    const unsigned char *name    = (const unsigned char *)"hello";
    const unsigned char *pointer = (const unsigned char *)"hello";
    cJSON_bool result = compare_pointers(name, pointer, cJSON_True);
    TEST_ASSERT_TRUE_MESSAGE(result, "Expected true for exact match (case sensitive)");
}

/* Test 3: Case insensitive match */
void test_compare_pointers_case_insensitive_match(void)
{
    const unsigned char *name    = (const unsigned char *)"Hello";
    const unsigned char *pointer = (const unsigned char *)"hello";
    cJSON_bool result = compare_pointers(name, pointer, cJSON_False);
    TEST_ASSERT_TRUE_MESSAGE(result, "Expected true for case-insensitive match");
}

/* Test 4: Case sensitive mismatch due to different case */
void test_compare_pointers_case_sensitive_mismatch(void)
{
    const unsigned char *name    = (const unsigned char *)"Hello";
    const unsigned char *pointer = (const unsigned char *)"hello";
    cJSON_bool result = compare_pointers(name, pointer, cJSON_True);
    TEST_ASSERT_FALSE_MESSAGE(result, "Expected false for case-sensitive mismatch");
}

/* Test 5: Escaped tilde (~0 represents '~') */
void test_compare_pointers_escaped_tilde(void)
{
    /* pointer contains "~0" which should match name "~" */
    const unsigned char *name    = (const unsigned char *)"~";
    const unsigned char *pointer = (const unsigned char *)"~0";
    cJSON_bool result = compare_pointers(name, pointer, cJSON_True);
    TEST_ASSERT_TRUE_MESSAGE(result, "Expected true: ~0 in pointer matches ~ in name");
}

/* Test 6: Escaped slash (~1 represents '/') */
void test_compare_pointers_escaped_slash(void)
{
    /* pointer contains "~1" which should match name "/" */
    const unsigned char *name    = (const unsigned char *)"/";
    const unsigned char *pointer = (const unsigned char *)"~1";
    cJSON_bool result = compare_pointers(name, pointer, cJSON_True);
    TEST_ASSERT_TRUE_MESSAGE(result, "Expected true: ~1 in pointer matches / in name");
}

/* Test 7: Pointer stops at '/' delimiter — partial match */
void test_compare_pointers_stops_at_slash_delimiter(void)
{
    /* pointer = "foo/bar", name = "foo" — should match up to '/' */
    const unsigned char *name    = (const unsigned char *)"foo";
    const unsigned char *pointer = (const unsigned char *)"foo/bar";
    cJSON_bool result = compare_pointers(name, pointer, cJSON_True);
    TEST_ASSERT_TRUE_MESSAGE(result, "Expected true: pointer stops at '/' and name matches segment");
}

/* Test 8: Pointer segment shorter than name — mismatch */
void test_compare_pointers_pointer_shorter_than_name(void)
{
    /* pointer = "fo/bar", name = "foo" — 'o' vs '/' mismatch at end */
    const unsigned char *name    = (const unsigned char *)"foo";
    const unsigned char *pointer = (const unsigned char *)"fo/bar";
    cJSON_bool result = compare_pointers(name, pointer, cJSON_True);
    TEST_ASSERT_FALSE_MESSAGE(result, "Expected false: pointer segment shorter than name");
}

/* Test 9: Both empty strings — should match */
void test_compare_pointers_both_empty(void)
{
    const unsigned char *name    = (const unsigned char *)"";
    const unsigned char *pointer = (const unsigned char *)"";
    cJSON_bool result = compare_pointers(name, pointer, cJSON_True);
    TEST_ASSERT_TRUE_MESSAGE(result, "Expected true: both empty strings match");
}

/* Test 10: Invalid escape sequence returns false */
void test_compare_pointers_invalid_escape_sequence(void)
{
    /* pointer contains "~2" which is not a valid escape */
    const unsigned char *name    = (const unsigned char *)"~";
    const unsigned char *pointer = (const unsigned char *)"~2";
    cJSON_bool result = compare_pointers(name, pointer, cJSON_True);
    TEST_ASSERT_FALSE_MESSAGE(result, "Expected false: ~2 is an invalid escape sequence");
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_compare_pointers_null_name_returns_false);
    RUN_TEST(test_compare_pointers_null_pointer_returns_false);
    RUN_TEST(test_compare_pointers_exact_match_case_sensitive);
    RUN_TEST(test_compare_pointers_case_insensitive_match);
    RUN_TEST(test_compare_pointers_case_sensitive_mismatch);
    RUN_TEST(test_compare_pointers_escaped_tilde);
    RUN_TEST(test_compare_pointers_escaped_slash);
    RUN_TEST(test_compare_pointers_stops_at_slash_delimiter);
    RUN_TEST(test_compare_pointers_pointer_shorter_than_name);
    RUN_TEST(test_compare_pointers_both_empty);
    RUN_TEST(test_compare_pointers_invalid_escape_sequence);
    return UNITY_END();
}