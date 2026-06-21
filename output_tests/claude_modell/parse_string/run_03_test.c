#include "cJSON.h"
#include "unity.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "cJSON.c"


/* ------------------------------------------------------------------ */
/* Helper: parse a JSON string value via the public cJSON_Parse API    */
/* ------------------------------------------------------------------ */

static cJSON *parse_json_string(const char *json)
{
    return cJSON_Parse(json);
}

/* ------------------------------------------------------------------ */
/* setUp / tearDown                                                     */
/* ------------------------------------------------------------------ */

void setUp(void)
{
    /* nothing needed */
}

void tearDown(void)
{
    /* nothing needed */
}

/* ------------------------------------------------------------------ */
/* Test cases                                                           */
/* ------------------------------------------------------------------ */

/* Basic simple string */
void test_parse_string_simple(void)
{
    cJSON *item = parse_json_string("\"hello\"");
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_EQUAL_INT(cJSON_String, item->type);
    TEST_ASSERT_NOT_NULL(item->valuestring);
    TEST_ASSERT_EQUAL_STRING("hello", item->valuestring);
    cJSON_Delete(item);
}

/* Empty string */
void test_parse_string_empty(void)
{
    cJSON *item = parse_json_string("\"\"");
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_EQUAL_INT(cJSON_String, item->type);
    TEST_ASSERT_NOT_NULL(item->valuestring);
    TEST_ASSERT_EQUAL_STRING("", item->valuestring);
    cJSON_Delete(item);
}

/* String with spaces */
void test_parse_string_with_spaces(void)
{
    cJSON *item = parse_json_string("\"hello world\"");
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_EQUAL_INT(cJSON_String, item->type);
    TEST_ASSERT_EQUAL_STRING("hello world", item->valuestring);
    cJSON_Delete(item);
}

/* Escape sequence: backspace \b */
void test_parse_string_escape_backspace(void)
{
    cJSON *item = parse_json_string("\"a\\bb\"");
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_EQUAL_INT(cJSON_String, item->type);
    TEST_ASSERT_NOT_NULL(item->valuestring);
    TEST_ASSERT_EQUAL_INT('a',  (unsigned char)item->valuestring[0]);
    TEST_ASSERT_EQUAL_INT('\b', (unsigned char)item->valuestring[1]);
    TEST_ASSERT_EQUAL_INT('b',  (unsigned char)item->valuestring[2]);
    TEST_ASSERT_EQUAL_INT('\0', (unsigned char)item->valuestring[3]);
    cJSON_Delete(item);
}

/* Escape sequence: form-feed \f */
void test_parse_string_escape_formfeed(void)
{
    cJSON *item = parse_json_string("\"a\\fb\"");
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_EQUAL_INT(cJSON_String, item->type);
    TEST_ASSERT_EQUAL_INT('\f', (unsigned char)item->valuestring[1]);
    cJSON_Delete(item);
}

/* Escape sequence: newline \n */
void test_parse_string_escape_newline(void)
{
    cJSON *item = parse_json_string("\"a\\nb\"");
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_EQUAL_INT(cJSON_String, item->type);
    TEST_ASSERT_EQUAL_INT('\n', (unsigned char)item->valuestring[1]);
    cJSON_Delete(item);
}

/* Escape sequence: carriage return \r */
void test_parse_string_escape_carriage_return(void)
{
    cJSON *item = parse_json_string("\"a\\rb\"");
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_EQUAL_INT(cJSON_String, item->type);
    TEST_ASSERT_EQUAL_INT('\r', (unsigned char)item->valuestring[1]);
    cJSON_Delete(item);
}

/* Escape sequence: tab \t */
void test_parse_string_escape_tab(void)
{
    cJSON *item = parse_json_string("\"a\\tb\"");
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_EQUAL_INT(cJSON_String, item->type);
    TEST_ASSERT_EQUAL_INT('\t', (unsigned char)item->valuestring[1]);
    cJSON_Delete(item);
}

/* Escape sequence: escaped quote \" */
void test_parse_string_escape_quote(void)
{
    cJSON *item = parse_json_string("\"a\\\"b\"");
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_EQUAL_INT(cJSON_String, item->type);
    TEST_ASSERT_EQUAL_INT('"', (unsigned char)item->valuestring[1]);
    cJSON_Delete(item);
}

/* Escape sequence: escaped backslash \\ */
void test_parse_string_escape_backslash(void)
{
    cJSON *item = parse_json_string("\"a\\\\b\"");
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_EQUAL_INT(cJSON_String, item->type);
    TEST_ASSERT_EQUAL_INT('\\', (unsigned char)item->valuestring[1]);
    cJSON_Delete(item);
}

/* Escape sequence: escaped forward slash \/ */
void test_parse_string_escape_forward_slash(void)
{
    cJSON *item = parse_json_string("\"a\\/b\"");
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_EQUAL_INT(cJSON_String, item->type);
    TEST_ASSERT_EQUAL_INT('/', (unsigned char)item->valuestring[1]);
    cJSON_Delete(item);
}

