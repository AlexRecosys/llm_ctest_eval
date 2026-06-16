#include "cJSON.c"
#include "unity.h"
#include <string.h>
#include <stdlib.h>

/* File-scope static variables / fixtures */
static cJSON *test_item = NULL;

/* Helper functions and macros */
static void cleanup_test_item(void)
{
    if (test_item != NULL)
    {
        cJSON_Delete(test_item);
        test_item = NULL;
    }
}

/* Test cases */

void test_cJSON_Parse_should_parse_empty_object(void)
{
    const char *json = "{}";
    test_item = cJSON_Parse(json);
    TEST_ASSERT_NOT_NULL(test_item);
    TEST_ASSERT_EQUAL_INT(cJSON_Object, test_item->type);
    TEST_ASSERT_NULL(test_item->child);
    TEST_ASSERT_NULL(cJSON_GetErrorPtr());
}

void test_cJSON_Parse_should_parse_empty_array(void)
{
    const char *json = "[]";
    test_item = cJSON_Parse(json);
    TEST_ASSERT_NOT_NULL(test_item);
    TEST_ASSERT_EQUAL_INT(cJSON_Array, test_item->type);
    TEST_ASSERT_NULL(test_item->child);
    TEST_ASSERT_NULL(cJSON_GetErrorPtr());
}

void test_cJSON_Parse_should_parse_simple_string(void)
{
    const char *json = "\"hello\"";
    test_item = cJSON_Parse(json);
    TEST_ASSERT_NOT_NULL(test_item);
    TEST_ASSERT_EQUAL_INT(cJSON_String, test_item->type);
    TEST_ASSERT_EQUAL_STRING("hello", test_item->valuestring);
    TEST_ASSERT_NULL(cJSON_GetErrorPtr());
}

void test_cJSON_Parse_should_parse_simple_number(void)
{
    const char *json = "123.456";
    test_item = cJSON_Parse(json);
    TEST_ASSERT_NOT_NULL(test_item);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item->type);
    TEST_ASSERT_DOUBLE_WITHIN(0.000001, 123.456, test_item->valuedouble);
    TEST_ASSERT_NULL(cJSON_GetErrorPtr());
}

void test_cJSON_Parse_should_parse_true_literal(void)
{
    const char *json = "true";
    test_item = cJSON_Parse(json);
    TEST_ASSERT_NOT_NULL(test_item);
    TEST_ASSERT_EQUAL_INT(cJSON_True, test_item->type);
    TEST_ASSERT_NULL(cJSON_GetErrorPtr());
}

void test_cJSON_Parse_should_parse_false_literal(void)
{
    const char *json = "false";
    test_item = cJSON_Parse(json);
    TEST_ASSERT_NOT_NULL(test_item);
    TEST_ASSERT_EQUAL_INT(cJSON_False, test_item->type);
    TEST_ASSERT_NULL(cJSON_GetErrorPtr());
}

void test_cJSON_Parse_should_parse_null_literal(void)
{
    const char *json = "null";
    test_item = cJSON_Parse(json);
    TEST_ASSERT_NOT_NULL(test_item);
    TEST_ASSERT_EQUAL_INT(cJSON_NULL, test_item->type);
    TEST_ASSERT_NULL(cJSON_GetErrorPtr());
}

void test_cJSON_Parse_should_parse_nested_object(void)
{
    const char *json = "{\"a\":1,\"b\":{\"c\":2}}";
    test_item = cJSON_Parse(json);
    TEST_ASSERT_NOT_NULL(test_item);
    TEST_ASSERT_EQUAL_INT(cJSON_Object, test_item->type);
    TEST_ASSERT_NOT_NULL(test_item->child);
    TEST_ASSERT_NULL(cJSON_GetErrorPtr());

    cJSON *a = cJSON_GetObjectItem(test_item, "a");
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, a->type);
    TEST_ASSERT_EQUAL_DOUBLE(1.0, a->valuedouble);

    cJSON *b = cJSON_GetObjectItem(test_item, "b");
    TEST_ASSERT_NOT_NULL(b);
    TEST_ASSERT_EQUAL_INT(cJSON_Object, b->type);
    TEST_ASSERT_NOT_NULL(b->child);

    cJSON *c = cJSON_GetObjectItem(b, "c");
    TEST_ASSERT_NOT_NULL(c);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, c->type);
    TEST_ASSERT_EQUAL_DOUBLE(2.0, c->valuedouble);
}

