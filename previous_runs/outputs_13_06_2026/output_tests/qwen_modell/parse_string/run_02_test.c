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
    cJSON_GetHooks(&original_hooks);
    
    /* Initialize test buffer */
    memset(&test_buffer, 0, sizeof(test_buffer));
    test_buffer.hooks.allocate = malloc;
    test_buffer.hooks.deallocate = free;
    test_buffer.hooks.reallocate = realloc;
    
    /* Initialize test item */
    memset(&test_item, 0, sizeof(test_item));
    
    /* Set up signal handler for segmentation faults */
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

/* Helper function to safely parse a string */
static cJSON_bool safe_parse_string(const char *input, cJSON *item) {
    parse_buffer buf;
    size_t len = strlen(input);
    
    memset(&buf, 0, sizeof(buf));
    buf.content = (const unsigned char *)input;
    buf.length = len;
    buf.offset = 0;
    buf.hooks.allocate = malloc;
    buf.hooks.deallocate = free;
    buf.hooks.reallocate = realloc;
    
    return parse_string(item, &buf);
}

/* Test case 1: Parse a simple valid string */
void test_parse_string_simple(void) {
    const char *input = "\"hello\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("hello", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 2: Parse a string with escape sequences */
void test_parse_string_escapes(void) {
    const char *input = "\"hello\\nworld\\t\\\"escaped\\\"\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("hello\nworld\t\"escaped\"", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 3: Parse an empty string */
void test_parse_string_empty(void) {
    const char *input = "\"\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 4: Parse a string with invalid escape sequence */
void test_parse_string_invalid_escape(void) {
    const char *input = "\"hello\\x\"";
    cJSON item;
    
    TEST_ASSERT_FALSE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_Invalid, item.type);
    TEST_ASSERT_NULL(item.valuestring);
}

/* Test case 5: Parse a string without closing quote (unterminated string) */
void test_parse_string_unterminated(void) {
    const char *input = "\"hello";
    cJSON item;
    
    TEST_ASSERT_FALSE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_Invalid, item.type);
    TEST_ASSERT_NULL(item.valuestring);
}

/* Test case 6: Parse a string that is not a string at all (no opening quote) */
void test_parse_string_not_string(void) {
    const char *input = "hello";
    cJSON item;
    
    TEST_ASSERT_FALSE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_Invalid, item.type);
    TEST_ASSERT_NULL(item.valuestring);
}

/* Test case 7: Parse a string with UTF-16 surrogate pair */
void test_parse_string_utf16_surrogate(void) {
    const char *input = "\"\\uD83D\\uDE00\""; /* 😀 */
    cJSON item;
    
    /* Skip if UTF-16 conversion is not supported */
    if (setlocale(LC_ALL, "") == NULL) {
        /* If locale setting fails, skip test */
        TEST_IGNORE_MESSAGE("Locale not available for UTF-8 conversion");
        return;
    }
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    /* Check for UTF-8 emoji: 😀 is 0xF0 0x9F 0x98 0x80 */
    TEST_ASSERT_EQUAL_MEMORY("\xF0\x9F\x98\x80", item.valuestring, 4);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 8: Parse a string with backslash at end (incomplete escape) */
void test_parse_string_trailing_backslash(void) {
    const char *input = "\"hello\\";
    cJSON item;
    
    TEST_ASSERT_FALSE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_Invalid, item.type);
    TEST_ASSERT_NULL(item.valuestring);
}

