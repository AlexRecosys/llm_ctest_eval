#include "cJSON.h"
#include "unity.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "cJSON.c"


/* ------------------------------------------------------------------ */
/* Helper: parse a JSON string value using the public cJSON_Parse API  */
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
    TEST_ASSERT_EQUAL_STRING("a\"b", item->valuestring);
    cJSON_Delete(item);
}

/* Escape sequence: escaped backslash \\ */
void test_parse_string_escape_backslash(void)
{
    cJSON *item = parse_json_string("\"a\\\\b\"");
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_EQUAL_INT(cJSON_String, item->type);
    TEST_ASSERT_EQUAL_STRING("a\\b", item->valuestring);
    cJSON_Delete(item);
}

/* Escape sequence: escaped forward slash \/ */
void test_parse_string_escape_forward_slash(void)
{
    cJSON *item = parse_json_string("\"a\\/b\"");
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_EQUAL_INT(cJSON_String, item->type);
    TEST_ASSERT_EQUAL_STRING("a/b", item->valuestring);
    cJSON_Delete(item);
}

/* Multiple escape sequences in one string */
void test_parse_string_multiple_escapes(void)
{
    cJSON *item = parse_json_string("\"\\t\\n\\r\"");
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_EQUAL_INT(cJSON_String, item->type);
    TEST_ASSERT_EQUAL_INT('\t', (unsigned char)item->valuestring[0]);
    TEST_ASSERT_EQUAL_INT('\n', (unsigned char)item->valuestring[1]);
    TEST_ASSERT_EQUAL_INT('\r', (unsigned char)item->valuestring[2]);
    TEST_ASSERT_EQUAL_INT('\0', (unsigned char)item->valuestring[3]);
    cJSON_Delete(item);
}

/* UTF-16 escape: basic ASCII via \uXXXX */
void test_parse_string_utf16_ascii(void)
{
    /* \u0041 = 'A' */
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
    /* \u00E9 = é (U+00E9) encoded as UTF-8: 0xC3 0xA9 */
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
    /* \uD83D\uDE00 = U+1F600 (grinning face emoji) */
    cJSON *item = parse_json_string("\"\\uD83D\\uDE00\"");
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_EQUAL_INT(cJSON_String, item->type);
    TEST_ASSERT_NOT_NULL(item->valuestring);
    /* UTF-8 encoding of U+1F600: F0 9F 98 80 */
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

/* Fail: string ending with lone backslash */
void test_parse_string_fail_trailing_backslash(void)
{
    cJSON *item = parse_json_string("\"hello\\");
    TEST_ASSERT_NULL(item);
}

/* Fail: invalid escape sequence */
void test_parse_string_fail_invalid_escape(void)
{
    cJSON *item = parse_json_string("\"\\q\"");
    TEST_ASSERT_NULL(item);
}

/* Fail: lone high surrogate without low surrogate */
void test_parse_string_fail_lone_high_surrogate(void)
{
    cJSON *item = parse_json_string("\"\\uD800\"");
    TEST_ASSERT_NULL(item);
}

/* Fail: lone low surrogate */
void test_parse_string_fail_lone_low_surrogate(void)
{
    cJSON *item = parse_json_string("\"\\uDC00\"");
    TEST_ASSERT_NULL(item);
}

/* String with all printable ASCII characters */
void test_parse_string_printable_ascii(void)
{
    /* Build a JSON string containing printable ASCII 0x20..0x7E (excluding " and \) */
    char json[256];
    char expected[256];
    int ji = 0, ei = 0;
    unsigned char c;

    json[ji++] = '"';
    for (c = 0x20; c <= 0x7E; c++)
    {
        if (c == '"' || c == '\\')
            continue;
        json[ji++] = (char)c;
        expected[ei++] = (char)c;
    }
    json[ji++] = '"';
    json[ji] = '\0';
    expected[ei] = '\0';

    cJSON *item = parse_json_string(json);
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_EQUAL_INT(cJSON_String, item->type);
    TEST_ASSERT_EQUAL_STRING(expected, item->valuestring);
    cJSON_Delete(item);
}

/* String inside an object */
void test_parse_string_inside_object(void)
{
    cJSON *root = parse_json_string("{\"key\":\"value\"}");
    TEST_ASSERT_NOT_NULL(root);
    TEST_ASSERT_EQUAL_INT(cJSON_Object, root->type);

    cJSON *item = cJSON_GetObjectItem(root, "key");
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_EQUAL_INT(cJSON_String, item->type);
    TEST_ASSERT_EQUAL_STRING("value", item->valuestring);
    cJSON_Delete(root);
}

/* String inside an array */
void test_parse_string_inside_array(void)
{
    cJSON *root = parse_json_string("[\"first\",\"second\"]");
    TEST_ASSERT_NOT_NULL(root);
    TEST_ASSERT_EQUAL_INT(cJSON_Array, root->type);

    cJSON *first = cJSON_GetArrayItem(root, 0);
    TEST_ASSERT_NOT_NULL(first);
    TEST_ASSERT_EQUAL_INT(cJSON_String, first->type);
    TEST_ASSERT_EQUAL_STRING("first", first->valuestring);

    cJSON *second = cJSON_GetArrayItem(root, 1);
    TEST_ASSERT_NOT_NULL(second);
    TEST_ASSERT_EQUAL_INT(cJSON_String, second->type);
    TEST_ASSERT_EQUAL_STRING("second", second->valuestring);

    cJSON_Delete(root);
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
        json[i + 1] = 'a';
        expected[i] = 'a';
    }
    json[1025] = '"';
    json[1026] = '\0';
    expected[1024] = '\0';

    cJSON *item = parse_json_string(json);
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_EQUAL_INT(cJSON_String, item->type);
    TEST_ASSERT_EQUAL_STRING(expected, item->valuestring);
    cJSON_Delete(item);
}

