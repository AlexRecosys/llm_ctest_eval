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

/* External declaration of parse_string (we'll declare it as extern for testing) */
extern cJSON_bool parse_string(cJSON * const item, parse_buffer * const input_buffer);

/* Helper function to convert UTF-16 surrogate pair to UTF-8 (used by parse_string) */
static size_t utf16_literal_to_utf8(const unsigned char * const input_pointer, const unsigned char * const input_end, unsigned char **output_pointer)
{
    unsigned int codepoint = 0;
    unsigned int surrogate_low = 0;
    size_t i = 0;
    size_t length = 0;

    if ((input_end - input_pointer) < 6)
    {
        return 0;
    }

    /* Parse first 4 hex digits */
    for (i = 2; i < 6; i++)
    {
        codepoint <<= 4;
        if (input_pointer[i] >= '0' && input_pointer[i] <= '9')
        {
            codepoint |= input_pointer[i] - '0';
        }
        else if (input_pointer[i] >= 'a' && input_pointer[i] <= 'f')
        {
            codepoint |= input_pointer[i] - 'a' + 10;
        }
        else if (input_pointer[i] >= 'A' && input_pointer[i] <= 'F')
        {
            codepoint |= input_pointer[i] - 'A' + 10;
        }
        else
        {
            return 0;
        }
    }

    /* Check for surrogate pair */
    if (codepoint >= 0xD800 && codepoint <= 0xDBFF)
    {
        /* High surrogate - must have low surrogate following */
        if ((input_end - input_pointer) < 12 ||
            input_pointer[6] != '\\' || input_pointer[7] != 'u')
        {
            return 0;
        }

        /* Parse low surrogate */
        for (i = 8; i < 12; i++)
        {
            surrogate_low <<= 4;
            if (input_pointer[i] >= '0' && input_pointer[i] <= '9')
            {
                surrogate_low |= input_pointer[i] - '0';
            }
            else if (input_pointer[i] >= 'a' && input_pointer[i] <= 'f')
            {
                surrogate_low |= input_pointer[i] - 'a' + 10;
            }
            else if (input_pointer[i] >= 'A' && input_pointer[i] <= 'F')
            {
                surrogate_low |= input_pointer[i] - 'A' + 10;
            }
            else
            {
                return 0;
            }
        }

        if (surrogate_low < 0xDC00 || surrogate_low > 0xDFFF)
        {
            return 0;
        }

        codepoint = 0x10000 + ((codepoint - 0xD800) << 10) + (surrogate_low - 0xDC00);
    }

    /* Encode UTF-8 */
    if (codepoint < 0x80)
    {
        (*output_pointer)[0] = (unsigned char)codepoint;
        length = 1;
    }
    else if (codepoint < 0x800)
    {
        (*output_pointer)[0] = 0xC0 | ((codepoint >> 6) & 0x1F);
        (*output_pointer)[1] = 0x80 | (codepoint & 0x3F);
        length = 2;
    }
    else if (codepoint < 0x10000)
    {
        (*output_pointer)[0] = 0xE0 | ((codepoint >> 12) & 0x0F);
        (*output_pointer)[1] = 0x80 | ((codepoint >> 6) & 0x3F);
        (*output_pointer)[2] = 0x80 | (codepoint & 0x3F);
        length = 3;
    }
    else
    {
        (*output_pointer)[0] = 0xF0 | ((codepoint >> 18) & 0x07);
        (*output_pointer)[1] = 0x80 | ((codepoint >> 12) & 0x3F);
        (*output_pointer)[2] = 0x80 | ((codepoint >> 6) & 0x3F);
        (*output_pointer)[3] = 0x80 | (codepoint & 0x3F);
        length = 4;
    }

    *output_pointer += length;
    return length + 5; /* 6 chars for \uXXXX, or 12 for \uXXXX\uXXXX */
}

/* Global test fixtures */
static cJSON_Hooks test_hooks = { malloc, free };
static cJSON *test_item = NULL;
static parse_buffer test_buffer = { NULL, 0, 0, { NULL, NULL } };
static unsigned char *test_buffer_content = NULL;

