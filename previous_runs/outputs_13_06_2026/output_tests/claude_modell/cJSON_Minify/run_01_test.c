#include "cJSON.c"
#include "unity.h"
#include "cJSON.h"
#include <stdlib.h>
#include <string.h>
#include <signal.h>

/* File-scope variables */
static volatile int segfault_occurred = 0;

/* Signal handler for SIGSEGV */
static void sigsegv_handler(int sig)
{
    (void)sig;
    segfault_occurred = 1;
    /* We cannot safely longjmp here in all cases, so just mark and return */
    TEST_FAIL_MESSAGE("Segmentation fault occurred during test");
}

void setUp(void)
{
    segfault_occurred = 0;
    signal(SIGSEGV, sigsegv_handler);
}

void tearDown(void)
{
    signal(SIGSEGV, SIG_DFL);
    segfault_occurred = 0;
}

/* Helper: duplicate a string literal into a writable heap buffer */
static char *make_writable(const char *src)
{
    size_t len = strlen(src);
    char *buf = (char *)malloc(len + 1);
    if (buf != NULL)
    {
        memcpy(buf, src, len + 1);
    }
    return buf;
}

/* Test 1: NULL input — should return without crashing */
void test_minify_null_input(void)
{
    /* Should not crash or segfault */
    cJSON_Minify(NULL);
    TEST_ASSERT_FALSE_MESSAGE(segfault_occurred, "cJSON_Minify(NULL) caused a segfault");
}

/* Test 2: Whitespace-only string — result should be empty */
void test_minify_whitespace_only(void)
{
    char *input = make_writable("   \t\r\n  \t  ");
    TEST_ASSERT_NOT_NULL(input);

    cJSON_Minify(input);

    TEST_ASSERT_EQUAL_STRING_MESSAGE("", input, "Whitespace-only input should minify to empty string");

    free(input);
}

/* Test 3: Simple JSON object with whitespace */
void test_minify_simple_object_with_whitespace(void)
{
    char *input = make_writable("{ \"key\" : \"value\" }");
    TEST_ASSERT_NOT_NULL(input);

    cJSON_Minify(input);

    TEST_ASSERT_EQUAL_STRING_MESSAGE("{\"key\":\"value\"}", input,
        "Simple JSON object should have whitespace removed outside strings");

    free(input);
}

/* Test 4: JSON with single-line comment */
void test_minify_single_line_comment(void)
{
    char *input = make_writable("{\"a\":1}// this is a comment\n{\"b\":2}");
    TEST_ASSERT_NOT_NULL(input);

    cJSON_Minify(input);

    /* After the comment is stripped, the result should not contain the comment text */
    TEST_ASSERT_NULL_MESSAGE(strstr(input, "//"), "Single-line comment should be removed");
    TEST_ASSERT_NULL_MESSAGE(strstr(input, "this is a comment"), "Comment text should be removed");

    free(input);
}

/* Test 5: JSON with multiline comment */
void test_minify_multiline_comment(void)
{
    char *input = make_writable("{\"x\":/* this is\na multiline\ncomment */42}");
    TEST_ASSERT_NOT_NULL(input);

    cJSON_Minify(input);

    TEST_ASSERT_NULL_MESSAGE(strstr(input, "/*"), "Multiline comment start should be removed");
    TEST_ASSERT_NULL_MESSAGE(strstr(input, "*/"), "Multiline comment end should be removed");
    TEST_ASSERT_NULL_MESSAGE(strstr(input, "multiline"), "Multiline comment content should be removed");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("{\"x\":42}", input,
        "JSON with multiline comment should minify correctly");

    free(input);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_minify_null_input);
    RUN_TEST(test_minify_whitespace_only);
    RUN_TEST(test_minify_simple_object_with_whitespace);
    RUN_TEST(test_minify_single_line_comment);
    RUN_TEST(test_minify_multiline_comment);
    return UNITY_END();
}