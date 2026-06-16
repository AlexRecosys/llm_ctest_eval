#include "cJSON.c"
#include "unity.h"
#include "cJSON.h"
#include <string.h>
#include <stdlib.h>

/* ===== setUp / tearDown ===== */

void setUp(void)
{
}

void tearDown(void)
{
}

/* ===== helper macro ===== */

#define MINIFY_AND_CHECK(input, expected)               \
    do {                                                \
        char buf[1024];                                 \
        strncpy(buf, (input), sizeof(buf) - 1);        \
        buf[sizeof(buf) - 1] = '\0';                   \
        cJSON_Minify(buf);                              \
        TEST_ASSERT_EQUAL_STRING((expected), buf);      \
    } while (0)

/* ===== test cases ===== */

void test_minify_null_pointer_does_not_crash(void)
{
    /* Should return immediately without crashing */
    cJSON_Minify(NULL);
    TEST_ASSERT_TRUE(1); /* reached here without crash */
}

void test_minify_empty_string(void)
{
    char buf[] = "";
    cJSON_Minify(buf);
    TEST_ASSERT_EQUAL_STRING("", buf);
}

void test_minify_no_whitespace(void)
{
    MINIFY_AND_CHECK("{\"a\":1}", "{\"a\":1}");
}

void test_minify_spaces_removed(void)
{
    MINIFY_AND_CHECK("{ \"a\" : 1 }", "{\"a\":1}");
}

void test_minify_tabs_removed(void)
{
    MINIFY_AND_CHECK("{\t\"a\"\t:\t1\t}", "{\"a\":1}");
}

void test_minify_newlines_removed(void)
{
    MINIFY_AND_CHECK("{\n\"a\"\n:\n1\n}", "{\"a\":1}");
}

void test_minify_carriage_returns_removed(void)
{
    MINIFY_AND_CHECK("{\r\"a\"\r:\r1\r}", "{\"a\":1}");
}

void test_minify_mixed_whitespace_removed(void)
{
    MINIFY_AND_CHECK("{\r\n\t \"a\" \r\n\t : \r\n\t 1 \r\n\t}", "{\"a\":1}");
}

void test_minify_oneline_comment_removed(void)
{
    MINIFY_AND_CHECK("{// comment\n\"a\":1}", "{\"a\":1}");
}

void test_minify_oneline_comment_at_end(void)
{
    MINIFY_AND_CHECK("{\"a\":1}// trailing comment", "{\"a\":1}");
}

void test_minify_multiline_comment_removed(void)
{
    MINIFY_AND_CHECK("{/* comment */\"a\":1}", "{\"a\":1}");
}

void test_minify_multiline_comment_spanning_lines(void)
{
    MINIFY_AND_CHECK("{/* line1\nline2\nline3 */\"a\":1}", "{\"a\":1}");
}

void test_minify_string_whitespace_preserved(void)
{
    MINIFY_AND_CHECK("{\"key\":\"hello world\"}", "{\"key\":\"hello world\"}");
}

void test_minify_string_with_internal_spaces_preserved(void)
{
    MINIFY_AND_CHECK("{ \"key\" : \"  spaces  \" }", "{\"key\":\"  spaces  \"}");
}

void test_minify_string_with_escaped_quote(void)
{
    MINIFY_AND_CHECK("{ \"key\" : \"he said \\\"hi\\\"\" }", "{\"key\":\"he said \\\"hi\\\"\"}");
}

void test_minify_string_with_escaped_backslash(void)
{
    MINIFY_AND_CHECK("{ \"key\" : \"back\\\\slash\" }", "{\"key\":\"back\\\\slash\"}");
}

void test_minify_array_with_whitespace(void)
{
    MINIFY_AND_CHECK("[ 1 , 2 , 3 ]", "[1,2,3]");
}

void test_minify_nested_object(void)
{
    MINIFY_AND_CHECK("{ \"a\" : { \"b\" : 2 } }", "{\"a\":{\"b\":2}}");
}

void test_minify_nested_array(void)
{
    MINIFY_AND_CHECK("[ [ 1 , 2 ] , [ 3 , 4 ] ]", "[[1,2],[3,4]]");
}

void test_minify_boolean_values(void)
{
    MINIFY_AND_CHECK("{ \"a\" : true , \"b\" : false }", "{\"a\":true,\"b\":false}");
}

void test_minify_null_value(void)
{
    MINIFY_AND_CHECK("{ \"a\" : null }", "{\"a\":null}");
}

void test_minify_number_values(void)
{
    MINIFY_AND_CHECK("{ \"a\" : 3.14 , \"b\" : -1 }", "{\"a\":3.14,\"b\":-1}");
}

void test_minify_only_whitespace(void)
{
    MINIFY_AND_CHECK("   \t\r\n   ", "");
}

void test_minify_only_oneline_comment(void)
{
    MINIFY_AND_CHECK("// just a comment\n", "");
}

void test_minify_only_multiline_comment(void)
{
    MINIFY_AND_CHECK("/* just a comment */", "");
}

void test_minify_forward_slash_not_comment(void)
{
    /* A single '/' not followed by '/' or '*' should be kept */
    MINIFY_AND_CHECK("{\"a\":1/2}", "{\"a\":1/2}");
}

void test_minify_multiple_oneline_comments(void)
{
    MINIFY_AND_CHECK("// c1\n{// c2\n\"a\":1// c3\n}", "{\"a\":1}");
}