void setUp(void)
{
    test_item = cJSON_CreateNull();
    test_hooks.malloc_fn = malloc;
    test_hooks.free_fn = free;
    test_buffer_content = NULL;
}

void tearDown(void)
{
    if (test_item != NULL)
    {
        cJSON_Delete(test_item);
        test_item = NULL;
    }
    if (test_buffer_content != NULL)
    {
        free(test_buffer_content);
        test_buffer_content = NULL;
    }
}

/* Helper function to prepare parse_buffer with a string */
static void setup_parse_buffer(const char *input)
{
    size_t len = strlen(input);
    test_buffer_content = (unsigned char *)malloc(len + 1);
    TEST_ASSERT_NOT_NULL_MESSAGE(test_buffer_content, "Failed to allocate buffer");
    memcpy(test_buffer_content, input, len + 1);
    test_buffer.content = test_buffer_content;
    test_buffer.length = len;
    test_buffer.offset = 0;
    test_buffer.hooks = test_hooks;
}

/* Test 1: Parse a simple valid string */
static void test_parse_string_simple(void)
{
    setup_parse_buffer("\"hello\"");
    TEST_ASSERT_TRUE_MESSAGE(parse_string(test_item, &test_buffer), "Failed to parse simple string");
    TEST_ASSERT_EQUAL_INT_MESSAGE(cJSON_String, test_item->type, "Incorrect type after parsing");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("hello", test_item->valuestring, "Incorrect string value");
}

/* Test 2: Parse string with escape sequences */
static void test_parse_string_escapes(void)
{
    setup_parse_buffer("\"line1\\nline2\\t\\\"quoted\\\"\"");
    TEST_ASSERT_TRUE_MESSAGE(parse_string(test_item, &test_buffer), "Failed to parse string with escapes");
    TEST_ASSERT_EQUAL_INT_MESSAGE(cJSON_String, test_item->type, "Incorrect type after parsing");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("line1\nline2\t\"quoted\"", test_item->valuestring, "Incorrect string value with escapes");
}

/* Test 3: Parse string with invalid escape sequence */
static void test_parse_string_invalid_escape(void)
{
    setup_parse_buffer("\"invalid\\x\"");
    TEST_ASSERT_FALSE_MESSAGE(parse_string(test_item, &test_buffer), "Should fail on invalid escape sequence");
}

/* Test 4: Parse string with incomplete UTF-16 surrogate pair */
static void test_parse_string_invalid_utf16(void)
{
    setup_parse_buffer("\"\\uD800\""); /* High surrogate without low surrogate */
    TEST_ASSERT_FALSE_MESSAGE(parse_string(test_item, &test_buffer), "Should fail on incomplete UTF-16 surrogate");
}

/* Test 5: Parse string with valid UTF-16 surrogate pair (emoji) */
static void test_parse_string_valid_utf16(void)
{
    /* U+1F600 (grinning face emoji) encoded as surrogate pair: \uD83D\uDE00 */
    setup_parse_buffer("\"\\uD83D\\uDE00\"");
    TEST_ASSERT_TRUE_MESSAGE(parse_string(test_item, &test_buffer), "Failed to parse UTF-16 surrogate pair");
    TEST_ASSERT_EQUAL_INT_MESSAGE(cJSON_String, test_item->type, "Incorrect type after parsing");
    /* Verify the UTF-8 output: 0xF0 0x9F 0x98 0x80 */
    TEST_ASSERT_EQUAL_MEMORY_MESSAGE("\xF0\x9F\x98\x80", test_item->valuestring, 4, "Incorrect UTF-8 encoding");
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_parse_string_simple);
    RUN_TEST(test_parse_string_escapes);
    RUN_TEST(test_parse_string_invalid_escape);
    RUN_TEST(test_parse_string_invalid_utf16);
    RUN_TEST(test_parse_string_valid_utf16);

    return UNITY_END();
}