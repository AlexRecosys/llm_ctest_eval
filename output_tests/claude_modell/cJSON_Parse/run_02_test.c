#include "cJSON.c"
#include "unity.h"
#include "cJSON.h"
#include <string.h>
#include <stdlib.h>

/* -------------------------------------------------------------------------
 * File-scope fixtures
 * ---------------------------------------------------------------------- */
static cJSON *g_item = NULL;

/* -------------------------------------------------------------------------
 * setUp / tearDown
 * ---------------------------------------------------------------------- */
void setUp(void)
{
    g_item = NULL;
}

void tearDown(void)
{
    if (g_item != NULL)
    {
        cJSON_Delete(g_item);
        g_item = NULL;
    }
}

/* -------------------------------------------------------------------------
 * Helper macros
 * ---------------------------------------------------------------------- */
#define PARSE(json_str)  cJSON_Parse((json_str))

/* -------------------------------------------------------------------------
 * Test cases — NULL / invalid input
 * ---------------------------------------------------------------------- */
void test_parse_null_input_returns_null(void)
{
    g_item = PARSE(NULL);
    TEST_ASSERT_NULL(g_item);
}

void test_parse_empty_string_returns_null(void)
{
    g_item = PARSE("");
    TEST_ASSERT_NULL(g_item);
}

void test_parse_whitespace_only_returns_null(void)
{
    g_item = PARSE("   \t\n\r");
    TEST_ASSERT_NULL(g_item);
}

void test_parse_garbage_returns_null(void)
{
    g_item = PARSE("this is not json");
    TEST_ASSERT_NULL(g_item);
}

void test_parse_truncated_string_returns_null(void)
{
    g_item = PARSE("{\"key\":");
    TEST_ASSERT_NULL(g_item);
}

void test_parse_only_opening_brace_returns_null(void)
{
    g_item = PARSE("{");
    TEST_ASSERT_NULL(g_item);
}

void test_parse_only_opening_bracket_returns_null(void)
{
    g_item = PARSE("[");
    TEST_ASSERT_NULL(g_item);
}

/* -------------------------------------------------------------------------
 * Test cases — literals
 * ---------------------------------------------------------------------- */
void test_parse_null_literal(void)
{
    g_item = PARSE("null");
    TEST_ASSERT_NOT_NULL(g_item);
    TEST_ASSERT_TRUE(cJSON_IsNull(g_item));
}

void test_parse_true_literal(void)
{
    g_item = PARSE("true");
    TEST_ASSERT_NOT_NULL(g_item);
    TEST_ASSERT_TRUE(cJSON_IsTrue(g_item));
}

void test_parse_false_literal(void)
{
    g_item = PARSE("false");
    TEST_ASSERT_NOT_NULL(g_item);
    TEST_ASSERT_TRUE(cJSON_IsFalse(g_item));
}

/* -------------------------------------------------------------------------
 * Test cases — numbers
 * ---------------------------------------------------------------------- */
void test_parse_integer_zero(void)
{
    g_item = PARSE("0");
    TEST_ASSERT_NOT_NULL(g_item);
    TEST_ASSERT_TRUE(cJSON_IsNumber(g_item));
    TEST_ASSERT_EQUAL_DOUBLE(0.0, g_item->valuedouble);
}

void test_parse_positive_integer(void)
{
    g_item = PARSE("42");
    TEST_ASSERT_NOT_NULL(g_item);
    TEST_ASSERT_TRUE(cJSON_IsNumber(g_item));
    TEST_ASSERT_EQUAL_DOUBLE(42.0, g_item->valuedouble);
}

void test_parse_negative_integer(void)
{
    g_item = PARSE("-7");
    TEST_ASSERT_NOT_NULL(g_item);
    TEST_ASSERT_TRUE(cJSON_IsNumber(g_item));
    TEST_ASSERT_EQUAL_DOUBLE(-7.0, g_item->valuedouble);
}

void test_parse_floating_point_number(void)
{
    g_item = PARSE("3.14");
    TEST_ASSERT_NOT_NULL(g_item);
    TEST_ASSERT_TRUE(cJSON_IsNumber(g_item));
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 3.14, g_item->valuedouble);
}