void test_cJSON_Parse_should_parse_array_with_mixed_types(void)
{
    const char *json = "[1,\"two\",true,null,false]";
    test_item = cJSON_Parse(json);
    TEST_ASSERT_NOT_NULL(test_item);
    TEST_ASSERT_EQUAL_INT(cJSON_Array, test_item->type);
    TEST_ASSERT_NOT_NULL(test_item->child);
    TEST_ASSERT_NULL(cJSON_GetErrorPtr());

    cJSON *item0 = cJSON_GetArrayItem(test_item, 0);
    TEST_ASSERT_NOT_NULL(item0);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, item0->type);
    TEST_ASSERT_EQUAL_DOUBLE(1.0, item0->valuedouble);

    cJSON *item1 = cJSON_GetArrayItem(test_item, 1);
    TEST_ASSERT_NOT_NULL(item1);
    TEST_ASSERT_EQUAL_INT(cJSON_String, item1->type);
    TEST_ASSERT_EQUAL_STRING("two", item1->valuestring);

    cJSON *item2 = cJSON_GetArrayItem(test_item, 2);
    TEST_ASSERT_NOT_NULL(item2);
    TEST_ASSERT_EQUAL_INT(cJSON_True, item2->type);

    cJSON *item3 = cJSON_GetArrayItem(test_item, 3);
    TEST_ASSERT_NOT_NULL(item3);
    TEST_ASSERT_EQUAL_INT(cJSON_NULL, item3->type);

    cJSON *item4 = cJSON_GetArrayItem(test_item, 4);
    TEST_ASSERT_NOT_NULL(item4);
    TEST_ASSERT_EQUAL_INT(cJSON_False, item4->type);
}

void test_cJSON_Parse_should_handle_null_input(void)
{
    test_item = cJSON_Parse(NULL);
    TEST_ASSERT_NULL(test_item);
    TEST_ASSERT_NOT_NULL(cJSON_GetErrorPtr());
}

void test_cJSON_Parse_should_handle_empty_string(void)
{
    test_item = cJSON_Parse("");
    TEST_ASSERT_NULL(test_item);
    TEST_ASSERT_NOT_NULL(cJSON_GetErrorPtr());
}

void test_cJSON_Parse_should_handle_invalid_json(void)
{
    const char *json = "{invalid}";
    test_item = cJSON_Parse(json);
    TEST_ASSERT_NULL(test_item);
    TEST_ASSERT_NOT_NULL(cJSON_GetErrorPtr());
}

void test_cJSON_Parse_should_handle_string_with_escaped_characters(void)
{
    const char *json = "\"hello\\nworld\\t\\\"escaped\\\"\"";
    test_item = cJSON_Parse(json);
    TEST_ASSERT_NOT_NULL(test_item);
    TEST_ASSERT_EQUAL_INT(cJSON_String, test_item->type);
    TEST_ASSERT_EQUAL_STRING("hello\nworld\t\"escaped\"", test_item->valuestring);
    TEST_ASSERT_NULL(cJSON_GetErrorPtr());
}

void test_cJSON_Parse_should_handle_unicode_escape_sequence(void)
{
    const char *json = "\"\\u0041\\u0042\\u0043\"";
    test_item = cJSON_Parse(json);
    TEST_ASSERT_NOT_NULL(test_item);
    TEST_ASSERT_EQUAL_INT(cJSON_String, test_item->type);
    TEST_ASSERT_EQUAL_STRING("ABC", test_item->valuestring);
    TEST_ASSERT_NULL(cJSON_GetErrorPtr());
}

void test_cJSON_Parse_should_handle_negative_number(void)
{
    const char *json = "-123.45";
    test_item = cJSON_Parse(json);
    TEST_ASSERT_NOT_NULL(test_item);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item->type);
    TEST_ASSERT_DOUBLE_WITHIN(0.000001, -123.45, test_item->valuedouble);
    TEST_ASSERT_NULL(cJSON_GetErrorPtr());
}

void test_cJSON_Parse_should_handle_scientific_notation(void)
{
    const char *json = "1.23e-4";
    test_item = cJSON_Parse(json);
    TEST_ASSERT_NOT_NULL(test_item);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item->type);
    TEST_ASSERT_DOUBLE_WITHIN(0.000001, 0.000123, test_item->valuedouble);
    TEST_ASSERT_NULL(cJSON_GetErrorPtr());
}

void test_cJSON_Parse_should_handle_large_integer(void)
{
    const char *json = "9223372036854775807"; /* 2^63 - 1 */
    test_item = cJSON_Parse(json);
    TEST_ASSERT_NOT_NULL(test_item);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item->type);
    /* Note: cJSON stores numbers as double; precision may be lost for very large integers */
    TEST_ASSERT_NOT_EQUAL_DOUBLE(0.0, test_item->valuedouble);
    TEST_ASSERT_NULL(cJSON_GetErrorPtr());
}

