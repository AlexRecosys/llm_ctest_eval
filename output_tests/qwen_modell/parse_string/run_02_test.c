#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "unity.h"
#include "cJSON.h"

/* Internal types and functions from cJSON.c (declared extern for testing) */
typedef struct parse_buffer
{
    const unsigned char *content;
    size_t length;
    size_t offset;
    cJSON_Hooks hooks;
} parse_buffer;

static const unsigned char* buffer_at_offset(const parse_buffer * const buffer)
{
    if (buffer == NULL || buffer->content == NULL)
    {
        return NULL;
    }
    return buffer->content + buffer->offset;
}

/* External declaration of internal function under test */
extern cJSON_bool parse_string(cJSON * const item, parse_buffer * const input_buffer);

/* Helper: convert UTF-16 escape sequence to UTF-8 */
static size_t utf16_literal_to_utf8(const unsigned char *input_pointer, const unsigned char *input_end, unsigned char **output_pointer)
{
    /* Minimal implementation for test coverage */
    unsigned int codepoint = 0;
    unsigned int high_surrogate = 0;
    size_t i = 0;

    if ((input_end - input_pointer) < 6) /* \uXXXX */
    {
        return 0;
    }

    for (i = 2; i < 6; i++)
    {
        unsigned char c = input_pointer[i];
        codepoint <<= 4;
        if (c >= '0' && c <= '9')
        {
            codepoint |= c - '0';
        }
        else if (c >= 'a' && c <= 'f')
        {
            codepoint |= c - 'a' + 10;
        }
        else if (c >= 'A' && c <= 'F')
        {
            codepoint |= c - 'A' + 10;
        }
        else
        {
            return 0;
        }
    }

    /* Handle surrogate pairs */
    if (codepoint >= 0xD800 && codepoint <= 0xDBFF)
    {
        /* High surrogate: expect \uXXXX\uXXXX */
        if ((input_end - input_pointer) < 12)
        {
            return 0;
        }
        if (input_pointer[6] != '\\' || input_pointer[7] != 'u')
        {
            return 0;
        }
        high_surrogate = codepoint;
        codepoint = 0;

        for (i = 8; i < 12; i++)
        {
            unsigned char c = input_pointer[i];
            codepoint <<= 4;
            if (c >= '0' && c <= '9')
            {
                codepoint |= c - '0';
            }
            else if (c >= 'a' && c <= 'f')
            {
                codepoint |= c - 'a' + 10;
            }
            else if (c >= 'A' && c <= 'F')
            {
                codepoint |= c - 'A' + 10;
            }
            else
            {
                return 0;
            }
        }

        if (codepoint < 0xDC00 || codepoint > 0xDFFF)
        {
            return 0;
        }

        codepoint = 0x10000 + ((high_surrogate - 0xD800) << 10) + (codepoint - 0xDC00);
    }

    /* Encode UTF-8 */
    if (codepoint < 0x80)
    {
        (*output_pointer)[0] = (unsigned char)codepoint;
        *output_pointer += 1;
        return 6;
    }
    else if (codepoint < 0x800)
    {
        (*output_pointer)[0] = 0xC0 | (codepoint >> 6);
        (*output_pointer)[1] = 0x80 | (codepoint & 0x3F);
        *output_pointer += 2;
        return 6;
    }
    else if (codepoint < 0x10000)
    {
        (*output_pointer)[0] = 0xE0 | (codepoint >> 12);
        (*output_pointer)[1] = 0x80 | ((codepoint >> 6) & 0x3F);
        (*output_pointer)[2] = 0x80 | (codepoint & 0x3F);
        *output_pointer += 3;
        return 6;
    }
    else
    {
        (*output_pointer)[0] = 0xF0 | (codepoint >> 18);
        (*output_pointer)[1] = 0x80 | ((codepoint >> 12) & 0x3F);
        (*output_pointer)[2] = 0x80 | ((codepoint >> 6) & 0x3F);
        (*output_pointer)[3] = 0x80 | (codepoint & 0x3F);
        *output_pointer += 4;
        return 12;
    }
}

