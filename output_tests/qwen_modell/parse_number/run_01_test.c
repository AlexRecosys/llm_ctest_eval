#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <limits.h>
#include "unity.h"
#include "cJSON.h"

/* Include the internal header to access parse_buffer and parse_number */
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
    size_t len = strlen(str);
    unsigned char *content = (unsigned char *)malloc(len + 1);
    TEST_ASSERT_NOT_NULL(content);
    memcpy(content, str, len + 1); /* includes '\0' */

    parse_buffer *buf = (parse_buffer *)malloc(sizeof(parse_buffer));
    TEST_ASSERT_NOT_NULL(buf);

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
    buffer = create_buffer_from_string("123");
    item = cJSON_CreateNumber(0.0);
    TEST_ASSERT_NOT_NULL(item);

    cJSON_bool result = parse_number(item, buffer);

    TEST_ASSERT_TRUE_MESSAGE(result, "parse_number should succeed");
    TEST_ASSERT_EQUAL_INT(123, item->valueint);
    TEST_ASSERT_EQUAL_DOUBLE(123.0, item->valuedouble);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, item->type);
    TEST_ASSERT_EQUAL_SIZE(3, buffer->offset);
}

/* Test 2: Parse a negative number with decimal point */
static void test_parse_number_should_parse_negative_decimal(void)
{
    buffer = create_buffer_from_string("-45.67e2");
    item = cJSON_CreateNumber(0.0);
    TEST_ASSERT_NOT_NULL(item);

    cJSON_bool result = parse_number(item, buffer);

    TEST_ASSERT_TRUE_MESSAGE(result, "parse_number should succeed");
    TEST_ASSERT_EQUAL_DOUBLE(-4567.0, item->valuedouble);
    TEST_ASSERT_EQUAL_INT(-4567, item->valueint);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, item->type);
    TEST_ASSERT_EQUAL_SIZE(8, buffer->offset);
}

/* Test 3: Parse number with overflow (should saturate to INT_MAX) */
static void test_parse_number_should_saturate_on_overflow(void)
{
    buffer = create_buffer_from_string("1e308");
    item = cJSON_CreateNumber(0.0);
    TEST_ASSERT_NOT_NULL(item);

    cJSON_bool result = parse_number(item, buffer);

    TEST_ASSERT_TRUE_MESSAGE(result, "parse_number should succeed");
    TEST_ASSERT_EQUAL_DOUBLE(1e308, item->valuedouble);
    TEST_ASSERT_EQUAL_INT(INT_MAX, item->valueint);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, item->type);
}

/* Test 4: Parse invalid number (non-numeric start) */
static void test_parse_number_should_fail_on_invalid_start(void)
{
    buffer = create_buffer_from_string("abc123");
    item = cJSON_CreateNumber(0.0);
    TEST_ASSERT_NOT_NULL(item);

    cJSON_bool result = parse_number(item, buffer);

    TEST_ASSERT_FALSE_MESSAGE(result, "parse_number should fail");
    TEST_ASSERT_EQUAL_DOUBLE(0.0, item->valuedouble);
    TEST_ASSERT_EQUAL_INT(0, item->valueint);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, item->type); /* type unchanged */
    TEST_ASSERT_EQUAL_SIZE(0, buffer->offset);
}

/* Test 5: Parse number with locale-specific decimal point (ensure '.' replaced) */
static void test_parse_number_should_handle_decimal_point_replacement(void)
{
    /* Temporarily set locale to one using comma as decimal separator */
    char *old_locale = setlocale(LC_NUMERIC, NULL);
    TEST_ASSERT_NOT_NULL(old_locale);
    char *locale_copy = strdup(old_locale);
    TEST_ASSERT_NOT_NULL(locale_copy);

    /* Try to set a locale that uses comma as decimal separator */
    if (setlocale(LC_NUMERIC, "de_DE.UTF-8") == NULL &&
        setlocale(LC_NUMERIC, "de_DE") == NULL &&
        setlocale(LC_NUMERIC, "de") == NULL)
    {
        /* Skip if no German locale available */
        free(locale_copy);
        TEST_IGNORE_MESSAGE("No German locale available; skipping decimal point test");
        return;
    }

    buffer = create_buffer_from_string("12.34");
    item = cJSON_CreateNumber(0.0);
    TEST_ASSERT_NOT_NULL(item);

    cJSON_bool result = parse_number(item, buffer);

    /* Restore original locale */
    setlocale(LC_NUMERIC, locale_copy);
    free(locale_copy);

    TEST_ASSERT_TRUE_MESSAGE(result, "parse_number should succeed with decimal point");
    TEST_ASSERT_WITHIN(0.001, 12.34, item->valuedouble);
    TEST_ASSERT_EQUAL_INT(12, item->valueint);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, item->type);
    TEST_ASSERT_EQUAL_SIZE(5, buffer->offset);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_parse_number_should_parse_simple_integer);
    RUN_TEST(test_parse_number_should_parse_negative_decimal);
    RUN_TEST(test_parse_number_should_saturate_on_overflow);
    RUN_TEST(test_parse_number_should_fail_on_invalid_start);
    RUN_TEST(test_parse_number_should_handle_decimal_point_replacement);

    return UNITY_END();
}