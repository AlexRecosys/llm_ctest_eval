#include "cJSON.c"
#include "unity.h"
#include "cJSON.h"
#include <string.h>
#include <stdlib.h>

/* -------------------------------------------------------------------------
 * Helper macro – copy a string literal into a stack buffer so cJSON_Minify
 * can modify it in-place (it must not point to read-only memory).
 * ------------------------------------------------------------------------- */
#define MAKE_BUF(name, literal) \
    char name[sizeof(literal)]; \
    memcpy(name, literal, sizeof(literal))

/* -------------------------------------------------------------------------
 * setUp / tearDown
 * ------------------------------------------------------------------------- */
void setUp(void)
{
    /* nothing needed */
}

void tearDown(void)
{
    /* nothing needed */
}

/* =========================================================================
 * Test cases
 * ========================================================================= */

/* NULL input must not crash */
void test_minify_null_input(void)
{
    cJSON_Minify(NULL);
    /* reaching here means no crash / segfault */
    TEST_ASSERT_TRUE(1);
}

/* Empty string stays empty */
void test_minify_empty_string(void)
{
    MAKE_BUF(buf, "");
    cJSON_Minify(buf);
    TEST_ASSERT_EQUAL_STRING("", buf);
}

/* String with only whitespace becomes empty */
void test_minify_only_spaces(void)
{
    MAKE_BUF(buf, "   ");
    cJSON_Minify(buf);
    TEST_ASSERT_EQUAL_STRING("", buf);
}

/* Mixed whitespace characters become empty */
void test_minify_only_whitespace_mixed(void)
{
    MAKE_BUF(buf, " \t\r\n \t");
    cJSON_Minify(buf);
    TEST_ASSERT_EQUAL_STRING("", buf);
}

/* Already minified JSON is unchanged */
void test_minify_already_minified(void)
{
    MAKE_BUF(buf, "{\"a\":1}");
    cJSON_Minify(buf);
    TEST_ASSERT_EQUAL_STRING("{\"a\":1}", buf);
}

/* Leading and trailing whitespace is removed */
void test_minify_leading_trailing_whitespace(void)
{
    MAKE_BUF(buf, "  {\"a\":1}  ");
    cJSON_Minify(buf);
    TEST_ASSERT_EQUAL_STRING("{\"a\":1}", buf);
}

/* Whitespace between tokens is removed */
void test_minify_whitespace_between_tokens(void)
{
    MAKE_BUF(buf, "{ \"a\" : 1 }");
    cJSON_Minify(buf);
    TEST_ASSERT_EQUAL_STRING("{\"a\":1}", buf);
}

/* Newlines and tabs between tokens are removed */
void test_minify_newlines_and_tabs(void)
{
    MAKE_BUF(buf, "{\n\t\"a\"\t:\n1\n}");
    cJSON_Minify(buf);
    TEST_ASSERT_EQUAL_STRING("{\"a\":1}", buf);
}

/* Whitespace inside a string value must be preserved */
void test_minify_whitespace_inside_string_preserved(void)
{
    MAKE_BUF(buf, "{ \"key\" : \"hello world\" }");
    cJSON_Minify(buf);
    TEST_ASSERT_EQUAL_STRING("{\"key\":\"hello world\"}", buf);
}

/* Escaped quote inside a string must not confuse the parser */
void test_minify_escaped_quote_in_string(void)
{
    MAKE_BUF(buf, "{ \"k\" : \"say \\\"hi\\\"\" }");
    cJSON_Minify(buf);
    TEST_ASSERT_EQUAL_STRING("{\"k\":\"say \\\"hi\\\"\"}", buf);
}

/* Escaped backslash followed by quote inside a string */
void test_minify_escaped_backslash_then_quote(void)
{
    MAKE_BUF(buf, "{ \"k\" : \"a\\\\\\\"b\" }");
    cJSON_Minify(buf);
    TEST_ASSERT_EQUAL_STRING("{\"k\":\"a\\\\\\\"b\"}", buf);
}

/* Single-line comment is removed */
void test_minify_single_line_comment(void)
{
    MAKE_BUF(buf, "{// comment\n\"a\":1}");
    cJSON_Minify(buf);
    TEST_ASSERT_EQUAL_STRING("{\"a\":1}", buf);
}

/* Single-line comment at end of input (no trailing newline) */
void test_minify_single_line_comment_at_end(void)
{
    MAKE_BUF(buf, "{\"a\":1}// trailing comment");
    cJSON_Minify(buf);
    TEST_ASSERT_EQUAL_STRING("{\"a\":1}", buf);
}

/* Multi-line comment is removed */
void test_minify_multiline_comment(void)
{
    MAKE_BUF(buf, "{/* comment */\"a\":1}");
    cJSON_Minify(buf);
    TEST_ASSERT_EQUAL_STRING("{\"a\":1}", buf);
}

/* Multi-line comment spanning multiple lines */
void test_minify_multiline_comment_with_newlines(void)
{
    MAKE_BUF(buf, "{\n/* line1\nline2 */\n\"a\":1\n}");
    cJSON_Minify(buf);
    TEST_ASSERT_EQUAL_STRING("{\"a\":1}", buf);
}

/* Multiple comments */
void test_minify_multiple_comments(void)
{
    MAKE_BUF(buf, "// c1\n{/* c2 */\"a\":1// c3\n}");
    cJSON_Minify(buf);
    TEST_ASSERT_EQUAL_STRING("{\"a\":1}", buf);
}

/* A lone '/' that is not a comment start is kept */
void test_minify_lone_slash_kept(void)
{
    /* A bare '/' is not valid JSON but the function should keep it */
    MAKE_BUF(buf, "{\"a\":1/2}");
    cJSON_Minify(buf);
    TEST_ASSERT_EQUAL_STRING("{\"a\":1/2}", buf);
}

