#include "cJSON.c"
#include "unity.h"
#include "cJSON.h"
#include <stdlib.h>
#include <string.h>

/* File-scope variables */
static cJSON item;
static parse_buffer input_buffer;

/* Helper: initialize a parse_buffer with a given string (including surrounding quotes) */
static void init_parse_buffer(parse_buffer *buf, const char *content)
{
    buf->content = (const unsigned char *)content;
    buf->length = strlen(content);
    buf->offset = 0;
    buf->depth = 0;
    buf->hooks.allocate = malloc;
    buf->hooks.deallocate = free;
    buf->hooks.reallocate = realloc;
}

/* Helper: initialize a cJSON item to zeroed state */
static void init_item(cJSON *it)
{
    memset(it, 0, sizeof(cJSON));
}

void setUp(void)
{
    init_item(&item);
    memset(&input_buffer, 0, sizeof(parse_buffer));
    input_buffer.hooks.allocate = malloc;
    input_buffer.hooks.deallocate = free;
    input_buffer.hooks.reallocate = realloc;
}

void tearDown(void)
{
    if (item.valuestring != NULL)
    {
        free(item.valuestring);
        item.valuestring = NULL;
    }
}

/* -------------------------------------------------------------------------
 * Test cases
 * ------------------------------------------------------------------------- */

/* 1. Simple ASCII string */
void test_parse_string_simple_ascii(void)
{
    const char *json = "\"hello\"";
    init_parse_buffer(&input_buffer, json);

    cJSON_bool result = parse_string(&item, &input_buffer);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_NOT_NULL(item.valuestring);
    TEST_ASSERT_EQUAL_STRING("hello", item.valuestring);
}

/* 2. Empty string */
void test_parse_string_empty(void)
{
    const char *json = "\"\"";
    init_parse_buffer(&input_buffer, json);

    cJSON_bool result = parse_string(&item, &input_buffer);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_NOT_NULL(item.valuestring);
    TEST_ASSERT_EQUAL_STRING("", item.valuestring);
}

/* 3. Not a string — starts with something other than '"' */
void test_parse_string_not_a_string(void)
{
    const char *json = "hello";
    init_parse_buffer(&input_buffer, json);

    cJSON_bool result = parse_string(&item, &input_buffer);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(item.valuestring);
}

/* 4. Unterminated string (no closing quote) */
void test_parse_string_unterminated(void)
{
    const char *json = "\"hello";
    init_parse_buffer(&input_buffer, json);

    cJSON_bool result = parse_string(&item, &input_buffer);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(item.valuestring);
}

/* 5. Escape sequence: newline \n */
void test_parse_string_escape_newline(void)
{
    const char *json = "\"hello\\nworld\"";
    init_parse_buffer(&input_buffer, json);

    cJSON_bool result = parse_string(&item, &input_buffer);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_NOT_NULL(item.valuestring);
    TEST_ASSERT_EQUAL_STRING("hello\nworld", item.valuestring);
}

/* 6. Escape sequence: tab \t */
void test_parse_string_escape_tab(void)
{
    const char *json = "\"col1\\tcol2\"";
    init_parse_buffer(&input_buffer, json);

    cJSON_bool result = parse_string(&item, &input_buffer);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("col1\tcol2", item.valuestring);
}

/* 7. Escape sequence: carriage return \r */
void test_parse_string_escape_carriage_return(void)
{
    const char *json = "\"line\\rend\"";
    init_parse_buffer(&input_buffer, json);

    cJSON_bool result = parse_string(&item, &input_buffer);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("line\rend", item.valuestring);
}

/* 8. Escape sequence: backspace \b */
void test_parse_string_escape_backspace(void)
{
    const char *json = "\"a\\bb\"";
    init_parse_buffer(&input_buffer, json);

    cJSON_bool result = parse_string(&item, &input_buffer);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("a\bb", item.valuestring);
}

/* 9. Escape sequence: form feed \f */
void test_parse_string_escape_formfeed(void)
{
    const char *json = "\"a\\fb\"";
    init_parse_buffer(&input_buffer, json);

    cJSON_bool result = parse_string(&item, &input_buffer);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("a\fb", item.valuestring);
}

/* 10. Escape sequence: escaped quote \" */
void test_parse_string_escape_quote(void)
{
    const char *json = "\"say \\\"hi\\\"\"";
    init_parse_buffer(&input_buffer, json);

    cJSON_bool result = parse_string(&item, &input_buffer);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("say \"hi\"", item.valuestring);
}

/* 11. Escape sequence: escaped backslash \\ */
void test_parse_string_escape_backslash(void)
{
    const char *json = "\"path\\\\to\\\\file\"";
    init_parse_buffer(&input_buffer, json);

    cJSON_bool result = parse_string(&item, &input_buffer);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("path\\to\\file", item.valuestring);
}

