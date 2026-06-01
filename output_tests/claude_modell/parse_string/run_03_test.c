#include "cJSON.c"
#include "unity.h"
#include "cJSON.h"
#include <stdlib.h>
#include <string.h>
#include <signal.h>

/* File-scope fixtures */
static cJSON item;
static parse_buffer buf;

/* Signal handler for SIGSEGV */
static void sigsegv_handler(int sig)
{
    (void)sig;
    TEST_FAIL_MESSAGE("Segmentation fault (SIGSEGV) caught during test execution");
}

/* Helper: initialize a parse_buffer pointing at the given string */
static void init_parse_buffer(parse_buffer *pb, const unsigned char *content, size_t length, size_t offset)
{
    memset(pb, 0, sizeof(parse_buffer));
    pb->content          = content;
    pb->length           = length;
    pb->offset           = offset;
    pb->depth            = 0;
    pb->hooks.allocate   = malloc;
    pb->hooks.deallocate = free;
    pb->hooks.reallocate = realloc;
}

void setUp(void)
{
    signal(SIGSEGV, sigsegv_handler);
    memset(&item, 0, sizeof(cJSON));
    memset(&buf, 0, sizeof(parse_buffer));
}

void tearDown(void)
{
    /* Free valuestring if parse_string allocated it */
    if (item.valuestring != NULL)
    {
        free(item.valuestring);
        item.valuestring = NULL;
    }
    signal(SIGSEGV, SIG_DFL);
}

/* -----------------------------------------------------------------------
 * Test 1: Simple ASCII string parses correctly
 * Input JSON fragment (starting at offset 0): "hello"
 * ----------------------------------------------------------------------- */
void test_parse_string_simple_ascii(void)
{
    const unsigned char *content = (const unsigned char *)"\"hello\"";
    init_parse_buffer(&buf, content, strlen((const char *)content), 0);

    cJSON_bool result = parse_string(&item, &buf);

    TEST_ASSERT_TRUE_MESSAGE(result, "parse_string should return true for a simple ASCII string");
    TEST_ASSERT_EQUAL_INT_MESSAGE(cJSON_String, item.type, "item type should be cJSON_String");
    TEST_ASSERT_NOT_NULL_MESSAGE(item.valuestring, "valuestring should not be NULL");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("hello", item.valuestring, "valuestring should equal 'hello'");
    /* offset should be positioned just past the closing quote */
    TEST_ASSERT_EQUAL_UINT_MESSAGE((size_t)6, buf.offset, "offset should be past the closing quote");
}

/* -----------------------------------------------------------------------
 * Test 2: Empty string parses correctly
 * Input JSON fragment: ""
 * ----------------------------------------------------------------------- */
void test_parse_string_empty_string(void)
{
    const unsigned char *content = (const unsigned char *)"\"\"";
    init_parse_buffer(&buf, content, strlen((const char *)content), 0);

    cJSON_bool result = parse_string(&item, &buf);

    TEST_ASSERT_TRUE_MESSAGE(result, "parse_string should return true for an empty string");
    TEST_ASSERT_EQUAL_INT_MESSAGE(cJSON_String, item.type, "item type should be cJSON_String");
    TEST_ASSERT_NOT_NULL_MESSAGE(item.valuestring, "valuestring should not be NULL");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("", item.valuestring, "valuestring should be empty");
}

/* -----------------------------------------------------------------------
 * Test 3: String with escape sequences (\n, \t, \r, \b, \f, \\, \/, \")
 * Input JSON fragment: "line1\nline2\ttab"
 * ----------------------------------------------------------------------- */
void test_parse_string_escape_sequences(void)
{
    const unsigned char *content = (const unsigned char *)"\"line1\\nline2\\ttab\"";
    init_parse_buffer(&buf, content, strlen((const char *)content), 0);

    cJSON_bool result = parse_string(&item, &buf);

    TEST_ASSERT_TRUE_MESSAGE(result, "parse_string should return true for string with escape sequences");
    TEST_ASSERT_EQUAL_INT_MESSAGE(cJSON_String, item.type, "item type should be cJSON_String");
    TEST_ASSERT_NOT_NULL_MESSAGE(item.valuestring, "valuestring should not be NULL");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("line1\nline2\ttab", item.valuestring,
                                     "valuestring should have escape sequences resolved");
}

/* -----------------------------------------------------------------------
 * Test 4: Fail — input does not start with a double-quote
 * Input JSON fragment: 42 (a number, not a string)
 * ----------------------------------------------------------------------- */
void test_parse_string_not_a_string(void)
{
    const unsigned char *content = (const unsigned char *)"42";
    init_parse_buffer(&buf, content, strlen((const char *)content), 0);

    cJSON_bool result = parse_string(&item, &buf);

    TEST_ASSERT_FALSE_MESSAGE(result, "parse_string should return false when input does not start with '\"'");
    /* item type should remain unchanged (0 / cJSON_Invalid) */
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, item.type, "item type should remain 0 on failure");
    TEST_ASSERT_NULL_MESSAGE(item.valuestring, "valuestring should be NULL on failure");
}

/* -----------------------------------------------------------------------
 * Test 5: Fail — unterminated string (no closing quote)
 * Input JSON fragment: "hello  (missing closing quote)
 * ----------------------------------------------------------------------- */
void test_parse_string_unterminated(void)
{
    const unsigned char *content = (const unsigned char *)"\"hello";
    init_parse_buffer(&buf, content, strlen((const char *)content), 0);

    cJSON_bool result = parse_string(&item, &buf);

    TEST_ASSERT_FALSE_MESSAGE(result, "parse_string should return false for an unterminated string");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, item.type, "item type should remain 0 on failure");
    TEST_ASSERT_NULL_MESSAGE(item.valuestring, "valuestring should be NULL on failure");
}

/* -----------------------------------------------------------------------
 * main
 * ----------------------------------------------------------------------- */
int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_parse_string_simple_ascii);
    RUN_TEST(test_parse_string_empty_string);
    RUN_TEST(test_parse_string_escape_sequences);
    RUN_TEST(test_parse_string_not_a_string);
    RUN_TEST(test_parse_string_unterminated);
    return UNITY_END();
}