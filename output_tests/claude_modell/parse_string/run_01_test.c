#include "cJSON.c"
#include "unity.h"
#include "cJSON.h"
#include <stdlib.h>
#include <string.h>

/* File-scope variables */
static cJSON item;
static parse_buffer buffer;

/* Helper: initialize a parse_buffer with a given string */
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

/* Helper: reset the item struct */
static void reset_item(cJSON *it)
{
    memset(it, 0, sizeof(cJSON));
}

void setUp(void)
{
    reset_item(&item);
    memset(&buffer, 0, sizeof(parse_buffer));
}

void tearDown(void)
{
    if (item.valuestring != NULL)
    {
        free(item.valuestring);
        item.valuestring = NULL;
    }
}

/* ---- Test cases ---- */

/* Simple plain string */
void test_parse_string_simple(void)
{
    const char *input = "\"hello\"";
    init_parse_buffer(&buffer, input);

    cJSON_bool result = parse_string(&item, &buffer);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_NOT_NULL(item.valuestring);
    TEST_ASSERT_EQUAL_STRING("hello", item.valuestring);
}

/* Empty string */
void test_parse_string_empty(void)
{
    const char *input = "\"\"";
    init_parse_buffer(&buffer, input);

    cJSON_bool result = parse_string(&item, &buffer);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_NOT_NULL(item.valuestring);
    TEST_ASSERT_EQUAL_STRING("", item.valuestring);
}

/* String with escape sequences: \n \t \r \b \f */
void test_parse_string_escape_newline(void)
{
    const char *input = "\"line1\\nline2\"";
    init_parse_buffer(&buffer, input);

    cJSON_bool result = parse_string(&item, &buffer);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("line1\nline2", item.valuestring);
}

void test_parse_string_escape_tab(void)
{
    const char *input = "\"col1\\tcol2\"";
    init_parse_buffer(&buffer, input);

    cJSON_bool result = parse_string(&item, &buffer);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("col1\tcol2", item.valuestring);
}

void test_parse_string_escape_carriage_return(void)
{
    const char *input = "\"foo\\rbar\"";
    init_parse_buffer(&buffer, input);

    cJSON_bool result = parse_string(&item, &buffer);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("foo\rbar", item.valuestring);
}

void test_parse_string_escape_backspace(void)
{
    const char *input = "\"foo\\bbar\"";
    init_parse_buffer(&buffer, input);

    cJSON_bool result = parse_string(&item, &buffer);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("foo\bbar", item.valuestring);
}

void test_parse_string_escape_formfeed(void)
{
    const char *input = "\"foo\\fbar\"";
    init_parse_buffer(&buffer, input);

    cJSON_bool result = parse_string(&item, &buffer);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("foo\fbar", item.valuestring);
}

/* Escaped quote */
void test_parse_string_escape_quote(void)
{
    const char *input = "\"say \\\"hello\\\"\"";
    init_parse_buffer(&buffer, input);

    cJSON_bool result = parse_string(&item, &buffer);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("say \"hello\"", item.valuestring);
}

/* Escaped backslash */
void test_parse_string_escape_backslash(void)
{
    const char *input = "\"path\\\\to\\\\file\"";
    init_parse_buffer(&buffer, input);

    cJSON_bool result = parse_string(&item, &buffer);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("path\\to\\file", item.valuestring);
}

/* Escaped forward slash */
void test_parse_string_escape_forward_slash(void)
{
    const char *input = "\"a\\/b\"";
    init_parse_buffer(&buffer, input);

    cJSON_bool result = parse_string(&item, &buffer);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("a/b", item.valuestring);
}

/* Not a string: starts with something other than '"' */
void test_parse_string_not_a_string(void)
{
    const char *input = "hello";
    init_parse_buffer(&buffer, input);

    cJSON_bool result = parse_string(&item, &buffer);

    TEST_ASSERT_FALSE(result);
}

/* Unterminated string (no closing quote) */
void test_parse_string_unterminated(void)
{
    const char *input = "\"hello";
    init_parse_buffer(&buffer, input);

    cJSON_bool result = parse_string(&item, &buffer);

    TEST_ASSERT_FALSE(result);
}

/* Backslash at end of buffer (no character after backslash) */
void test_parse_string_backslash_at_end(void)
{
    const char *input = "\"hello\\";
    init_parse_buffer(&buffer, input);

    cJSON_bool result = parse_string(&item, &buffer);

    TEST_ASSERT_FALSE(result);
}

