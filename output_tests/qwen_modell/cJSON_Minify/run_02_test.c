#include "cJSON.c"
#include "unity.h"
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>

/* Global variables for signal handling */
static sig_atomic_t segv_caught = 0;
static sigjmp_buf jump_buffer;

/* Signal handler for SIGSEGV */
static void segv_handler(int sig) {
    (void)sig;
    segv_caught = 1;
    siglongjmp(jump_buffer, 1);
}

/* setUp and tearDown must be non-static per requirements */
void setUp(void) {
    segv_caught = 0;
    signal(SIGSEGV, segv_handler);
}

void tearDown(void) {
    signal(SIGSEGV, SIG_DFL);
}

/* Helper to safely run minify with segv protection */
static void run_minify_with_protection(char *input) {
    if (sigsetjmp(jump_buffer, 1) == 0) {
        cJSON_Minify(input);
    } else {
        TEST_FAIL_MESSAGE("Segmentation fault detected in cJSON_Minify");
    }
}

/* Test 1: Minify JSON with whitespace only */
static void test_cJSON_Minify_should_remove_all_whitespace(void) {
    char input[] = "  {  \"key\" : \"value\" , \"num\" : 123 }  ";
    char expected[] = "{\"key\":\"value\",\"num\":123}";

    run_minify_with_protection(input);

    TEST_ASSERT_EQUAL_STRING_MESSAGE(expected, input,
        "cJSON_Minify should remove all whitespace and comments");
}

/* Test 2: Minify JSON with comments */
static void test_cJSON_Minify_should_remove_comments(void) {
    char input[] = "{ // this is a comment\n\"key\": \"value\" /* multiline\ncomment */}";
    char expected[] = "{\"key\":\"value\"}";

    run_minify_with_protection(input);

    TEST_ASSERT_EQUAL_STRING_MESSAGE(expected, input,
        "cJSON_Minify should remove both single-line and multi-line comments");
}

/* Test 3: Minify JSON with string containing whitespace */
static void test_cJSON_Minify_should_preserve_whitespace_in_strings(void) {
    char input[] = "{ \"key\" : \"  value with spaces  \" }";
    char expected[] = "{\"key\":\"  value with spaces  \"}";

    run_minify_with_protection(input);

    TEST_ASSERT_EQUAL_STRING_MESSAGE(expected, input,
        "cJSON_Minify should preserve whitespace inside strings");
}

/* Test 4: Minify empty string */
static void test_cJSON_Minify_should_handle_empty_string(void) {
    char input[] = "";
    char expected[] = "";

    run_minify_with_protection(input);

    TEST_ASSERT_EQUAL_STRING_MESSAGE(expected, input,
        "cJSON_Minify should handle empty string correctly");
}

/* Test 5: Minify NULL input */
static void test_cJSON_Minify_should_handle_NULL_input(void) {
    /* Should not crash */
    cJSON_Minify(NULL);
    /* If we reach here, test passes */
    TEST_ASSERT_TRUE_MESSAGE(1, "cJSON_Minify(NULL) should not crash");
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_cJSON_Minify_should_remove_all_whitespace);
    RUN_TEST(test_cJSON_Minify_should_remove_comments);
    RUN_TEST(test_cJSON_Minify_should_preserve_whitespace_in_strings);
    RUN_TEST(test_cJSON_Minify_should_handle_empty_string);
    RUN_TEST(test_cJSON_Minify_should_handle_NULL_input);

    return UNITY_END();
}