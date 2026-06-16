#include "cJSON.c"
#include "unity.h"

/* Helper function to allocate and copy a string for testing */
static char* strdup_test(const char* src) {
    size_t len = strlen(src) + 1;
    char* dst = (char*)malloc(len);
    TEST_ASSERT_NOT_NULL_MESSAGE(dst, "Memory allocation failed");
    memcpy(dst, src, len);
    return dst;
}

/* Test: NULL input */
void test_cJSON_Minify_NULL_input(void) {
    cJSON_Minify(NULL);
    /* Should not crash */
    TEST_ASSERT_TRUE(true);
}

/* Test: Empty string */
void test_cJSON_Minify_empty_string(void) {
    char input[] = "";
    cJSON_Minify(input);
    TEST_ASSERT_EQUAL_STRING("", input);
}

/* Test: Whitespace only */
void test_cJSON_Minify_whitespace_only(void) {
    char input[] = "   \t\n\r  ";
    cJSON_Minify(input);
    TEST_ASSERT_EQUAL_STRING("", input);
}

/* Test: Simple JSON with spaces */
void test_cJSON_Minify_simple_spaces(void) {
    char input[] = " { \"key\" : \"value\" } ";
    cJSON_Minify(input);
    TEST_ASSERT_EQUAL_STRING("{\"key\":\"value\"}", input);
}

/* Test: JSON with newlines and tabs */
void test_cJSON_Minify_newlines_tabs(void) {
    char input[] = "{\n\t\"key\"\n:\n\"value\"\n}";
    cJSON_Minify(input);
    TEST_ASSERT_EQUAL_STRING("{\"key\":\"value\"}", input);
}

/* Test: String with escaped quotes and backslashes */
void test_cJSON_Minify_string_with_escapes(void) {
    char input[] = " { \"a\": \"b\\\"c\" } ";
    cJSON_Minify(input);
    TEST_ASSERT_EQUAL_STRING("{\"a\":\"b\\\"c\"}", input);
}

/* Test: String with embedded newlines (valid in JSON strings) */
void test_cJSON_Minify_string_with_embedded_newlines(void) {
    char input[] = " { \"a\": \"b\nc\" } ";
    cJSON_Minify(input);
    TEST_ASSERT_EQUAL_STRING("{\"a\":\"b\nc\"}", input);
}

/* Test: Single-line comments (//) */
void test_cJSON_Minify_single_line_comment(void) {
    char input[] = "{ // comment\n\"key\":\"value\"}";
    cJSON_Minify(input);
    TEST_ASSERT_EQUAL_STRING("{\"key\":\"value\"}", input);
}

/* Test: Multi-line comments (/* *) */
void test_cJSON_Minify_multi_line_comment(void) {
    char input[] = "{ /* comment\nline2 */\"key\":\"value\"}";
    cJSON_Minify(input);
    TEST_ASSERT_EQUAL_STRING("{\"key\":\"value\"}", input);
}

/* Test: Comment at start */
void test_cJSON_Minify_comment_at_start(void) {
    char input[] = "/* comment */ {\"key\":\"value\"}";
    cJSON_Minify(input);
    TEST_ASSERT_EQUAL_STRING("{\"key\":\"value\"}", input);
}

/* Test: Comment spanning multiple lines */
void test_cJSON_Minify_multiline_comment_spanning_lines(void) {
    char input[] = "/*\nline1\nline2\n*/{\"key\":\"value\"}";
    cJSON_Minify(input);
    TEST_ASSERT_EQUAL_STRING("{\"key\":\"value\"}", input);
}

/* Test: Mixed whitespace and comments */
void test_cJSON_Minify_mixed_whitespace_and_comments(void) {
    char input[] = "  \n/* comment */ { \t \"a\" \n : \r \"b\" // comment\n }  ";
    cJSON_Minify(input);
    TEST_ASSERT_EQUAL_STRING("{\"a\":\"b\"}", input);
}

/* Test: String containing comment-like sequences (not actual comments) */
void test_cJSON_Minify_comment_like_in_string(void) {
    char input[] = " { \"a\": \"// not a comment\" } ";
    cJSON_Minify(input);
    TEST_ASSERT_EQUAL_STRING("{\"a\":\"// not a comment\"}", input);
}

/* Test: String containing /* inside (not actual comment) */
void test_cJSON_Minify_multiline_comment_like_in_string(void) {
    char input[] = " { \"a\": \"/* not a comment */\" } ";
    cJSON_Minify(input);
    TEST_ASSERT_EQUAL_STRING("{\"a\":\"/* not a comment */\"}", input);
}

