#include "cJSON.c"
#include "unity.h"
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>
#include <locale.h>

/* Global variables for test fixtures */
static cJSON_Hooks original_hooks = {0};
static parse_buffer test_buffer = {0};
static unsigned char *test_input = NULL;
static cJSON test_item = {0};

/* Signal handling for segmentation faults */
static sig_atomic_t segv_caught = 0;
static jmp_buf segv_env;

static void segv_handler(int sig) {
    (void)sig;
    segv_caught = 1;
    longjmp(segv_env, 1);
}

/* Helper function to set up parse buffer */
static void setup_parse_buffer(const char *input) {
    size_t len = strlen(input);
    
    /* Free previous test input if any */
    if (test_input != NULL) {
        free(test_input);
        test_input = NULL;
    }
    
    /* Allocate and copy input */
    test_input = (unsigned char *)malloc(len + 1);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_input, "Failed to allocate test input buffer");
    memcpy(test_input, input, len + 1);
    
    /* Initialize buffer */
    memset(&test_buffer, 0, sizeof(test_buffer));
    test_buffer.content = test_input;
    test_buffer.length = len;
    test_buffer.offset = 0;
    test_buffer.hooks.allocate = malloc;
    test_buffer.hooks.deallocate = free;
    test_buffer.hooks.reallocate = realloc;
    
    /* Initialize item */
    memset(&test_item, 0, sizeof(test_item));
}

/* Helper function to safely call parse_string with segv protection */
static cJSON_bool safe_parse_string(cJSON *item, parse_buffer *buf) {
    if (setjmp(segv_env) == 0) {
        segv_caught = 0;
        return parse_string(item, buf);
    } else {
        return false;
    }
}

void setUp(void) {
    /* Save original hooks */
    cJSON_GetHooks(&original_hooks);
    
    /* Set up signal handler for segv */
    signal(SIGSEGV, segv_handler);
    
    /* Initialize locale for decimal point handling */
    setlocale(LC_NUMERIC, "C");
}

void tearDown(void) {
    /* Restore original hooks */
    cJSON_InitHooks(&original_hooks);
    
    /* Clean up test input buffer */
    if (test_input != NULL) {
        free(test_input);
        test_input = NULL;
    }
    
    /* Clean up test item if it has allocated string */
    if (test_item.valuestring != NULL) {
        free(test_item.valuestring);
        test_item.valuestring = NULL;
    }
    
    /* Reset segv handler */
    signal(SIGSEGV, SIG_DFL);
}

/* Test 1: Parse a simple valid string */
void test_parse_string_simple(void) {
    setup_parse_buffer("\"hello\"");
    
    TEST_ASSERT_TRUE(safe_parse_string(&test_item, &test_buffer));
    TEST_ASSERT_EQUAL_INT(cJSON_String, test_item.type);
    TEST_ASSERT_EQUAL_STRING("hello", test_item.valuestring);
    TEST_ASSERT_EQUAL_SIZE(7, test_buffer.offset); /* includes closing quote */
}

/* Test 2: Parse string with escape sequences */
void test_parse_string_escapes(void) {
    setup_parse_buffer("\"hello\\nworld\\t\\\"escaped\\\"\"");
    
    TEST_ASSERT_TRUE(safe_parse_string(&test_item, &test_buffer));
    TEST_ASSERT_EQUAL_INT(cJSON_String, test_item.type);
    TEST_ASSERT_EQUAL_STRING("hello\nworld\t\"escaped\"", test_item.valuestring);
    TEST_ASSERT_EQUAL_SIZE(28, test_buffer.offset);
}

/* Test 3: Parse empty string */
void test_parse_string_empty(void) {
    setup_parse_buffer("\"\"");
    
    TEST_ASSERT_TRUE(safe_parse_string(&test_item, &test_buffer));
    TEST_ASSERT_EQUAL_INT(cJSON_String, test_item.type);
    TEST_ASSERT_EQUAL_STRING("", test_item.valuestring);
    TEST_ASSERT_EQUAL_SIZE(2, test_buffer.offset);
}

/* Test 4: Parse string with invalid escape sequence */
void test_parse_string_invalid_escape(void) {
    setup_parse_buffer("\"hello\\x\"");
    
    TEST_ASSERT_FALSE(safe_parse_string(&test_item, &test_buffer));
    TEST_ASSERT_EQUAL_INT(cJSON_Invalid, test_item.type);
    TEST_ASSERT_NULL(test_item.valuestring);
}

/* Test 5: Parse unterminated string */
void test_parse_string_unterminated(void) {
    setup_parse_buffer("\"hello");
    
    TEST_ASSERT_FALSE(safe_parse_string(&test_item, &test_buffer));
    TEST_ASSERT_EQUAL_INT(cJSON_Invalid, test_item.type);
    TEST_ASSERT_NULL(test_item.valuestring);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_parse_string_simple);
    RUN_TEST(test_parse_string_escapes);
    RUN_TEST(test_parse_string_empty);
    RUN_TEST(test_parse_string_invalid_escape);
    RUN_TEST(test_parse_string_unterminated);
    
    return UNITY_END();
}