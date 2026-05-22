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
 * Helper utilities
 * ------------------------------------------------------------------------- */

/*
 * Use the public cJSON_Parse / cJSON_ParseWithLength API to exercise
 * parse_string indirectly (it is static, so we cannot call it directly).
 * We wrap a bare string value in a trivial JSON object so the parser
 * reaches parse_string.
 *
 * build_json_string_doc() wraps a raw JSON string token (including the
 * surrounding quotes) inside {"v": <token>} so that cJSON_Parse will
 * call parse_string on it.
 *
 * Returns a heap-allocated buffer that the caller must free().
 */
static char *build_json_string_doc(const char *json_string_token)
{
    /* {"v": <token>} */
    size_t len = strlen(json_string_token) + 8 /* {"v": } */ + 1;
    char *buf = (char *)malloc(len);
    if (buf == NULL) return NULL;
    snprintf(buf, len, "{\"v\": %s}", json_string_token);
    return buf;
}

/*
 * Parse a JSON string token and return the cJSON_String item's valuestring,
 * or NULL on failure.  The returned cJSON object is stored in the global
 * parsed_item so tearDown() can free it.
 */
static const char *parse_string_token(const char *json_string_token)
{
    char *doc = build_json_string_doc(json_string_token);
    if (doc == NULL) return NULL;

    if (parsed_item != NULL)
    {
        cJSON_Delete(parsed_item);
        parsed_item = NULL;
    }

    cJSON *root = cJSON_Parse(doc);
    free(doc);

    if (root == NULL) return NULL;

    cJSON *v = cJSON_GetObjectItem(root, "v");
    if (v == NULL || !cJSON_IsString(v))
    {
        cJSON_Delete(root);
        return NULL;
    }

    /* Store root so tearDown can free it */
    parsed_item = root;
    return v->valuestring;
}

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
 * Test cases
 * ------------------------------------------------------------------------- */

/* 1. Simple ASCII string – happy path */
void test_parse_string_simple_ascii(void)
{
    const char *result = parse_string_token("\"hello world\"");

    TEST_ASSERT_NOT_NULL_MESSAGE(result, "Expected non-NULL valuestring for simple ASCII string");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("hello world", result,
                                     "Parsed string should equal the original ASCII content");

    TEST_ASSERT_NOT_NULL_MESSAGE(parsed_item, "Root cJSON object should not be NULL");
    cJSON *v = cJSON_GetObjectItem(parsed_item, "v");
    TEST_ASSERT_NOT_NULL_MESSAGE(v, "Key 'v' should exist in parsed object");
    TEST_ASSERT_TRUE_MESSAGE(cJSON_IsString(v), "Item type should be cJSON_String");
}

/* 2. Empty string */
void test_parse_string_empty(void)
{
    const char *result = parse_string_token("\"\"");

    TEST_ASSERT_NOT_NULL_MESSAGE(result, "Expected non-NULL valuestring for empty string");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("", result,
                                     "Parsed empty string should yield an empty C string");
}

/* 3. String with common escape sequences (\n, \t, \r, \\, \") */
void test_parse_string_escape_sequences(void)
{
    /* JSON: "line1\nline2\ttab\\back\"quote" */
    const char *result = parse_string_token("\"line1\\nline2\\ttab\\\\back\\\"quote\"");

    TEST_ASSERT_NOT_NULL_MESSAGE(result, "Expected non-NULL valuestring for escape-sequence string");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("line1\nline2\ttab\\back\"quote", result,
                                     "Escape sequences should be decoded correctly");
}

/* 4. String with a Unicode escape sequence (\uXXXX -> UTF-8) */
void test_parse_string_unicode_escape(void)
{
    /*
     * JSON: "\u0041\u0042\u0043"  =>  "ABC"
     * U+0041 = 'A', U+0042 = 'B', U+0043 = 'C'
     */
    const char *result = parse_string_token("\"\\u0041\\u0042\\u0043\"");

    TEST_ASSERT_NOT_NULL_MESSAGE(result, "Expected non-NULL valuestring for Unicode escape string");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("ABC", result,
                                     "\\uXXXX escapes should be decoded to the correct UTF-8 bytes");
}

/* 5. Unterminated string – parse must fail and return NULL */
void test_parse_string_unterminated_fails(void)
{
    /*
     * Build a raw JSON document with an unterminated string value so that
     * parse_string hits the "string ended unexpectedly" goto fail path.
     * We use cJSON_ParseWithLength with the exact byte count so the parser
     * sees the buffer end before the closing quote.
     */
    const char *doc = "{\"v\": \"unterminated";
    size_t doc_len  = strlen(doc);

    cJSON *root = cJSON_ParseWithLength(doc, doc_len);

    /* The parse must fail – root should be NULL */
    TEST_ASSERT_NULL_MESSAGE(root,
                             "Parsing an unterminated JSON string should return NULL");

    /* Defensive cleanup in case the implementation unexpectedly succeeds */
    if (root != NULL)
    {
        cJSON_Delete(root);
    }
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
    RUN_TEST(test_parse_string_unicode_escape);
    RUN_TEST(test_parse_string_unterminated_fails);

    return UNITY_END();
}