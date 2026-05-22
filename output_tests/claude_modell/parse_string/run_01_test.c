#include "unity.h"
#include "cJSON.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* -------------------------------------------------------------------------
 * File-scope fixtures
 * ------------------------------------------------------------------------- */
static cJSON *parsed_item = NULL;

/* -------------------------------------------------------------------------
 * setUp / tearDown
 * ------------------------------------------------------------------------- */
void setUp(void)
{
    parsed_item = NULL;
}

void tearDown(void)
{
    if (parsed_item != NULL)
    {
        cJSON_Delete(parsed_item);
        parsed_item = NULL;
    }
}

/* -------------------------------------------------------------------------
 * Helper: parse a JSON string value using the public cJSON_Parse API.
 * The function under test (parse_string) is static, so we exercise it
 * indirectly through cJSON_Parse / cJSON_ParseWithLength.
 * ------------------------------------------------------------------------- */
static cJSON *parse_json_string(const char *json_text)
{
    return cJSON_Parse(json_text);
}

/* -------------------------------------------------------------------------
 * Test 1 – Simple ASCII string is parsed correctly
 * ------------------------------------------------------------------------- */
void test_parse_string_simple_ascii(void)
{
    const char *json = "\"hello world\"";

    parsed_item = parse_json_string(json);

    TEST_ASSERT_NOT_NULL_MESSAGE(parsed_item, "cJSON_Parse should return a non-NULL item for a valid string");
    TEST_ASSERT_EQUAL_INT_MESSAGE(cJSON_String, parsed_item->type,
                                  "Parsed item type should be cJSON_String");
    TEST_ASSERT_NOT_NULL_MESSAGE(parsed_item->valuestring,
                                 "valuestring should not be NULL after parsing");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("hello world", parsed_item->valuestring,
                                     "valuestring should match the input text");
}

/* -------------------------------------------------------------------------
 * Test 2 – Empty string is parsed correctly
 * ------------------------------------------------------------------------- */
void test_parse_string_empty(void)
{
    const char *json = "\"\"";

    parsed_item = parse_json_string(json);

    TEST_ASSERT_NOT_NULL_MESSAGE(parsed_item, "cJSON_Parse should return a non-NULL item for an empty string");
    TEST_ASSERT_EQUAL_INT_MESSAGE(cJSON_String, parsed_item->type,
                                  "Parsed item type should be cJSON_String");
    TEST_ASSERT_NOT_NULL_MESSAGE(parsed_item->valuestring,
                                 "valuestring should not be NULL for an empty string");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("", parsed_item->valuestring,
                                     "valuestring should be an empty C string");
}

/* -------------------------------------------------------------------------
 * Test 3 – Standard escape sequences are decoded correctly
 * ------------------------------------------------------------------------- */
void test_parse_string_escape_sequences(void)
{
    /* JSON: "\b\f\n\r\t" */
    const char *json = "\"\\b\\f\\n\\r\\t\"";
    const char expected[] = { '\b', '\f', '\n', '\r', '\t', '\0' };

    parsed_item = parse_json_string(json);

    TEST_ASSERT_NOT_NULL_MESSAGE(parsed_item, "cJSON_Parse should succeed for escape sequences");
    TEST_ASSERT_EQUAL_INT_MESSAGE(cJSON_String, parsed_item->type,
                                  "Parsed item type should be cJSON_String");
    TEST_ASSERT_NOT_NULL_MESSAGE(parsed_item->valuestring, "valuestring must not be NULL");
    TEST_ASSERT_EQUAL_MEMORY_MESSAGE(expected, parsed_item->valuestring,
                                     sizeof(expected) - 1,
                                     "Escape sequences should be decoded to their byte values");
}

/* -------------------------------------------------------------------------
 * Test 4 – Escaped quote and backslash inside a string
 * ------------------------------------------------------------------------- */
void test_parse_string_escaped_quote_and_backslash(void)
{
    /* JSON: "say \"hi\" and \\" */
    const char *json = "\"say \\\"hi\\\" and \\\\\"";
    const char *expected = "say \"hi\" and \\";

    parsed_item = parse_json_string(json);

    TEST_ASSERT_NOT_NULL_MESSAGE(parsed_item,
                                 "cJSON_Parse should succeed for escaped quotes and backslashes");
    TEST_ASSERT_EQUAL_INT_MESSAGE(cJSON_String, parsed_item->type,
                                  "Parsed item type should be cJSON_String");
    TEST_ASSERT_NOT_NULL_MESSAGE(parsed_item->valuestring, "valuestring must not be NULL");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(expected, parsed_item->valuestring,
                                     "Escaped quote and backslash should be decoded correctly");
}

/* -------------------------------------------------------------------------
 * Test 5 – Unterminated string returns NULL (parse failure)
 * ------------------------------------------------------------------------- */
void test_parse_string_unterminated_fails(void)
{
    /* A string that is never closed – parse_string should fail */
    const char *json = "\"unterminated string";

    parsed_item = parse_json_string(json);

    TEST_ASSERT_NULL_MESSAGE(parsed_item,
                             "cJSON_Parse should return NULL for an unterminated string literal");
}

/* -------------------------------------------------------------------------
 * main
 * ------------------------------------------------------------------------- */
int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_parse_string_simple_ascii);
    RUN_TEST(test_parse_string_empty);
    RUN_TEST(test_parse_string_escape_sequences);
    RUN_TEST(test_parse_string_escaped_quote_and_backslash);
    RUN_TEST(test_parse_string_unterminated_fails);

    return UNITY_END();
}