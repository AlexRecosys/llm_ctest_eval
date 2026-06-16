#include "cJSON.c"
#include "unity.h"
#include "cJSON.h"
#include <string.h>
#include <stdlib.h>

/* -------------------------------------------------------------------------
 * Helper macros / functions
 * ---------------------------------------------------------------------- */

static char *make_mutable(const char *src)
{
    size_t len = strlen(src);
    char *buf = (char *)malloc(len + 1);
    if (buf == NULL)
    {
        return NULL;
    }
    memcpy(buf, src, len + 1);
    return buf;
}

/* -------------------------------------------------------------------------
 * setUp / tearDown
 * ---------------------------------------------------------------------- */

void setUp(void)
{
    /* nothing needed */
}

void tearDown(void)
{
    /* nothing needed */
}

/* -------------------------------------------------------------------------
 * Test cases
 * ---------------------------------------------------------------------- */

void test_minify_null_pointer_does_not_crash(void)
{
    /* Should return immediately without crashing */
    cJSON_Minify(NULL);
    TEST_PASS();
}

void test_minify_empty_string(void)
{
    char buf[] = "";
    cJSON_Minify(buf);
    TEST_ASSERT_EQUAL_STRING("", buf);
}

void test_minify_no_whitespace(void)
{
    char buf[] = "{\"key\":\"value\"}";
    cJSON_Minify(buf);
    TEST_ASSERT_EQUAL_STRING("{\"key\":\"value\"}", buf);
}

void test_minify_removes_spaces(void)
{
    char *buf = make_mutable("{ \"key\" : \"value\" }");
    TEST_ASSERT_NOT_NULL(buf);
    cJSON_Minify(buf);
    TEST_ASSERT_EQUAL_STRING("{\"key\":\"value\"}", buf);
    free(buf);
}

void test_minify_removes_tabs(void)
{
    char *buf = make_mutable("{\t\"key\"\t:\t\"value\"\t}");
    TEST_ASSERT_NOT_NULL(buf);
    cJSON_Minify(buf);
    TEST_ASSERT_EQUAL_STRING("{\"key\":\"value\"}", buf);
    free(buf);
}

void test_minify_removes_newlines(void)
{
    char *buf = make_mutable("{\n\"key\"\n:\n\"value\"\n}");
    TEST_ASSERT_NOT_NULL(buf);
    cJSON_Minify(buf);
    TEST_ASSERT_EQUAL_STRING("{\"key\":\"value\"}", buf);
    free(buf);
}

void test_minify_removes_carriage_returns(void)
{
    char *buf = make_mutable("{\r\"key\"\r:\r\"value\"\r}");
    TEST_ASSERT_NOT_NULL(buf);
    cJSON_Minify(buf);
    TEST_ASSERT_EQUAL_STRING("{\"key\":\"value\"}", buf);
    free(buf);
}

void test_minify_removes_mixed_whitespace(void)
{
    char *buf = make_mutable(" \t\r\n{ \t\r\n\"key\" \t\r\n: \t\r\n\"value\" \t\r\n} \t\r\n");
    TEST_ASSERT_NOT_NULL(buf);
    cJSON_Minify(buf);
    TEST_ASSERT_EQUAL_STRING("{\"key\":\"value\"}", buf);
    free(buf);
}

void test_minify_preserves_whitespace_inside_strings(void)
{
    char *buf = make_mutable("{ \"key\" : \"hello world\" }");
    TEST_ASSERT_NOT_NULL(buf);
    cJSON_Minify(buf);
    TEST_ASSERT_EQUAL_STRING("{\"key\":\"hello world\"}", buf);
    free(buf);
}

void test_minify_preserves_tabs_inside_strings(void)
{
    char *buf = make_mutable("{ \"key\" : \"hello\tworld\" }");
    TEST_ASSERT_NOT_NULL(buf);
    cJSON_Minify(buf);
    TEST_ASSERT_EQUAL_STRING("{\"key\":\"hello\tworld\"}", buf);
    free(buf);
}

void test_minify_preserves_newlines_inside_strings(void)
{
    char *buf = make_mutable("{ \"key\" : \"hello\\nworld\" }");
    TEST_ASSERT_NOT_NULL(buf);
    cJSON_Minify(buf);
    TEST_ASSERT_EQUAL_STRING("{\"key\":\"hello\\nworld\"}", buf);
    free(buf);
}

void test_minify_removes_single_line_comment(void)
{
    char *buf = make_mutable("{\"key\":\"value\"// this is a comment\n}");
    TEST_ASSERT_NOT_NULL(buf);
    cJSON_Minify(buf);
    TEST_ASSERT_EQUAL_STRING("{\"key\":\"value\"}", buf);
    free(buf);
}

void test_minify_removes_multiline_comment(void)
{
    char *buf = make_mutable("{\"key\":\"value\"/* this is\na comment */}");
    TEST_ASSERT_NOT_NULL(buf);
    cJSON_Minify(buf);
    TEST_ASSERT_EQUAL_STRING("{\"key\":\"value\"}", buf);
    free(buf);
}

