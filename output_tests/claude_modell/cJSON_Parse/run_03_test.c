#include "cJSON.c"
#include "unity.h"
#include "cJSON.h"
#include <string.h>
#include <stdlib.h>

/* File-scope variables */
static cJSON *parsed_item = NULL;

/* setUp and tearDown */
void setUp(void)
{
    parsed_item = NULL;
}

void tearDown(void)
{
    if (parsed_item != NULL)
    {
        cJSON_Delete(parsed_item);
        parsed_item = NULL;
    }
}

/* Helper macros */
#define PARSE_AND_STORE(json_str) \
    parsed_item = cJSON_Parse(json_str); \
    TEST_ASSERT_NOT_NULL_MESSAGE(parsed_item, "cJSON_Parse returned NULL for: " json_str)

/* ===== Test Cases ===== */

void test_parse_null_literal(void)
{
    parsed_item = cJSON_Parse("null");
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_TRUE(cJSON_IsNull(parsed_item));
}

void test_parse_true_literal(void)
{
    parsed_item = cJSON_Parse("true");
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_TRUE(cJSON_IsTrue(parsed_item));
}

void test_parse_false_literal(void)
{
    parsed_item = cJSON_Parse("false");
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_TRUE(cJSON_IsFalse(parsed_item));
}

void test_parse_integer_number(void)
{
    parsed_item = cJSON_Parse("42");
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_TRUE(cJSON_IsNumber(parsed_item));
    TEST_ASSERT_EQUAL_DOUBLE(42.0, parsed_item->valuedouble);
}

void test_parse_negative_number(void)
{
    parsed_item = cJSON_Parse("-7");
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_TRUE(cJSON_IsNumber(parsed_item));
    TEST_ASSERT_EQUAL_DOUBLE(-7.0, parsed_item->valuedouble);
}

void test_parse_floating_point_number(void)
{
    parsed_item = cJSON_Parse("3.14");
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_TRUE(cJSON_IsNumber(parsed_item));
    TEST_ASSERT_DOUBLE_WITHIN(1e-6, 3.14, parsed_item->valuedouble);
}

void test_parse_zero(void)
{
    parsed_item = cJSON_Parse("0");
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_TRUE(cJSON_IsNumber(parsed_item));
    TEST_ASSERT_EQUAL_DOUBLE(0.0, parsed_item->valuedouble);
}

void test_parse_simple_string(void)
{
    parsed_item = cJSON_Parse("\"hello\"");
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_TRUE(cJSON_IsString(parsed_item));
    TEST_ASSERT_EQUAL_STRING("hello", parsed_item->valuestring);
}

void test_parse_empty_string(void)
{
    parsed_item = cJSON_Parse("\"\"");
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_TRUE(cJSON_IsString(parsed_item));
    TEST_ASSERT_EQUAL_STRING("", parsed_item->valuestring);
}

void test_parse_string_with_escape(void)
{
    parsed_item = cJSON_Parse("\"hello\\nworld\"");
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_TRUE(cJSON_IsString(parsed_item));
    TEST_ASSERT_EQUAL_STRING("hello\nworld", parsed_item->valuestring);
}

void test_parse_empty_array(void)
{
    parsed_item = cJSON_Parse("[]");
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_TRUE(cJSON_IsArray(parsed_item));
    TEST_ASSERT_EQUAL_INT(0, cJSON_GetArraySize(parsed_item));
}

void test_parse_array_with_elements(void)
{
    parsed_item = cJSON_Parse("[1, 2, 3]");
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_TRUE(cJSON_IsArray(parsed_item));
    TEST_ASSERT_EQUAL_INT(3, cJSON_GetArraySize(parsed_item));

    cJSON *item0 = cJSON_GetArrayItem(parsed_item, 0);
    cJSON *item1 = cJSON_GetArrayItem(parsed_item, 1);
    cJSON *item2 = cJSON_GetArrayItem(parsed_item, 2);

    TEST_ASSERT_NOT_NULL(item0);
    TEST_ASSERT_NOT_NULL(item1);
    TEST_ASSERT_NOT_NULL(item2);

    TEST_ASSERT_EQUAL_DOUBLE(1.0, item0->valuedouble);
    TEST_ASSERT_EQUAL_DOUBLE(2.0, item1->valuedouble);
    TEST_ASSERT_EQUAL_DOUBLE(3.0, item2->valuedouble);
}

