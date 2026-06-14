#include "cJSON_Utils.c"
#include "unity.h"
#include "cJSON.h"
#include <stdlib.h>
#include <string.h>
#include <signal.h>

/* Signal handling for segfault protection */
static volatile int segfault_occurred = 0;

static void sigsegv_handler(int sig)
{
    (void)sig;
    segfault_occurred = 1;
    /* We can't easily longjmp from here in all cases, so just flag it */
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
    cJSON_bool result = compare_pointers(NULL, (const unsigned char *)"foo", 1);
    TEST_ASSERT_FALSE_MESSAGE(result, "compare_pointers with NULL name should return false");
}

void test_compare_pointers_null_pointer_returns_false(void)
{
    cJSON_bool result = compare_pointers((const unsigned char *)"foo", NULL, 1);
    TEST_ASSERT_FALSE_MESSAGE(result, "compare_pointers with NULL pointer should return false");
}

/* Test 2: Exact match, case sensitive */
void test_compare_pointers_exact_match_case_sensitive(void)
{
    const unsigned char *name    = (const unsigned char *)"hello";
    const unsigned char *pointer = (const unsigned char *)"hello";
    cJSON_bool result = compare_pointers(name, pointer, 1);
    TEST_ASSERT_TRUE_MESSAGE(result, "Exact match should return true (case sensitive)");
}

/* Test 3: Case-insensitive match */
void test_compare_pointers_case_insensitive_match(void)
{
    const unsigned char *name    = (const unsigned char *)"Hello";
    const unsigned char *pointer = (const unsigned char *)"hello";
    cJSON_bool result = compare_pointers(name, pointer, 0);
    TEST_ASSERT_TRUE_MESSAGE(result, "Case-insensitive match should return true");
}

/* Test 4: Escaped tilde (~0 represents '~') */
void test_compare_pointers_escaped_tilde(void)
{
    /* name contains '~', pointer contains '~0' */
    const unsigned char *name    = (const unsigned char *)"a~b";
    const unsigned char *pointer = (const unsigned char *)"a~0b";
    cJSON_bool result = compare_pointers(name, pointer, 1);
    TEST_ASSERT_TRUE_MESSAGE(result, "Escaped tilde (~0) should match '~' in name");
}

/* Test 5: Escaped slash (~1 represents '/') */
void test_compare_pointers_escaped_slash(void)
{
    /* name contains '/', pointer contains '~1' */
    const unsigned char *name    = (const unsigned char *)"a/b";
    const unsigned char *pointer = (const unsigned char *)"a~1b";
    cJSON_bool result = compare_pointers(name, pointer, 1);
    TEST_ASSERT_TRUE_MESSAGE(result, "Escaped slash (~1) should match '/' in name");
}

/* Test 6: Pointer segment ends at '/' — name matches up to that point */
void test_compare_pointers_pointer_ends_at_slash(void)
{
    /* pointer = "foo/bar", name = "foo" — should match the "foo" segment */
    const unsigned char *name    = (const unsigned char *)"foo";
    const unsigned char *pointer = (const unsigned char *)"foo/bar";
    cJSON_bool result = compare_pointers(name, pointer, 1);
    TEST_ASSERT_TRUE_MESSAGE(result, "Pointer segment 'foo' should match name 'foo' when pointer has trailing '/bar'");
}

/* Test 7: Mismatch returns false */
void test_compare_pointers_mismatch_returns_false(void)
{
    const unsigned char *name    = (const unsigned char *)"abc";
    const unsigned char *pointer = (const unsigned char *)"xyz";
    cJSON_bool result = compare_pointers(name, pointer, 1);
    TEST_ASSERT_FALSE_MESSAGE(result, "Mismatched strings should return false");
}

/* Test 8: Name longer than pointer segment returns false */
void test_compare_pointers_name_longer_than_pointer(void)
{
    const unsigned char *name    = (const unsigned char *)"foobar";
    const unsigned char *pointer = (const unsigned char *)"foo";
    cJSON_bool result = compare_pointers(name, pointer, 1);
    TEST_ASSERT_FALSE_MESSAGE(result, "Name longer than pointer segment should return false");
}

/* Test 9: Pointer segment longer than name returns false */
void test_compare_pointers_pointer_longer_than_name(void)
{
    const unsigned char *name    = (const unsigned char *)"foo";
    const unsigned char *pointer = (const unsigned char *)"foobar";
    cJSON_bool result = compare_pointers(name, pointer, 1);
    TEST_ASSERT_FALSE_MESSAGE(result, "Pointer segment longer than name should return false");
}

/* Test 10: Both empty strings match */
void test_compare_pointers_both_empty(void)
{
    const unsigned char *name    = (const unsigned char *)"";
    const unsigned char *pointer = (const unsigned char *)"";
    cJSON_bool result = compare_pointers(name, pointer, 1);
    TEST_ASSERT_TRUE_MESSAGE(result, "Both empty strings should match");
}

/* Test 11: Case sensitive mismatch that would match case-insensitively */
void test_compare_pointers_case_sensitive_mismatch(void)
{
    const unsigned char *name    = (const unsigned char *)"Hello";
    const unsigned char *pointer = (const unsigned char *)"hello";
    cJSON_bool result = compare_pointers(name, pointer, 1);
    TEST_ASSERT_FALSE_MESSAGE(result, "Case-sensitive compare should fail for 'Hello' vs 'hello'");
}

/* Test 12: Invalid escape sequence returns false */
void test_compare_pointers_invalid_escape_sequence(void)
{
    /* pointer has '~2' which is not a valid escape */
    const unsigned char *name    = (const unsigned char *)"a2b";
    const unsigned char *pointer = (const unsigned char *)"a~2b";
    cJSON_bool result = compare_pointers(name, pointer, 1);
    TEST_ASSERT_FALSE_MESSAGE(result, "Invalid escape sequence ~2 should return false");
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_compare_pointers_null_name_returns_false);
    RUN_TEST(test_compare_pointers_null_pointer_returns_false);
    RUN_TEST(test_compare_pointers_exact_match_case_sensitive);
    RUN_TEST(test_compare_pointers_case_insensitive_match);
    RUN_TEST(test_compare_pointers_escaped_tilde);
    RUN_TEST(test_compare_pointers_escaped_slash);
    RUN_TEST(test_compare_pointers_pointer_ends_at_slash);
    RUN_TEST(test_compare_pointers_mismatch_returns_false);
    RUN_TEST(test_compare_pointers_name_longer_than_pointer);
    RUN_TEST(test_compare_pointers_pointer_longer_than_name);
    RUN_TEST(test_compare_pointers_both_empty);
    RUN_TEST(test_compare_pointers_case_sensitive_mismatch);
    RUN_TEST(test_compare_pointers_invalid_escape_sequence);
    return UNITY_END();
}