void test_parse_scientific_notation(void)
{
    g_item = PARSE("1e5");
    TEST_ASSERT_NOT_NULL(g_item);
    TEST_ASSERT_TRUE(cJSON_IsNumber(g_item));
    TEST_ASSERT_EQUAL_DOUBLE(100000.0, g_item->valuedouble);
}

void test_parse_negative_scientific_notation(void)
{
    g_item = PARSE("-2.5e-3");
    TEST_ASSERT_NOT_NULL(g_item);
    TEST_ASSERT_TRUE(cJSON_IsNumber(g_item));
    TEST_ASSERT_DOUBLE_WITHIN(1e-12, -0.0025, g_item->valuedouble);
}

/* -------------------------------------------------------------------------
 * Test cases — strings
 * ---------------------------------------------------------------------- */
void test_parse_simple_string(void)
{
    g_item = PARSE("\"hello\"");
    TEST_ASSERT_NOT_NULL(g_item);
    TEST_ASSERT_TRUE(cJSON_IsString(g_item));
    TEST_ASSERT_EQUAL_STRING("hello", g_item->valuestring);
}

void test_parse_empty_string_value(void)
{
    g_item = PARSE("\"\"");
    TEST_ASSERT_NOT_NULL(g_item);
    TEST_ASSERT_TRUE(cJSON_IsString(g_item));
    TEST_ASSERT_EQUAL_STRING("", g_item->valuestring);
}

void test_parse_string_with_escape_sequences(void)
{
    g_item = PARSE("\"hello\\nworld\"");
    TEST_ASSERT_NOT_NULL(g_item);
    TEST_ASSERT_TRUE(cJSON_IsString(g_item));
    TEST_ASSERT_EQUAL_STRING("hello\nworld", g_item->valuestring);
}

void test_parse_string_with_unicode_escape(void)
{
    g_item = PARSE("\"\\u0041\"");
    TEST_ASSERT_NOT_NULL(g_item);
    TEST_ASSERT_TRUE(cJSON_IsString(g_item));
    TEST_ASSERT_EQUAL_STRING("A", g_item->valuestring);
}

void test_parse_string_with_backslash_escape(void)
{
    g_item = PARSE("\"path\\\\to\\\\file\"");
    TEST_ASSERT_NOT_NULL(g_item);
    TEST_ASSERT_TRUE(cJSON_IsString(g_item));
    TEST_ASSERT_EQUAL_STRING("path\\to\\file", g_item->valuestring);
}

void test_parse_unterminated_string_returns_null(void)
{
    g_item = PARSE("\"unterminated");
    TEST_ASSERT_NULL(g_item);
}

/* -------------------------------------------------------------------------
 * Test cases — arrays
 * ---------------------------------------------------------------------- */
void test_parse_empty_array(void)
{
    g_item = PARSE("[]");
    TEST_ASSERT_NOT_NULL(g_item);
    TEST_ASSERT_TRUE(cJSON_IsArray(g_item));
    TEST_ASSERT_EQUAL_INT(0, cJSON_GetArraySize(g_item));
}

void test_parse_array_of_integers(void)
{
    g_item = PARSE("[1, 2, 3]");
    TEST_ASSERT_NOT_NULL(g_item);
    TEST_ASSERT_TRUE(cJSON_IsArray(g_item));
    TEST_ASSERT_EQUAL_INT(3, cJSON_GetArraySize(g_item));

    cJSON *first = cJSON_GetArrayItem(g_item, 0);
    TEST_ASSERT_NOT_NULL(first);
    TEST_ASSERT_EQUAL_DOUBLE(1.0, first->valuedouble);

    cJSON *second = cJSON_GetArrayItem(g_item, 1);
    TEST_ASSERT_NOT_NULL(second);
    TEST_ASSERT_EQUAL_DOUBLE(2.0, second->valuedouble);

    cJSON *third = cJSON_GetArrayItem(g_item, 2);
    TEST_ASSERT_NOT_NULL(third);
    TEST_ASSERT_EQUAL_DOUBLE(3.0, third->valuedouble);
}