/* String with unicode null character \u0000 */
void test_parse_string_unicode_null(void)
{
    /* \u0000 should produce a null byte in the output */
    cJSON *item = parse_json_string("\"\\u0000\"");
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_EQUAL_INT(cJSON_String, item->type);
    TEST_ASSERT_NOT_NULL(item->valuestring);
    /* The first byte should be 0x00 (null) */
    TEST_ASSERT_EQUAL_INT(0x00, (unsigned char)item->valuestring[0]);
    cJSON_Delete(item);
}

/* String with mixed content: text + escapes + unicode */
void test_parse_string_mixed_content(void)
{
    cJSON *item = parse_json_string("\"Hello\\nWorld\\u0021\"");
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_EQUAL_INT(cJSON_String, item->type);
    TEST_ASSERT_NOT_NULL(item->valuestring);
    TEST_ASSERT_EQUAL_INT('H',  (unsigned char)item->valuestring[0]);
    TEST_ASSERT_EQUAL_INT('e',  (unsigned char)item->valuestring[1]);
    TEST_ASSERT_EQUAL_INT('l',  (unsigned char)item->valuestring[2]);
    TEST_ASSERT_EQUAL_INT('l',  (unsigned char)item->valuestring[3]);
    TEST_ASSERT_EQUAL_INT('o',  (unsigned char)item->valuestring[4]);
    TEST_ASSERT_EQUAL_INT('\n', (unsigned char)item->valuestring[5]);
    TEST_ASSERT_EQUAL_INT('W',  (unsigned char)item->valuestring[6]);
    TEST_ASSERT_EQUAL_INT('o',  (unsigned char)item->valuestring[7]);
    TEST_ASSERT_EQUAL_INT('r',  (unsigned char)item->valuestring[8]);
    TEST_ASSERT_EQUAL_INT('l',  (unsigned char)item->valuestring[9]);
    TEST_ASSERT_EQUAL_INT('d',  (unsigned char)item->valuestring[10]);
    TEST_ASSERT_EQUAL_INT('!',  (unsigned char)item->valuestring[11]);
    cJSON_Delete(item);
}

/* Verify item type is exactly cJSON_String (not a combination) */
void test_parse_string_type_is_exactly_cjson_string(void)
{
    cJSON *item = parse_json_string("\"test\"");
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_EQUAL_INT(cJSON_String, item->type);
    TEST_ASSERT_TRUE(cJSON_IsString(item));
    cJSON_Delete(item);
}

/* Verify that a parsed string is not treated as other types */
void test_parse_string_not_other_types(void)
{
    cJSON *item = parse_json_string("\"test\"");
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_FALSE(cJSON_IsNumber(item));
    TEST_ASSERT_FALSE(cJSON_IsArray(item));
    TEST_ASSERT_FALSE(cJSON_IsObject(item));
    TEST_ASSERT_FALSE(cJSON_IsNull(item));
    TEST_ASSERT_FALSE(cJSON_IsBool(item));
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
    RUN_TEST(test_parse_string_fail_lone_high_surrogate);
    RUN_TEST(test_parse_string_fail_lone_low_surrogate);
    RUN_TEST(test_parse_string_printable_ascii);
    RUN_TEST(test_parse_string_inside_object);
    RUN_TEST(test_parse_string_inside_array);
    RUN_TEST(test_parse_string_long);
    RUN_TEST(test_parse_string_unicode_null);
    RUN_TEST(test_parse_string_mixed_content);
    RUN_TEST(test_parse_string_type_is_exactly_cjson_string);
    RUN_TEST(test_parse_string_not_other_types);

    return UNITY_END();
}