#include "cJSON.c"
#include "unity.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Helper to create a parse_buffer from a string */
static parse_buffer make_parse_buffer(const char *input) {
    parse_buffer buffer;
    size_t len = strlen(input);
    unsigned char *content = (unsigned char *)malloc(len + 1);
    memcpy(content, input, len + 1);
    buffer.content = content;
    buffer.length = len;
    buffer.offset = 0;
    buffer.hooks.malloc_fn = malloc;
    buffer.hooks.free_fn = free;
    return buffer;
}

/* Helper to free parse_buffer content */
static void free_parse_buffer(parse_buffer *buffer) {
    if (buffer && buffer->content) {
        buffer->hooks.free_fn(buffer->content);
        buffer->content = NULL;
    }
}

/* Helper to create a cJSON item */
static cJSON *make_cJSON_item(void) {
    cJSON *item = (cJSON *)malloc(sizeof(cJSON));
    if (item) {
        memset(item, 0, sizeof(cJSON));
        item->type = cJSON_Invalid;
        item->valuestring = NULL;
    }
    return item;
}

/* Helper to free cJSON item (without using cJSON_Delete to avoid side effects) */
static void free_cJSON_item(cJSON *item) {
    if (item) {
        if (item->valuestring) {
            free(item->valuestring);
        }
        free(item);
    }
}

/* Helper to compare expected string with item->valuestring */
static void assert_string_equal_message(const cJSON *item, const char *expected, const char *msg) {
    if (item == NULL || item->valuestring == NULL) {
        TEST_FAIL_MESSAGE(msg ? msg : "Item or valuestring is NULL");
    }
    TEST_ASSERT_EQUAL_STRING_MESSAGE(expected, item->valuestring, msg);
}

/* Test: parse_string should fail if input does not start with '"' */
void test_parse_string_not_a_string(void) {
    parse_buffer buffer = make_parse_buffer("abc");
    cJSON *item = make_cJSON_item();
    TEST_ASSERT_NOT_NULL(item);

    cJSON_bool result = parse_string(item, &buffer);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL_INT(cJSON_Invalid, item->type);
    TEST_ASSERT_NULL(item->valuestring);

    free_cJSON_item(item);
    free_parse_buffer(&buffer);
}

/* Test: parse_string should fail if string is unterminated */
void test_parse_string_unterminated(void) {
    parse_buffer buffer = make_parse_buffer("\"hello");
    cJSON *item = make_cJSON_item();
    TEST_ASSERT_NOT_NULL(item);

    cJSON_bool result = parse_string(item, &buffer);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL_INT(cJSON_Invalid, item->type);
    TEST_ASSERT_NULL(item->valuestring);

    free_cJSON_item(item);
    free_parse_buffer(&buffer);
}

/* Test: parse_string should parse a simple string */
void test_parse_string_simple(void) {
    parse_buffer buffer = make_parse_buffer("\"hello\"");
    cJSON *item = make_cJSON_item();
    TEST_ASSERT_NOT_NULL(item);

    cJSON_bool result = parse_string(item, &buffer);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(cJSON_String, item->type);
    assert_string_equal_message(item, "hello", "Simple string mismatch");

    free_cJSON_item(item);
    free_parse_buffer(&buffer);
}

/* Test: parse_string should handle empty string */
void test_parse_string_empty(void) {
    parse_buffer buffer = make_parse_buffer("\"\"");
    cJSON *item = make_cJSON_item();
    TEST_ASSERT_NOT_NULL(item);

    cJSON_bool result = parse_string(item, &buffer);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(cJSON_String, item->type);
    TEST_ASSERT_EQUAL_STRING("", item->valuestring);

    free_cJSON_item(item);
    free_parse_buffer(&buffer);
}

/* Test: parse_string should handle escape sequences */
void test_parse_string_escapes(void) {
    parse_buffer buffer = make_parse_buffer("\"\\b\\f\\n\\r\\t\\\\\\\"\\/");
    cJSON *item = make_cJSON_item();
    TEST_ASSERT_NOT_NULL(item);

    cJSON_bool result = parse_string(item, &buffer);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(cJSON_String, item->type);
    /* Expected: \b \f \n \r \t \ \ " / */
    TEST_ASSERT_EQUAL_MEMORY("\b\f\n\r\t\\\"/", item->valuestring, 8);
    TEST_ASSERT_EQUAL_INT('\0', item->valuestring[8]);

    free_cJSON_item(item);
    free_parse_buffer(&buffer);
}