/* Multiple escape sequences in one string */
void test_parse_string_multiple_escapes(void)
{
    cJSON *item = parse_json_string("\"\\n\\t\\r\"");
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_EQUAL_INT(cJSON_String, item->type);
    TEST_ASSERT_EQUAL_INT('\n', (unsigned char)item->valuestring[0]);
    TEST_ASSERT_EQUAL_INT('\t', (unsigned char)item->valuestring[1]);
    TEST_ASSERT_EQUAL_INT('\r', (unsigned char)item->valuestring[2]);
    TEST_ASSERT_EQUAL_INT('\0', (unsigned char)item->valuestring[3]);
    cJSON_Delete(item);
}

/* UTF-16 escape: basic ASCII via \uXXXX */
void test_parse_string_utf16_ascii(void)
{
    /* \u0041 is 'A' */
    cJSON *item = parse_json_string("\"\\u0041\"");
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_EQUAL_INT(cJSON_String, item->type);
    TEST_ASSERT_NOT_NULL(item->valuestring);
    TEST_ASSERT_EQUAL_INT('A', (unsigned char)item->valuestring[0]);
    cJSON_Delete(item);
}

/* UTF-16 escape: two-byte UTF-8 character */
void test_parse_string_utf16_two_byte(void)
{
    /* \u00E9 is é (U+00E9), encoded as 0xC3 0xA9 in UTF-8 */
    cJSON *item = parse_json_string("\"\\u00E9\"");
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_EQUAL_INT(cJSON_String, item->type);
    TEST_ASSERT_NOT_NULL(item->valuestring);
    TEST_ASSERT_EQUAL_INT((unsigned char)0xC3, (unsigned char)item->valuestring[0]);
    TEST_ASSERT_EQUAL_INT((unsigned char)0xA9, (unsigned char)item->valuestring[1]);
    cJSON_Delete(item);
}

/* UTF-16 surrogate pair */
void test_parse_string_utf16_surrogate_pair(void)
{
    /* \uD83D\uDE00 is U+1F600 (grinning face emoji), UTF-8: F0 9F 98 80 */
    cJSON *item = parse_json_string("\"\\uD83D\\uDE00\"");
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_EQUAL_INT(cJSON_String, item->type);
    TEST_ASSERT_NOT_NULL(item->valuestring);
    TEST_ASSERT_EQUAL_INT((unsigned char)0xF0, (unsigned char)item->valuestring[0]);
    TEST_ASSERT_EQUAL_INT((unsigned char)0x9F, (unsigned char)item->valuestring[1]);
    TEST_ASSERT_EQUAL_INT((unsigned char)0x98, (unsigned char)item->valuestring[2]);
    TEST_ASSERT_EQUAL_INT((unsigned char)0x80, (unsigned char)item->valuestring[3]);
    cJSON_Delete(item);
}

/* Fail: not a string (starts with a number) */
void test_parse_string_fail_not_string(void)
{
    cJSON *item = parse_json_string("42");
    /* 42 is a valid JSON number, not a string */
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, item->type);
    cJSON_Delete(item);
}

/* Fail: unterminated string */
void test_parse_string_fail_unterminated(void)
{
    cJSON *item = parse_json_string("\"hello");
    TEST_ASSERT_NULL(item);
}

/* Fail: backslash at end of input */
void test_parse_string_fail_trailing_backslash(void)
{
    cJSON *item = parse_json_string("\"hello\\");
    TEST_ASSERT_NULL(item);
}

/* Fail: invalid escape sequence */
void test_parse_string_fail_invalid_escape(void)
{
    cJSON *item = parse_json_string("\"\\x41\"");
    TEST_ASSERT_NULL(item);
}

/* Fail: incomplete surrogate pair (high surrogate only) */
void test_parse_string_fail_incomplete_surrogate(void)
{
    cJSON *item = parse_json_string("\"\\uD83D\"");
    TEST_ASSERT_NULL(item);
}

/* Fail: high surrogate followed by non-surrogate */
void test_parse_string_fail_bad_surrogate_pair(void)
{
    cJSON *item = parse_json_string("\"\\uD83D\\u0041\"");
    TEST_ASSERT_NULL(item);
}

/* String with only escape sequences */
void test_parse_string_only_escapes(void)
{
    cJSON *item = parse_json_string("\"\\\\\\\"\"");
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_EQUAL_INT(cJSON_String, item->type);
    TEST_ASSERT_EQUAL_INT('\\', (unsigned char)item->valuestring[0]);
    TEST_ASSERT_EQUAL_INT('"',  (unsigned char)item->valuestring[1]);
    TEST_ASSERT_EQUAL_INT('\0', (unsigned char)item->valuestring[2]);
    cJSON_Delete(item);
}

/* Long string */
void test_parse_string_long(void)
{
    char json[1024 + 3];
    char expected[1024 + 1];
    int i;

    json[0] = '"';
    for (i = 0; i < 1024; i++)
    {
        json[i + 1]     = 'a';
        expected[i]     = 'a';
    }
    json[1025]    = '"';
    json[1026]    = '\0';
    expected[1024] = '\0';

    cJSON *item = parse_json_string(json);
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_EQUAL_INT(cJSON_String, item->type);
    TEST_ASSERT_EQUAL_STRING(expected, item->valuestring);
    cJSON_Delete(item);
}

