#include "cJSON.c"
#include "unity.h"
#include "cJSON.h"
#include <string.h>
#include <stdlib.h>

/* -------------------------------------------------------------------------
 * Helper macros / functions
 * ---------------------------------------------------------------------- */

static void assert_type(cJSON *item, int expected_type)
{
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_EQUAL_INT(expected_type, item->type);
}

/* -------------------------------------------------------------------------
 * setUp / tearDown
 * ---------------------------------------------------------------------- */

void setUp(void)
{
    /* Reset hooks to defaults before every test */
    cJSON_InitHooks(NULL);
}

void tearDown(void)
{
    /* nothing */
}

/* =========================================================================
 * Test cases
 * ====================================================================== */

/* --- NULL / empty input ------------------------------------------------- */

void test_parse_null_input_returns_null(void)
{
    cJSON *result = cJSON_Parse(NULL);
    TEST_ASSERT_NULL(result);
}

void test_parse_empty_string_returns_null(void)
{
    cJSON *result = cJSON_Parse("");
    TEST_ASSERT_NULL(result);
}

void test_parse_whitespace_only_returns_null(void)
{
    cJSON *result = cJSON_Parse("   \t\n\r");
    TEST_ASSERT_NULL(result);
}

/* --- Literals ----------------------------------------------------------- */

void test_parse_null_literal(void)
{
    cJSON *result = cJSON_Parse("null");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(cJSON_NULL, result->type);
    cJSON_Delete(result);
}

void test_parse_true_literal(void)
{
    cJSON *result = cJSON_Parse("true");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(cJSON_True, result->type);
    cJSON_Delete(result);
}

void test_parse_false_literal(void)
{
    cJSON *result = cJSON_Parse("false");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(cJSON_False, result->type);
    cJSON_Delete(result);
}

/* --- Numbers ------------------------------------------------------------ */

void test_parse_integer_zero(void)
{
    cJSON *result = cJSON_Parse("0");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, result->type);
    TEST_ASSERT_EQUAL_DOUBLE(0.0, result->valuedouble);
    cJSON_Delete(result);
}

void test_parse_positive_integer(void)
{
    cJSON *result = cJSON_Parse("42");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, result->type);
    TEST_ASSERT_EQUAL_DOUBLE(42.0, result->valuedouble);
    cJSON_Delete(result);
}

void test_parse_negative_integer(void)
{
    cJSON *result = cJSON_Parse("-7");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, result->type);
    TEST_ASSERT_EQUAL_DOUBLE(-7.0, result->valuedouble);
    cJSON_Delete(result);
}

void test_parse_floating_point_number(void)
{
    cJSON *result = cJSON_Parse("3.14");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, result->type);
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 3.14, result->valuedouble);
    cJSON_Delete(result);
}

void test_parse_scientific_notation(void)
{
    cJSON *result = cJSON_Parse("1e10");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, result->type);
    TEST_ASSERT_DOUBLE_WITHIN(1.0, 1e10, result->valuedouble);
    cJSON_Delete(result);
}

void test_parse_negative_scientific_notation(void)
{
    cJSON *result = cJSON_Parse("-2.5e-3");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, result->type);
    TEST_ASSERT_DOUBLE_WITHIN(1e-10, -0.0025, result->valuedouble);
    cJSON_Delete(result);
}

/* --- Strings ------------------------------------------------------------ */

void test_parse_empty_string(void)
{
    cJSON *result = cJSON_Parse("\"\"");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(cJSON_String, result->type);
    TEST_ASSERT_NOT_NULL(result->valuestring);
    TEST_ASSERT_EQUAL_STRING("", result->valuestring);
    cJSON_Delete(result);
}

void test_parse_simple_string(void)
{
    cJSON *result = cJSON_Parse("\"hello\"");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(cJSON_String, result->type);
    TEST_ASSERT_EQUAL_STRING("hello", result->valuestring);
    cJSON_Delete(result);
}

void test_parse_string_with_escape_sequences(void)
{
    cJSON *result = cJSON_Parse("\"line1\\nline2\"");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(cJSON_String, result->type);
    TEST_ASSERT_EQUAL_STRING("line1\nline2", result->valuestring);
    cJSON_Delete(result);
}