/* Test: parse_string should fail on invalid escape sequence */
void test_parse_string_invalid_escape(void) {
    parse_buffer buffer = make_parse_buffer("\"\\x\"");
    cJSON *item = make_cJSON_item();
    TEST_ASSERT_NOT_NULL(item);

    cJSON_bool result = parse_string(item, &buffer);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL_INT(cJSON_Invalid, item->type);
    TEST_ASSERT_NULL(item->valuestring);

    free_cJSON_item(item);
    free_parse_buffer(&buffer);
}

/* Test: parse_string should handle trailing backslash (fail) */
void test_parse_string_trailing_backslash(void) {
    parse_buffer buffer = make_parse_buffer("\"test\\");
    cJSON *item = make_cJSON_item();
    TEST_ASSERT_NOT_NULL(item);

    cJSON_bool result = parse_string(item, &buffer);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL_INT(cJSON_Invalid, item->type);
    TEST_ASSERT_NULL(item->valuestring);

    free_cJSON_item(item);
    free_parse_buffer(&buffer);
}

/* Test: parse_string should handle UTF-16 surrogate pairs */
void test_parse_string_utf16_surrogate_pair(void) {
    /* U+1F600 (grinning face emoji) encoded as surrogate pair: \uD83D\uDE00 */
    parse_buffer buffer = make_parse_buffer("\"\\uD83D\\uDE00\"");
    cJSON *item = make_cJSON_item();
    TEST_ASSERT_NOT_NULL(item);

    cJSON_bool result = parse_string(item, &buffer);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(cJSON_String, item->type);
    /* UTF-8 encoding of U+1F600 is 0xF0 0x9F 0x98 0x80 */
    TEST_ASSERT_EQUAL_MEMORY("\xF0\x9F\x98\x80", item->valuestring, 4);
    TEST_ASSERT_EQUAL_INT('\0', item->valuestring[4]);

    free_cJSON_item(item);
    free_parse_buffer(&buffer);
}

/* Test: parse_string should fail on incomplete UTF-16 surrogate */
void test_parse_string_incomplete_utf16(void) {
    parse_buffer buffer = make_parse_buffer("\"\\uD83D\"");
    cJSON *item = make_cJSON_item();
    TEST_ASSERT_NOT_NULL(item);

    cJSON_bool result = parse_string(item, &buffer);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL_INT(cJSON_Invalid, item->type);
    TEST_ASSERT_NULL(item->valuestring);

    free_cJSON_item(item);
    free_parse_buffer(&buffer);
}

/* Test: parse_string should handle null-terminated buffer correctly */
void test_parse_string_null_terminated_buffer(void) {
    parse_buffer buffer = make_parse_buffer("\"test\"");
    cJSON *item = make_cJSON_item();
    TEST_ASSERT_NOT_NULL(item);

    cJSON_bool result = parse_string(item, &buffer);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(cJSON_String, item->type);
    assert_string_equal_message(item, "test", "Null-terminated buffer mismatch");

    free_cJSON_item(item);
    free_parse_buffer(&buffer);
}

/* Test: parse_string should handle allocation failure gracefully */
void test_parse_string_allocation_failure(void) {
    /* Temporarily replace malloc with a failing one */
    cJSON_Hooks original_hooks = {malloc, free};
    cJSON_Hooks failing_hooks = {NULL, free};
    cJSON_InitHooks(&failing_hooks);

    parse_buffer buffer = make_parse_buffer("\"hello\"");
    cJSON *item = make_cJSON_item();
    TEST_ASSERT_NOT_NULL(item);

    cJSON_bool result = parse_string(item, &buffer);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL_INT(cJSON_Invalid, item->type);
    TEST_ASSERT_NULL(item->valuestring);

    free_cJSON_item(item);
    free_parse_buffer(&buffer);

    /* Restore original hooks */
    cJSON_InitHooks(&original_hooks);
}