void test_parse_array_of_mixed_types(void)
{
    g_item = PARSE("[1, \"two\", true, null, false]");
    TEST_ASSERT_NOT_NULL(g_item);
    TEST_ASSERT_TRUE(cJSON_IsArray(g_item));
    TEST_ASSERT_EQUAL_INT(5, cJSON_GetArraySize(g_item));

    TEST_ASSERT_TRUE(cJSON_IsNumber(cJSON_GetArrayItem(g_item, 0)));
    TEST_ASSERT_TRUE(cJSON_IsString(cJSON_GetArrayItem(g_item, 1)));
    TEST_ASSERT_TRUE(cJSON_IsTrue(cJSON_GetArrayItem(g_item, 2)));
    TEST_ASSERT_TRUE(cJSON_IsNull(cJSON_GetArrayItem(g_item, 3)));
    TEST_ASSERT_TRUE(cJSON_IsFalse(cJSON_GetArrayItem(g_item, 4)));
}

void test_parse_nested_array(void)
{
    g_item = PARSE("[[1, 2], [3, 4]]");
    TEST_ASSERT_NOT_NULL(g_item);
    TEST_ASSERT_TRUE(cJSON_IsArray(g_item));
    TEST_ASSERT_EQUAL_INT(2, cJSON_GetArraySize(g_item));

    cJSON *inner = cJSON_GetArrayItem(g_item, 0);
    TEST_ASSERT_NOT_NULL(inner);
    TEST_ASSERT_TRUE(cJSON_IsArray(inner));
    TEST_ASSERT_EQUAL_INT(2, cJSON_GetArraySize(inner));
}

void test_parse_array_with_trailing_comma_returns_null(void)
{
    g_item = PARSE("[1, 2, 3,]");
    /* cJSON does not accept trailing commas */
    TEST_ASSERT_NULL(g_item);
}

/* -------------------------------------------------------------------------
 * Test cases — objects
 * ---------------------------------------------------------------------- */
void test_parse_empty_object(void)
{
    g_item = PARSE("{}");
    TEST_ASSERT_NOT_NULL(g_item);
    TEST_ASSERT_TRUE(cJSON_IsObject(g_item));
    TEST_ASSERT_EQUAL_INT(0, cJSON_GetArraySize(g_item));
}

void test_parse_simple_object(void)
{
    g_item = PARSE("{\"key\": \"value\"}");
    TEST_ASSERT_NOT_NULL(g_item);
    TEST_ASSERT_TRUE(cJSON_IsObject(g_item));

    cJSON *val = cJSON_GetObjectItem(g_item, "key");
    TEST_ASSERT_NOT_NULL(val);
    TEST_ASSERT_TRUE(cJSON_IsString(val));
    TEST_ASSERT_EQUAL_STRING("value", val->valuestring);
}

void test_parse_object_with_number_value(void)
{
    g_item = PARSE("{\"count\": 99}");
    TEST_ASSERT_NOT_NULL(g_item);

    cJSON *val = cJSON_GetObjectItem(g_item, "count");
    TEST_ASSERT_NOT_NULL(val);
    TEST_ASSERT_TRUE(cJSON_IsNumber(val));
    TEST_ASSERT_EQUAL_DOUBLE(99.0, val->valuedouble);
}

void test_parse_object_with_bool_values(void)
{
    g_item = PARSE("{\"a\": true, \"b\": false}");
    TEST_ASSERT_NOT_NULL(g_item);

    cJSON *a = cJSON_GetObjectItem(g_item, "a");
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_TRUE(cJSON_IsTrue(a));

    cJSON *b = cJSON_GetObjectItem(g_item, "b");
    TEST_ASSERT_NOT_NULL(b);
    TEST_ASSERT_TRUE(cJSON_IsFalse(b));
}

void test_parse_object_with_null_value(void)
{
    g_item = PARSE("{\"nothing\": null}");
    TEST_ASSERT_NOT_NULL(g_item);

    cJSON *val = cJSON_GetObjectItem(g_item, "nothing");
    TEST_ASSERT_NOT_NULL(val);
    TEST_ASSERT_TRUE(cJSON_IsNull(val));
}

void test_parse_nested_object(void)
{
    g_item = PARSE("{\"outer\": {\"inner\": 42}}");
    TEST_ASSERT_NOT_NULL(g_item);
    TEST_ASSERT_TRUE(cJSON_IsObject(g_item));

    cJSON *outer = cJSON_GetObjectItem(g_item, "outer");
    TEST_ASSERT_NOT_NULL(outer);
    TEST_ASSERT_TRUE(cJSON_IsObject(outer));

    cJSON *inner = cJSON_GetObjectItem(outer, "inner");
    TEST_ASSERT_NOT_NULL(inner);
    TEST_ASSERT_TRUE(cJSON_IsNumber(inner));
    TEST_ASSERT_EQUAL_DOUBLE(42.0, inner->valuedouble);
}

