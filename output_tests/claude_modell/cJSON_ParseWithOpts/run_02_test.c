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

/* -------------------------------------------------------------------------
 * Test cases
 * ---------------------------------------------------------------------- */

/* NULL input must return NULL */
void test_ParseWithOpts_null_input_returns_null(void)
{
    const char *end = NULL;
    cJSON *result = cJSON_ParseWithOpts(NULL, &end, 0);
    TEST_ASSERT_NULL(result);
}

/* NULL input with NULL return_parse_end must return NULL without crash */
void test_ParseWithOpts_null_input_null_end_returns_null(void)
{
    cJSON *result = cJSON_ParseWithOpts(NULL, NULL, 0);
    TEST_ASSERT_NULL(result);
}

/* Simple integer number */
void test_ParseWithOpts_simple_number(void)
{
    const char *end = NULL;
    parsed_item = cJSON_ParseWithOpts("42", &end, 0);
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_TRUE(cJSON_IsNumber(parsed_item));
    TEST_ASSERT_EQUAL_DOUBLE(42.0, parsed_item->valuedouble);
}

/* Simple string */
void test_ParseWithOpts_simple_string(void)
{
    const char *end = NULL;
    parsed_item = cJSON_ParseWithOpts("\"hello\"", &end, 0);
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_TRUE(cJSON_IsString(parsed_item));
    TEST_ASSERT_EQUAL_STRING("hello", parsed_item->valuestring);
}

/* Simple boolean true */
void test_ParseWithOpts_true_value(void)
{
    const char *end = NULL;
    parsed_item = cJSON_ParseWithOpts("true", &end, 0);
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_TRUE(cJSON_IsTrue(parsed_item));
}

/* Simple boolean false */
void test_ParseWithOpts_false_value(void)
{
    const char *end = NULL;
    parsed_item = cJSON_ParseWithOpts("false", &end, 0);
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_TRUE(cJSON_IsFalse(parsed_item));
}

/* null JSON value */
void test_ParseWithOpts_null_json_value(void)
{
    const char *end = NULL;
    parsed_item = cJSON_ParseWithOpts("null", &end, 0);
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_TRUE(cJSON_IsNull(parsed_item));
}

/* Simple array */
void test_ParseWithOpts_simple_array(void)
{
    const char *end = NULL;
    parsed_item = cJSON_ParseWithOpts("[1,2,3]", &end, 0);
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_TRUE(cJSON_IsArray(parsed_item));
    TEST_ASSERT_EQUAL_INT(3, cJSON_GetArraySize(parsed_item));
}

/* Simple object */
void test_ParseWithOpts_simple_object(void)
{
    const char *end = NULL;
    parsed_item = cJSON_ParseWithOpts("{\"key\":\"value\"}", &end, 0);
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_TRUE(cJSON_IsObject(parsed_item));
    cJSON *item = cJSON_GetObjectItem(parsed_item, "key");
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_EQUAL_STRING("value", item->valuestring);
}

/* return_parse_end is set after successful parse */
void test_ParseWithOpts_return_parse_end_set_on_success(void)
{
    const char *input = "123";
    const char *end = NULL;
    parsed_item = cJSON_ParseWithOpts(input, &end, 0);
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_NOT_NULL(end);
    /* end should point past the parsed content */
    TEST_ASSERT_TRUE(end >= input);
}

/* return_parse_end NULL is accepted (no crash) */
void test_ParseWithOpts_null_return_parse_end_no_crash(void)
{
    parsed_item = cJSON_ParseWithOpts("123", NULL, 0);
    TEST_ASSERT_NOT_NULL(parsed_item);
}

/* require_null_terminated=1 with clean null-terminated JSON succeeds */
void test_ParseWithOpts_require_null_terminated_clean_json(void)
{
    const char *end = NULL;
    parsed_item = cJSON_ParseWithOpts("{\"a\":1}", &end, 1);
    TEST_ASSERT_NOT_NULL(parsed_item);
}

/* require_null_terminated=1 with trailing garbage fails */
void test_ParseWithOpts_require_null_terminated_trailing_garbage(void)
{
    /* Build a buffer that has JSON followed by non-null garbage.
     * We embed the extra character manually so strlen still sees the full
     * string but the parser sees trailing content after the JSON value. */
    const char *input = "123 garbage";
    const char *end = NULL;
    /* With require_null_terminated=1 the parser should fail because there
     * is non-whitespace content after the value before the null terminator. */
    parsed_item = cJSON_ParseWithOpts(input, &end, 1);
    /* The result must be NULL because trailing garbage is present */
    TEST_ASSERT_NULL(parsed_item);
}

