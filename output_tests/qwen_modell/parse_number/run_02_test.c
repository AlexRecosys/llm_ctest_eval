#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <float.h>
#include "unity.h"
#include "cJSON.h"

/* Include the internal header where parse_number is declared */
#include "cJSON.c"

/* Global fixtures */
static cJSON_Hooks default_hooks = { malloc, free };
static cJSON *test_item = NULL;
static parse_buffer test_buffer = { 0 };
static unsigned char test_content[1024] = { 0 };

void setUp(void)
{
    /* Initialize cJSON hooks */
    cJSON_InitHooks(&default_hooks);

    /* Allocate and initialize test item */
    test_item = cJSON_CreateNull();
    TEST_ASSERT_NOT_NULL(test_item);

    /* Initialize parse buffer */
    test_buffer.content = test_content;
    test_buffer.length = sizeof(test_content);
    test_buffer.offset = 0;
    test_buffer.hooks = default_hooks;
}

void tearDown(void)
{
    if (test_item != NULL)
    {
        cJSON_Delete(test_item);
        test_item = NULL;
    }
}

/* Helper to set up buffer with a number string */
static void setup_buffer_with_number(const char *number_str)
{
    size_t len = strlen(number_str);
    TEST_ASSERT_TRUE(len < sizeof(test_content));
    memset(test_content, 0, sizeof(test_content));
    memcpy(test_content, number_str, len);
    test_buffer.offset = 0;
    test_buffer.length = len;
}

/* Test 1: Parse a simple integer */
static void test_parse_number_should_parse_simple_integer(void)
{
    setup_buffer_with_number("42");
    TEST_ASSERT_TRUE(parse_number(test_item, &test_buffer));
    TEST_ASSERT_EQUAL_INT(42, test_item->valueint);
    TEST_ASSERT_EQUAL_DOUBLE(42.0, test_item->valuedouble);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item->type);
    TEST_ASSERT_EQUAL_UINT(2, test_buffer.offset);
}

/* Test 2: Parse a negative number with decimal point */
static void test_parse_number_should_parse_negative_decimal(void)
{
    setup_buffer_with_number("-3.14159");
    TEST_ASSERT_TRUE(parse_number(test_item, &test_buffer));
    TEST_ASSERT_EQUAL_DOUBLE(-3.14159, test_item->valuedouble, DBL_EPSILON);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item->type);
    TEST_ASSERT_EQUAL_UINT(8, test_buffer.offset);
}

/* Test 3: Parse scientific notation */
static void test_parse_number_should_parse_scientific_notation(void)
{
    setup_buffer_with_number("1.23e10");
    TEST_ASSERT_TRUE(parse_number(test_item, &test_buffer));
    TEST_ASSERT_EQUAL_DOUBLE(1.23e10, test_item->valuedouble, 1e5); /* relaxed tolerance for large numbers */
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item->type);
    TEST_ASSERT_EQUAL_UINT(7, test_buffer.offset);
}

/* Test 4: Parse number with overflow (should saturate to INT_MAX) */
static void test_parse_number_should_saturate_on_overflow(void)
{
    setup_buffer_with_number("999999999999999999999.0");
    TEST_ASSERT_TRUE(parse_number(test_item, &test_buffer));
    TEST_ASSERT_EQUAL_INT(INT_MAX, test_item->valueint);
    TEST_ASSERT_EQUAL_DOUBLE(999999999999999999999.0, test_item->valuedouble);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item->type);
}

/* Test 5: Parse invalid input (no digits) */
static void test_parse_number_should_fail_on_invalid_input(void)
{
    setup_buffer_with_number("abc");
    TEST_ASSERT_FALSE(parse_number(test_item, &test_buffer));
    TEST_ASSERT_EQUAL_INT(cJSON_NULL, test_item->type); /* unchanged */
    TEST_ASSERT_EQUAL_UINT(0, test_buffer.offset);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_parse_number_should_parse_simple_integer);
    RUN_TEST(test_parse_number_should_parse_negative_decimal);
    RUN_TEST(test_parse_number_should_parse_scientific_notation);
    RUN_TEST(test_parse_number_should_saturate_on_overflow);
    RUN_TEST(test_parse_number_should_fail_on_invalid_input);

    return UNITY_END();
}