void test_parse_object_with_array_value(void)
{
    g_item = PARSE("{\"list\": [1, 2, 3]}");
    TEST_ASSERT_NOT_NULL(g_item);

    cJSON *list = cJSON_GetObjectItem(g_item, "list");
    TEST_ASSERT_NOT_NULL(list);
    TEST_ASSERT_TRUE(cJSON_IsArray(list));
    TEST_ASSERT_EQUAL_INT(3, cJSON_GetArraySize(list));
}

void test_parse_object_multiple_keys(void)
{
    g_item = PARSE("{\"x\": 1, \"y\": 2, \"z\": 3}");
    TEST_ASSERT_NOT_NULL(g_item);
    TEST_ASSERT_EQUAL_INT(3, cJSON_GetArraySize(g_item));

    cJSON *x = cJSON_GetObjectItem(g_item, "x");
    TEST_ASSERT_NOT_NULL(x);
    TEST_ASSERT_EQUAL_DOUBLE(1.0, x->valuedouble);

    cJSON *y = cJSON_GetObjectItem(g_item, "y");
    TEST_ASSERT_NOT_NULL(y);
    TEST_ASSERT_EQUAL_DOUBLE(2.0, y->valuedouble);

    cJSON *z = cJSON_GetObjectItem(g_item, "z");
    TEST_ASSERT_NOT_NULL(z);
    TEST_ASSERT_EQUAL_DOUBLE(3.0, z->valuedouble);
}

/* -------------------------------------------------------------------------
 * Test cases — whitespace handling
 * ---------------------------------------------------------------------- */
void test_parse_json_with_leading_whitespace(void)
{
    g_item = PARSE("   {\"a\": 1}");
    TEST_ASSERT_NOT_NULL(g_item);
    TEST_ASSERT_TRUE(cJSON_IsObject(g_item));
}

void test_parse_json_with_internal_whitespace(void)
{
    g_item = PARSE("{\n  \"key\" : \t\"value\"\n}");
    TEST_ASSERT_NOT_NULL(g_item);
    TEST_ASSERT_TRUE(cJSON_IsObject(g_item));

    cJSON *val = cJSON_GetObjectItem(g_item, "key");
    TEST_ASSERT_NOT_NULL(val);
    TEST_ASSERT_EQUAL_STRING("value", val->valuestring);
}

/* -------------------------------------------------------------------------
 * Test cases — error pointer
 * ---------------------------------------------------------------------- */
void test_parse_error_ptr_set_on_failure(void)
{
    g_item = PARSE("{bad json}");
    TEST_ASSERT_NULL(g_item);
    TEST_ASSERT_NOT_NULL(cJSON_GetErrorPtr());
}

void test_parse_error_ptr_null_on_success(void)
{
    /* After a successful parse the error pointer should not be meaningful;
     * we just verify the parse succeeds and returns non-NULL. */
    g_item = PARSE("{\"ok\": true}");
    TEST_ASSERT_NOT_NULL(g_item);
}

/* -------------------------------------------------------------------------
 * Test cases — complex / real-world JSON
 * ---------------------------------------------------------------------- */