/* Global test fixtures */
static cJSON_Hooks test_hooks = {
    .malloc_fn = malloc,
    .free_fn = free
};

static parse_buffer *test_buffer = NULL;
static cJSON test_item;

/* setUp / tearDown */
void setUp(void)
{
    test_buffer = (parse_buffer*)malloc(sizeof(parse_buffer));
    TEST_ASSERT_NOT_NULL(test_buffer);
    test_buffer->content = NULL;
    test_buffer->length = 0;
    test_buffer->offset = 0;
    test_buffer->hooks = test_hooks;

    memset(&test_item, 0, sizeof(test_item));
}

void tearDown(void)
{
    if (test_buffer != NULL)
    {
        if (test_buffer->content != NULL)
        {
            free((void*)test_buffer->content);
        }
        free(test_buffer);
        test_buffer = NULL;
    }
    /* Clear test_item to avoid dangling pointers */
    test_item.type = cJSON_Invalid;
    test_item.valuestring = NULL;
}

/* Helper to prepare buffer with a string */
static void prepare_buffer(const char *input)
{
    size_t len = strlen(input);
    test_buffer->content = (const unsigned char*)malloc(len + 1);
    TEST_ASSERT_NOT_NULL(test_buffer->content);
    memcpy((void*)test_buffer->content, input, len + 1);
    test_buffer->length = len;
    test_buffer->offset = 0;
}

/* Test Cases */

void test_parse_string_simple(void)
{
    prepare_buffer("\"hello\"");
    TEST_ASSERT_TRUE(parse_string(&test_item, test_buffer));
    TEST_ASSERT_EQUAL_INT(cJSON_String, test_item.type);
    TEST_ASSERT_EQUAL_STRING("hello", test_item.valuestring);
    TEST_ASSERT_EQUAL_UINT(7, test_buffer->offset);
}

void test_parse_string_with_escapes(void)
{
    prepare_buffer("\"line\\nbreak\\twith\\\"quotes\\\"\"");
    TEST_ASSERT_TRUE(parse_string(&test_item, test_buffer));
    TEST_ASSERT_EQUAL_INT(cJSON_String, test_item.type);
    TEST_ASSERT_EQUAL_STRING("line\nbreak\twith\"quotes\"", test_item.valuestring);
    TEST_ASSERT_EQUAL_UINT(30, test_buffer->offset);
}

void test_parse_string_empty(void)
{
    prepare_buffer("\"\"");
    TEST_ASSERT_TRUE(parse_string(&test_item, test_buffer));
    TEST_ASSERT_EQUAL_INT(cJSON_String, test_item.type);
    TEST_ASSERT_EQUAL_STRING("", test_item.valuestring);
    TEST_ASSERT_EQUAL_UINT(2, test_buffer->offset);
}

void test_parse_string_unterminated(void)
{
    prepare_buffer("\"unterminated");
    TEST_ASSERT_FALSE(parse_string(&test_item, test_buffer));
    TEST_ASSERT_NULL(test_item.valuestring);
    TEST_ASSERT_EQUAL_INT(cJSON_Invalid, test_item.type);
}

void test_parse_string_invalid_escape(void)
{
    prepare_buffer("\"invalid\\x\"");
    TEST_ASSERT_FALSE(parse_string(&test_item, test_buffer));
    TEST_ASSERT_NULL(test_item.valuestring);
    TEST_ASSERT_EQUAL_INT(cJSON_Invalid, test_item.type);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_parse_string_simple);
    RUN_TEST(test_parse_string_with_escapes);
    RUN_TEST(test_parse_string_empty);
    RUN_TEST(test_parse_string_unterminated);
    RUN_TEST(test_parse_string_invalid_escape);

    return UNITY_END();
}