/* Invalid escape sequence */
void test_parse_string_invalid_escape(void)
{
    const char *input = "\"hello\\x\"";
    init_parse_buffer(&buffer, input);

    cJSON_bool result = parse_string(&item, &buffer);

    TEST_ASSERT_FALSE(result);
}

/* UTF-16 escape: basic ASCII character via \u */
void test_parse_string_utf16_ascii(void)
{
    /* \u0041 = 'A' */
    const char *input = "\"\\u0041\"";
    init_parse_buffer(&buffer, input);

    cJSON_bool result = parse_string(&item, &buffer);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(item.valuestring);
    TEST_ASSERT_EQUAL_STRING("A", item.valuestring);
}

/* UTF-16 escape: two-byte UTF-8 character */
void test_parse_string_utf16_two_byte(void)
{
    /* \u00E9 = é (U+00E9) */
    const char *input = "\"\\u00E9\"";
    init_parse_buffer(&buffer, input);

    cJSON_bool result = parse_string(&item, &buffer);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(item.valuestring);
    /* UTF-8 encoding of U+00E9: 0xC3 0xA9 */
    TEST_ASSERT_EQUAL_HEX(0xC3, (unsigned char)item.valuestring[0]);
    TEST_ASSERT_EQUAL_HEX(0xA9, (unsigned char)item.valuestring[1]);
    TEST_ASSERT_EQUAL_HEX(0x00, (unsigned char)item.valuestring[2]);
}

/* UTF-16 surrogate pair */
void test_parse_string_utf16_surrogate_pair(void)
{
    /* U+1F600 = 😀, encoded as surrogate pair \uD83D\uDE00 */
    const char *input = "\"\\uD83D\\uDE00\"";
    init_parse_buffer(&buffer, input);

    cJSON_bool result = parse_string(&item, &buffer);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(item.valuestring);
    /* UTF-8 encoding of U+1F600: F0 9F 98 80 */
    TEST_ASSERT_EQUAL_HEX(0xF0, (unsigned char)item.valuestring[0]);
    TEST_ASSERT_EQUAL_HEX(0x9F, (unsigned char)item.valuestring[1]);
    TEST_ASSERT_EQUAL_HEX(0x98, (unsigned char)item.valuestring[2]);
    TEST_ASSERT_EQUAL_HEX(0x80, (unsigned char)item.valuestring[3]);
}

/* Invalid UTF-16: lone high surrogate without low surrogate */
void test_parse_string_utf16_lone_high_surrogate(void)
{
    const char *input = "\"\\uD83D\"";
    init_parse_buffer(&buffer, input);

    cJSON_bool result = parse_string(&item, &buffer);

    TEST_ASSERT_FALSE(result);
}

/* Invalid UTF-16: incomplete \u sequence */
void test_parse_string_utf16_incomplete(void)
{
    const char *input = "\"\\u004\"";
    init_parse_buffer(&buffer, input);

    cJSON_bool result = parse_string(&item, &buffer);

    TEST_ASSERT_FALSE(result);
}

/* String with multiple escape sequences */
void test_parse_string_multiple_escapes(void)
{
    const char *input = "\"\\t\\n\\r\"";
    init_parse_buffer(&buffer, input);

    cJSON_bool result = parse_string(&item, &buffer);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("\t\n\r", item.valuestring);
}

/* Verify buffer offset is updated correctly after parsing */
void test_parse_string_offset_updated(void)
{
    const char *input = "\"hello\"";
    init_parse_buffer(&buffer, input);

    cJSON_bool result = parse_string(&item, &buffer);

    TEST_ASSERT_TRUE(result);
    /* offset should point past the closing quote */
    TEST_ASSERT_EQUAL_UINT(strlen("\"hello\""), buffer.offset);
}

/* String with only spaces */
void test_parse_string_spaces(void)
{
    const char *input = "\"   \"";
    init_parse_buffer(&buffer, input);

    cJSON_bool result = parse_string(&item, &buffer);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("   ", item.valuestring);
}

/* Long string */
void test_parse_string_long(void)
{
    char long_input[1024 + 3];
    char expected[1024 + 1];
    memset(expected, 'a', 1024);
    expected[1024] = '\0';

    long_input[0] = '"';
    memset(long_input + 1, 'a', 1024);
    long_input[1025] = '"';
    long_input[1026] = '\0';

    init_parse_buffer(&buffer, long_input);

    cJSON_bool result = parse_string(&item, &buffer);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING(expected, item.valuestring);
}