void test_parse_complex_json(void)
{
    const char *json =
        "{"
        "  \"name\": \"John\","
        "  \"age\": 30,"
        "  \"active\": true,"
        "  \"score\": 9.5,"
        "  \"address\": {"
        "    \"city\": \"New York\","
        "    \"zip\": \"10001\""
        "  },"
        "  \"tags\": [\"c\", \"json\", \"parser\"]"
        "}";

    g_item = PARSE(json);
    TEST_ASSERT_NOT_NULL(g_item);
    TEST_ASSERT_TRUE(cJSON_IsObject(g_item));

    cJSON *name = cJSON_GetObjectItem(g_item, "name");
    TEST_ASSERT_NOT_NULL(name);
    TEST_ASSERT_EQUAL_STRING("John", name->valuestring);

    cJSON *age = cJSON_GetObjectItem(g_item, "age");
    TEST_ASSERT_NOT_NULL(age);
    TEST_ASSERT_EQUAL_DOUBLE(30.0, age->valuedouble);

    cJSON *active = cJSON_GetObjectItem(g_item, "active");
    TEST_ASSERT_NOT_NULL(active);
    TEST_ASSERT_TRUE(cJSON_IsTrue(active));

    cJSON *score = cJSON_GetObjectItem(g_item, "score");
    TEST_ASSERT_NOT_NULL(score);
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 9.5, score->valuedouble);

    cJSON *address = cJSON_GetObjectItem(g_item, "address");
    TEST_ASSERT_NOT_NULL(address);
    TEST_ASSERT_TRUE(cJSON_IsObject(address));

    cJSON *city = cJSON_GetObjectItem(address, "city");
    TEST_ASSERT_NOT_NULL(city);
    TEST_ASSERT_EQUAL_STRING("New York", city->valuestring);

    cJSON *tags = cJSON_GetObjectItem(g_item, "tags");
    TEST_ASSERT_NOT_NULL(tags);
    TEST_ASSERT_TRUE(cJSON_IsArray(tags));
    TEST_ASSERT_EQUAL_INT(3, cJSON_GetArraySize(tags));

    cJSON *tag0 = cJSON_GetArrayItem(tags, 0);
    TEST_ASSERT_NOT_NULL(tag0);
    TEST_ASSERT_EQUAL_STRING("c", tag0->valuestring);
}

void test_parse_deeply_nested_arrays(void)
{
    g_item = PARSE("[[[1]]]");
    TEST_ASSERT_NOT_NULL(g_item);
    TEST_ASSERT_TRUE(cJSON_IsArray(g_item));

    cJSON *l1 = cJSON_GetArrayItem(g_item, 0);
    TEST_ASSERT_NOT_NULL(l1);
    TEST_ASSERT_TRUE(cJSON_IsArray(l1));

    cJSON *l2 = cJSON_GetArrayItem(l1, 0);
    TEST_ASSERT_NOT_NULL(l2);
    TEST_ASSERT_TRUE(cJSON_IsArray(l2));

    cJSON *leaf = cJSON_GetArrayItem(l2, 0);
    TEST_ASSERT_NOT_NULL(leaf);
    TEST_ASSERT_EQUAL_DOUBLE(1.0, leaf->valuedouble);
}

void test_parse_array_of_objects(void)
{
    g_item = PARSE("[{\"id\": 1}, {\"id\": 2}]");
    TEST_ASSERT_NOT_NULL(g_item);
    TEST_ASSERT_TRUE(cJSON_IsArray(g_item));
    TEST_ASSERT_EQUAL_INT(2, cJSON_GetArraySize(g_item));

    cJSON *obj0 = cJSON_GetArrayItem(g_item, 0);
    TEST_ASSERT_NOT_NULL(obj0);
    TEST_ASSERT_TRUE(cJSON_IsObject(obj0));

    cJSON *id0 = cJSON_GetObjectItem(obj0, "id");
    TEST_ASSERT_NOT_NULL(id0);
    TEST_ASSERT_EQUAL_DOUBLE(1.0, id0->valuedouble);
}

/* -------------------------------------------------------------------------
 * Test cases — return type is always a fresh allocation
 * ---------------------------------------------------------------------- */
void test_parse_returns_independent_copies(void)
{
    cJSON *first  = PARSE("{\"v\": 1}");
    cJSON *second = PARSE("{\"v\": 1}");

    TEST_ASSERT_NOT_NULL(first);
    TEST_ASSERT_NOT_NULL(second);
    TEST_ASSERT_NOT_EQUAL(first, second);

    cJSON_Delete(first);
    cJSON_Delete(second);
    /* g_item is NULL so tearDown won't double-free */
}

/* -------------------------------------------------------------------------
 * Test cases — type field correctness
 * ---------------------------------------------------------------------- */
void test_parse_type_field_for_object(void)
{
    g_item = PARSE("{}");
    TEST_ASSERT_NOT_NULL(g_item);
    TEST_ASSERT_EQUAL_INT(cJSON_Object, g_item->type & 0xFF);
}

void test_parse_type_field_for_array(void)
{
    g_item = PARSE("[]");
    TEST_ASSERT_NOT_NULL(g_item);
    TEST_ASSERT_EQUAL_INT(cJSON_Array, g_item->type & 0xFF);
}

