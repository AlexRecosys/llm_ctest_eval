#include "cJSON_utils.c"
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
    /* We can't safely longjmp here in all cases, so just mark it */
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
    TEST_ASSERT_FALSE_MESSAGE(result, "Expected false when name is NULL");
}

void test_compare_pointers_null_pointer_returns_false(void)
{
    cJSON_bool result = compare_pointers((const unsigned char *)"foo", NULL, 1);
    TEST_ASSERT_FALSE_MESSAGE(result, "Expected false when pointer is NULL");
}

/* Test 2: Exact match, case sensitive */
void test_compare_pointers_exact_match_case_sensitive(void)
{
    const unsigned char *name    = (const unsigned char *)"hello";
    const unsigned char *pointer = (const unsigned char *)"hello";
    cJSON_bool result = compare_pointers(name, pointer, 1);
    TEST_ASSERT_TRUE_MESSAGE(result, "Expected true for exact match (case sensitive)");
}

/* Test 3: Case insensitive match */
void test_compare_pointers_case_insensitive_match(void)
{
    const unsigned char *name    = (const unsigned char *)"Hello";
    const unsigned char *pointer = (const unsigned char *)"hello";
    cJSON_bool result = compare_pointers(name, pointer, 0);
    TEST_ASSERT_TRUE_MESSAGE(result, "Expected true for case-insensitive match");
}

/* Test 4: Case sensitive mismatch due to case difference */
void test_compare_pointers_case_sensitive_mismatch(void)
{
    const unsigned char *name    = (const unsigned char *)"Hello";
    const unsigned char *pointer = (const unsigned char *)"hello";
    cJSON_bool result = compare_pointers(name, pointer, 1);
    TEST_ASSERT_FALSE_MESSAGE(result, "Expected false for case-sensitive mismatch");
}

/* Test 5: Escaped tilde sequences (~0 -> ~, ~1 -> /) */
void test_compare_pointers_escaped_tilde_zero(void)
{
    /* pointer contains ~0 which should match name containing ~ */
    const unsigned char *name    = (const unsigned char *)"a~b";
    const unsigned char *pointer = (const unsigned char *)"a~0b";
    cJSON_bool result = compare_pointers(name, pointer, 1);
    TEST_ASSERT_TRUE_MESSAGE(result, "Expected true: ~0 in pointer should match ~ in name");
}

void test_compare_pointers_escaped_tilde_one(void)
{
    /* pointer contains ~1 which should match name containing / */
    const unsigned char *name    = (const unsigned char *)"a/b";
    const unsigned char *pointer = (const unsigned char *)"a~1b";
    cJSON_bool result = compare_pointers(name, pointer, 1);
    TEST_ASSERT_TRUE_MESSAGE(result, "Expected true: ~1 in pointer should match / in name");
}

/* Test 6: Pointer segment ends at '/' — only compare up to the slash */
void test_compare_pointers_stops_at_slash(void)
{
    /* name is "foo", pointer is "foo/bar" — should match up to the slash */
    const unsigned char *name    = (const unsigned char *)"foo";
    const unsigned char *pointer = (const unsigned char *)"foo/bar";
    cJSON_bool result = compare_pointers(name, pointer, 1);
    TEST_ASSERT_TRUE_MESSAGE(result, "Expected true: pointer segment 'foo' matches name 'foo'");
}

/* Test 7: Pointer segment shorter than name — should return false */
void test_compare_pointers_pointer_shorter_than_name(void)
{
    const unsigned char *name    = (const unsigned char *)"foobar";
    const unsigned char *pointer = (const unsigned char *)"foo/bar";
    cJSON_bool result = compare_pointers(name, pointer, 1);
    TEST_ASSERT_FALSE_MESSAGE(result, "Expected false: pointer segment 'foo' does not match name 'foobar'");
}

/* Test 8: Both empty strings — should return true */
void test_compare_pointers_both_empty(void)
{
    const unsigned char *name    = (const unsigned char *)"";
    const unsigned char *pointer = (const unsigned char *)"";
    cJSON_bool result = compare_pointers(name, pointer, 1);
    TEST_ASSERT_TRUE_MESSAGE(result, "Expected true: both empty strings match");
}

/* Test 9: Invalid escape sequence (~2) should return false */
void test_compare_pointers_invalid_escape_sequence(void)
{
    /* ~2 is not a valid escape; name has 'a', pointer has ~2 */
    const unsigned char *name    = (const unsigned char *)"a~b";
    const unsigned char *pointer = (const unsigned char *)"a~2b";
    cJSON_bool result = compare_pointers(name, pointer, 1);
    TEST_ASSERT_FALSE_MESSAGE(result, "Expected false: ~2 is an invalid escape sequence");
}

/* Test 10: name longer than pointer segment */
void test_compare_pointers_name_longer_than_pointer(void)
{
    const unsigned char *name    = (const unsigned char *)"foobar";
    const unsigned char *pointer = (const unsigned char *)"foo";
    cJSON_bool result = compare_pointers(name, pointer, 1);
    TEST_ASSERT_FALSE_MESSAGE(result, "Expected false: name 'foobar' longer than pointer 'foo'");
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_compare_pointers_null_name_returns_false);
    RUN_TEST(test_compare_pointers_null_pointer_returns_false);
    RUN_TEST(test_compare_pointers_exact_match_case_sensitive);
    RUN_TEST(test_compare_pointers_case_insensitive_match);
    RUN_TEST(test_compare_pointers_case_sensitive_mismatch);
    RUN_TEST(test_compare_pointers_escaped_tilde_zero);
    RUN_TEST(test_compare_pointers_escaped_tilde_one);
    RUN_TEST(test_compare_pointers_stops_at_slash);
    RUN_TEST(test_compare_pointers_pointer_shorter_than_name);
    RUN_TEST(test_compare_pointers_both_empty);
    RUN_TEST(test_compare_pointers_invalid_escape_sequence);
    RUN_TEST(test_compare_pointers_name_longer_than_pointer);
    return UNITY_END();
}