/* String with null byte in content (length-based, not null-terminated) */
void test_parse_string_with_unicode_null(void)
{
    /* \u0000 is a valid JSON escape for the null character */
    const char *input = "\"\\u0000\"";
    init_parse_buffer(&buffer, input);

    cJSON_bool result = parse_string(&item, &buffer);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(item.valuestring);
    /* The first byte should be 0x00 (null character encoded as UTF-8) */
    TEST_ASSERT_EQUAL_HEX(0x00, (unsigned char)item.valuestring[0]);
}

/* Verify item type is set to cJSON_String on success */
void test_parse_string_sets_type(void)
{
    const char *input = "\"test\"";
    init_parse_buffer(&buffer, input);

    cJSON_bool result = parse_string(&item, &buffer);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
}

/* Verify valuestring is NULL on failure */
void test_parse_string_valuestring_null_on_fail(void)
{
    const char *input = "not_a_string";
    init_parse_buffer(&buffer, input);

    cJSON_bool result = parse_string(&item, &buffer);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(item.valuestring);
}

/* String that is just a single character */
void test_parse_string_single_char(void)
{
    const char *input = "\"x\"";
    init_parse_buffer(&buffer, input);

    cJSON_bool result = parse_string(&item, &buffer);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("x", item.valuestring);
}

/* Three-byte UTF-8 via \u escape */
void test_parse_string_utf16_three_byte(void)
{
    /* \u4E2D = 中 (U+4E2D) */
    const char *input = "\"\\u4E2D\"";
    init_parse_buffer(&buffer, input);

    cJSON_bool result = parse_string(&item, &buffer);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(item.valuestring);
    /* UTF-8 encoding of U+4E2D: E4 B8 AD */
    TEST_ASSERT_EQUAL_HEX(0xE4, (unsigned char)item.valuestring[0]);
    TEST_ASSERT_EQUAL_HEX(0xB8, (unsigned char)item.valuestring[1]);
    TEST_ASSERT_EQUAL_HEX(0xAD, (unsigned char)item.valuestring[2]);
}

/* Input that starts with a number, not a string */
void test_parse_string_starts_with_number(void)
{
    const char *input = "42";
    init_parse_buffer(&buffer, input);

    cJSON_bool result = parse_string(&item, &buffer);

    TEST_ASSERT_FALSE(result);
}

/* String with escaped backslash followed by n (should be backslash then newline) */
void test_parse_string_backslash_then_newline(void)
{
    const char *input = "\"\\\\n\"";
    init_parse_buffer(&buffer, input);

    cJSON_bool result = parse_string(&item, &buffer);

    TEST_ASSERT_TRUE(result);
    /* \\n in JSON = literal backslash followed by 'n' */
    TEST_ASSERT_EQUAL_STRING("\\n", item.valuestring);
}

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_parse_string_simple);
    RUN_TEST(test_parse_string_empty);
    RUN_TEST(test_parse_string_escape_newline);
    RUN_TEST(test_parse_string_escape_tab);
    RUN_TEST(test_parse_string_escape_carriage_return);
    RUN_TEST(test_parse_string_escape_backspace);
    RUN_TEST(test_parse_string_escape_formfeed);
    RUN_TEST(test_parse_string_escape_quote);
    RUN_TEST(test_parse_string_escape_backslash);
    RUN_TEST(test_parse_string_escape_forward_slash);
    RUN_TEST(test_parse_string_not_a_string);
    RUN_TEST(test_parse_string_unterminated);
    RUN_TEST(test_parse_string_backslash_at_end);
    RUN_TEST(test_parse_string_invalid_escape);
    RUN_TEST(test_parse_string_utf16_ascii);
    RUN_TEST(test_parse_string_utf16_two_byte);
    RUN_TEST(test_parse_string_utf16_surrogate_pair);
    RUN_TEST(test_parse_string_utf16_lone_high_surrogate);
    RUN_TEST(test_parse_string_utf16_incomplete);
    RUN_TEST(test_parse_string_multiple_escapes);
    RUN_TEST(test_parse_string_offset_updated);
    RUN_TEST(test_parse_string_spaces);
    RUN_TEST(test_parse_string_long);
    RUN_TEST(test_parse_string_with_unicode_null);
    RUN_TEST(test_parse_string_sets_type);
    RUN_TEST(test_parse_string_valuestring_null_on_fail);
    RUN_TEST(test_parse_string_single_char);
    RUN_TEST(test_parse_string_utf16_three_byte);
    RUN_TEST(test_parse_string_starts_with_number);
    RUN_TEST(test_parse_string_backslash_then_newline);
    return UNITY_END();
}