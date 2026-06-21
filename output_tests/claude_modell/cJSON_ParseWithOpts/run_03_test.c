#include "cJSON.c"
#include "unity.h"
#include "cJSON.h"
#include <stddef.h>
#include <string.h>

/* File-scope variables */
static cJSON *parsed_item = NULL;

/* Helper: free parsed item safely */
static void free_parsed(void)
{
    if (parsed_item != NULL)
    {
        cJSON_Delete(parsed_item);
        parsed_item = NULL;
    }
}

void setUp(void)
{
    parsed_item = NULL;
}

void tearDown(void)
{
    free_parsed();
}

/* ------------------------------------------------------------------ */
/* Test cases                                                           */
/* ------------------------------------------------------------------ */

/* NULL input must return NULL */
void test_ParseWithOpts_null_input_returns_null(void)
{
    cJSON *result = cJSON_ParseWithOpts(NULL, NULL, 0);
    TEST_ASSERT_NULL(result);
}

/* NULL input with return_parse_end must return NULL and not crash */
void test_ParseWithOpts_null_input_with_return_parse_end_returns_null(void)
{
    const char *end = NULL;
    cJSON *result = cJSON_ParseWithOpts(NULL, &end, 0);
    TEST_ASSERT_NULL(result);
}

/* Simple JSON object parses successfully */
void test_ParseWithOpts_simple_object(void)
{
    const char *json = "{\"key\":\"value\"}";
    parsed_item = cJSON_ParseWithOpts(json, NULL, 0);
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_TRUE(cJSON_IsObject(parsed_item));
}

/* Simple JSON array parses successfully */
void test_ParseWithOpts_simple_array(void)
{
    const char *json = "[1,2,3]";
    parsed_item = cJSON_ParseWithOpts(json, NULL, 0);
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_TRUE(cJSON_IsArray(parsed_item));
    TEST_ASSERT_EQUAL_INT(3, cJSON_GetArraySize(parsed_item));
}

/* JSON number parses successfully */
void test_ParseWithOpts_number(void)
{
    const char *json = "42";
    parsed_item = cJSON_ParseWithOpts(json, NULL, 0);
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_TRUE(cJSON_IsNumber(parsed_item));
    TEST_ASSERT_EQUAL_DOUBLE(42.0, parsed_item->valuedouble);
}

/* JSON string parses successfully */
void test_ParseWithOpts_string(void)
{
    const char *json = "\"hello\"";
    parsed_item = cJSON_ParseWithOpts(json, NULL, 0);
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_TRUE(cJSON_IsString(parsed_item));
    TEST_ASSERT_EQUAL_STRING("hello", parsed_item->valuestring);
}

/* JSON true parses successfully */
void test_ParseWithOpts_true(void)
{
    const char *json = "true";
    parsed_item = cJSON_ParseWithOpts(json, NULL, 0);
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_TRUE(cJSON_IsTrue(parsed_item));
}

/* JSON false parses successfully */
void test_ParseWithOpts_false(void)
{
    const char *json = "false";
    parsed_item = cJSON_ParseWithOpts(json, NULL, 0);
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_TRUE(cJSON_IsFalse(parsed_item));
}

/* JSON null parses successfully */
void test_ParseWithOpts_null_value(void)
{
    const char *json = "null";
    parsed_item = cJSON_ParseWithOpts(json, NULL, 0);
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_TRUE(cJSON_IsNull(parsed_item));
}

/* Invalid JSON returns NULL */
void test_ParseWithOpts_invalid_json_returns_null(void)
{
    const char *json = "{invalid}";
    parsed_item = cJSON_ParseWithOpts(json, NULL, 0);
    TEST_ASSERT_NULL(parsed_item);
}

/* Empty string returns NULL */
void test_ParseWithOpts_empty_string_returns_null(void)
{
    const char *json = "";
    parsed_item = cJSON_ParseWithOpts(json, NULL, 0);
    TEST_ASSERT_NULL(parsed_item);
}

