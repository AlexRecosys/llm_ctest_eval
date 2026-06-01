#include "cJSON.c"
#include "unity.h"
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>
#include <locale.h>

/* Global variables for test fixtures */
static cJSON_Hooks original_hooks = {0};
static cJSON *test_item = NULL;
static parse_buffer test_buffer = {0};
static unsigned char test_input[1024] = {0};

/* Signal handling for segmentation faults */
static sig_atomic_t segv_caught = 0;
static jmp_buf segv_env;

static void segv_handler(int sig) {
    (void)sig;
    segv_caught = 1;
    longjmp(segv_env, 1);
}

/* Helper to safely run parse_string with signal protection */
static cJSON_bool safe_parse_string(cJSON *item, parse_buffer *buf) {
    segv_caught = 0;
    if (setjmp(segv_env) == 0) {
        return parse_string(item, buf);
    } else {
        return false;
    }
}

void setUp(void) {
    /* Save original hooks */
    cJSON_GetHooks(&original_hooks);
    
    /* Initialize test item */
    test_item = cJSON_CreateNull();
    TEST_ASSERT_NOT_NULL(test_item);
    
    /* Initialize parse buffer with default hooks */
    memset(&test_buffer, 0, sizeof(test_buffer));
    test_buffer.hooks.allocate = malloc;
    test_buffer.hooks.deallocate = free;
    test_buffer.hooks.reallocate = realloc;
    
    /* Set up test input buffer */
    memset(test_input, 0, sizeof(test_input));
    test_buffer.content = test_input;
    test_buffer.length = 0;
    test_buffer.offset = 0;
}

void tearDown(void) {
    /* Clean up test item */
    if (test_item != NULL) {
        cJSON_Delete(test_item);
        test_item = NULL;
    }
    
    /* Restore original hooks */
    cJSON_InitHooks(&original_hooks);
}

/* Test 1: Parse a simple valid string */
void test_parse_string_simple(void) {
    const char *input = "\"hello\"";
    size_t len = strlen(input);
    
    /* Setup buffer */
    memcpy(test_input, input, len);
    test_buffer.content = test_input;
    test_buffer.length = len;
    test_buffer.offset = 0;
    
    /* Parse */
    cJSON_bool result = safe_parse_string(test_item, &test_buffer);
    
    TEST_ASSERT_TRUE_MESSAGE(result, "Parsing simple string should succeed");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("hello", test_item->valuestring, "String value should match input");
    TEST_ASSERT_EQUAL_INT_MESSAGE(cJSON_String, test_item->type, "Type should be cJSON_String");
}

/* Test 2: Parse string with escape sequences */
void test_parse_string_escapes(void) {
    const char *input = "\"hello\\nworld\\t\\\"escaped\\\"\"";
    size_t len = strlen(input);
    
    /* Setup buffer */
    memcpy(test_input, input, len);
    test_buffer.content = test_input;
    test_buffer.length = len;
    test_buffer.offset = 0;
    
    /* Parse */
    cJSON_bool result = safe_parse_string(test_item, &test_buffer);
    
    TEST_ASSERT_TRUE_MESSAGE(result, "Parsing string with escapes should succeed");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("hello\nworld\t\"escaped\"", test_item->valuestring, "Escaped string should be unescaped correctly");
}

/* Test 3: Parse empty string */
void test_parse_string_empty(void) {
    const char *input = "\"\"";
    size_t len = strlen(input);
    
    /* Setup buffer */
    memcpy(test_input, input, len);
    test_buffer.content = test_input;
    test_buffer.length = len;
    test_buffer.offset = 0;
    
    /* Parse */
    cJSON_bool result = safe_parse_string(test_item, &test_buffer);
    
    TEST_ASSERT_TRUE_MESSAGE(result, "Parsing empty string should succeed");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("", test_item->valuestring, "Empty string should be empty");
    TEST_ASSERT_EQUAL_INT_MESSAGE(cJSON_String, test_item->type, "Type should be cJSON_String");
}

/* Test 4: Parse invalid string (missing closing quote) */
void test_parse_string_unclosed(void) {
    const char *input = "\"hello";
    size_t len = strlen(input);
    
    /* Setup buffer */
    memcpy(test_input, input, len);
    test_buffer.content = test_input;
    test_buffer.length = len;
    test_buffer.offset = 0;
    
    /* Parse */
    cJSON_bool result = safe_parse_string(test_item, &test_buffer);
    
    TEST_ASSERT_FALSE_MESSAGE(result, "Parsing unclosed string should fail");
    TEST_ASSERT_NULL_MESSAGE(test_item->valuestring, "valuestring should be NULL on failure");
}

/* Test 5: Parse string with invalid escape sequence */
void test_parse_string_invalid_escape(void) {
    const char *input = "\"hello\\x\"";
    size_t len = strlen(input);
    
    /* Setup buffer */
    memcpy(test_input, input, len);
    test_buffer.content = test_input;
    test_buffer.length = len;
    test_buffer.offset = 0;
    
    /* Parse */
    cJSON_bool result = safe_parse_string(test_item, &test_buffer);
    
    TEST_ASSERT_FALSE_MESSAGE(result, "Parsing string with invalid escape should fail");
    TEST_ASSERT_NULL_MESSAGE(test_item->valuestring, "valuestring should be NULL on failure");
}

int main(void) {
    /* Set locale for proper decimal point handling */
    setlocale(LC_ALL, "C");
    
    /* Set up signal handler for segmentation faults */
    signal(SIGSEGV, segv_handler);
    
    UNITY_BEGIN();
    
    RUN_TEST(test_parse_string_simple);
    RUN_TEST(test_parse_string_escapes);
    RUN_TEST(test_parse_string_empty);
    RUN_TEST(test_parse_string_unclosed);
    RUN_TEST(test_parse_string_invalid_escape);
    
    return UNITY_END();
}