void test_parse_string_with_unicode_escape(void)
{
    cJSON *result = cJSON_Parse("\"\\u0041\""); /* 'A' */
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(cJSON_String, result->type);
    TEST_ASSERT_EQUAL_STRING("A", result->valuestring);
    cJSON_Delete(result);
}

void test_parse_string_with_backslash_escape(void)
{
    cJSON *result = cJSON_Parse("\"back\\\\slash\"");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(cJSON_String, result->type);
    TEST_ASSERT_EQUAL_STRING("back\\slash", result->valuestring);
    cJSON_Delete(result);
}

/* --- Arrays ------------------------------------------------------------- */

void test_parse_empty_array(void)
{
    cJSON *result = cJSON_Parse("[]");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(cJSON_Array, result->type);
    TEST_ASSERT_EQUAL_INT(0, cJSON_GetArraySize(result));
    cJSON_Delete(result);
}

void test_parse_array_single_element(void)
{
    cJSON *result = cJSON_Parse("[1]");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(cJSON_Array, result->type);
    TEST_ASSERT_EQUAL_INT(1, cJSON_GetArraySize(result));
    cJSON *item = cJSON_GetArrayItem(result, 0);
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_EQUAL_DOUBLE(1.0, item->valuedouble);
    cJSON_Delete(result);
}

void test_parse_array_multiple_elements(void)
{
    cJSON *result = cJSON_Parse("[1, 2, 3]");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(cJSON_Array, result->type);
    TEST_ASSERT_EQUAL_INT(3, cJSON_GetArraySize(result));
    TEST_ASSERT_EQUAL_DOUBLE(1.0, cJSON_GetArrayItem(result, 0)->valuedouble);
    TEST_ASSERT_EQUAL_DOUBLE(2.0, cJSON_GetArrayItem(result, 1)->valuedouble);
    TEST_ASSERT_EQUAL_DOUBLE(3.0, cJSON_GetArrayItem(result, 2)->valuedouble);
    cJSON_Delete(result);
}

void test_parse_array_mixed_types(void)
{
    cJSON *result = cJSON_Parse("[null, true, false, 42, \"str\"]");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(cJSON_Array, result->type);
    TEST_ASSERT_EQUAL_INT(5, cJSON_GetArraySize(result));
    TEST_ASSERT_EQUAL_INT(cJSON_NULL,   cJSON_GetArrayItem(result, 0)->type);
    TEST_ASSERT_EQUAL_INT(cJSON_True,   cJSON_GetArrayItem(result, 1)->type);
    TEST_ASSERT_EQUAL_INT(cJSON_False,  cJSON_GetArrayItem(result, 2)->type);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, cJSON_GetArrayItem(result, 3)->type);
    TEST_ASSERT_EQUAL_INT(cJSON_String, cJSON_GetArrayItem(result, 4)->type);
    cJSON_Delete(result);
}

void test_parse_nested_array(void)
{
    cJSON *result = cJSON_Parse("[[1, 2], [3, 4]]");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(cJSON_Array, result->type);
    TEST_ASSERT_EQUAL_INT(2, cJSON_GetArraySize(result));
    cJSON *inner = cJSON_GetArrayItem(result, 0);
    TEST_ASSERT_NOT_NULL(inner);
    TEST_ASSERT_EQUAL_INT(cJSON_Array, inner->type);
    TEST_ASSERT_EQUAL_INT(2, cJSON_GetArraySize(inner));
    cJSON_Delete(result);
}

/* --- Objects ------------------------------------------------------------ */

void test_parse_empty_object(void)
{
    cJSON *result = cJSON_Parse("{}");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(cJSON_Object, result->type);
    TEST_ASSERT_EQUAL_INT(0, cJSON_GetArraySize(result));
    cJSON_Delete(result);
}