/* Array with whitespace */
void test_minify_array_with_whitespace(void)
{
    MAKE_BUF(buf, "[ 1 , 2 , 3 ]");
    cJSON_Minify(buf);
    TEST_ASSERT_EQUAL_STRING("[1,2,3]", buf);
}

/* Nested object with whitespace */
void test_minify_nested_object(void)
{
    MAKE_BUF(buf, "{ \"a\" : { \"b\" : 2 } }");
    cJSON_Minify(buf);
    TEST_ASSERT_EQUAL_STRING("{\"a\":{\"b\":2}}", buf);
}

/* Boolean and null values */
void test_minify_boolean_and_null(void)
{
    MAKE_BUF(buf, "{ \"t\" : true , \"f\" : false , \"n\" : null }");
    cJSON_Minify(buf);
    TEST_ASSERT_EQUAL_STRING("{\"t\":true,\"f\":false,\"n\":null}", buf);
}

/* String containing a comment-like sequence is preserved */
void test_minify_comment_like_sequence_in_string(void)
{
    MAKE_BUF(buf, "{ \"url\" : \"http://example.com\" }");
    cJSON_Minify(buf);
    TEST_ASSERT_EQUAL_STRING("{\"url\":\"http://example.com\"}", buf);
}

/* String containing /* sequence is preserved */
void test_minify_multiline_comment_like_in_string(void)
{
    MAKE_BUF(buf, "{ \"v\" : \"a /* b */ c\" }");
    cJSON_Minify(buf);
    TEST_ASSERT_EQUAL_STRING("{\"v\":\"a /* b */ c\"}", buf);
}

/* Carriage-return only whitespace */
void test_minify_carriage_return_whitespace(void)
{
    MAKE_BUF(buf, "{\r\"a\"\r:\r1\r}");
    cJSON_Minify(buf);
    TEST_ASSERT_EQUAL_STRING("{\"a\":1}", buf);
}

/* Result is still parseable by cJSON_Parse after minification */
void test_minify_result_is_parseable(void)
{
    MAKE_BUF(buf, "{\n  \"name\" : \"Alice\" ,\n  \"age\" : 30\n}");
    cJSON_Minify(buf);
    TEST_ASSERT_EQUAL_STRING("{\"name\":\"Alice\",\"age\":30}", buf);

    cJSON *parsed = cJSON_Parse(buf);
    TEST_ASSERT_NOT_NULL(parsed);
    cJSON_Delete(parsed);
}

/* String with tab inside is preserved */
void test_minify_tab_inside_string_preserved(void)
{
    MAKE_BUF(buf, "{ \"k\" : \"a\tb\" }");
    cJSON_Minify(buf);
    TEST_ASSERT_EQUAL_STRING("{\"k\":\"a\tb\"}", buf);
}

/* Empty object with whitespace */
void test_minify_empty_object_with_whitespace(void)
{
    MAKE_BUF(buf, "{  }");
    cJSON_Minify(buf);
    TEST_ASSERT_EQUAL_STRING("{}", buf);
}

/* Empty array with whitespace */
void test_minify_empty_array_with_whitespace(void)
{
    MAKE_BUF(buf, "[  ]");
    cJSON_Minify(buf);
    TEST_ASSERT_EQUAL_STRING("[]", buf);
}

/* Single string value */
void test_minify_single_string_value(void)
{
    MAKE_BUF(buf, "  \"hello world\"  ");
    cJSON_Minify(buf);
    TEST_ASSERT_EQUAL_STRING("\"hello world\"", buf);
}

/* Single number value */
void test_minify_single_number_value(void)
{
    MAKE_BUF(buf, "  42  ");
    cJSON_Minify(buf);
    TEST_ASSERT_EQUAL_STRING("42", buf);
}

/* =========================================================================
 * main
 * ========================================================================= */
int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_minify_null_input);
    RUN_TEST(test_minify_empty_string);
    RUN_TEST(test_minify_only_spaces);
    RUN_TEST(test_minify_only_whitespace_mixed);
    RUN_TEST(test_minify_already_minified);
    RUN_TEST(test_minify_leading_trailing_whitespace);
    RUN_TEST(test_minify_whitespace_between_tokens);
    RUN_TEST(test_minify_newlines_and_tabs);
    RUN_TEST(test_minify_whitespace_inside_string_preserved);
    RUN_TEST(test_minify_escaped_quote_in_string);
    RUN_TEST(test_minify_escaped_backslash_then_quote);
    RUN_TEST(test_minify_single_line_comment);
    RUN_TEST(test_minify_single_line_comment_at_end);
    RUN_TEST(test_minify_multiline_comment);
    RUN_TEST(test_minify_multiline_comment_with_newlines);
    RUN_TEST(test_minify_multiple_comments);
    RUN_TEST(test_minify_lone_slash_kept);
    RUN_TEST(test_minify_array_with_whitespace);
    RUN_TEST(test_minify_nested_object);
    RUN_TEST(test_minify_boolean_and_null);
    RUN_TEST(test_minify_comment_like_sequence_in_string);
    RUN_TEST(test_minify_multiline_comment_like_in_string);
    RUN_TEST(test_minify_carriage_return_whitespace);
    RUN_TEST(test_minify_result_is_parseable);
    RUN_TEST(test_minify_tab_inside_string_preserved);
    RUN_TEST(test_minify_empty_object_with_whitespace);
    RUN_TEST(test_minify_empty_array_with_whitespace);
    RUN_TEST(test_minify_single_string_value);
    RUN_TEST(test_minify_single_number_value);
    return UNITY_END();
}