void test_minify_removes_multiple_single_line_comments(void)
{
    char *buf = make_mutable("// comment1\n{// comment2\n\"key\":\"value\"// comment3\n}");
    TEST_ASSERT_NOT_NULL(buf);
    cJSON_Minify(buf);
    TEST_ASSERT_EQUAL_STRING("{\"key\":\"value\"}", buf);
    free(buf);
}

void test_minify_removes_multiple_multiline_comments(void)
{
    char *buf = make_mutable("/* c1 */{/* c2 */\"key\":\"value\"/* c3 */}");
    TEST_ASSERT_NOT_NULL(buf);
    cJSON_Minify(buf);
    TEST_ASSERT_EQUAL_STRING("{\"key\":\"value\"}", buf);
    free(buf);
}

void test_minify_single_slash_not_comment(void)
{
    /* A single '/' that is not followed by '/' or '*' should be kept */
    char *buf = make_mutable("{\"key\":1/2}");
    TEST_ASSERT_NOT_NULL(buf);
    cJSON_Minify(buf);
    TEST_ASSERT_EQUAL_STRING("{\"key\":1/2}", buf);
    free(buf);
}

void test_minify_complex_object(void)
{
    char *buf = make_mutable(
        "{\n"
        "    \"name\" : \"John\",\n"
        "    \"age\"  : 30,\n"
        "    \"city\" : \"New York\"\n"
        "}\n");
    TEST_ASSERT_NOT_NULL(buf);
    cJSON_Minify(buf);
    TEST_ASSERT_EQUAL_STRING("{\"name\":\"John\",\"age\":30,\"city\":\"New York\"}", buf);
    free(buf);
}

void test_minify_array(void)
{
    char *buf = make_mutable("[ 1 , 2 , 3 ]");
    TEST_ASSERT_NOT_NULL(buf);
    cJSON_Minify(buf);
    TEST_ASSERT_EQUAL_STRING("[1,2,3]", buf);
    free(buf);
}

void test_minify_nested_object(void)
{
    char *buf = make_mutable("{ \"outer\" : { \"inner\" : 42 } }");
    TEST_ASSERT_NOT_NULL(buf);
    cJSON_Minify(buf);
    TEST_ASSERT_EQUAL_STRING("{\"outer\":{\"inner\":42}}", buf);
    free(buf);
}

void test_minify_boolean_values(void)
{
    char *buf = make_mutable("{ \"a\" : true , \"b\" : false }");
    TEST_ASSERT_NOT_NULL(buf);
    cJSON_Minify(buf);
    TEST_ASSERT_EQUAL_STRING("{\"a\":true,\"b\":false}", buf);
    free(buf);
}

void test_minify_null_value(void)
{
    char *buf = make_mutable("{ \"a\" : null }");
    TEST_ASSERT_NOT_NULL(buf);
    cJSON_Minify(buf);
    TEST_ASSERT_EQUAL_STRING("{\"a\":null}", buf);
    free(buf);
}

void test_minify_string_with_escaped_quote(void)
{
    char *buf = make_mutable("{ \"key\" : \"val\\\"ue\" }");
    TEST_ASSERT_NOT_NULL(buf);
    cJSON_Minify(buf);
    TEST_ASSERT_EQUAL_STRING("{\"key\":\"val\\\"ue\"}", buf);
    free(buf);
}

void test_minify_string_with_escaped_backslash(void)
{
    char *buf = make_mutable("{ \"key\" : \"val\\\\ue\" }");
    TEST_ASSERT_NOT_NULL(buf);
    cJSON_Minify(buf);
    TEST_ASSERT_EQUAL_STRING("{\"key\":\"val\\\\ue\"}", buf);
    free(buf);
}

void test_minify_only_whitespace(void)
{
    char *buf = make_mutable("   \t\r\n   ");
    TEST_ASSERT_NOT_NULL(buf);
    cJSON_Minify(buf);
    TEST_ASSERT_EQUAL_STRING("", buf);
    free(buf);
}

void test_minify_only_single_line_comment(void)
{
    char *buf = make_mutable("// just a comment\n");
    TEST_ASSERT_NOT_NULL(buf);
    cJSON_Minify(buf);
    TEST_ASSERT_EQUAL_STRING("", buf);
    free(buf);
}

void test_minify_only_multiline_comment(void)
{
    char *buf = make_mutable("/* just a comment */");
    TEST_ASSERT_NOT_NULL(buf);
    cJSON_Minify(buf);
    TEST_ASSERT_EQUAL_STRING("", buf);
    free(buf);
}

void test_minify_result_is_valid_json(void)
{
    char *buf = make_mutable(
        "{\n"
        "    \"name\" : \"Alice\",\n"
        "    \"scores\" : [ 10 , 20 , 30 ]\n"
        "}\n");
    TEST_ASSERT_NOT_NULL(buf);
    cJSON_Minify(buf);

    /* The minified result should still parse as valid JSON */
    cJSON *parsed = cJSON_Parse(buf);
    TEST_ASSERT_NOT_NULL_MESSAGE(parsed, "Minified JSON should still be parseable");
    cJSON_Delete(parsed);
    free(buf);
}