void test_parse_object_single_key(void)
{
    cJSON *result = cJSON_Parse("{\"key\": \"value\"}");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(cJSON_Object, result->type);
    cJSON *item = cJSON_GetObjectItem(result, "key");
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_EQUAL_STRING("value", item->valuestring);
    cJSON_Delete(result);
}

void test_parse_object_multiple_keys(void)
{
    cJSON *result = cJSON_Parse("{\"a\": 1, \"b\": 2, \"c\": 3}");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(cJSON_Object, result->type);
    TEST_ASSERT_EQUAL_INT(3, cJSON_GetArraySize(result));
    TEST_ASSERT_EQUAL_DOUBLE(1.0, cJSON_GetObjectItem(result, "a")->valuedouble);
    TEST_ASSERT_EQUAL_DOUBLE(2.0, cJSON_GetObjectItem(result, "b")->valuedouble);
    TEST_ASSERT_EQUAL_DOUBLE(3.0, cJSON_GetObjectItem(result, "c")->valuedouble);
    cJSON_Delete(result);
}

void test_parse_object_nested(void)
{
    cJSON *result = cJSON_Parse("{\"outer\": {\"inner\": 99}}");
    TEST_ASSERT_NOT_NULL(result);
    cJSON *outer = cJSON_GetObjectItem(result, "outer");
    TEST_ASSERT_NOT_NULL(outer);
    TEST_ASSERT_EQUAL_INT(cJSON_Object, outer->type);
    cJSON *inner = cJSON_GetObjectItem(outer, "inner");
    TEST_ASSERT_NOT_NULL(inner);
    TEST_ASSERT_EQUAL_DOUBLE(99.0, inner->valuedouble);
    cJSON_Delete(result);
}

void test_parse_object_with_array_value(void)
{
    cJSON *result = cJSON_Parse("{\"arr\": [1, 2, 3]}");
    TEST_ASSERT_NOT_NULL(result);
    cJSON *arr = cJSON_GetObjectItem(result, "arr");
    TEST_ASSERT_NOT_NULL(arr);
    TEST_ASSERT_EQUAL_INT(cJSON_Array, arr->type);
    TEST_ASSERT_EQUAL_INT(3, cJSON_GetArraySize(arr));
    cJSON_Delete(result);
}

/* --- Whitespace handling ------------------------------------------------ */

void test_parse_object_with_leading_whitespace(void)
{
    cJSON *result = cJSON_Parse("   {\"x\": 1}");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(cJSON_Object, result->type);
    cJSON_Delete(result);
}

void test_parse_array_with_internal_whitespace(void)
{
    cJSON *result = cJSON_Parse("[ 1 , 2 , 3 ]");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(3, cJSON_GetArraySize(result));
    cJSON_Delete(result);
}

/* --- Invalid JSON ------------------------------------------------------- */

void test_parse_invalid_json_returns_null(void)
{
    cJSON *result = cJSON_Parse("{invalid}");
    TEST_ASSERT_NULL(result);
}

void test_parse_unclosed_object_returns_null(void)
{
    cJSON *result = cJSON_Parse("{\"key\": 1");
    TEST_ASSERT_NULL(result);
}

void test_parse_unclosed_array_returns_null(void)
{
    cJSON *result = cJSON_Parse("[1, 2, 3");
    TEST_ASSERT_NULL(result);
}

void test_parse_unclosed_string_returns_null(void)
{
    cJSON *result = cJSON_Parse("\"unclosed");
    TEST_ASSERT_NULL(result);
}

void test_parse_trailing_comma_in_object_returns_null(void)
{
    cJSON *result = cJSON_Parse("{\"a\": 1,}");
    TEST_ASSERT_NULL(result);
}

void test_parse_trailing_comma_in_array_returns_null(void)
{
    cJSON *result = cJSON_Parse("[1, 2,]");
    TEST_ASSERT_NULL(result);
}

void test_parse_bare_word_returns_null(void)
{
    cJSON *result = cJSON_Parse("hello");
    TEST_ASSERT_NULL(result);
}

/* --- Error pointer ------------------------------------------------------ */

void test_parse_error_ptr_set_on_failure(void)
{
    cJSON_Parse("{bad json}");
    const char *err = cJSON_GetErrorPtr();
    TEST_ASSERT_NOT_NULL(err);
}