/* return_parse_end is set on success */
void test_ParseWithOpts_return_parse_end_set_on_success(void)
{
    const char *json = "42";
    const char *end = NULL;
    parsed_item = cJSON_ParseWithOpts(json, &end, 0);
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_NOT_NULL(end);
}

/* return_parse_end points past the parsed content on success */
void test_ParseWithOpts_return_parse_end_points_past_content(void)
{
    const char *json = "42";
    const char *end = NULL;
    parsed_item = cJSON_ParseWithOpts(json, &end, 0);
    TEST_ASSERT_NOT_NULL(parsed_item);
    /* end should point to or past the last character */
    TEST_ASSERT_TRUE(end >= json);
    TEST_ASSERT_TRUE(end <= json + strlen(json));
}

/* return_parse_end is set on failure to point to error location */
void test_ParseWithOpts_return_parse_end_set_on_failure(void)
{
    const char *json = "{invalid}";
    const char *end = NULL;
    parsed_item = cJSON_ParseWithOpts(json, &end, 0);
    TEST_ASSERT_NULL(parsed_item);
    /* end should be set to the error location */
    TEST_ASSERT_NOT_NULL(end);
}

/* require_null_terminated=1 with properly null-terminated JSON succeeds */
void test_ParseWithOpts_require_null_terminated_valid(void)
{
    const char *json = "{\"a\":1}";
    const char *end = NULL;
    parsed_item = cJSON_ParseWithOpts(json, &end, 1);
    TEST_ASSERT_NOT_NULL(parsed_item);
}

/* require_null_terminated=0 with valid JSON succeeds */
void test_ParseWithOpts_no_require_null_terminated_valid(void)
{
    const char *json = "{\"a\":1}";
    parsed_item = cJSON_ParseWithOpts(json, NULL, 0);
    TEST_ASSERT_NOT_NULL(parsed_item);
}

/* Nested object parses correctly */
void test_ParseWithOpts_nested_object(void)
{
    const char *json = "{\"outer\":{\"inner\":99}}";
    parsed_item = cJSON_ParseWithOpts(json, NULL, 0);
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_TRUE(cJSON_IsObject(parsed_item));

    cJSON *outer = cJSON_GetObjectItem(parsed_item, "outer");
    TEST_ASSERT_NOT_NULL(outer);
    TEST_ASSERT_TRUE(cJSON_IsObject(outer));

    cJSON *inner = cJSON_GetObjectItem(outer, "inner");
    TEST_ASSERT_NOT_NULL(inner);
    TEST_ASSERT_TRUE(cJSON_IsNumber(inner));
    TEST_ASSERT_EQUAL_DOUBLE(99.0, inner->valuedouble);
}

/* Nested array parses correctly */
void test_ParseWithOpts_nested_array(void)
{
    const char *json = "[[1,2],[3,4]]";
    parsed_item = cJSON_ParseWithOpts(json, NULL, 0);
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_TRUE(cJSON_IsArray(parsed_item));
    TEST_ASSERT_EQUAL_INT(2, cJSON_GetArraySize(parsed_item));
}

/* JSON with trailing whitespace and no require_null_terminated */
void test_ParseWithOpts_trailing_whitespace_no_require(void)
{
    const char *json = "42   ";
    parsed_item = cJSON_ParseWithOpts(json, NULL, 0);
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_TRUE(cJSON_IsNumber(parsed_item));
}

/* JSON with trailing content and require_null_terminated=1 should fail */
void test_ParseWithOpts_trailing_garbage_require_null_terminated(void)
{
    /* "42garbage" — after parsing 42, there is non-whitespace trailing content */
    const char *json = "42garbage";
    const char *end = NULL;
    parsed_item = cJSON_ParseWithOpts(json, &end, 1);
    /* With require_null_terminated, trailing non-null content should cause failure */
    TEST_ASSERT_NULL(parsed_item);
}