void test_cJSON_Parse_should_handle_whitespace(void)
{
    const char *json = "  {  \"key\"  :  \"value\"  }  ";
    test_item = cJSON_Parse(json);
    TEST_ASSERT_NOT_NULL(test_item);
    TEST_ASSERT_EQUAL_INT(cJSON_Object, test_item->type);
    TEST_ASSERT_NOT_NULL(test_item->child);
    TEST_ASSERT_EQUAL_STRING("value", test_item->child->valuestring);
    TEST_ASSERT_NULL(cJSON_GetErrorPtr());
}

void test_cJSON_Parse_should_handle_array_with_trailing_comma_failure(void)
{
    /* Trailing comma is invalid in standard JSON */
    const char *json = "[1,2,3,]";
    test_item = cJSON_Parse(json);
    TEST_ASSERT_NULL(test_item);
    TEST_ASSERT_NOT_NULL(cJSON_GetErrorPtr());
}

void test_cJSON_Parse_should_handle_deeply_nested_structure(void)
{
    /* Build a deeply nested structure: {"a":{"b":{"c":{...}}}} */
    char *json = (char *)malloc(1024);
    TEST_ASSERT_NOT_NULL(json);
    strcpy(json, "{\"a\":{");
    for (int i = 1; i < 10; i++)
    {
        strcat(json, "\"b\":{");
    }
    strcat(json, "\"value\":42");
    for (int i = 0; i < 10; i++)
    {
        strcat(json, "}");
    }
    strcat(json, "}}");

    test_item = cJSON_Parse(json);
    TEST_ASSERT_NOT_NULL(test_item);
    TEST_ASSERT_EQUAL_INT(cJSON_Object, test_item->type);
    TEST_ASSERT_NULL(cJSON_GetErrorPtr());

    /* Navigate to the innermost object */
    cJSON *inner = test_item;
    for (int i = 0; i < 10; i++)
    {
        TEST_ASSERT_NOT_NULL(inner->child);
        inner = inner->child;
    }
    TEST_ASSERT_NOT_NULL(inner);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, inner->type);
    TEST_ASSERT_EQUAL_DOUBLE(42.0, inner->valuedouble);

    free(json);
}

void test_cJSON_Parse_should_handle_raw_string(void)
{
    /* Raw JSON strings are not directly supported by cJSON_Parse; they must be quoted */
    /* This test ensures raw unquoted strings are rejected */
    const char *json = "raw";
    test_item = cJSON_Parse(json);
    TEST_ASSERT_NULL(test_item);
    TEST_ASSERT_NOT_NULL(cJSON_GetErrorPtr());
}

void test_cJSON_Parse_should_handle_boolean_in_object(void)
{
    const char *json = "{\"flag\":true}";
    test_item = cJSON_Parse(json);
    TEST_ASSERT_NOT_NULL(test_item);
    TEST_ASSERT_EQUAL_INT(cJSON_Object, test_item->type);
    cJSON *flag = cJSON_GetObjectItem(test_item, "flag");
    TEST_ASSERT_NOT_NULL(flag);
    TEST_ASSERT_EQUAL_INT(cJSON_True, flag->type);
    TEST_ASSERT_NULL(cJSON_GetErrorPtr());
}

void test_cJSON_Parse_should_handle_null_in_array(void)
{
    const char *json = "[null]";
    test_item = cJSON_Parse(json);
    TEST_ASSERT_NOT_NULL(test_item);
    TEST_ASSERT_EQUAL_INT(cJSON_Array, test_item->type);
    cJSON *item = cJSON_GetArrayItem(test_item, 0);
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_EQUAL_INT(cJSON_NULL, item->type);
    TEST_ASSERT_NULL(cJSON_GetErrorPtr());
}

void test_cJSON_Parse_should_handle_string_with_special_characters(void)
{
    const char *json = "\"!@#$%^&*()_+-=[]{}|;:',.<>?/~`\"";
    test_item = cJSON_Parse(json);
    TEST_ASSERT_NOT_NULL(test_item);
    TEST_ASSERT_EQUAL_INT(cJSON_String, test_item->type);
    TEST_ASSERT_EQUAL_STRING("!@#$%^&*()_+-=[]{}|;:',.<>?/~`", test_item->valuestring);
    TEST_ASSERT_NULL(cJSON_GetErrorPtr());
}