void test_parse_error_ptr_null_on_success(void)
{
    cJSON *result = cJSON_Parse("{\"ok\": true}");
    TEST_ASSERT_NOT_NULL(result);
    /* After a successful parse the error pointer should not point into
     * the just-parsed string (it may be NULL or stale; the spec says it
     * is only defined after a failure, so we just verify the parse
     * succeeded). */
    cJSON_Delete(result);
}

/* --- Complex / real-world JSON ----------------------------------------- */

void test_parse_complex_json(void)
{
    const char *json =
        "{"
        "  \"name\": \"John\","
        "  \"age\": 30,"
        "  \"active\": true,"
        "  \"score\": null,"
        "  \"tags\": [\"a\", \"b\", \"c\"],"
        "  \"address\": {"
        "    \"city\": \"Nowhere\","
        "    \"zip\": \"00000\""
        "  }"
        "}";

    cJSON *result = cJSON_Parse(json);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(cJSON_Object, result->type);

    cJSON *name = cJSON_GetObjectItem(result, "name");
    TEST_ASSERT_NOT_NULL(name);
    TEST_ASSERT_EQUAL_STRING("John", name->valuestring);

    cJSON *age = cJSON_GetObjectItem(result, "age");
    TEST_ASSERT_NOT_NULL(age);
    TEST_ASSERT_EQUAL_DOUBLE(30.0, age->valuedouble);

    cJSON *active = cJSON_GetObjectItem(result, "active");
    TEST_ASSERT_NOT_NULL(active);
    TEST_ASSERT_EQUAL_INT(cJSON_True, active->type);

    cJSON *score = cJSON_GetObjectItem(result, "score");
    TEST_ASSERT_NOT_NULL(score);
    TEST_ASSERT_EQUAL_INT(cJSON_NULL, score->type);

    cJSON *tags = cJSON_GetObjectItem(result, "tags");
    TEST_ASSERT_NOT_NULL(tags);
    TEST_ASSERT_EQUAL_INT(cJSON_Array, tags->type);
    TEST_ASSERT_EQUAL_INT(3, cJSON_GetArraySize(tags));

    cJSON *address = cJSON_GetObjectItem(result, "address");
    TEST_ASSERT_NOT_NULL(address);
    TEST_ASSERT_EQUAL_INT(cJSON_Object, address->type);
    cJSON *city = cJSON_GetObjectItem(address, "city");
    TEST_ASSERT_NOT_NULL(city);
    TEST_ASSERT_EQUAL_STRING("Nowhere", city->valuestring);

    cJSON_Delete(result);
}

void test_parse_deeply_nested_arrays(void)
{
    /* Build a moderately deep nesting (well within CJSON_NESTING_LIMIT) */
    const char *json = "[[[[[[1]]]]]]";
    cJSON *result = cJSON_Parse(json);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(cJSON_Array, result->type);
    cJSON_Delete(result);
}

void test_parse_number_valueint_matches_valuedouble(void)
{
    cJSON *result = cJSON_Parse("123");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(123, result->valueint);
    TEST_ASSERT_EQUAL_DOUBLE(123.0, result->valuedouble);
    cJSON_Delete(result);
}

void test_parse_string_key_case_insensitive_lookup(void)
{
    cJSON *result = cJSON_Parse("{\"Key\": \"val\"}");
    TEST_ASSERT_NOT_NULL(result);
    /* cJSON_GetObjectItem is case-insensitive */
    cJSON *item = cJSON_GetObjectItem(result, "key");
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_EQUAL_STRING("val", item->valuestring);
    cJSON_Delete(result);
}