/* Test: Nested objects and arrays */
void test_cJSON_Minify_nested_structures(void) {
    char input[] = " { \"a\": [ 1 , 2 , { \"b\" : \"c\" } ] } ";
    cJSON_Minify(input);
    TEST_ASSERT_EQUAL_STRING("{\"a\":[1,2,{\"b\":\"c\"}]}", input);
}

/* Test: Numbers and booleans */
void test_cJSON_Minify_numbers_and_booleans(void) {
    char input[] = " { \"n\": 123.456e-7 , \"b\": true , \"nul\": null , \"f\": false } ";
    cJSON_Minify(input);
    TEST_ASSERT_EQUAL_STRING("{\"n\":123.456e-7,\"b\":true,\"nul\":null,\"f\":false}", input);
}

/* Test: Raw JSON (cJSON_Raw type) */
void test_cJSON_Minify_raw_json(void) {
    char input[] = " { \"r\": <raw json> } ";
    cJSON_Minify(input);
    /* Note: raw JSON is not standard JSON, but if present, it's preserved as-is */
    TEST_ASSERT_EQUAL_STRING("{\"r\":<raw json>}", input);
}

/* Test: Unbalanced quotes (invalid JSON, but minifier should not crash) */
void test_cJSON_Minify_unbalanced_quotes(void) {
    char input[] = " { \"a\": \"b } ";
    cJSON_Minify(input);
    /* Behavior is undefined for invalid input, but must not crash */
    /* We just check it doesn't crash and produces some null-terminated string */
    TEST_ASSERT_NOT_NULL(input);
    TEST_ASSERT_EQUAL_INT('\0', input[strlen(input)]);
}

/* Test: Very long string with many whitespace */
void test_cJSON_Minify_long_string_with_whitespace(void) {
    char input[1024];
    size_t i;
    for (i = 0; i < 512; i++) {
        input[i] = ' ';
    }
    input[512] = '{';
    input[513] = '"';
    input[514] = 'a';
    input[515] = '"';
    input[516] = ':';
    input[517] = '"';
    input[518] = 'v';
    input[519] = '"';
    input[520] = '}';
    for (i = 521; i < 1023; i++) {
        input[i] = ' ';
    }
    input[1023] = '\0';

    cJSON_Minify(input);
    TEST_ASSERT_EQUAL_STRING("{\"a\":\"v\"}", input);
}

/* Test: String containing backslash followed by newline (valid escape in JSON) */
void test_cJSON_Minify_backslash_newline_in_string(void) {
    char input[] = " { \"a\": \"b\\\nc\" } ";
    cJSON_Minify(input);
    /* In JSON, backslash followed by newline is invalid, but minifier should preserve as-is */
    TEST_ASSERT_EQUAL_STRING("{\"a\":\"b\\\nc\"}", input);
}

int main(void) {
    unity_install_sighandler();
    UNITY_BEGIN();

    RUN_TEST(test_cJSON_Minify_NULL_input);
    RUN_TEST(test_cJSON_Minify_empty_string);
    RUN_TEST(test_cJSON_Minify_whitespace_only);
    RUN_TEST(test_cJSON_Minify_simple_spaces);
    RUN_TEST(test_cJSON_Minify_newlines_tabs);
    RUN_TEST(test_cJSON_Minify_string_with_escapes);
    RUN_TEST(test_cJSON_Minify_string_with_embedded_newlines);
    RUN_TEST(test_cJSON_Minify_single_line_comment);
    RUN_TEST(test_cJSON_Minify_multi_line_comment);
    RUN_TEST(test_cJSON_Minify_comment_at_start);
    RUN_TEST(test_cJSON_Minify_multiline_comment_spanning_lines);
    RUN_TEST(test_cJSON_Minify_mixed_whitespace_and_comments);
    RUN_TEST(test_cJSON_Minify_comment_like_in_string);
    RUN_TEST(test_cJSON_Minify_multiline_comment_like_in_string);
    RUN_TEST(test_cJSON_Minify_nested_structures);
    RUN_TEST(test_cJSON_Minify_numbers_and_booleans);
    RUN_TEST(test_cJSON_Minify_raw_json);
    RUN_TEST(test_cJSON_Minify_unbalanced_quotes);
    RUN_TEST(test_cJSON_Minify_long_string_with_whitespace);
    RUN_TEST(test_cJSON_Minify_backslash_newline_in_string);

    return UNITY_END();
}