void test_cJSON_Parse_should_handle_zero_number(void)
{
    const char *json = "0";
    test_item = cJSON_Parse(json);
    TEST_ASSERT_NOT_NULL(test_item);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item->type);
    TEST_ASSERT_EQUAL_DOUBLE(0.0, test_item->valuedouble);
    TEST_ASSERT_NULL(cJSON_GetErrorPtr());
}

void test_cJSON_Parse_should_handle_negative_zero_number(void)
{
    const char *json = "-0";
    test_item = cJSON_Parse(json);
    TEST_ASSERT_NOT_NULL(test_item);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item->type);
    TEST_ASSERT_EQUAL_DOUBLE(-0.0, test_item->valuedouble);
    TEST_ASSERT_NULL(cJSON_GetErrorPtr());
}

void test_cJSON_Parse_should_handle_string_with_unicode_surrogate_pair(void)
{
    /* Unicode surrogate pair for U+1F600 (grinning face emoji) */
    const char *json = "\"\\uD83D\\uDE00\"";
    test_item = cJSON_Parse(json);
    TEST_ASSERT_NOT_NULL(test_item);
    TEST_ASSERT_EQUAL_INT(cJSON_String, test_item->type);
    /* cJSON converts surrogate pairs to UTF-8 */
    TEST_ASSERT_EQUAL_STRING("\xF0\x9F\x98\x80", test_item->valuestring);
    TEST_ASSERT_NULL(cJSON_GetErrorPtr());
}

void test_cJSON_Parse_should_handle_multiple_top_level_values_failure(void)
{
    /* Multiple top-level values are invalid in standard JSON */
    const char *json = "1 2";
    test_item = cJSON_Parse(json);
    TEST_ASSERT_NULL(test_item);
    TEST_ASSERT_NOT_NULL(cJSON_GetErrorPtr());
}

void test_cJSON_Parse_should_handle_comment_failure(void)
{
    /* Comments are not supported in standard JSON */
    const char *json = "{/* comment */\"key\":1}";
    test_item = cJSON_Parse(json);
    TEST_ASSERT_NULL(test_item);
    TEST_ASSERT_NOT_NULL(cJSON_GetErrorPtr());
}

void setUp(void)
{
    test_item = NULL;
}

void tearDown(void)
{
    cleanup_test_item();
}

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_cJSON_Parse_should_parse_empty_object);
    RUN_TEST(test_cJSON_Parse_should_parse_empty_array);
    RUN_TEST(test_cJSON_Parse_should_parse_simple_string);
    RUN_TEST(test_cJSON_Parse_should_parse_simple_number);
    RUN_TEST(test_cJSON_Parse_should_parse_true_literal);
    RUN_TEST(test_cJSON_Parse_should_parse_false_literal);
    RUN_TEST(test_cJSON_Parse_should_parse_null_literal);
    RUN_TEST(test_cJSON_Parse_should_parse_nested_object);
    RUN_TEST(test_cJSON_Parse_should_parse_array_with_mixed_types);
    RUN_TEST(test_cJSON_Parse_should_handle_null_input);
    RUN_TEST(test_cJSON_Parse_should_handle_empty_string);
    RUN_TEST(test_cJSON_Parse_should_handle_invalid_json);
    RUN_TEST(test_cJSON_Parse_should_handle_string_with_escaped_characters);
    RUN_TEST(test_cJSON_Parse_should_handle_unicode_escape_sequence);
    RUN_TEST(test_cJSON_Parse_should_handle_negative_number);
    RUN_TEST(test_cJSON_Parse_should_handle_scientific_notation);
    RUN_TEST(test_cJSON_Parse_should_handle_large_integer);
    RUN_TEST(test_cJSON_Parse_should_handle_whitespace);
    RUN_TEST(test_cJSON_Parse_should_handle_array_with_trailing_comma_failure);
    RUN_TEST(test_cJSON_Parse_should_handle_deeply_nested_structure);
    RUN_TEST(test_cJSON_Parse_should_handle_raw_string);
    RUN_TEST(test_cJSON_Parse_should_handle_boolean_in_object);
    RUN_TEST(test_cJSON_Parse_should_handle_null_in_array);
    RUN_TEST(test_cJSON_Parse_should_handle_string_with_special_characters);
    RUN_TEST(test_cJSON_Parse_should_handle_zero_number);
    RUN_TEST(test_cJSON_Parse_should_handle_negative_zero_number);
    RUN_TEST(test_cJSON_Parse_should_handle_string_with_unicode_surrogate_pair);
    RUN_TEST(test_cJSON_Parse_should_handle_multiple_top_level_values_failure);
    RUN_TEST(test_cJSON_Parse_should_handle_comment_failure);
    return UNITY_END();
}