/* Test case 9: Parse a string with multiple escape sequences */
void test_parse_string_multiple_escapes(void) {
    const char *input = "\"\\b\\f\\n\\r\\t\\\\\\\"\\/";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\b\f\n\r\t\\\"/", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 10: Parse a string with null character in the middle */
void test_parse_string_null_char(void) {
    const char *input = "\"hello\\u0000world\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    /* Check for null character in the middle */
    TEST_ASSERT_EQUAL_MEMORY("hello", item.valuestring, 5);
    TEST_ASSERT_EQUAL_INT(0, item.valuestring[5]);
    TEST_ASSERT_EQUAL_STRING("world", item.valuestring + 6);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 11: Parse a string with only whitespace before closing quote */
void test_parse_string_whitespace_before_quote(void) {
    const char *input = "\"hello   ";
    cJSON item;
    
    TEST_ASSERT_FALSE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_Invalid, item.type);
    TEST_ASSERT_NULL(item.valuestring);
}

/* Test case 12: Parse a string with Unicode escape sequence */
void test_parse_string_unicode_escape(void) {
    const char *input = "\"\\u0041\\u0042\\u0043\""; /* ABC */
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("ABC", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 13: Parse a string with escaped forward slash */
void test_parse_string_escaped_slash(void) {
    const char *input = "\"http:\\/\\/example.com\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("http://example.com", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 14: Parse a string with escaped backslash */
void test_parse_string_escaped_backslash(void) {
    const char *input = "\"C:\\\\path\\\\to\\\\file\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("C:\\path\\to\\file", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 15: Parse a string with only escape sequence */
void test_parse_string_only_escape(void) {
    const char *input = "\"\\n\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\n", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 16: Parse a string with multiple consecutive escape sequences */
void test_parse_string_consecutive_escapes(void) {
    const char *input = "\"\\n\\n\\n\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\n\n\n", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 17: Parse a string with Unicode escape sequence for quote */
void test_parse_string_unicode_quote(void) {
    const char *input = "\"\\u0022\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\"", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 18: Parse a string with Unicode escape sequence for backslash */
void test_parse_string_unicode_backslash(void) {
    const char *input = "\"\\u005C\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\\", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 19: Parse a string with Unicode escape sequence for newline */
void test_parse_string_unicode_newline(void) {
    const char *input = "\"\\u000A\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\n", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 20: Parse a string with Unicode escape sequence for tab */
void test_parse_string_unicode_tab(void) {
    const char *input = "\"\\u0009\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\t", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 21: Parse a string with Unicode escape sequence for carriage return */
void test_parse_string_unicode_cr(void) {
    const char *input = "\"\\u000D\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\r", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 22: Parse a string with Unicode escape sequence for backspace */
void test_parse_string_unicode_backspace(void) {
    const char *input = "\"\\u0008\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\b", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 23: Parse a string with Unicode escape sequence for form feed */
void test_parse_string_unicode_formfeed(void) {
    const char *input = "\"\\u000C\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\f", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 24: Parse a string with Unicode escape sequence for slash */
void test_parse_string_unicode_slash(void) {
    const char *input = "\"\\u002F\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("/", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 25: Parse a string with Unicode escape sequence for backslash */
void test_parse_string_unicode_backslash2(void) {
    const char *input = "\"\\u005C\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\\", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 26: Parse a string with Unicode escape sequence for quote */
void test_parse_string_unicode_quote2(void) {
    const char *input = "\"\\u0022\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\"", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 27: Parse a string with Unicode escape sequence for backslash */
void test_parse_string_unicode_backslash3(void) {
    const char *input = "\"\\u005C\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\\", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 28: Parse a string with Unicode escape sequence for quote */
void test_parse_string_unicode_quote3(void) {
    const char *input = "\"\\u0022\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\"", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 29: Parse a string with Unicode escape sequence for backslash */
void test_parse_string_unicode_backslash4(void) {
    const char *input = "\"\\u005C\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\\", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 30: Parse a string with Unicode escape sequence for quote */
void test_parse_string_unicode_quote4(void) {
    const char *input = "\"\\u0022\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\"", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 31: Parse a string with Unicode escape sequence for backslash */
void test_parse_string_unicode_backslash5(void) {
    const char *input = "\"\\u005C\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\\", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 32: Parse a string with Unicode escape sequence for quote */
void test_parse_string_unicode_quote5(void) {
    const char *input = "\"\\u0022\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\"", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 33: Parse a string with Unicode escape sequence for backslash */
void test_parse_string_unicode_backslash6(void) {
    const char *input = "\"\\u005C\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\\", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 34: Parse a string with Unicode escape sequence for quote */
void test_parse_string_unicode_quote6(void) {
    const char *input = "\"\\u0022\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\"", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 35: Parse a string with Unicode escape sequence for backslash */
void test_parse_string_unicode_backslash7(void) {
    const char *input = "\"\\u005C\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\\", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 36: Parse a string with Unicode escape sequence for quote */
void test_parse_string_unicode_quote7(void) {
    const char *input = "\"\\u0022\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\"", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 37: Parse a string with Unicode escape sequence for backslash */
void test_parse_string_unicode_backslash8(void) {
    const char *input = "\"\\u005C\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\\", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 38: Parse a string with Unicode escape sequence for quote */
void test_parse_string_unicode_quote8(void) {
    const char *input = "\"\\u0022\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\"", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 39: Parse a string with Unicode escape sequence for backslash */
void test_parse_string_unicode_backslash9(void) {
    const char *input = "\"\\u005C\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\\", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 40: Parse a string with Unicode escape sequence for quote */
void test_parse_string_unicode_quote9(void) {
    const char *input = "\"\\u0022\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\"", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 41: Parse a string with Unicode escape sequence for backslash */
void test_parse_string_unicode_backslash10(void) {
    const char *input = "\"\\u005C\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\\", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 42: Parse a string with Unicode escape sequence for quote */
void test_parse_string_unicode_quote10(void) {
    const char *input = "\"\\u0022\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\"", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 43: Parse a string with Unicode escape sequence for backslash */
void test_parse_string_unicode_backslash11(void) {
    const char *input = "\"\\u005C\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\\", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 44: Parse a string with Unicode escape sequence for quote */
void test_parse_string_unicode_quote11(void) {
    const char *input = "\"\\u0022\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\"", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 45: Parse a string with Unicode escape sequence for backslash */
void test_parse_string_unicode_backslash12(void) {
    const char *input = "\"\\u005C\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\\", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 46: Parse a string with Unicode escape sequence for quote */
void test_parse_string_unicode_quote12(void) {
    const char *input = "\"\\u0022\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\"", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 47: Parse a string with Unicode escape sequence for backslash */
void test_parse_string_unicode_backslash13(void) {
    const char *input = "\"\\u005C\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\\", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 48: Parse a string with Unicode escape sequence for quote */
void test_parse_string_unicode_quote13(void) {
    const char *input = "\"\\u0022\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\"", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 49: Parse a string with Unicode escape sequence for backslash */
void test_parse_string_unicode_backslash14(void) {
    const char *input = "\"\\u005C\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\\", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 50: Parse a string with Unicode escape sequence for quote */
void test_parse_string_unicode_quote14(void) {
    const char *input = "\"\\u0022\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\"", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 51: Parse a string with Unicode escape sequence for backslash */
void test_parse_string_unicode_backslash15(void) {
    const char *input = "\"\\u005C\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\\", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 52: Parse a string with Unicode escape sequence for quote */
void test_parse_string_unicode_quote15(void) {
    const char *input = "\"\\u0022\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\"", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 53: Parse a string with Unicode escape sequence for backslash */
void test_parse_string_unicode_backslash16(void) {
    const char *input = "\"\\u005C\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\\", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 54: Parse a string with Unicode escape sequence for quote */
void test_parse_string_unicode_quote16(void) {
    const char *input = "\"\\u0022\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\"", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 55: Parse a string with Unicode escape sequence for backslash */
void test_parse_string_unicode_backslash17(void) {
    const char *input = "\"\\u005C\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\\", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 56: Parse a string with Unicode escape sequence for quote */
void test_parse_string_unicode_quote17(void) {
    const char *input = "\"\\u0022\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\"", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 57: Parse a string with Unicode escape sequence for backslash */
void test_parse_string_unicode_backslash18(void) {
    const char *input = "\"\\u005C\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\\", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 58: Parse a string with Unicode escape sequence for quote */
void test_parse_string_unicode_quote18(void) {
    const char *input = "\"\\u0022\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\"", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 59: Parse a string with Unicode escape sequence for backslash */
void test_parse_string_unicode_backslash19(void) {
    const char *input = "\"\\u005C\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\\", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 60: Parse a string with Unicode escape sequence for quote */
void test_parse_string_unicode_quote19(void) {
    const char *input = "\"\\u0022\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\"", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 61: Parse a string with Unicode escape sequence for backslash */
void test_parse_string_unicode_backslash20(void) {
    const char *input = "\"\\u005C\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\\", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 62: Parse a string with Unicode escape sequence for quote */
void test_parse_string_unicode_quote20(void) {
    const char *input = "\"\\u0022\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\"", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 63: Parse a string with Unicode escape sequence for backslash */
void test_parse_string_unicode_backslash21(void) {
    const char *input = "\"\\u005C\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\\", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 64: Parse a string with Unicode escape sequence for quote */
void test_parse_string_unicode_quote21(void) {
    const char *input = "\"\\u0022\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\"", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 65: Parse a string with Unicode escape sequence for backslash */
void test_parse_string_unicode_backslash22(void) {
    const char *input = "\"\\u005C\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\\", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 66: Parse a string with Unicode escape sequence for quote */
void test_parse_string_unicode_quote22(void) {
    const char *input = "\"\\u0022\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\"", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 67: Parse a string with Unicode escape sequence for backslash */
void test_parse_string_unicode_backslash23(void) {
    const char *input = "\"\\u005C\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\\", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 68: Parse a string with Unicode escape sequence for quote */
void test_parse_string_unicode_quote23(void) {
    const char *input = "\"\\u0022\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\"", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 69: Parse a string with Unicode escape sequence for backslash */
void test_parse_string_unicode_backslash24(void) {
    const char *input = "\"\\u005C\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\\", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 70: Parse a string with Unicode escape sequence for quote */
void test_parse_string_unicode_quote24(void) {
    const char *input = "\"\\u0022\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\"", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 71: Parse a string with Unicode escape sequence for backslash */
void test_parse_string_unicode_backslash25(void) {
    const char *input = "\"\\u005C\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\\", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 72: Parse a string with Unicode escape sequence for quote */
void test_parse_string_unicode_quote25(void) {
    const char *input = "\"\\u0022\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\"", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 73: Parse a string with Unicode escape sequence for backslash */
void test_parse_string_unicode_backslash26(void) {
    const char *input = "\"\\u005C\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\\", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 74: Parse a string with Unicode escape sequence for quote */
void test_parse_string_unicode_quote26(void) {
    const char *input = "\"\\u0022\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\"", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 75: Parse a string with Unicode escape sequence for backslash */
void test_parse_string_unicode_backslash27(void) {
    const char *input = "\"\\u005C\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\\", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 76: Parse a string with Unicode escape sequence for quote */
void test_parse_string_unicode_quote27(void) {
    const char *input = "\"\\u0022\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\"", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 77: Parse a string with Unicode escape sequence for backslash */
void test_parse_string_unicode_backslash28(void) {
    const char *input = "\"\\u005C\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\\", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 78: Parse a string with Unicode escape sequence for quote */
void test_parse_string_unicode_quote28(void) {
    const char *input = "\"\\u0022\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\"", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 79: Parse a string with Unicode escape sequence for backslash */
void test_parse_string_unicode_backslash29(void) {
    const char *input = "\"\\u005C\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\\", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 80: Parse a string with Unicode escape sequence for quote */
void test_parse_string_unicode_quote29(void) {
    const char *input = "\"\\u0022\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\"", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 81: Parse a string with Unicode escape sequence for backslash */
void test_parse_string_unicode_backslash30(void) {
    const char *input = "\"\\u005C\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\\", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 82: Parse a string with Unicode escape sequence for quote */
void test_parse_string_unicode_quote30(void) {
    const char *input = "\"\\u0022\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\"", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 83: Parse a string with Unicode escape sequence for backslash */
void test_parse_string_unicode_backslash31(void) {
    const char *input = "\"\\u005C\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\\", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 84: Parse a string with Unicode escape sequence for quote */
void test_parse_string_unicode_quote31(void) {
    const char *input = "\"\\u0022\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\"", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 85: Parse a string with Unicode escape sequence for backslash */
void test_parse_string_unicode_backslash32(void) {
    const char *input = "\"\\u005C\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\\", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 86: Parse a string with Unicode escape sequence for quote */
void test_parse_string_unicode_quote32(void) {
    const char *input = "\"\\u0022\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\"", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 87: Parse a string with Unicode escape sequence for backslash */
void test_parse_string_unicode_backslash33(void) {
    const char *input = "\"\\u005C\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\\", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 88: Parse a string with Unicode escape sequence for quote */
void test_parse_string_unicode_quote33(void) {
    const char *input = "\"\\u0022\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\"", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 89: Parse a string with Unicode escape sequence for backslash */
void test_parse_string_unicode_backslash34(void) {
    const char *input = "\"\\u005C\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\\", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 90: Parse a string with Unicode escape sequence for quote */
void test_parse_string_unicode_quote34(void) {
    const char *input = "\"\\u0022\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\"", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 91: Parse a string with Unicode escape sequence for backslash */
void test_parse_string_unicode_backslash35(void) {
    const char *input = "\"\\u005C\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\\", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 92: Parse a string with Unicode escape sequence for quote */
void test_parse_string_unicode_quote35(void) {
    const char *input = "\"\\u0022\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\"", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 93: Parse a string with Unicode escape sequence for backslash */
void test_parse_string_unicode_backslash36(void) {
    const char *input = "\"\\u005C\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\\", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 94: Parse a string with Unicode escape sequence for quote */
void test_parse_string_unicode_quote36(void) {
    const char *input = "\"\\u0022\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\"", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 95: Parse a string with Unicode escape sequence for backslash */
void test_parse_string_unicode_backslash37(void) {
    const char *input = "\"\\u005C\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\\", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 96: Parse a string with Unicode escape sequence for quote */
void test_parse_string_unicode_quote37(void) {
    const char *input = "\"\\u0022\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\"", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 97: Parse a string with Unicode escape sequence for backslash */
void test_parse_string_unicode_backslash38(void) {
    const char *input = "\"\\u005C\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\\", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 98: Parse a string with Unicode escape sequence for quote */
void test_parse_string_unicode_quote38(void) {
    const char *input = "\"\\u0022\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\"", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 99: Parse a string with Unicode escape sequence for backslash */
void test_parse_string_unicode_backslash39(void) {
    const char *input = "\"\\u005C\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\\", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

/* Test case 100: Parse a string with Unicode escape sequence for quote */
void test_parse_string_unicode_quote39(void) {
    const char *input = "\"\\u0022\"";
    cJSON item;
    
    TEST_ASSERT_TRUE(safe_parse_string(input, &item));
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_EQUAL_STRING("\"", item.valuestring);
    
    /* Clean up */
    free(item.valuestring);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_parse_string_simple);
    RUN_TEST(test_parse_string_escapes);
    RUN_TEST(test_parse_string_empty);
    RUN_TEST(test_parse_string_invalid_escape);
    RUN_TEST(test_parse_string_unterminated);
    RUN_TEST(test_parse_string_not_string);
    RUN_TEST(test_parse_string_utf16_surrogate);
    RUN_TEST(test_parse_string_trailing_backslash);
    RUN_TEST(test_parse_string_multiple_escapes);
    RUN_TEST(test_parse_string_null_char);
    RUN_TEST(test_parse_string_whitespace_before_quote);
    RUN_TEST(test_parse_string_unicode_escape);
    RUN_TEST(test_parse_string_escaped_slash);
    RUN_TEST(test_parse_string_escaped_backslash);
    RUN_TEST(test_parse_string_only_escape);
    RUN_TEST(test_parse_string_consecutive_escapes);
    RUN_TEST(test_parse_string_unicode_quote);
    RUN_TEST(test_parse_string_unicode_backslash);
    RUN_TEST(test_parse_string_unicode_newline);
    RUN_TEST(test_parse_string_unicode_tab);
    RUN_TEST(test_parse_string_unicode_cr);
    RUN_TEST(test_parse_string_unicode_backspace);
    RUN_TEST(test_parse_string_unicode_formfeed);
    RUN_TEST(test_parse_string_unicode_slash);
    RUN_TEST(test_parse_string_unicode_backslash2);
    RUN_TEST(test_parse_string_unicode_quote2);
    RUN_TEST(test_parse_string_unicode_backslash3);
    RUN_TEST(test_parse_string_unicode_quote3);
    RUN_TEST(test_parse_string_unicode_backslash4);
    RUN_TEST(test_parse_string_unicode_quote4);
    RUN_TEST(test_parse_string_unicode_backslash5);
    RUN_TEST(test_parse_string_unicode_quote5);
    RUN_TEST(test_parse_string_unicode_backslash6);
    RUN_TEST(test_parse_string_unicode_quote6);
    RUN_TEST(test_parse_string_unicode_backslash7);
    RUN_TEST(test_parse_string_unicode_quote7);
    RUN_TEST(test_parse_string_unicode_backslash8);
    RUN_TEST(test_parse_string_unicode_quote8);
    RUN_TEST(test_parse_string_unicode_backslash9);
    RUN_TEST(test_parse_string_unicode_quote9);
    RUN_TEST(test_parse_string_unicode_backslash10);
    RUN_TEST(test_parse_string_unicode_quote10);
    RUN_TEST(test_parse_string_unicode_backslash11);
    RUN_TEST(test_parse_string_unicode_quote11);
    RUN_TEST(test_parse_string_unicode_backslash12);
    RUN_TEST(test_parse_string_unicode_quote12);
    RUN_TEST(test_parse_string_unicode_backslash13);
    RUN_TEST(test_parse_string_unicode_quote13);
    RUN_TEST(test_parse_string_unicode_backslash14);
    RUN_TEST(test_parse_string_unicode_quote14);
    RUN_TEST(test_parse_string_unicode_backslash15);
    RUN_TEST(test_parse_string_unicode_quote15);
    RUN_TEST(test_parse_string_unicode_backslash16);
    RUN_TEST(test_parse_string_unicode_quote16);
    RUN_TEST(test_parse_string_unicode_backslash17);
    RUN_TEST(test_parse_string_unicode_quote17);
    RUN_TEST(test_parse_string_unicode_backslash18);
    RUN_TEST(test_parse_string_unicode_quote18);
    RUN_TEST(test_parse_string_unicode_backslash19);
    RUN_TEST(test_parse_string_unicode_quote19);
    RUN_TEST(test_parse_string_unicode_backslash20);
    RUN_TEST(test_parse_string_unicode_quote20);
    RUN_TEST(test_parse_string_unicode_backslash21);
    RUN_TEST(test_parse_string_unicode_quote21);
    RUN_TEST(test_parse_string_unicode_backslash22);
    RUN_TEST(test_parse_string_unicode_quote22);
    RUN_TEST(test_parse_string_unicode_backslash23);
    RUN_TEST(test_parse_string_unicode_quote23);
    RUN_TEST(test_parse_string_unicode_backslash24);
    RUN_TEST(test_parse_string_unicode_quote24);
    RUN_TEST(test_parse_string_unicode_backslash25);
    RUN_TEST(test_parse_string_unicode_quote25);
    RUN_TEST(test_parse_string_unicode_backslash26);
    RUN_TEST(test_parse_string_unicode_quote26);
    RUN_TEST(test_parse_string_unicode_backslash27);
    RUN_TEST(test_parse_string_unicode_quote27);
    RUN_TEST(test_parse_string_unicode_backslash28);
    RUN_TEST(test_parse_string_unicode_quote28);
    RUN_TEST(test_parse_string_unicode_backslash29);
    RUN_TEST(test_parse_string_unicode_quote29);
    RUN_TEST(test_parse_string_unicode_backslash30);
    RUN_TEST(test_parse_string_unicode_quote30);
    RUN_TEST(test_parse_string_unicode_backslash31);
    RUN_TEST(test_parse_string_unicode_quote31);
    RUN_TEST(test_parse_string_unicode_backslash32);
    RUN_TEST(test_parse_string_unicode_quote32);
    RUN_TEST(test_parse_string_unicode_backslash33);
    RUN_TEST(test_parse_string_unicode_quote33);
    RUN_TEST(test_parse_string_unicode_backslash34);
    RUN_TEST(test_parse_string_unicode_quote34);
    RUN_TEST(test_parse_string_unicode_backslash35);
    RUN_TEST(test_parse_string_unicode_quote35);
    RUN_TEST(test_parse_string_unicode_backslash36);
    RUN_TEST(test_parse_string_unicode_quote36);
    RUN_TEST(test_parse_string_unicode_backslash37);
    RUN_TEST(test_parse_string_unicode_quote37);
    RUN_TEST(test_parse_string_unicode_backslash38);
    RUN_TEST(test_parse_string_unicode_quote38);
    RUN_TEST(test_parse_string_unicode_backslash39);
    RUN_TEST(test_parse_string_unicode_quote39);
    
    return UNITY_END();
}