void test_parse_array_mixed_types(void)
{
    parsed_item = cJSON_Parse("[1, \"two\", true, null]");
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_TRUE(cJSON_IsArray(parsed_item));
    TEST_ASSERT_EQUAL_INT(4, cJSON_GetArraySize(parsed_item));

    cJSON *item0 = cJSON_GetArrayItem(parsed_item, 0);
    cJSON *item1 = cJSON_GetArrayItem(parsed_item, 1);
    cJSON *item2 = cJSON_GetArrayItem(parsed_item, 2);
    cJSON *item3 = cJSON_GetArrayItem(parsed_item, 3);

    TEST_ASSERT_TRUE(cJSON_IsNumber(item0));
    TEST_ASSERT_TRUE(cJSON_IsString(item1));
    TEST_ASSERT_TRUE(cJSON_IsTrue(item2));
    TEST_ASSERT_TRUE(cJSON_IsNull(item3));
}

void test_parse_empty_object(void)
{
    parsed_item = cJSON_Parse("{}");
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_TRUE(cJSON_IsObject(parsed_item));
    TEST_ASSERT_EQUAL_INT(0, cJSON_GetArraySize(parsed_item));
}

void test_parse_simple_object(void)
{
    parsed_item = cJSON_Parse("{\"key\": \"value\"}");
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_TRUE(cJSON_IsObject(parsed_item));

    cJSON *key_item = cJSON_GetObjectItem(parsed_item, "key");
    TEST_ASSERT_NOT_NULL(key_item);
    TEST_ASSERT_TRUE(cJSON_IsString(key_item));
    TEST_ASSERT_EQUAL_STRING("value", key_item->valuestring);
}

void test_parse_object_with_number(void)
{
    parsed_item = cJSON_Parse("{\"count\": 42}");
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_TRUE(cJSON_IsObject(parsed_item));

    cJSON *count_item = cJSON_GetObjectItem(parsed_item, "count");
    TEST_ASSERT_NOT_NULL(count_item);
    TEST_ASSERT_TRUE(cJSON_IsNumber(count_item));
    TEST_ASSERT_EQUAL_DOUBLE(42.0, count_item->valuedouble);
}