void test_minify_multiple_multiline_comments(void)
{
    MINIFY_AND_CHECK("/* c1 */{/* c2 */\"a\":/* c3 */1}", "{\"a\":1}");
}

void test_minify_string_containing_comment_like_text(void)
{
    /* Comments inside strings must NOT be stripped */
    MINIFY_AND_CHECK("{\"key\":\"// not a comment\"}", "{\"key\":\"// not a comment\"}");
}

void test_minify_string_containing_multiline_comment_like_text(void)
{
    MINIFY_AND_CHECK("{\"key\":\"/* not a comment */\"}", "{\"key\":\"/* not a comment */\"}");
}

void test_minify_complex_json(void)
{
    const char *input =
        "{\n"
        "    \"name\" : \"John\",\n"
        "    \"age\"  : 30,\n"
        "    \"cars\" : [ \"Ford\", \"BMW\", \"Fiat\" ]\n"
        "}\n";
    const char *expected = "{\"name\":\"John\",\"age\":30,\"cars\":[\"Ford\",\"BMW\",\"Fiat\"]}";
    MINIFY_AND_CHECK(input, expected);
}

void test_minify_string_with_newline_escape(void)
{
    /* \n inside a string literal (escaped) should be preserved */
    MINIFY_AND_CHECK("{ \"key\" : \"line1\\nline2\" }", "{\"key\":\"line1\\nline2\"}");
}

void test_minify_result_is_valid_json(void)
{
    char buf[] = "{ \"a\" : 1 , \"b\" : [ 2 , 3 ] }";
    cJSON_Minify(buf);
    cJSON *parsed = cJSON_Parse(buf);
    TEST_ASSERT_NOT_NULL(parsed);
    cJSON_Delete(parsed);
}

void test_minify_idempotent(void)
{
    char buf1[256];
    char buf2[256];
    const char *input = "{ \"a\" : 1 , \"b\" : [ 2 , 3 ] }";

    strncpy(buf1, input, sizeof(buf1) - 1);
    buf1[sizeof(buf1) - 1] = '\0';
    cJSON_Minify(buf1);

    strncpy(buf2, buf1, sizeof(buf2) - 1);
    buf2[sizeof(buf2) - 1] = '\0';
    cJSON_Minify(buf2);

    TEST_ASSERT_EQUAL_STRING(buf1, buf2);
}

void test_minify_single_value_true(void)
{
    MINIFY_AND_CHECK("  true  ", "true");
}

void test_minify_single_value_false(void)
{
    MINIFY_AND_CHECK("  false  ", "false");
}

void test_minify_single_value_null(void)
{
    MINIFY_AND_CHECK("  null  ", "null");
}

void test_minify_single_number(void)
{
    MINIFY_AND_CHECK("  42  ", "42");
}

void test_minify_string_with_unicode_escape(void)
{
    MINIFY_AND_CHECK("{ \"key\" : \"\\u0041\" }", "{\"key\":\"\\u0041\"}");
}

/* ===== main ===== */

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_minify_null_pointer_does_not_crash);
    RUN_TEST(test_minify_empty_string);
    RUN_TEST(test_minify_no_whitespace);
    RUN_TEST(test_minify_spaces_removed);
    RUN_TEST(test_minify_tabs_removed);
    RUN_TEST(test_minify_newlines_removed);
    RUN_TEST(test_minify_carriage_returns_removed);
    RUN_TEST(test_minify_mixed_whitespace_removed);
    RUN_TEST(test_minify_oneline_comment_removed);
    RUN_TEST(test_minify_oneline_comment_at_end);
    RUN_TEST(test_minify_multiline_comment_removed);
    RUN_TEST(test_minify_multiline_comment_spanning_lines);
    RUN_TEST(test_minify_string_whitespace_preserved);
    RUN_TEST(test_minify_string_with_internal_spaces_preserved);
    RUN_TEST(test_minify_string_with_escaped_quote);
    RUN_TEST(test_minify_string_with_escaped_backslash);
    RUN_TEST(test_minify_array_with_whitespace);
    RUN_TEST(test_minify_nested_object);
    RUN_TEST(test_minify_nested_array);
    RUN_TEST(test_minify_boolean_values);
    RUN_TEST(test_minify_null_value);
    RUN_TEST(test_minify_number_values);
    RUN_TEST(test_minify_only_whitespace);
    RUN_TEST(test_minify_only_oneline_comment);
    RUN_TEST(test_minify_only_multiline_comment);
    RUN_TEST(test_minify_forward_slash_not_comment);
    RUN_TEST(test_minify_multiple_oneline_comments);
    RUN_TEST(test_minify_multiple_multiline_comments);
    RUN_TEST(test_minify_string_containing_comment_like_text);
    RUN_TEST(test_minify_string_containing_multiline_comment_like_text);
    RUN_TEST(test_minify_complex_json);
    RUN_TEST(test_minify_string_with_newline_escape);
    RUN_TEST(test_minify_result_is_valid_json);
    RUN_TEST(test_minify_idempotent);
    RUN_TEST(test_minify_single_value_true);
    RUN_TEST(test_minify_single_value_false);
    RUN_TEST(test_minify_single_value_null);
    RUN_TEST(test_minify_single_number);
    RUN_TEST(test_minify_string_with_unicode_escape);
    return UNITY_END();
}