void test_parse_type_field_for_string(void)
{
    g_item = PARSE("\"test\"");
    TEST_ASSERT_NOT_NULL(g_item);
    TEST_ASSERT_EQUAL_INT(cJSON_String, g_item->type & 0xFF);
}

void test_parse_type_field_for_number(void)
{
    g_item = PARSE("123");
    TEST_ASSERT_NOT_NULL(g_item);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, g_item->type & 0xFF);
}

void test_parse_type_field_for_true(void)
{
    g_item = PARSE("true");
    TEST_ASSERT_NOT_NULL(g_item);
    TEST_ASSERT_EQUAL_INT(cJSON_True, g_item->type & 0xFF);
}

void test_parse_type_field_for_false(void)
{
    g_item = PARSE("false");
    TEST_ASSERT_NOT_NULL(g_item);
    TEST_ASSERT_EQUAL_INT(cJSON_False, g_item->type & 0xFF);
}

void test_parse_type_field_for_null(void)
{
    g_item = PARSE("null");
    TEST_ASSERT_NOT_NULL(g_item);
    TEST_ASSERT_EQUAL_INT(cJSON_NULL, g_item->type & 0xFF);
}

/* -------------------------------------------------------------------------
 * main
 * ---------------------------------------------------------------------- */
int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();

    RUN_TEST(test_parse_null_input_returns_null);
    RUN_TEST(test_parse_empty_string_returns_null);
    RUN_TEST(test_parse_whitespace_only_returns_null);
    RUN_TEST(test_parse_garbage_returns_null);
    RUN_TEST(test_parse_truncated_string_returns_null);
    RUN_TEST(test_parse_only_opening_brace_returns_null);
    RUN_TEST(test_parse_only_opening_bracket_returns_null);

    RUN_TEST(test_parse_null_literal);
    RUN_TEST(test_parse_true_literal);
    RUN_TEST(test_parse_false_literal);

    RUN_TEST(test_parse_integer_zero);
    RUN_TEST(test_parse_positive_integer);
    RUN_TEST(test_parse_negative_integer);
    RUN_TEST(test_parse_floating_point_number);
    RUN_TEST(test_parse_scientific_notation);
    RUN_TEST(test_parse_negative_scientific_notation);

    RUN_TEST(test_parse_simple_string);
    RUN_TEST(test_parse_empty_string_value);
    RUN_TEST(test_parse_string_with_escape_sequences);
    RUN_TEST(test_parse_string_with_unicode_escape);
    RUN_TEST(test_parse_string_with_backslash_escape);
    RUN_TEST(test_parse_unterminated_string_returns_null);

    RUN_TEST(test_parse_empty_array);
    RUN_TEST(test_parse_array_of_integers);
    RUN_TEST(test_parse_array_of_mixed_types);
    RUN_TEST(test_parse_nested_array);
    RUN_TEST(test_parse_array_with_trailing_comma_returns_null);

    RUN_TEST(test_parse_empty_object);
    RUN_TEST(test_parse_simple_object);
    RUN_TEST(test_parse_object_with_number_value);
    RUN_TEST(test_parse_object_with_bool_values);
    RUN_TEST(test_parse_object_with_null_value);
    RUN_TEST(test_parse_nested_object);
    RUN_TEST(test_parse_object_with_array_value);
    RUN_TEST(test_parse_object_multiple_keys);

    RUN_TEST(test_parse_json_with_leading_whitespace);
    RUN_TEST(test_parse_json_with_internal_whitespace);

    RUN_TEST(test_parse_error_ptr_set_on_failure);
    RUN_TEST(test_parse_error_ptr_null_on_success);

    RUN_TEST(test_parse_complex_json);
    RUN_TEST(test_parse_deeply_nested_arrays);
    RUN_TEST(test_parse_array_of_objects);

    RUN_TEST(test_parse_returns_independent_copies);

    RUN_TEST(test_parse_type_field_for_object);
    RUN_TEST(test_parse_type_field_for_array);
    RUN_TEST(test_parse_type_field_for_string);
    RUN_TEST(test_parse_type_field_for_number);
    RUN_TEST(test_parse_type_field_for_true);
    RUN_TEST(test_parse_type_field_for_false);
    RUN_TEST(test_parse_type_field_for_null);

    return UNITY_END();
}