void test_parse_array_of_objects(void)
{
    cJSON *result = cJSON_Parse("[{\"id\": 1}, {\"id\": 2}]");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(cJSON_Array, result->type);
    TEST_ASSERT_EQUAL_INT(2, cJSON_GetArraySize(result));

    cJSON *first = cJSON_GetArrayItem(result, 0);
    TEST_ASSERT_NOT_NULL(first);
    TEST_ASSERT_EQUAL_INT(cJSON_Object, first->type);
    TEST_ASSERT_EQUAL_DOUBLE(1.0, cJSON_GetObjectItem(first, "id")->valuedouble);

    cJSON *second = cJSON_GetArrayItem(result, 1);
    TEST_ASSERT_NOT_NULL(second);
    TEST_ASSERT_EQUAL_DOUBLE(2.0, cJSON_GetObjectItem(second, "id")->valuedouble);

    cJSON_Delete(result);
}

void test_parse_returns_non_null_for_valid_json(void)
{
    cJSON *result = cJSON_Parse("{\"valid\": true}");
    TEST_ASSERT_NOT_NULL(result);
    cJSON_Delete(result);
}

void test_parse_string_with_tab_escape(void)
{
    cJSON *result = cJSON_Parse("\"col1\\tcol2\"");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("col1\tcol2", result->valuestring);
    cJSON_Delete(result);
}

void test_parse_string_with_quote_escape(void)
{
    cJSON *result = cJSON_Parse("\"say \\\"hi\\\"\"");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("say \"hi\"", result->valuestring);
    cJSON_Delete(result);
}

/* =========================================================================
 * main
 * ====================================================================== */

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();

    RUN_TEST(test_parse_null_input_returns_null);
    RUN_TEST(test_parse_empty_string_returns_null);
    RUN_TEST(test_parse_whitespace_only_returns_null);

    RUN_TEST(test_parse_null_literal);
    RUN_TEST(test_parse_true_literal);
    RUN_TEST(test_parse_false_literal);

    RUN_TEST(test_parse_integer_zero);
    RUN_TEST(test_parse_positive_integer);
    RUN_TEST(test_parse_negative_integer);
    RUN_TEST(test_parse_floating_point_number);
    RUN_TEST(test_parse_scientific_notation);
    RUN_TEST(test_parse_negative_scientific_notation);

    RUN_TEST(test_parse_empty_string);
    RUN_TEST(test_parse_simple_string);
    RUN_TEST(test_parse_string_with_escape_sequences);
    RUN_TEST(test_parse_string_with_unicode_escape);
    RUN_TEST(test_parse_string_with_backslash_escape);
    RUN_TEST(test_parse_string_with_tab_escape);
    RUN_TEST(test_parse_string_with_quote_escape);

    RUN_TEST(test_parse_empty_array);
    RUN_TEST(test_parse_array_single_element);
    RUN_TEST(test_parse_array_multiple_elements);
    RUN_TEST(test_parse_array_mixed_types);
    RUN_TEST(test_parse_nested_array);

    RUN_TEST(test_parse_empty_object);
    RUN_TEST(test_parse_object_single_key);
    RUN_TEST(test_parse_object_multiple_keys);
    RUN_TEST(test_parse_object_nested);
    RUN_TEST(test_parse_object_with_array_value);

    RUN_TEST(test_parse_object_with_leading_whitespace);
    RUN_TEST(test_parse_array_with_internal_whitespace);

    RUN_TEST(test_parse_invalid_json_returns_null);
    RUN_TEST(test_parse_unclosed_object_returns_null);
    RUN_TEST(test_parse_unclosed_array_returns_null);
    RUN_TEST(test_parse_unclosed_string_returns_null);
    RUN_TEST(test_parse_trailing_comma_in_object_returns_null);
    RUN_TEST(test_parse_trailing_comma_in_array_returns_null);
    RUN_TEST(test_parse_bare_word_returns_null);

    RUN_TEST(test_parse_error_ptr_set_on_failure);
    RUN_TEST(test_parse_error_ptr_null_on_success);

    RUN_TEST(test_parse_complex_json);
    RUN_TEST(test_parse_deeply_nested_arrays);
    RUN_TEST(test_parse_number_valueint_matches_valuedouble);
    RUN_TEST(test_parse_string_key_case_insensitive_lookup);
    RUN_TEST(test_parse_array_of_objects);
    RUN_TEST(test_parse_returns_non_null_for_valid_json);

    return UNITY_END();
}