/* require_null_terminated=0 with trailing content still parses the value */
void test_ParseWithOpts_no_require_null_terminated_trailing_content(void)
{
    const char *input = "123 garbage";
    const char *end = NULL;
    parsed_item = cJSON_ParseWithOpts(input, &end, 0);
    /* Without the null-terminated requirement the number is parsed */
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_TRUE(cJSON_IsNumber(parsed_item));
    TEST_ASSERT_EQUAL_DOUBLE(123.0, parsed_item->valuedouble);
}

/* Invalid JSON returns NULL */
void test_ParseWithOpts_invalid_json_returns_null(void)
{
    const char *end = NULL;
    parsed_item = cJSON_ParseWithOpts("{invalid}", &end, 0);
    TEST_ASSERT_NULL(parsed_item);
}

/* Empty string returns NULL */
void test_ParseWithOpts_empty_string_returns_null(void)
{
    const char *end = NULL;
    parsed_item = cJSON_ParseWithOpts("", &end, 0);
    TEST_ASSERT_NULL(parsed_item);
}

/* Whitespace-only string returns NULL */
void test_ParseWithOpts_whitespace_only_returns_null(void)
{
    const char *end = NULL;
    parsed_item = cJSON_ParseWithOpts("   ", &end, 0);
    TEST_ASSERT_NULL(parsed_item);
}

/* Nested object */
void test_ParseWithOpts_nested_object(void)
{
    const char *end = NULL;
    parsed_item = cJSON_ParseWithOpts("{\"outer\":{\"inner\":42}}", &end, 0);
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_TRUE(cJSON_IsObject(parsed_item));
    cJSON *outer = cJSON_GetObjectItem(parsed_item, "outer");
    TEST_ASSERT_NOT_NULL(outer);
    TEST_ASSERT_TRUE(cJSON_IsObject(outer));
    cJSON *inner = cJSON_GetObjectItem(outer, "inner");
    TEST_ASSERT_NOT_NULL(inner);
    TEST_ASSERT_EQUAL_DOUBLE(42.0, inner->valuedouble);
}

/* Nested array */
void test_ParseWithOpts_nested_array(void)
{
    const char *end = NULL;
    parsed_item = cJSON_ParseWithOpts("[[1,2],[3,4]]", &end, 0);
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_TRUE(cJSON_IsArray(parsed_item));
    TEST_ASSERT_EQUAL_INT(2, cJSON_GetArraySize(parsed_item));
}

/* Floating point number */
void test_ParseWithOpts_floating_point_number(void)
{
    const char *end = NULL;
    parsed_item = cJSON_ParseWithOpts("3.14", &end, 0);
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_TRUE(cJSON_IsNumber(parsed_item));
    TEST_ASSERT_DOUBLE_WITHIN(0.0001, 3.14, parsed_item->valuedouble);
}

/* Negative number */
void test_ParseWithOpts_negative_number(void)
{
    const char *end = NULL;
    parsed_item = cJSON_ParseWithOpts("-99", &end, 0);
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_TRUE(cJSON_IsNumber(parsed_item));
    TEST_ASSERT_EQUAL_DOUBLE(-99.0, parsed_item->valuedouble);
}

/* Unicode string */
void test_ParseWithOpts_unicode_string(void)
{
    const char *end = NULL;
    parsed_item = cJSON_ParseWithOpts("\"\\u0041\"", &end, 0);
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_TRUE(cJSON_IsString(parsed_item));
    /* \u0041 is 'A' */
    TEST_ASSERT_EQUAL_STRING("A", parsed_item->valuestring);
}

/* Empty array */
void test_ParseWithOpts_empty_array(void)
{
    const char *end = NULL;
    parsed_item = cJSON_ParseWithOpts("[]", &end, 0);
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_TRUE(cJSON_IsArray(parsed_item));
    TEST_ASSERT_EQUAL_INT(0, cJSON_GetArraySize(parsed_item));
}

/* Empty object */
void test_ParseWithOpts_empty_object(void)
{
    const char *end = NULL;
    parsed_item = cJSON_ParseWithOpts("{}", &end, 0);
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_TRUE(cJSON_IsObject(parsed_item));
    TEST_ASSERT_EQUAL_INT(0, cJSON_GetArraySize(parsed_item));
}

/* return_parse_end points past the last parsed character on success */
void test_ParseWithOpts_return_parse_end_points_past_value(void)
{
    const char *input = "42";
    const char *end = NULL;
    parsed_item = cJSON_ParseWithOpts(input, &end, 0);
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_NOT_NULL(end);
    /* end must be at or after the start of input */
    TEST_ASSERT_TRUE((size_t)(end - input) <= strlen(input));
}