void test_parse_object_multiple_keys(void)
{
    parsed_item = cJSON_Parse("{\"a\": 1, \"b\": 2, \"c\": 3}");
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_TRUE(cJSON_IsObject(parsed_item));
    TEST_ASSERT_EQUAL_INT(3, cJSON_GetArraySize(parsed_item));

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

void test_parse_nested_object(void)
{
    parsed_item = cJSON_Parse("{\"outer\": {\"inner\": 99}}");
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

void test_parse_nested_array(void)
{
    parsed_item = cJSON_Parse("[[1, 2], [3, 4]]");
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_TRUE(cJSON_IsArray(parsed_item));
    TEST_ASSERT_EQUAL_INT(2, cJSON_GetArraySize(parsed_item));

    cJSON *sub0 = cJSON_GetArrayItem(parsed_item, 0);
    cJSON *sub1 = cJSON_GetArrayItem(parsed_item, 1);

    TEST_ASSERT_NOT_NULL(sub0);
    TEST_ASSERT_NOT_NULL(sub1);
    TEST_ASSERT_TRUE(cJSON_IsArray(sub0));
    TEST_ASSERT_TRUE(cJSON_IsArray(sub1));

    TEST_ASSERT_EQUAL_INT(2, cJSON_GetArraySize(sub0));
    TEST_ASSERT_EQUAL_INT(2, cJSON_GetArraySize(sub1));
}

void test_parse_null_input_returns_null(void)
{
    parsed_item = cJSON_Parse(NULL);
    TEST_ASSERT_NULL(parsed_item);
}

void test_parse_empty_string_input_returns_null(void)
{
    parsed_item = cJSON_Parse("");
    TEST_ASSERT_NULL(parsed_item);
}

void test_parse_invalid_json_returns_null(void)
{
    parsed_item = cJSON_Parse("{invalid}");
    TEST_ASSERT_NULL(parsed_item);
}

void test_parse_invalid_json_sets_error_ptr(void)
{
    parsed_item = cJSON_Parse("{invalid}");
    TEST_ASSERT_NULL(parsed_item);
    TEST_ASSERT_NOT_NULL(cJSON_GetErrorPtr());
}

void test_parse_truncated_json_returns_null(void)
{
    parsed_item = cJSON_Parse("{\"key\":");
    TEST_ASSERT_NULL(parsed_item);
}

void test_parse_only_whitespace_returns_null(void)
{
    parsed_item = cJSON_Parse("   ");
    TEST_ASSERT_NULL(parsed_item);
}

void test_parse_json_with_leading_whitespace(void)
{
    parsed_item = cJSON_Parse("   42");
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_TRUE(cJSON_IsNumber(parsed_item));
    TEST_ASSERT_EQUAL_DOUBLE(42.0, parsed_item->valuedouble);
}

void test_parse_json_with_trailing_whitespace(void)
{
    parsed_item = cJSON_Parse("42   ");
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_TRUE(cJSON_IsNumber(parsed_item));
    TEST_ASSERT_EQUAL_DOUBLE(42.0, parsed_item->valuedouble);
}

void test_parse_scientific_notation(void)
{
    parsed_item = cJSON_Parse("1e10");
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_TRUE(cJSON_IsNumber(parsed_item));
    TEST_ASSERT_DOUBLE_WITHIN(1.0, 1e10, parsed_item->valuedouble);
}

void test_parse_negative_scientific_notation(void)
{
    parsed_item = cJSON_Parse("-1.5e-3");
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_TRUE(cJSON_IsNumber(parsed_item));
    TEST_ASSERT_DOUBLE_WITHIN(1e-10, -1.5e-3, parsed_item->valuedouble);
}

void test_parse_unicode_escape_in_string(void)
{
    parsed_item = cJSON_Parse("\"\\u0041\"");
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_TRUE(cJSON_IsString(parsed_item));
    TEST_ASSERT_EQUAL_STRING("A", parsed_item->valuestring);
}

void test_parse_object_with_bool_values(void)
{
    parsed_item = cJSON_Parse("{\"flag\": true, \"other\": false}");
    TEST_ASSERT_NOT_NULL(parsed_item);

    cJSON *flag = cJSON_GetObjectItem(parsed_item, "flag");
    cJSON *other = cJSON_GetObjectItem(parsed_item, "other");

    TEST_ASSERT_NOT_NULL(flag);
    TEST_ASSERT_NOT_NULL(other);
    TEST_ASSERT_TRUE(cJSON_IsTrue(flag));
    TEST_ASSERT_TRUE(cJSON_IsFalse(other));
}

void test_parse_object_with_null_value(void)
{
    parsed_item = cJSON_Parse("{\"nothing\": null}");
    TEST_ASSERT_NOT_NULL(parsed_item);

    cJSON *nothing = cJSON_GetObjectItem(parsed_item, "nothing");
    TEST_ASSERT_NOT_NULL(nothing);
    TEST_ASSERT_TRUE(cJSON_IsNull(nothing));
}

void test_parse_complex_json(void)
{
    const char *json = "{"
                       "\"name\": \"John\","
                       "\"age\": 30,"
                       "\"active\": true,"
                       "\"address\": {"
                       "  \"city\": \"New York\","
                       "  \"zip\": \"10001\""
                       "},"
                       "\"scores\": [95, 87, 92]"
                       "}";

    parsed_item = cJSON_Parse(json);
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_TRUE(cJSON_IsObject(parsed_item));

    cJSON *name = cJSON_GetObjectItem(parsed_item, "name");
    TEST_ASSERT_NOT_NULL(name);
    TEST_ASSERT_EQUAL_STRING("John", name->valuestring);

    cJSON *age = cJSON_GetObjectItem(parsed_item, "age");
    TEST_ASSERT_NOT_NULL(age);
    TEST_ASSERT_EQUAL_DOUBLE(30.0, age->valuedouble);

    cJSON *active = cJSON_GetObjectItem(parsed_item, "active");
    TEST_ASSERT_NOT_NULL(active);
    TEST_ASSERT_TRUE(cJSON_IsTrue(active));

    cJSON *address = cJSON_GetObjectItem(parsed_item, "address");
    TEST_ASSERT_NOT_NULL(address);
    TEST_ASSERT_TRUE(cJSON_IsObject(address));

    cJSON *city = cJSON_GetObjectItem(address, "city");
    TEST_ASSERT_NOT_NULL(city);
    TEST_ASSERT_EQUAL_STRING("New York", city->valuestring);

    cJSON *scores = cJSON_GetObjectItem(parsed_item, "scores");
    TEST_ASSERT_NOT_NULL(scores);
    TEST_ASSERT_TRUE(cJSON_IsArray(scores));
    TEST_ASSERT_EQUAL_INT(3, cJSON_GetArraySize(scores));
}

void test_parse_returns_non_null_for_valid_json(void)
{
    parsed_item = cJSON_Parse("{\"x\": 1}");
    TEST_ASSERT_NOT_NULL(parsed_item);
}

void test_parse_array_of_strings(void)
{
    parsed_item = cJSON_Parse("[\"a\", \"b\", \"c\"]");
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_TRUE(cJSON_IsArray(parsed_item));
    TEST_ASSERT_EQUAL_INT(3, cJSON_GetArraySize(parsed_item));

    cJSON *item0 = cJSON_GetArrayItem(parsed_item, 0);
    cJSON *item1 = cJSON_GetArrayItem(parsed_item, 1);
    cJSON *item2 = cJSON_GetArrayItem(parsed_item, 2);

    TEST_ASSERT_EQUAL_STRING("a", item0->valuestring);
    TEST_ASSERT_EQUAL_STRING("b", item1->valuestring);
    TEST_ASSERT_EQUAL_STRING("c", item2->valuestring);
}

void test_parse_string_with_tab_escape(void)
{
    parsed_item = cJSON_Parse("\"col1\\tcol2\"");
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_TRUE(cJSON_IsString(parsed_item));
    TEST_ASSERT_EQUAL_STRING("col1\tcol2", parsed_item->valuestring);
}

void test_parse_string_with_backslash_escape(void)
{
    parsed_item = cJSON_Parse("\"path\\\\to\\\\file\"");
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_TRUE(cJSON_IsString(parsed_item));
    TEST_ASSERT_EQUAL_STRING("path\\to\\file", parsed_item->valuestring);
}

void test_parse_object_case_insensitive_lookup(void)
{
    parsed_item = cJSON_Parse("{\"Key\": 1}");
    TEST_ASSERT_NOT_NULL(parsed_item);

    cJSON *item = cJSON_GetObjectItem(parsed_item, "key");
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_EQUAL_DOUBLE(1.0, item->valuedouble);
}

void test_parse_deeply_nested_array(void)
{
    parsed_item = cJSON_Parse("[[[1]]]");
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_TRUE(cJSON_IsArray(parsed_item));

    cJSON *level1 = cJSON_GetArrayItem(parsed_item, 0);
    TEST_ASSERT_NOT_NULL(level1);
    TEST_ASSERT_TRUE(cJSON_IsArray(level1));

    cJSON *level2 = cJSON_GetArrayItem(level1, 0);
    TEST_ASSERT_NOT_NULL(level2);
    TEST_ASSERT_TRUE(cJSON_IsArray(level2));

    cJSON *leaf = cJSON_GetArrayItem(level2, 0);
    TEST_ASSERT_NOT_NULL(leaf);
    TEST_ASSERT_TRUE(cJSON_IsNumber(leaf));
    TEST_ASSERT_EQUAL_DOUBLE(1.0, leaf->valuedouble);
}

void test_parse_missing_closing_brace_returns_null(void)
{
    parsed_item = cJSON_Parse("{\"key\": 1");
    TEST_ASSERT_NULL(parsed_item);
}

void test_parse_missing_closing_bracket_returns_null(void)
{
    parsed_item = cJSON_Parse("[1, 2, 3");
    TEST_ASSERT_NULL(parsed_item);
}

void test_parse_number_type_is_correct(void)
{
    parsed_item = cJSON_Parse("100");
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, parsed_item->type);
}

void test_parse_string_type_is_correct(void)
{
    parsed_item = cJSON_Parse("\"test\"");
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_EQUAL_INT(cJSON_String, parsed_item->type);
}

void test_parse_array_type_is_correct(void)
{
    parsed_item = cJSON_Parse("[]");
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_EQUAL_INT(cJSON_Array, parsed_item->type);
}

void test_parse_object_type_is_correct(void)
{
    parsed_item = cJSON_Parse("{}");
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_EQUAL_INT(cJSON_Object, parsed_item->type);
}

void test_parse_null_type_is_correct(void)
{
    parsed_item = cJSON_Parse("null");
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_EQUAL_INT(cJSON_NULL, parsed_item->type);
}

void test_parse_true_type_is_correct(void)
{
    parsed_item = cJSON_Parse("true");
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_EQUAL_INT(cJSON_True, parsed_item->type);
}

void test_parse_false_type_is_correct(void)
{
    parsed_item = cJSON_Parse("false");
    TEST_ASSERT_NOT_NULL(parsed_item);
    TEST_ASSERT_EQUAL_INT(cJSON_False, parsed_item->type);
}

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_parse_null_literal);
    RUN_TEST(test_parse_true_literal);
    RUN_TEST(test_parse_false_literal);
    RUN_TEST(test_parse_integer_number);
    RUN_TEST(test_parse_negative_number);
    RUN_TEST(test_parse_floating_point_number);
    RUN_TEST(test_parse_zero);
    RUN_TEST(test_parse_simple_string);
    RUN_TEST(test_parse_empty_string);
    RUN_TEST(test_parse_string_with_escape);
    RUN_TEST(test_parse_empty_array);
    RUN_TEST(test_parse_array_with_elements);
    RUN_TEST(test_parse_array_mixed_types);
    RUN_TEST(test_parse_empty_object);
    RUN_TEST(test_parse_simple_object);
    RUN_TEST(test_parse_object_with_number);
    RUN_TEST(test_parse_object_multiple_keys);
    RUN_TEST(test_parse_nested_object);
    RUN_TEST(test_parse_nested_array);
    RUN_TEST(test_parse_null_input_returns_null);
    RUN_TEST(test_parse_empty_string_input_returns_null);
    RUN_TEST(test_parse_invalid_json_returns_null);
    RUN_TEST(test_parse_invalid_json_sets_error_ptr);
    RUN_TEST(test_parse_truncated_json_returns_null);
    RUN_TEST(test_parse_only_whitespace_returns_null);
    RUN_TEST(test_parse_json_with_leading_whitespace);
    RUN_TEST(test_parse_json_with_trailing_whitespace);
    RUN_TEST(test_parse_scientific_notation);
    RUN_TEST(test_parse_negative_scientific_notation);
    RUN_TEST(test_parse_unicode_escape_in_string);
    RUN_TEST(test_parse_object_with_bool_values);
    RUN_TEST(test_parse_object_with_null_value);
    RUN_TEST(test_parse_complex_json);
    RUN_TEST(test_parse_returns_non_null_for_valid_json);
    RUN_TEST(test_parse_array_of_strings);
    RUN_TEST(test_parse_string_with_tab_escape);
    RUN_TEST(test_parse_string_with_backslash_escape);
    RUN_TEST(test_parse_object_case_insensitive_lookup);
    RUN_TEST(test_parse_deeply_nested_array);
    RUN_TEST(test_parse_missing_closing_brace_returns_null);
    RUN_TEST(test_parse_missing_closing_bracket_returns_null);
    RUN_TEST(test_parse_number_type_is_correct);
    RUN_TEST(test_parse_string_type_is_correct);
    RUN_TEST(test_parse_array_type_is_correct);
    RUN_TEST(test_parse_object_type_is_correct);
    RUN_TEST(test_parse_null_type_is_correct);
    RUN_TEST(test_parse_true_type_is_correct);
    RUN_TEST(test_parse_false_type_is_correct);
    return UNITY_END();
}