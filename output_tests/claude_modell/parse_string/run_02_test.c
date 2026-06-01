#include "cJSON.c"
#include "unity.h"
#include "cJSON.h"
#include <stdlib.h>
#include <string.h>
#include <signal.h>

/* File-scope variables for setUp/tearDown */
static cJSON item_storage;
static cJSON *test_item = NULL;

/* Signal handler for SIGSEGV */
static void sigsegv_handler(int sig)
{
    (void)sig;
    TEST_FAIL_MESSAGE("Segmentation fault (SIGSEGV) caught during test execution");
}

/* Helper: build a parse_buffer pointing at a string literal */
static void make_parse_buffer(parse_buffer *buf, const unsigned char *content, size_t length, size_t offset)
{
    memset(buf, 0, sizeof(*buf));
    buf->content         = content;
    buf->length          = length;
    buf->offset          = offset;
    buf->hooks.allocate  = malloc;
    buf->hooks.deallocate = free;
    buf->hooks.reallocate = realloc;
}

void setUp(void)
{
    signal(SIGSEGV, sigsegv_handler);
    memset(&item_storage, 0, sizeof(item_storage));
    test_item = &item_storage;
}

void tearDown(void)
{
    /* Free valuestring if it was allocated by parse_string */
    if (test_item != NULL && test_item->valuestring != NULL)
    {
        free(test_item->valuestring);
        test_item->valuestring = NULL;
    }
    test_item = NULL;
    signal(SIGSEGV, SIG_DFL);
}

/* -----------------------------------------------------------------------
 * Test 1: Simple ASCII string — "hello"
 * Input buffer content: "hello"  (with surrounding quotes)
 * ----------------------------------------------------------------------- */
void test_parse_string_simple_ascii(void)
{
    const unsigned char *raw = (const unsigned char *)"\"hello\"";
    parse_buffer buf;
    make_parse_buffer(&buf, raw, strlen((const char *)raw), 0);

    cJSON_bool result = parse_string(test_item, &buf);

    TEST_ASSERT_TRUE_MESSAGE(result, "parse_string should return true for a simple ASCII string");
    TEST_ASSERT_EQUAL_INT_MESSAGE(cJSON_String, test_item->type, "item type should be cJSON_String");
    TEST_ASSERT_NOT_NULL_MESSAGE(test_item->valuestring, "valuestring should not be NULL");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("hello", test_item->valuestring, "valuestring should equal 'hello'");
}

/* -----------------------------------------------------------------------
 * Test 2: Empty string — ""
 * ----------------------------------------------------------------------- */
void test_parse_string_empty(void)
{
    const unsigned char *raw = (const unsigned char *)"\"\"";
    parse_buffer buf;
    make_parse_buffer(&buf, raw, strlen((const char *)raw), 0);

    cJSON_bool result = parse_string(test_item, &buf);

    TEST_ASSERT_TRUE_MESSAGE(result, "parse_string should return true for an empty string");
    TEST_ASSERT_EQUAL_INT_MESSAGE(cJSON_String, test_item->type, "item type should be cJSON_String");
    TEST_ASSERT_NOT_NULL_MESSAGE(test_item->valuestring, "valuestring should not be NULL");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("", test_item->valuestring, "valuestring should be empty");
}

/* -----------------------------------------------------------------------
 * Test 3: String with escape sequences — "\b\f\n\r\t"
 * ----------------------------------------------------------------------- */
void test_parse_string_escape_sequences(void)
{
    /* JSON: "\b\f\n\r\t" */
    const unsigned char *raw = (const unsigned char *)"\"\\b\\f\\n\\r\\t\"";
    parse_buffer buf;
    make_parse_buffer(&buf, raw, strlen((const char *)raw), 0);

    cJSON_bool result = parse_string(test_item, &buf);

    TEST_ASSERT_TRUE_MESSAGE(result, "parse_string should return true for escape sequences");
    TEST_ASSERT_EQUAL_INT_MESSAGE(cJSON_String, test_item->type, "item type should be cJSON_String");
    TEST_ASSERT_NOT_NULL_MESSAGE(test_item->valuestring, "valuestring should not be NULL");

    /* Expected decoded string: \b \f \n \r \t */
    const char expected[] = {'\b', '\f', '\n', '\r', '\t', '\0'};
    TEST_ASSERT_EQUAL_STRING_MESSAGE(expected, test_item->valuestring,
                                     "valuestring should contain decoded escape sequences");
}

/* -----------------------------------------------------------------------
 * Test 4: Fail — input does not start with a quote character
 * ----------------------------------------------------------------------- */
void test_parse_string_fail_not_a_string(void)
{
    const unsigned char *raw = (const unsigned char *)"hello\"";
    parse_buffer buf;
    make_parse_buffer(&buf, raw, strlen((const char *)raw), 0);

    cJSON_bool result = parse_string(test_item, &buf);

    TEST_ASSERT_FALSE_MESSAGE(result, "parse_string should return false when input does not start with '\"'");
    /* valuestring must not have been set */
    TEST_ASSERT_NULL_MESSAGE(test_item->valuestring, "valuestring should remain NULL on failure");
}

/* -----------------------------------------------------------------------
 * Test 5: Fail — unterminated string (no closing quote)
 * ----------------------------------------------------------------------- */
void test_parse_string_fail_unterminated(void)
{
    /* JSON: "hello  — no closing quote */
    const unsigned char *raw = (const unsigned char *)"\"hello";
    parse_buffer buf;
    make_parse_buffer(&buf, raw, strlen((const char *)raw), 0);

    cJSON_bool result = parse_string(test_item, &buf);

    TEST_ASSERT_FALSE_MESSAGE(result, "parse_string should return false for an unterminated string");
    TEST_ASSERT_NULL_MESSAGE(test_item->valuestring, "valuestring should remain NULL on failure");
}

/* -----------------------------------------------------------------------
 * main
 * ----------------------------------------------------------------------- */
int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_parse_string_simple_ascii);
    RUN_TEST(test_parse_string_empty);
    RUN_TEST(test_parse_string_escape_sequences);
    RUN_TEST(test_parse_string_fail_not_a_string);
    RUN_TEST(test_parse_string_fail_unterminated);
    return UNITY_END();
}