void test_minify_single_number(void)
{
    char *buf = make_mutable("  42  ");
    TEST_ASSERT_NOT_NULL(buf);
    cJSON_Minify(buf);
    TEST_ASSERT_EQUAL_STRING("42", buf);
    free(buf);
}

void test_minify_single_string(void)
{
    char *buf = make_mutable("  \"hello\"  ");
    TEST_ASSERT_NOT_NULL(buf);
    cJSON_Minify(buf);
    TEST_ASSERT_EQUAL_STRING("\"hello\"", buf);
    free(buf);
}

void test_minify_comment_inside_whitespace(void)
{
    char *buf = make_mutable("{ /* comment */ \"key\" : /* another */ \"value\" }");
    TEST_ASSERT_NOT_NULL(buf);
    cJSON_Minify(buf);
    TEST_ASSERT_EQUAL_STRING("{\"key\":\"value\"}", buf);
    free(buf);
}

void test_minify_string_containing_comment_like_text(void)
{
    /* Comment-like sequences inside strings must NOT be treated as comments */
    char *buf = make_mutable("{ \"key\" : \"http://example.com\" }");
    TEST_ASSERT_NOT_NULL(buf);
    cJSON_Minify(buf);
    TEST_ASSERT_EQUAL_STRING("{\"key\":\"http://example.com\"}", buf);
    free(buf);
}

void test_minify_string_containing_multiline_comment_like_text(void)
{
    char *buf = make_mutable("{ \"key\" : \"/* not a comment */\" }");
    TEST_ASSERT_NOT_NULL(buf);
    cJSON_Minify(buf);
    TEST_ASSERT_EQUAL_STRING("{\"key\":\"/* not a comment */\"}", buf);
    free(buf);
}

void test_minify_deeply_nested(void)
{
    char *buf = make_mutable("{ \"a\" : { \"b\" : { \"c\" : [ 1 , 2 , 3 ] } } }");
    TEST_ASSERT_NOT_NULL(buf);
    cJSON_Minify(buf);
    TEST_ASSERT_EQUAL_STRING("{\"a\":{\"b\":{\"c\":[1,2,3]}}}", buf);
    free(buf);
}

void test_minify_null_terminated_after_minify(void)
{
    char *buf = make_mutable("  { \"x\" : 1 }  ");
    TEST_ASSERT_NOT_NULL(buf);
    cJSON_Minify(buf);
    /* Verify null termination by checking strlen matches expected */
    TEST_ASSERT_EQUAL_INT((int)strlen("{\"x\":1}"), (int)strlen(buf));
    TEST_ASSERT_EQUAL_STRING("{\"x\":1}", buf);
    free(buf);
}

/* -------------------------------------------------------------------------
 * main
 * ---------------------------------------------------------------------- */

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_minify_null_pointer_does_not_crash);
    RUN_TEST(test_minify_empty_string);
    RUN_TEST(test_minify_no_whitespace);
    RUN_TEST(test_minify_removes_spaces);
    RUN_TEST(test_minify_removes_tabs);
    RUN_TEST(test_minify_removes_newlines);
    RUN_TEST(test_minify_removes_carriage_returns);
    RUN_TEST(test_minify_removes_mixed_whitespace);
    RUN_TEST(test_minify_preserves_whitespace_inside_strings);
    RUN_TEST(test_minify_preserves_tabs_inside_strings);
    RUN_TEST(test_minify_preserves_newlines_inside_strings);
    RUN_TEST(test_minify_removes_single_line_comment);
    RUN_TEST(test_minify_removes_multiline_comment);
    RUN_TEST(test_minify_removes_multiple_single_line_comments);
    RUN_TEST(test_minify_removes_multiple_multiline_comments);
    RUN_TEST(test_minify_single_slash_not_comment);
    RUN_TEST(test_minify_complex_object);
    RUN_TEST(test_minify_array);
    RUN_TEST(test_minify_nested_object);
    RUN_TEST(test_minify_boolean_values);
    RUN_TEST(test_minify_null_value);
    RUN_TEST(test_minify_string_with_escaped_quote);
    RUN_TEST(test_minify_string_with_escaped_backslash);
    RUN_TEST(test_minify_only_whitespace);
    RUN_TEST(test_minify_only_single_line_comment);
    RUN_TEST(test_minify_only_multiline_comment);
    RUN_TEST(test_minify_result_is_valid_json);
    RUN_TEST(test_minify_single_number);
    RUN_TEST(test_minify_single_string);
    RUN_TEST(test_minify_comment_inside_whitespace);
    RUN_TEST(test_minify_string_containing_comment_like_text);
    RUN_TEST(test_minify_string_containing_multiline_comment_like_text);
    RUN_TEST(test_minify_deeply_nested);
    RUN_TEST(test_minify_null_terminated_after_minify);
    return UNITY_END();
}