/* 12. Escape sequence: escaped forward slash \/ */
void test_parse_string_escape_forward_slash(void)
{
    const char *json = "\"a\\/b\"";
    init_parse_buffer(&input_buffer, json);

    cJSON_bool result = parse_string(&item, &input_buffer);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("a/b", item.valuestring);
}

/* 13. Invalid escape sequence (e.g. \x) */
void test_parse_string_invalid_escape(void)
{
    const char *json = "\"bad\\xescape\"";
    init_parse_buffer(&input_buffer, json);

    cJSON_bool result = parse_string(&item, &input_buffer);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(item.valuestring);
}

/* 14. Backslash at end of string (buffer overflow prevention) */
void test_parse_string_backslash_at_end(void)
{
    /* The string content ends with a backslash before the closing quote,
     * but we craft a buffer where the backslash is the last character
     * (no room for the escape character). */
    const char *json = "\"abc\\";
    init_parse_buffer(&input_buffer, json);

    cJSON_bool result = parse_string(&item, &input_buffer);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(item.valuestring);
}

/* 15. UTF-16 escape sequence: basic BMP character \u0041 = 'A' */
void test_parse_string_utf16_basic(void)
{
    const char *json = "\"\\u0041\"";
    init_parse_buffer(&input_buffer, json);

    cJSON_bool result = parse_string(&item, &input_buffer);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(item.valuestring);
    TEST_ASSERT_EQUAL_STRING("A", item.valuestring);
}

/* 16. UTF-16 escape sequence: \u0000 (null character) */
void test_parse_string_utf16_null_char(void)
{
    const char *json = "\"\\u0000\"";
    init_parse_buffer(&input_buffer, json);

    cJSON_bool result = parse_string(&item, &input_buffer);

    /* \u0000 encodes to a null byte in UTF-8; parse should succeed */
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(item.valuestring);
}

/* 17. UTF-16 escape: \u00e9 = 'é' (U+00E9) */
void test_parse_string_utf16_latin_extended(void)
{
    const char *json = "\"\\u00e9\"";
    init_parse_buffer(&input_buffer, json);

    cJSON_bool result = parse_string(&item, &input_buffer);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(item.valuestring);
    /* UTF-8 encoding of U+00E9 is 0xC3 0xA9 */
    TEST_ASSERT_EQUAL_HEX(0xC3, (unsigned char)item.valuestring[0]);
    TEST_ASSERT_EQUAL_HEX(0xA9, (unsigned char)item.valuestring[1]);
    TEST_ASSERT_EQUAL_HEX(0x00, (unsigned char)item.valuestring[2]);
}

/* 18. UTF-16 surrogate pair: \uD83D\uDE00 = U+1F600 (😀) */
void test_parse_string_utf16_surrogate_pair(void)
{
    const char *json = "\"\\uD83D\\uDE00\"";
    init_parse_buffer(&input_buffer, json);

    cJSON_bool result = parse_string(&item, &input_buffer);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(item.valuestring);
    /* UTF-8 encoding of U+1F600 is F0 9F 98 80 */
    TEST_ASSERT_EQUAL_HEX(0xF0, (unsigned char)item.valuestring[0]);
    TEST_ASSERT_EQUAL_HEX(0x9F, (unsigned char)item.valuestring[1]);
    TEST_ASSERT_EQUAL_HEX(0x98, (unsigned char)item.valuestring[2]);
    TEST_ASSERT_EQUAL_HEX(0x80, (unsigned char)item.valuestring[3]);
}

/* 19. High surrogate without low surrogate — should fail */
void test_parse_string_utf16_lone_high_surrogate(void)
{
    const char *json = "\"\\uD83D\"";
    init_parse_buffer(&input_buffer, json);

    cJSON_bool result = parse_string(&item, &input_buffer);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(item.valuestring);
}

/* 20. String with multiple escape sequences */
void test_parse_string_multiple_escapes(void)
{
    const char *json = "\"\\t\\n\\r\"";
    init_parse_buffer(&input_buffer, json);

    cJSON_bool result = parse_string(&item, &input_buffer);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("\t\n\r", item.valuestring);
}

/* 21. Long string */
void test_parse_string_long_string(void)
{
    /* Build a JSON string with 1000 'a' characters */
    char json[1004];
    json[0] = '"';
    memset(json + 1, 'a', 1000);
    json[1001] = '"';
    json[1002] = '\0';

    init_parse_buffer(&input_buffer, json);

    cJSON_bool result = parse_string(&item, &input_buffer);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(item.valuestring);
    TEST_ASSERT_EQUAL_INT(1000, (int)strlen(item.valuestring));
}