/* Test: parse_string should handle escaped slash and quote */
void test_parse_string_escaped_slash_quote(void) {
    parse_buffer buffer = make_parse_buffer("\"a\\/b\\\"c\"");
    cJSON *item = make_cJSON_item();
    TEST_ASSERT_NOT_NULL(item);

    cJSON_bool result = parse_string(item, &buffer);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(cJSON_String, item->type);
    assert_string_equal_message(item, "a/b\"c", "Escaped slash and quote mismatch");

    free_cJSON_item(item);
    free_parse_buffer(&buffer);
}

/* Test: parse_string should handle multiple consecutive escapes */
void test_parse_string_multiple_escapes(void) {
    parse_buffer buffer = make_parse_buffer("\"\\\\\\\"\"");
    cJSON *item = make_cJSON_item();
    TEST_ASSERT_NOT_NULL(item);

    cJSON_bool result = parse_string(item, &buffer);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(cJSON_String, item->type);
    assert_string_equal_message(item, "\\\"", "Multiple escapes mismatch");

    free_cJSON_item(item);
    free_parse_buffer(&buffer);
}

/* Test: parse_string should handle Unicode escape for common chars */
void test_parse_string_unicode_escape(void) {
    parse_buffer buffer = make_parse_buffer("\"\\u0041\"");
    cJSON *item = make_cJSON_item();
    TEST_ASSERT_NOT_NULL(item);

    cJSON_bool result = parse_string(item, &buffer);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(cJSON_String, item->type);
    assert_string_equal_message(item, "A", "Unicode escape mismatch");

    free_cJSON_item(item);
    free_parse_buffer(&buffer);
}

/* Test: parse_string should handle string with no escapes */
void test_parse_string_no_escapes(void) {
    parse_buffer buffer = make_parse_buffer("\"hello world 123\"");
    cJSON *item = make_cJSON_item();
    TEST_ASSERT_NOT_NULL(item);

    cJSON_bool result = parse_string(item, &buffer);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(cJSON_String, item->type);
    assert_string_equal_message(item, "hello world 123", "No escapes mismatch");

    free_cJSON_item(item);
    free_parse_buffer(&buffer);
}

/* Test: parse_string should handle string with only escapes */
void test_parse_string_only_escapes(void) {
    parse_buffer buffer = make_parse_buffer("\"\\n\\t\"");
    cJSON *item = make_cJSON_item();
    TEST_ASSERT_NOT_NULL(item);

    cJSON_bool result = parse_string(item, &buffer);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(cJSON_String, item->type);
    TEST_ASSERT_EQUAL_MEMORY("\n\t", item->valuestring, 2);
    TEST_ASSERT_EQUAL_INT('\0', item->valuestring[2]);

    free_cJSON_item(item);
    free_parse_buffer(&buffer);
}

/* Test: parse_string should handle buffer overflow on trailing backslash */
void test_parse_string_trailing_backslash_at_end(void) {
    parse_buffer buffer = make_parse_buffer("\"test\\");
    cJSON *item = make_cJSON_item();
    TEST_ASSERT_NOT_NULL(item);

    cJSON_bool result = parse_string(item, &buffer);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL_INT(cJSON_Invalid, item->type);
    TEST_ASSERT_NULL(item->valuestring);

    free_cJSON_item(item);
    free_parse_buffer(&buffer);
}

int main(void) {
    unity_install_sighandler();
    UNITY_BEGIN();

    RUN_TEST(test_parse_string_not_a_string);
    RUN_TEST(test_parse_string_unterminated);
    RUN_TEST(test_parse_string_simple);
    RUN_TEST(test_parse_string_empty);
    RUN_TEST(test_parse_string_escapes);
    RUN_TEST(test_parse_string_invalid_escape);
    RUN_TEST(test_parse_string_trailing_backslash);
    RUN_TEST(test_parse_string_utf16_surrogate_pair);
    RUN_TEST(test_parse_string_incomplete_utf16);
    RUN_TEST(test_parse_string_null_terminated_buffer);
    RUN_TEST(test_parse_string_allocation_failure);
    RUN_TEST(test_parse_string_escaped_slash_quote);
    RUN_TEST(test_parse_string_multiple_escapes);
    RUN_TEST(test_parse_string_unicode_escape);
    RUN_TEST(test_parse_string_no_escapes);
    RUN_TEST(test_parse_string_only_escapes);
    RUN_TEST(test_parse_string_trailing_backslash_at_end);

    return UNITY_END();
}