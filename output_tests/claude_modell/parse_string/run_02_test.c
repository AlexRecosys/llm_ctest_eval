#include "unity.h"
#include "cJSON.h"

#include <stdlib.h>
#include <string.h>

/* -------------------------------------------------------------------------
 * File-scope fixtures
 * ------------------------------------------------------------------------- */

/* We test parse_string indirectly through cJSON_Parse / cJSON_ParseWithLength
 * because parse_string is a static function.  Every test parses a JSON value
 * that IS a string (or is expected to fail) and then inspects the resulting
 * cJSON object.
 */

static cJSON *g_item = NULL;

/* -------------------------------------------------------------------------
 * Helper macros / functions
 * ------------------------------------------------------------------------- */

/*
 * parse_json_string – thin wrapper that calls cJSON_Parse and returns the
 * resulting cJSON node.  The caller is responsible for calling cJSON_Delete.
 */
static cJSON *parse_json_string(const char *json_text)
{
    return cJSON_Parse(json_text);
}

/*
 * parse_json_string_with_length – same but with an explicit length so we can
 * test truncated / unterminated inputs.
 */
static cJSON *parse_json_string_with_length(const char *json_text, size_t len)
{
    return cJSON_ParseWithLength(json_text, len);
}

/* -------------------------------------------------------------------------
 * setUp / tearDown
 * ------------------------------------------------------------------------- */

void setUp(void)
{
    g_item = NULL;
}

void tearDown(void)
{
    if (g_item != NULL)
    {
        cJSON_Delete(g_item);
        g_item = NULL;
    }
}

/* -------------------------------------------------------------------------
 * Test cases
 * ------------------------------------------------------------------------- */

/* 1. Simple plain ASCII string – no escape sequences */
void test_parse_string_simple_ascii(void)
{
    const char *json = "\"hello world\"";

    g_item = parse_json_string(json);

    TEST_ASSERT_NOT_NULL_MESSAGE(g_item, "cJSON_Parse should return a non-NULL item for a valid string");
    TEST_ASSERT_EQUAL_INT_MESSAGE(cJSON_String, g_item->type,
                                  "Parsed item type should be cJSON_String");
    TEST_ASSERT_NOT_NULL_MESSAGE(g_item->valuestring, "valuestring should not be NULL");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("hello world", g_item->valuestring,
                                     "valuestring should match the input text");
}

/* 2. String containing common escape sequences (\n, \t, \r, \\, \", \/) */
void test_parse_string_escape_sequences(void)
{
    /* JSON:  "line1\nline2\ttabbed\/slash\\back\"quote" */
    const char *json = "\"line1\\nline2\\ttabbed\\/slash\\\\back\\\"quote\"";

    g_item = parse_json_string(json);

    TEST_ASSERT_NOT_NULL_MESSAGE(g_item, "cJSON_Parse should succeed for a string with escape sequences");
    TEST_ASSERT_EQUAL_INT_MESSAGE(cJSON_String, g_item->type,
                                  "Parsed item type should be cJSON_String");
    TEST_ASSERT_NOT_NULL_MESSAGE(g_item->valuestring, "valuestring should not be NULL");

    /* Expected decoded value */
    const char *expected = "line1\nline2\ttabbed/slash\\back\"quote";
    TEST_ASSERT_EQUAL_STRING_MESSAGE(expected, g_item->valuestring,
                                     "Escape sequences should be decoded correctly");
}

/* 3. Empty string – edge case with zero content between the quotes */
void test_parse_string_empty(void)
{
    const char *json = "\"\"";

    g_item = parse_json_string(json);

    TEST_ASSERT_NOT_NULL_MESSAGE(g_item, "cJSON_Parse should succeed for an empty string");
    TEST_ASSERT_EQUAL_INT_MESSAGE(cJSON_String, g_item->type,
                                  "Parsed item type should be cJSON_String");
    TEST_ASSERT_NOT_NULL_MESSAGE(g_item->valuestring, "valuestring should not be NULL even for empty string");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("", g_item->valuestring,
                                     "valuestring should be an empty C string");
}

/* 4. Unterminated string – the closing quote is missing; parse must fail */
void test_parse_string_unterminated_fails(void)
{
    /* Provide the exact byte count so cJSON cannot read past the buffer */
    const char *json = "\"unterminated";
    size_t len = strlen(json); /* no closing quote */

    g_item = parse_json_string_with_length(json, len);

    TEST_ASSERT_NULL_MESSAGE(g_item,
                             "cJSON_Parse should return NULL for an unterminated string");
}

/* 5. String with a UTF-16 Unicode escape sequence (\uXXXX) */
void test_parse_string_unicode_escape(void)
{
    /* JSON: "\u0041\u0042\u0043"  =>  "ABC" */
    const char *json = "\"\\u0041\\u0042\\u0043\"";

    g_item = parse_json_string(json);

    TEST_ASSERT_NOT_NULL_MESSAGE(g_item,
                                 "cJSON_Parse should succeed for a string with \\uXXXX escapes");
    TEST_ASSERT_EQUAL_INT_MESSAGE(cJSON_String, g_item->type,
                                  "Parsed item type should be cJSON_String");
    TEST_ASSERT_NOT_NULL_MESSAGE(g_item->valuestring, "valuestring should not be NULL");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("ABC", g_item->valuestring,
                                     "\\u0041\\u0042\\u0043 should decode to \"ABC\"");
}

/* -------------------------------------------------------------------------
 * main
 * ------------------------------------------------------------------------- */

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_parse_string_simple_ascii);
    RUN_TEST(test_parse_string_escape_sequences);
    RUN_TEST(test_parse_string_empty);
    RUN_TEST(test_parse_string_unterminated_fails);
    RUN_TEST(test_parse_string_unicode_escape);

    return UNITY_END();
}