/* 22. String with spaces */
void test_parse_string_with_spaces(void)
{
    const char *json = "\"hello world\"";
    init_parse_buffer(&input_buffer, json);

    cJSON_bool result = parse_string(&item, &input_buffer);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("hello world", item.valuestring);
}

/* 23. String with numbers and special chars */
void test_parse_string_alphanumeric(void)
{
    const char *json = "\"abc123!@#\"";
    init_parse_buffer(&input_buffer, json);

    cJSON_bool result = parse_string(&item, &input_buffer);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("abc123!@#", item.valuestring);
}

/* 24. Buffer offset is updated correctly after parsing */
void test_parse_string_offset_updated(void)
{
    const char *json = "\"hello\"";
    init_parse_buffer(&input_buffer, json);

    cJSON_bool result = parse_string(&item, &input_buffer);

    TEST_ASSERT_TRUE(result);
    /* offset should point past the closing quote: length of "hello" = 5, plus 2 quotes = 7, offset = 7 */
    TEST_ASSERT_EQUAL_UINT((size_t)7, input_buffer.offset);
}

/* 25. item type is set to cJSON_String on success */
void test_parse_string_sets_type(void)
{
    const char *json = "\"test\"";
    init_parse_buffer(&input_buffer, json);

    cJSON_bool result = parse_string(&item, &input_buffer);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
}

/* 26. Starts with number — not a string */
void test_parse_string_starts_with_number(void)
{
    const char *json = "42";
    init_parse_buffer(&input_buffer, json);

    cJSON_bool result = parse_string(&item, &input_buffer);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(item.valuestring);
}

/* 27. Starts with '{' — not a string */
void test_parse_string_starts_with_brace(void)
{
    const char *json = "{\"key\":\"value\"}";
    init_parse_buffer(&input_buffer, json);

    cJSON_bool result = parse_string(&item, &input_buffer);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(item.valuestring);
}

/* 28. String with only escape sequences */
void test_parse_string_only_escapes(void)
{
    const char *json = "\"\\\\\\\"\"";
    init_parse_buffer(&input_buffer, json);

    cJSON_bool result = parse_string(&item, &input_buffer);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("\\\"", item.valuestring);
}

/* 29. UTF-16 with invalid hex digits — should fail */
void test_parse_string_utf16_invalid_hex(void)
{
    const char *json = "\"\\uGGGG\"";
    init_parse_buffer(&input_buffer, json);

    cJSON_bool result = parse_string(&item, &input_buffer);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(item.valuestring);
}

/* 30. String with unicode BMP character \u0048 = 'H' */
void test_parse_string_utf16_H(void)
{
    const char *json = "\"\\u0048ello\"";
    init_parse_buffer(&input_buffer, json);

    cJSON_bool result = parse_string(&item, &input_buffer);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("Hello", item.valuestring);
}

/* -------------------------------------------------------------------------
 * main
 * ------------------------------------------------------------------------- */
int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_parse_string_simple_ascii);
    RUN_TEST(test_parse_string_empty);
    RUN_TEST(test_parse_string_not_a_string);
    RUN_TEST(test_parse_string_unterminated);
    RUN_TEST(test_parse_string_escape_newline);
    RUN_TEST(test_parse_string_escape_tab);
    RUN_TEST(test_parse_string_escape_carriage_return);
    RUN_TEST(test_parse_string_escape_backspace);
    RUN_TEST(test_parse_string_escape_formfeed);
    RUN_TEST(test_parse_string_escape_quote);
    RUN_TEST(test_parse_string_escape_backslash);
    RUN_TEST(test_parse_string_escape_forward_slash);
    RUN_TEST(test_parse_string_invalid_escape);
    RUN_TEST(test_parse_string_backslash_at_end);
    RUN_TEST(test_parse_string_utf16_basic);
    RUN_TEST(test_parse_string_utf16_null_char);
    RUN_TEST(test_parse_string_utf16_latin_extended);
    RUN_TEST(test_parse_string_utf16_surrogate_pair);
    RUN_TEST(test_parse_string_utf16_lone_high_surrogate);
    RUN_TEST(test_parse_string_multiple_escapes);
    RUN_TEST(test_parse_string_long_string);
    RUN_TEST(test_parse_string_with_spaces);
    RUN_TEST(test_parse_string_alphanumeric);
    RUN_TEST(test_parse_string_offset_updated);
    RUN_TEST(test_parse_string_sets_type);
    RUN_TEST(test_parse_string_starts_with_number);
    RUN_TEST(test_parse_string_starts_with_brace);
    RUN_TEST(test_parse_string_only_escapes);
    RUN_TEST(test_parse_string_utf16_invalid_hex);
    RUN_TEST(test_parse_string_utf16_H);
    return UNITY_END();
}