/* String inside a JSON object */
void test_parse_string_in_object(void)
{
    cJSON *root = parse_json_string("{\"key\":\"value\"}");
    TEST_ASSERT_NOT_NULL(root);
    cJSON *item = cJSON_GetObjectItem(root, "key");
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_EQUAL_INT(cJSON_String, item->type);
    TEST_ASSERT_EQUAL_STRING("value", item->valuestring);
    cJSON_Delete(root);
}

/* String inside a JSON array */
void test_parse_string_in_array(void)
{
    cJSON *root = parse_json_string("[\"first\",\"second\"]");
    TEST_ASSERT_NOT_NULL(root);
    cJSON *first  = cJSON_GetArrayItem(root, 0);
    cJSON *second = cJSON_GetArrayItem(root, 1);
    TEST_ASSERT_NOT_NULL(first);
    TEST_ASSERT_NOT_NULL(second);
    TEST_ASSERT_EQUAL_STRING("first",  first->valuestring);
    TEST_ASSERT_EQUAL_STRING("second", second->valuestring);
    cJSON_Delete(root);
}

/* String with unicode three-byte sequence \u4E2D (中) */
void test_parse_string_utf16_three_byte(void)
{
    /* U+4E2D -> UTF-8: E4 B8 AD */
    cJSON *item = parse_json_string("\"\\u4E2D\"");
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_EQUAL_INT(cJSON_String, item->type);
    TEST_ASSERT_NOT_NULL(item->valuestring);
    TEST_ASSERT_EQUAL_INT((unsigned char)0xE4, (unsigned char)item->valuestring[0]);
    TEST_ASSERT_EQUAL_INT((unsigned char)0xB8, (unsigned char)item->valuestring[1]);
    TEST_ASSERT_EQUAL_INT((unsigned char)0xAD, (unsigned char)item->valuestring[2]);
    cJSON_Delete(item);
}

/* Null character via \u0000 */
void test_parse_string_utf16_null_char(void)
{
    cJSON *item = parse_json_string("\"\\u0000\"");
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_EQUAL_INT(cJSON_String, item->type);
    /* The string is allocated; first byte should be 0 (null char encoded) */
    TEST_ASSERT_NOT_NULL(item->valuestring);
    cJSON_Delete(item);
}

/* String with mixed content: text + escapes + unicode */
void test_parse_string_mixed_content(void)
{
    cJSON *item = parse_json_string("\"Hello\\nWorld\\u0021\"");
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_EQUAL_INT(cJSON_String, item->type);
    TEST_ASSERT_NOT_NULL(item->valuestring);
    /* "Hello\nWorld!" */
    TEST_ASSERT_EQUAL_INT('H',  (unsigned char)item->valuestring[0]);
    TEST_ASSERT_EQUAL_INT('\n', (unsigned char)item->valuestring[5]);
    TEST_ASSERT_EQUAL_INT('W',  (unsigned char)item->valuestring[6]);
    TEST_ASSERT_EQUAL_INT('!',  (unsigned char)item->valuestring[11]);
    cJSON_Delete(item);
}

/* ------------------------------------------------------------------ */
/* main                                                                 */
/* ------------------------------------------------------------------ */

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();

    RUN_TEST(test_parse_string_simple);
    RUN_TEST(test_parse_string_empty);
    RUN_TEST(test_parse_string_with_spaces);
    RUN_TEST(test_parse_string_escape_backspace);
    RUN_TEST(test_parse_string_escape_formfeed);
    RUN_TEST(test_parse_string_escape_newline);
    RUN_TEST(test_parse_string_escape_carriage_return);
    RUN_TEST(test_parse_string_escape_tab);
    RUN_TEST(test_parse_string_escape_quote);
    RUN_TEST(test_parse_string_escape_backslash);
    RUN_TEST(test_parse_string_escape_forward_slash);
    RUN_TEST(test_parse_string_multiple_escapes);
    RUN_TEST(test_parse_string_utf16_ascii);
    RUN_TEST(test_parse_string_utf16_two_byte);
    RUN_TEST(test_parse_string_utf16_surrogate_pair);
    RUN_TEST(test_parse_string_fail_not_string);
    RUN_TEST(test_parse_string_fail_unterminated);
    RUN_TEST(test_parse_string_fail_trailing_backslash);
    RUN_TEST(test_parse_string_fail_invalid_escape);
    RUN_TEST(test_parse_string_fail_incomplete_surrogate);
    RUN_TEST(test_parse_string_fail_bad_surrogate_pair);
    RUN_TEST(test_parse_string_only_escapes);
    RUN_TEST(test_parse_string_long);
    RUN_TEST(test_parse_string_in_object);
    RUN_TEST(test_parse_string_in_array);
    RUN_TEST(test_parse_string_utf16_three_byte);
    RUN_TEST(test_parse_string_utf16_null_char);
    RUN_TEST(test_parse_string_mixed_content);

    return UNITY_END();
}