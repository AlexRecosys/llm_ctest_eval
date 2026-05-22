#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <float.h>
#include "unity.h"
#include "cJSON.h"

/* Include the internal header for parse_buffer and parse_number */
#include "cJSON.c"

/* Global fixtures */
static cJSON_Hooks default_hooks = { malloc, free };
static parse_buffer *buffer = NULL;
static cJSON *item = NULL;

void setUp(void)
{
    cJSON_InitHooks(&default_hooks);
    buffer = NULL;
    item = cJSON_CreateNumber(0.0);
    TEST_ASSERT_NOT_NULL(item);
}

void tearDown(void)
{
    if (buffer != NULL)
    {
        if (buffer->content != NULL)
        {
            free(buffer->content);
        }
        free(buffer);
        buffer = NULL;
    }
    if (item != NULL)
    {
        cJSON_Delete(item);
        item = NULL;
    }
}

/* Helper to create a parse_buffer from a string */
static parse_buffer *create_buffer_from_string(const char *str)
{
    parse_buffer *buf = (parse_buffer *)malloc(sizeof(parse_buffer));
    TEST_ASSERT_NOT_NULL(buf);

    size_t len = strlen(str);
    unsigned char *content = (unsigned char *)malloc(len + 1);
    TEST_ASSERT_NOT_NULL(content);
    memcpy(content, str, len);
    content[len] = '\0';

    buf->content = content;
    buf->length = len;
    buf->offset = 0;
    buf->hooks = default_hooks;
    buf->depth = 0;

    return buf;
}

/* Test 1: Parse a simple integer */
static void test_parse_number_should_parse_simple_integer(void)
{
    const char *input = "42";
    buffer = create_buffer_from_string(input);

    cJSON_bool result = parse_number(item, buffer);

    TEST_ASSERT_TRUE_MESSAGE(result, "parse_number should succeed");
    TEST_ASSERT_EQUAL_INT_MESSAGE(42, item->valueint, "valueint should be 42");
    TEST_ASSERT_EQUAL_DOUBLE_MESSAGE(42.0, item->valuedouble, "valuedouble should be 42.0");
    TEST_ASSERT_EQUAL_INT_MESSAGE(cJSON_Number, item->type, "type should be cJSON_Number");
    TEST_ASSERT_EQUAL_SIZE_MESSAGE(strlen(input), buffer->offset, "offset should advance past number");
}

/* Test 2: Parse a negative decimal with scientific notation */
static void test_parse_number_should_parse_negative_decimal_scientific(void)
{
    const char *input = "-1.23e-4";
    buffer = create_buffer_from_string(input);

    cJSON_bool result = parse_number(item, buffer);

    TEST_ASSERT_TRUE_MESSAGE(result, "parse_number should succeed");
    TEST_ASSERT_EQUAL_DOUBLE_MESSAGE(-1.23e-4, item->valuedouble, "valuedouble should be -1.23e-4");
    TEST_ASSERT_EQUAL_INT_MESSAGE(cJSON_Number, item->type, "type should be cJSON_Number");
    TEST_ASSERT_EQUAL_SIZE_MESSAGE(strlen(input), buffer->offset, "offset should advance past number");
}

/* Test 3: Parse a number with overflow (should saturate to INT_MAX) */
static void test_parse_number_should_saturate_overflow(void)
{
    const char *input = "1e308";
    buffer = create_buffer_from_string(input);

    cJSON_bool result = parse_number(item, buffer);

    TEST_ASSERT_TRUE_MESSAGE(result, "parse_number should succeed");
    TEST_ASSERT_EQUAL_INT_MESSAGE(INT_MAX, item->valueint, "valueint should saturate to INT_MAX");
    TEST_ASSERT_EQUAL_DOUBLE_MESSAGE(1e308, item->valuedouble, "valuedouble should be 1e308");
    TEST_ASSERT_EQUAL_INT_MESSAGE(cJSON_Number, item->type, "type should be cJSON_Number");
}

/* Test 4: Parse a number with underflow (should saturate to INT_MIN) */
static void test_parse_number_should_saturate_underflow(void)
{
    const char *input = "-1e308";
    buffer = create_buffer_from_string(input);

    cJSON_bool result = parse_number(item, buffer);

    TEST_ASSERT_TRUE_MESSAGE(result, "parse_number should succeed");
    TEST_ASSERT_EQUAL_INT_MESSAGE(INT_MIN, item->valueint, "valueint should saturate to INT_MIN");
    TEST_ASSERT_EQUAL_DOUBLE_MESSAGE(-1e308, item->valuedouble, "valuedouble should be -1e308");
    TEST_ASSERT_EQUAL_INT_MESSAGE(cJSON_Number, item->type, "type should be cJSON_Number");
}

/* Test 5: Parse invalid input (no digits) */
static void test_parse_number_should_fail_on_invalid_input(void)
{
    const char *input = "abc";
    buffer = create_buffer_from_string(input);

    cJSON_bool result = parse_number(item, buffer);

    TEST_ASSERT_FALSE_MESSAGE(result, "parse_number should fail on invalid input");
    TEST_ASSERT_EQUAL_INT_MESSAGE(cJSON_Number, item->type, "type should remain cJSON_Number (unchanged)");
    TEST_ASSERT_EQUAL_SIZE_MESSAGE(0, buffer->offset, "offset should not advance");
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_parse_number_should_parse_simple_integer);
    RUN_TEST(test_parse_number_should_parse_negative_decimal_scientific);
    RUN_TEST(test_parse_number_should_saturate_overflow);
    RUN_TEST(test_parse_number_should_saturate_underflow);
    RUN_TEST(test_parse_number_should_fail_on_invalid_input);

    return UNITY_END();
}