/* Object with multiple keys */
void test_ParseWithOpts_object_multiple_keys(void)
{
    const char *json = "{\"a\":1,\"b\":2,\"c\":3}";
    parsed_item = cJSON_ParseWithOpts(json, NULL, 0);
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_TRUE(cJSON_IsObject(parsed_item));

    cJSON *a = cJSON_GetObjectItem(parsed_item, "a");
    cJSON *b = cJSON_GetObjectItem(parsed_item, "b");
    cJSON *c = cJSON_GetObjectItem(parsed_item, "c");

    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    TEST_ASSERT_NOT_NULL(c);

    TEST_ASSERT_EQUAL_DOUBLE(1.0, a->valuedouble);
    TEST_ASSERT_EQUAL_DOUBLE(2.0, b->valuedouble);
    TEST_ASSERT_EQUAL_DOUBLE(3.0, c->valuedouble);
}

/* Floating point number */
void test_ParseWithOpts_float_number(void)
{
    const char *json = "3.14";
    parsed_item = cJSON_ParseWithOpts(json, NULL, 0);
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_TRUE(cJSON_IsNumber(parsed_item));
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 3.14, parsed_item->valuedouble);
}

/* Negative number */
void test_ParseWithOpts_negative_number(void)
{
    const char *json = "-7";
    parsed_item = cJSON_ParseWithOpts(json, NULL, 0);
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_TRUE(cJSON_IsNumber(parsed_item));
    TEST_ASSERT_EQUAL_DOUBLE(-7.0, parsed_item->valuedouble);
}

/* Empty array */
void test_ParseWithOpts_empty_array(void)
{
    const char *json = "[]";
    parsed_item = cJSON_ParseWithOpts(json, NULL, 0);
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_TRUE(cJSON_IsArray(parsed_item));
    TEST_ASSERT_EQUAL_INT(0, cJSON_GetArraySize(parsed_item));
}

/* Empty object */
void test_ParseWithOpts_empty_object(void)
{
    const char *json = "{}";
    parsed_item = cJSON_ParseWithOpts(json, NULL, 0);
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_TRUE(cJSON_IsObject(parsed_item));
    TEST_ASSERT_EQUAL_INT(0, cJSON_GetArraySize(parsed_item));
}

/* Unicode string */
void test_ParseWithOpts_unicode_string(void)
{
    const char *json = "\"\\u0041\""; /* Unicode for 'A' */
    parsed_item = cJSON_ParseWithOpts(json, NULL, 0);
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_TRUE(cJSON_IsString(parsed_item));
    TEST_ASSERT_NOT_NULL(parsed_item->valuestring);
}

/* Escaped characters in string */
void test_ParseWithOpts_escaped_string(void)
{
    const char *json = "\"hello\\nworld\"";
    parsed_item = cJSON_ParseWithOpts(json, NULL, 0);
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_TRUE(cJSON_IsString(parsed_item));
    TEST_ASSERT_NOT_NULL(parsed_item->valuestring);
}

/* return_parse_end NULL with valid JSON does not crash */
void test_ParseWithOpts_null_return_parse_end_no_crash(void)
{
    const char *json = "{\"x\":10}";
    parsed_item = cJSON_ParseWithOpts(json, NULL, 1);
    TEST_ASSERT_NOT_NULL(parsed_item);
}

/* Whitespace-only string returns NULL */
void test_ParseWithOpts_whitespace_only_returns_null(void)
{
    const char *json = "   ";
    parsed_item = cJSON_ParseWithOpts(json, NULL, 0);
    TEST_ASSERT_NULL(parsed_item);
}

/* Array of strings */
void test_ParseWithOpts_array_of_strings(void)
{
    const char *json = "[\"a\",\"b\",\"c\"]";
    parsed_item = cJSON_ParseWithOpts(json, NULL, 0);
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_TRUE(cJSON_IsArray(parsed_item));
    TEST_ASSERT_EQUAL_INT(3, cJSON_GetArraySize(parsed_item));

    cJSON *first = cJSON_GetArrayItem(parsed_item, 0);
    TEST_ASSERT_NOT_NULL(first);
    TEST_ASSERT_TRUE(cJSON_IsString(first));
    TEST_ASSERT_EQUAL_STRING("a", first->valuestring);
}

