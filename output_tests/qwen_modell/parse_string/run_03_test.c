#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "unity.h"
#include "cJSON.h"

/* Internal types and functions from cJSON.c needed for testing parse_string */
typedef struct parse_buffer
{
    const unsigned char *content;
    size_t length;
    size_t offset;
    cJSON_Hooks hooks;
} parse_buffer;

static const unsigned char* buffer_at_offset(const parse_buffer * const buffer)
{
    return buffer->content + buffer->offset;
}

/* External declaration of parse_string (we'll declare it extern for testing) */
extern cJSON_bool parse_string(cJSON * const item, parse_buffer * const input_buffer);

/* Helper function to convert UTF-16 surrogate pair to UTF-8 (internal to cJSON) */
static size_t utf16_literal_to_utf8(const unsigned char *input_pointer, const unsigned char *input_end, unsigned char **output_pointer)
{
    /* This is a minimal stub for testing purposes. In real cJSON, this is fully implemented. */
    /* For unit tests, we'll assume no UTF-16 escapes are present unless explicitly handled */
    /* Since parse_string calls this, we must provide a working implementation */
    /* But per requirements, no mocks/stubs/fakes — so we must rely on cJSON's internal implementation */
    /* However, since we're only testing parse_string, and utf16_literal_to_utf8 is static in cJSON.c, */
    /* we need to declare it extern here for linking. */
    /* In practice, cJSON.c must be compiled and linked with this test file. */
    /* We'll just forward-declare it as extern */
    extern size_t utf16_literal_to_utf8(const unsigned char *input_pointer, const unsigned char *input_end, unsigned char **output_pointer);
    return utf16_literal_to_utf8(input_pointer, input_end, output_pointer);
}

/* Global fixture variables */
static cJSON_Hooks test_hooks = { malloc, free };
static cJSON *test_item = NULL;
static unsigned char *test_buffer = NULL;
static parse_buffer test_buffer_struct = {0};

void setUp(void)
{
    cJSON_InitHooks(&test_hooks);
    test_item = cJSON_CreateNull();
    test_buffer = NULL;
    memset(&test_buffer_struct, 0, sizeof(test_buffer_struct));
}

void tearDown(void)
{
    if (test_item != NULL)
    {
        cJSON_Delete(test_item);
        test_item = NULL;
    }
    if (test_buffer != NULL)
    {
        free(test_buffer);
        test_buffer = NULL;
    }
    /* Reset buffer struct */
    test_buffer_struct.content = NULL;
    test_buffer_struct.length = 0;
    test_buffer_struct.offset = 0;
}

/* Helper to prepare parse_buffer with a given string (including quotes) */
static void setup_parse_buffer(const char *input)
{
    size_t len = strlen(input);
    test_buffer = (unsigned char *)malloc(len + 1);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_buffer, "Failed to allocate test buffer");
    memcpy(test_buffer, input, len + 1); /* include null terminator for safety */
    test_buffer_struct.content = test_buffer;
    test_buffer_struct.length = len;
    test_buffer_struct.offset = 0;
    test_buffer_struct.hooks = test_hooks;
}

/* Test 1: Valid simple string */
void test_parse_string_simple(void)
{
    setup_parse_buffer("\"hello\"");
    TEST_ASSERT_TRUE(parse_string(test_item, &test_buffer_struct));
    TEST_ASSERT_EQUAL_INT(cJSON_String, test_item->type);
    TEST_ASSERT_EQUAL_STRING("hello", test_item->valuestring);
    TEST_ASSERT_EQUAL_UINT(7, test_buffer_struct.offset); /* 6 chars + 1 for closing quote */
}

/* Test 2: String with escape sequences */
void test_parse_string_escapes(void)
{
    setup_parse_buffer("\"line\\nbreak\\twith\\\"quotes\\\\backslash\"");
    TEST_ASSERT_TRUE(parse_string(test_item, &test_buffer_struct));
    TEST_ASSERT_EQUAL_INT(cJSON_String, test_item->type);
    TEST_ASSERT_EQUAL_STRING("line\nbreak\twith\"quotes\\backslash", test_item->valuestring);
}

/* Test 3: Empty string */
void test_parse_string_empty(void)
{
    setup_parse_buffer("\"\"");
    TEST_ASSERT_TRUE(parse_string(test_item, &test_buffer_struct));
    TEST_ASSERT_EQUAL_INT(cJSON_String, test_item->type);
    TEST_ASSERT_EQUAL_STRING("", test_item->valuestring);
    TEST_ASSERT_EQUAL_UINT(2, test_buffer_struct.offset);
}

/* Test 4: Invalid string (missing closing quote) */
void test_parse_string_unterminated(void)
{
    setup_parse_buffer("\"unclosed");
    TEST_ASSERT_FALSE(parse_string(test_item, &test_buffer_struct));
    TEST_ASSERT_EQUAL_INT(cJSON_Invalid, test_item->type);
    TEST_ASSERT_NULL(test_item->valuestring);
}

/* Test 5: Invalid string (not starting with quote) */
void test_parse_string_not_a_string(void)
{
    setup_parse_buffer("not_a_string");
    TEST_ASSERT_FALSE(parse_string(test_item, &test_buffer_struct));
    TEST_ASSERT_EQUAL_INT(cJSON_Invalid, test_item->type);
    TEST_ASSERT_NULL(test_item->valuestring);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_parse_string_simple);
    RUN_TEST(test_parse_string_escapes);
    RUN_TEST(test_parse_string_empty);
    RUN_TEST(test_parse_string_unterminated);
    RUN_TEST(test_parse_string_not_a_string);

    return UNITY_END();
}