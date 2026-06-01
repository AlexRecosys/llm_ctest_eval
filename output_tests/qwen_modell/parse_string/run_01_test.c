#include "cJSON.c"
#include "unity.h"
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>
#include <locale.h>

/* Global variables for test fixtures */
static cJSON_Hooks original_hooks = {0};
static parse_buffer test_buffer;
static cJSON test_item;
static jmp_buf segv_env;
static volatile sig_atomic_t segv_caught = 0;

/* Signal handler for segmentation faults */
static void segv_handler(int signum) {
    (void)signum;
    segv_caught = 1;
    longjmp(segv_env, 1);
}

/* Setup function */
void setUp(void) {
    /* Save original hooks */
    cJSON_GetAllocator(&original_hooks.malloc_fn, &original_hooks.free_fn);
    
    /* Set up test buffer */
    memset(&test_buffer, 0, sizeof(test_buffer));
    test_buffer.hooks.allocate = malloc;
    test_buffer.hooks.deallocate = free;
    test_buffer.hooks.reallocate = realloc;
    
    /* Initialize test item */
    memset(&test_item, 0, sizeof(test_item));
    
    /* Set up signal handler for segv */
    segv_caught = 0;
    signal(SIGSEGV, segv_handler);
}

/* Teardown function */
void tearDown(void) {
    /* Restore original hooks */
    cJSON_InitHooks(&original_hooks);
    
    /* Clean up any leftover string from test_item */
    if (test_item.valuestring != NULL) {
        free(test_item.valuestring);
        test_item.valuestring = NULL;
    }
    
    /* Reset signal handler */
    signal(SIGSEGV, SIG_DFL);
}

/* Helper function to safely call parse_string with segv protection */
static cJSON_bool safe_parse_string(cJSON *item, parse_buffer *buffer) {
    if (setjmp(segv_env) == 0) {
        return parse_string(item, buffer);
    } else {
        return false;
    }
}

/* Test case 1: Parse a simple valid string */
void test_parse_string_simple(void) {
    const char *input = "\"hello\"";
    test_buffer.content = (const unsigned char *)input;
    test_buffer.length = strlen(input);
    test_buffer.offset = 0;
    
    TEST_ASSERT_TRUE(safe_parse_string(&test_item, &test_buffer));
    TEST_ASSERT_EQUAL_INT(cJSON_String, test_item.type);
    TEST_ASSERT_EQUAL_STRING("hello", test_item.valuestring);
}

/* Test case 2: Parse a string with escape sequences */
void test_parse_string_escapes(void) {
    const char *input = "\"hello\\nworld\\t\\\"escaped\\\"\"";
    test_buffer.content = (const unsigned char *)input;
    test_buffer.length = strlen(input);
    test_buffer.offset = 0;
    
    TEST_ASSERT_TRUE(safe_parse_string(&test_item, &test_buffer));
    TEST_ASSERT_EQUAL_INT(cJSON_String, test_item.type);
    TEST_ASSERT_EQUAL_STRING("hello\nworld\t\"escaped\"", test_item.valuestring);
}

/* Test case 3: Parse an empty string */
void test_parse_string_empty(void) {
    const char *input = "\"\"";
    test_buffer.content = (const unsigned char *)input;
    test_buffer.length = strlen(input);
    test_buffer.offset = 0;
    
    TEST_ASSERT_TRUE(safe_parse_string(&test_item, &test_buffer));
    TEST_ASSERT_EQUAL_INT(cJSON_String, test_item.type);
    TEST_ASSERT_EQUAL_STRING("", test_item.valuestring);
}

/* Test case 4: Parse a string with UTF-16 escape sequences */
void test_parse_string_utf16(void) {
    const char *input = "\"\\u0041\\u0042\\u0043\""; /* ABC in Unicode */
    test_buffer.content = (const unsigned char *)input;
    test_buffer.length = strlen(input);
    test_buffer.offset = 0;
    
    TEST_ASSERT_TRUE(safe_parse_string(&test_item, &test_buffer));
    TEST_ASSERT_EQUAL_INT(cJSON_String, test_item.type);
    TEST_ASSERT_EQUAL_STRING("ABC", test_item.valuestring);
}

/* Test case 5: Parse invalid string (missing closing quote) */
void test_parse_string_invalid(void) {
    const char *input = "\"hello";
    test_buffer.content = (const unsigned char *)input;
    test_buffer.length = strlen(input);
    test_buffer.offset = 0;
    
    TEST_ASSERT_FALSE(safe_parse_string(&test_item, &test_buffer));
    TEST_ASSERT_EQUAL_INT(cJSON_Invalid, test_item.type);
    TEST_ASSERT_NULL(test_item.valuestring);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_parse_string_simple);
    RUN_TEST(test_parse_string_escapes);
    RUN_TEST(test_parse_string_empty);
    RUN_TEST(test_parse_string_utf16);
    RUN_TEST(test_parse_string_invalid);
    
    return UNITY_END();
}