/* Mixed array */
void test_ParseWithOpts_mixed_array(void)
{
    const char *json = "[1,\"two\",true,null,false]";
    parsed_item = cJSON_ParseWithOpts(json, NULL, 0);
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_TRUE(cJSON_IsArray(parsed_item));
    TEST_ASSERT_EQUAL_INT(5, cJSON_GetArraySize(parsed_item));
}

/* Exponential number */
void test_ParseWithOpts_exponential_number(void)
{
    const char *json = "1e10";
    parsed_item = cJSON_ParseWithOpts(json, NULL, 0);
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_TRUE(cJSON_IsNumber(parsed_item));
    TEST_ASSERT_DOUBLE_WITHIN(1.0, 1e10, parsed_item->valuedouble);
}

/* Truncated JSON returns NULL */
void test_ParseWithOpts_truncated_json_returns_null(void)
{
    const char *json = "{\"key\":";
    parsed_item = cJSON_ParseWithOpts(json, NULL, 0);
    TEST_ASSERT_NULL(parsed_item);
}

/* Mismatched brackets returns NULL */
void test_ParseWithOpts_mismatched_brackets_returns_null(void)
{
    const char *json = "[1,2,3}";
    parsed_item = cJSON_ParseWithOpts(json, NULL, 0);
    TEST_ASSERT_NULL(parsed_item);
}

/* ------------------------------------------------------------------ */
/* main                                                                 */
/* ------------------------------------------------------------------ */

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();

    RUN_TEST(test_ParseWithOpts_null_input_returns_null);
    RUN_TEST(test_ParseWithOpts_null_input_with_return_parse_end_returns_null);
    RUN_TEST(test_ParseWithOpts_simple_object);
    RUN_TEST(test_ParseWithOpts_simple_array);
    RUN_TEST(test_ParseWithOpts_number);
    RUN_TEST(test_ParseWithOpts_string);
    RUN_TEST(test_ParseWithOpts_true);
    RUN_TEST(test_ParseWithOpts_false);
    RUN_TEST(test_ParseWithOpts_null_value);
    RUN_TEST(test_ParseWithOpts_invalid_json_returns_null);
    RUN_TEST(test_ParseWithOpts_empty_string_returns_null);
    RUN_TEST(test_ParseWithOpts_return_parse_end_set_on_success);
    RUN_TEST(test_ParseWithOpts_return_parse_end_points_past_content);
    RUN_TEST(test_ParseWithOpts_return_parse_end_set_on_failure);
    RUN_TEST(test_ParseWithOpts_require_null_terminated_valid);
    RUN_TEST(test_ParseWithOpts_no_require_null_terminated_valid);
    RUN_TEST(test_ParseWithOpts_nested_object);
    RUN_TEST(test_ParseWithOpts_nested_array);
    RUN_TEST(test_ParseWithOpts_trailing_whitespace_no_require);
    RUN_TEST(test_ParseWithOpts_trailing_garbage_require_null_terminated);
    RUN_TEST(test_ParseWithOpts_object_multiple_keys);
    RUN_TEST(test_ParseWithOpts_float_number);
    RUN_TEST(test_ParseWithOpts_negative_number);
    RUN_TEST(test_ParseWithOpts_empty_array);
    RUN_TEST(test_ParseWithOpts_empty_object);
    RUN_TEST(test_ParseWithOpts_unicode_string);
    RUN_TEST(test_ParseWithOpts_escaped_string);
    RUN_TEST(test_ParseWithOpts_null_return_parse_end_no_crash);
    RUN_TEST(test_ParseWithOpts_whitespace_only_returns_null);
    RUN_TEST(test_ParseWithOpts_array_of_strings);
    RUN_TEST(test_ParseWithOpts_mixed_array);
    RUN_TEST(test_ParseWithOpts_exponential_number);
    RUN_TEST(test_ParseWithOpts_truncated_json_returns_null);
    RUN_TEST(test_ParseWithOpts_mismatched_brackets_returns_null);

    return UNITY_END();
}