/* return_parse_end is set on failure to point to error location */
void test_ParseWithOpts_return_parse_end_set_on_failure(void)
{
    const char *input = "{bad}";
    const char *end = NULL;
    parsed_item = cJSON_ParseWithOpts(input, &end, 0);
    TEST_ASSERT_NULL(parsed_item);
    /* On failure end should point to the error location */
    TEST_ASSERT_NOT_NULL(end);
}

/* Mixed types in array */
void test_ParseWithOpts_mixed_array(void)
{
    const char *end = NULL;
    parsed_item = cJSON_ParseWithOpts("[1,\"two\",true,null,false]", &end, 0);
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_TRUE(cJSON_IsArray(parsed_item));
    TEST_ASSERT_EQUAL_INT(5, cJSON_GetArraySize(parsed_item));
}

/* Object with multiple keys */
void test_ParseWithOpts_object_multiple_keys(void)
{
    const char *end = NULL;
    parsed_item = cJSON_ParseWithOpts("{\"a\":1,\"b\":2,\"c\":3}", &end, 0);
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

/* Exponent notation */
void test_ParseWithOpts_exponent_number(void)
{
    const char *end = NULL;
    parsed_item = cJSON_ParseWithOpts("1e3", &end, 0);
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_TRUE(cJSON_IsNumber(parsed_item));
    TEST_ASSERT_EQUAL_DOUBLE(1000.0, parsed_item->valuedouble);
}

/* String with escape sequences */
void test_ParseWithOpts_string_with_escapes(void)
{
    const char *end = NULL;
    parsed_item = cJSON_ParseWithOpts("\"line1\\nline2\"", &end, 0);
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_TRUE(cJSON_IsString(parsed_item));
    TEST_ASSERT_NOT_NULL(parsed_item->valuestring);
    /* Should contain a newline character */
    TEST_ASSERT_NOT_NULL(strchr(parsed_item->valuestring, '\n'));
}

/* require_null_terminated=1 with trailing whitespace only — should succeed
 * because whitespace after a value is acceptable */
void test_ParseWithOpts_require_null_terminated_trailing_whitespace(void)
{
    const char *end = NULL;
    /* Trailing spaces before the null terminator */
    parsed_item = cJSON_ParseWithOpts("42   ", &end, 1);
    /* Trailing whitespace is acceptable; parse should succeed */
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_TRUE(cJSON_IsNumber(parsed_item));
}

/* -------------------------------------------------------------------------
 * main
 * ---------------------------------------------------------------------- */
int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_ParseWithOpts_null_input_returns_null);
    RUN_TEST(test_ParseWithOpts_null_input_null_end_returns_null);
    RUN_TEST(test_ParseWithOpts_simple_number);
    RUN_TEST(test_ParseWithOpts_simple_string);
    RUN_TEST(test_ParseWithOpts_true_value);
    RUN_TEST(test_ParseWithOpts_false_value);
    RUN_TEST(test_ParseWithOpts_null_json_value);
    RUN_TEST(test_ParseWithOpts_simple_array);
    RUN_TEST(test_ParseWithOpts_simple_object);
    RUN_TEST(test_ParseWithOpts_return_parse_end_set_on_success);
    RUN_TEST(test_ParseWithOpts_null_return_parse_end_no_crash);
    RUN_TEST(test_ParseWithOpts_require_null_terminated_clean_json);
    RUN_TEST(test_ParseWithOpts_require_null_terminated_trailing_garbage);
    RUN_TEST(test_ParseWithOpts_no_require_null_terminated_trailing_content);
    RUN_TEST(test_ParseWithOpts_invalid_json_returns_null);
    RUN_TEST(test_ParseWithOpts_empty_string_returns_null);
    RUN_TEST(test_ParseWithOpts_whitespace_only_returns_null);
    RUN_TEST(test_ParseWithOpts_nested_object);
    RUN_TEST(test_ParseWithOpts_nested_array);
    RUN_TEST(test_ParseWithOpts_floating_point_number);
    RUN_TEST(test_ParseWithOpts_negative_number);
    RUN_TEST(test_ParseWithOpts_unicode_string);
    RUN_TEST(test_ParseWithOpts_empty_array);
    RUN_TEST(test_ParseWithOpts_empty_object);
    RUN_TEST(test_ParseWithOpts_return_parse_end_points_past_value);
    RUN_TEST(test_ParseWithOpts_return_parse_end_set_on_failure);
    RUN_TEST(test_ParseWithOpts_mixed_array);
    RUN_TEST(test_ParseWithOpts_object_multiple_keys);
    RUN_TEST(test_ParseWithOpts_exponent_number);
    RUN_TEST(test_ParseWithOpts_string_with_escapes);
    RUN_TEST(test_ParseWithOpts_require_null_terminated_trailing_whitespace);
    return UNITY_END();
}