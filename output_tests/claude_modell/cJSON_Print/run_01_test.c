#include "cJSON.c"
#include "unity.h"
#include "cJSON.h"
#include <stdlib.h>
#include <string.h>

/* setUp and tearDown */
void setUp(void)
{
    /* Reset hooks to defaults before each test */
    cJSON_InitHooks(NULL);
}

void tearDown(void)
{
    /* Nothing to tear down */
}

/* Helper: check that a printed string contains a substring */
static int contains_substring(const char *haystack, const char *needle)
{
    if (!haystack || !needle) return 0;
    return strstr(haystack, needle) != NULL;
}

/* Test: cJSON_Print returns NULL when passed NULL */
void test_cJSON_Print_null_item_returns_null(void)
{
    char *result = cJSON_Print(NULL);
    TEST_ASSERT_NULL(result);
}

/* Test: cJSON_Print on a cJSON_Null item produces "null" */
void test_cJSON_Print_null_value(void)
{
    cJSON *item = cJSON_CreateNull();
    TEST_ASSERT_NOT_NULL(item);

    char *result = cJSON_Print(item);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("null", result);

    free(result);
    cJSON_Delete(item);
}

/* Test: cJSON_Print on a cJSON_True item produces "true" */
void test_cJSON_Print_true_value(void)
{
    cJSON *item = cJSON_CreateTrue();
    TEST_ASSERT_NOT_NULL(item);

    char *result = cJSON_Print(item);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("true", result);

    free(result);
    cJSON_Delete(item);
}

/* Test: cJSON_Print on a cJSON_False item produces "false" */
void test_cJSON_Print_false_value(void)
{
    cJSON *item = cJSON_CreateFalse();
    TEST_ASSERT_NOT_NULL(item);

    char *result = cJSON_Print(item);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("false", result);

    free(result);
    cJSON_Delete(item);
}

/* Test: cJSON_Print on a number item */
void test_cJSON_Print_number_integer(void)
{
    cJSON *item = cJSON_CreateNumber(42.0);
    TEST_ASSERT_NOT_NULL(item);

    char *result = cJSON_Print(item);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(contains_substring(result, "42"));

    free(result);
    cJSON_Delete(item);
}

/* Test: cJSON_Print on a floating point number */
void test_cJSON_Print_number_float(void)
{
    cJSON *item = cJSON_CreateNumber(3.14);
    TEST_ASSERT_NOT_NULL(item);

    char *result = cJSON_Print(item);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(result);
    /* Should contain "3.14" or similar representation */
    TEST_ASSERT_TRUE(contains_substring(result, "3.14"));

    free(result);
    cJSON_Delete(item);
}

/* Test: cJSON_Print on a zero number */
void test_cJSON_Print_number_zero(void)
{
    cJSON *item = cJSON_CreateNumber(0.0);
    TEST_ASSERT_NOT_NULL(item);

    char *result = cJSON_Print(item);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(contains_substring(result, "0"));

    free(result);
    cJSON_Delete(item);
}

/* Test: cJSON_Print on a negative number */
void test_cJSON_Print_number_negative(void)
{
    cJSON *item = cJSON_CreateNumber(-99.0);
    TEST_ASSERT_NOT_NULL(item);

    char *result = cJSON_Print(item);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(contains_substring(result, "-99"));

    free(result);
    cJSON_Delete(item);
}

/* Test: cJSON_Print on a string item */
void test_cJSON_Print_string_simple(void)
{
    cJSON *item = cJSON_CreateString("hello");
    TEST_ASSERT_NOT_NULL(item);

    char *result = cJSON_Print(item);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("\"hello\"", result);

    free(result);
    cJSON_Delete(item);
}

/* Test: cJSON_Print on an empty string */
void test_cJSON_Print_string_empty(void)
{
    cJSON *item = cJSON_CreateString("");
    TEST_ASSERT_NOT_NULL(item);

    char *result = cJSON_Print(item);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("\"\"", result);

    free(result);
    cJSON_Delete(item);
}

/* Test: cJSON_Print on a string with special characters */
void test_cJSON_Print_string_with_special_chars(void)
{
    cJSON *item = cJSON_CreateString("hello\nworld");
    TEST_ASSERT_NOT_NULL(item);

    char *result = cJSON_Print(item);
    TEST_ASSERT_NOT_NULL(result);
    /* Should escape the newline */
    TEST_ASSERT_TRUE(contains_substring(result, "\\n"));

    free(result);
    cJSON_Delete(item);
}

/* Test: cJSON_Print on a string with quotes */
void test_cJSON_Print_string_with_quotes(void)
{
    cJSON *item = cJSON_CreateString("say \"hi\"");
    TEST_ASSERT_NOT_NULL(item);

    char *result = cJSON_Print(item);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(contains_substring(result, "\\\""));

    free(result);
    cJSON_Delete(item);
}

/* Test: cJSON_Print on an empty array */
void test_cJSON_Print_empty_array(void)
{
    cJSON *item = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(item);

    char *result = cJSON_Print(item);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(contains_substring(result, "["));
    TEST_ASSERT_TRUE(contains_substring(result, "]"));

    free(result);
    cJSON_Delete(item);
}

/* Test: cJSON_Print on an array with elements */
void test_cJSON_Print_array_with_elements(void)
{
    cJSON *item = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(item);

    cJSON_AddItemToArray(item, cJSON_CreateNumber(1.0));
    cJSON_AddItemToArray(item, cJSON_CreateNumber(2.0));
    cJSON_AddItemToArray(item, cJSON_CreateNumber(3.0));

    char *result = cJSON_Print(item);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(contains_substring(result, "1"));
    TEST_ASSERT_TRUE(contains_substring(result, "2"));
    TEST_ASSERT_TRUE(contains_substring(result, "3"));

    free(result);
    cJSON_Delete(item);
}

/* Test: cJSON_Print on an empty object */
void test_cJSON_Print_empty_object(void)
{
    cJSON *item = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(item);

    char *result = cJSON_Print(item);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(contains_substring(result, "{"));
    TEST_ASSERT_TRUE(contains_substring(result, "}"));

    free(result);
    cJSON_Delete(item);
}

/* Test: cJSON_Print on an object with a key-value pair */
void test_cJSON_Print_object_with_string_value(void)
{
    cJSON *item = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(item);

    cJSON_AddStringToObject(item, "key", "value");

    char *result = cJSON_Print(item);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(contains_substring(result, "\"key\""));
    TEST_ASSERT_TRUE(contains_substring(result, "\"value\""));

    free(result);
    cJSON_Delete(item);
}

/* Test: cJSON_Print on an object with a number value */
void test_cJSON_Print_object_with_number_value(void)
{
    cJSON *item = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(item);

    cJSON_AddNumberToObject(item, "count", 100.0);

    char *result = cJSON_Print(item);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(contains_substring(result, "\"count\""));
    TEST_ASSERT_TRUE(contains_substring(result, "100"));

    free(result);
    cJSON_Delete(item);
}

/* Test: cJSON_Print on an object with a boolean value */
void test_cJSON_Print_object_with_bool_value(void)
{
    cJSON *item = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(item);

    cJSON_AddTrueToObject(item, "flag");

    char *result = cJSON_Print(item);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(contains_substring(result, "\"flag\""));
    TEST_ASSERT_TRUE(contains_substring(result, "true"));

    free(result);
    cJSON_Delete(item);
}

/* Test: cJSON_Print on an object with a null value */
void test_cJSON_Print_object_with_null_value(void)
{
    cJSON *item = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(item);

    cJSON_AddNullToObject(item, "nothing");

    char *result = cJSON_Print(item);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(contains_substring(result, "\"nothing\""));
    TEST_ASSERT_TRUE(contains_substring(result, "null"));

    free(result);
    cJSON_Delete(item);
}

/* Test: cJSON_Print produces formatted output (contains newlines/tabs for formatting) */
void test_cJSON_Print_produces_formatted_output(void)
{
    cJSON *item = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(item);

    cJSON_AddStringToObject(item, "name", "test");

    char *result = cJSON_Print(item);
    TEST_ASSERT_NOT_NULL(result);

    /* Formatted output should contain whitespace/newlines */
    int has_formatting = (strchr(result, '\n') != NULL) || (strchr(result, '\t') != NULL);
    TEST_ASSERT_TRUE(has_formatting);

    free(result);
    cJSON_Delete(item);
}

/* Test: cJSON_Print vs cJSON_PrintUnformatted differ in whitespace */
void test_cJSON_Print_differs_from_unformatted(void)
{
    cJSON *item = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(item);

    cJSON_AddStringToObject(item, "key", "value");

    char *formatted = cJSON_Print(item);
    char *unformatted = cJSON_PrintUnformatted(item);

    TEST_ASSERT_NOT_NULL(formatted);
    TEST_ASSERT_NOT_NULL(unformatted);

    /* Formatted should be longer than unformatted due to whitespace */
    TEST_ASSERT_GREATER_THAN(strlen(unformatted), strlen(formatted));

    free(formatted);
    free(unformatted);
    cJSON_Delete(item);
}

/* Test: cJSON_Print on a nested object */
void test_cJSON_Print_nested_object(void)
{
    cJSON *root = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(root);

    cJSON *nested = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(nested);

    cJSON_AddStringToObject(nested, "inner_key", "inner_value");
    cJSON_AddItemToObject(root, "outer_key", nested);

    char *result = cJSON_Print(root);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(contains_substring(result, "outer_key"));
    TEST_ASSERT_TRUE(contains_substring(result, "inner_key"));
    TEST_ASSERT_TRUE(contains_substring(result, "inner_value"));

    free(result);
    cJSON_Delete(root);
}

/* Test: cJSON_Print on a nested array */
void test_cJSON_Print_nested_array(void)
{
    cJSON *root = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(root);

    cJSON *inner = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(inner);

    cJSON_AddItemToArray(inner, cJSON_CreateNumber(1.0));
    cJSON_AddItemToArray(inner, cJSON_CreateNumber(2.0));
    cJSON_AddItemToArray(root, inner);
    cJSON_AddItemToArray(root, cJSON_CreateString("hello"));

    char *result = cJSON_Print(root);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(contains_substring(result, "1"));
    TEST_ASSERT_TRUE(contains_substring(result, "2"));
    TEST_ASSERT_TRUE(contains_substring(result, "hello"));

    free(result);
    cJSON_Delete(root);
}

/* Test: cJSON_Print result can be re-parsed */
void test_cJSON_Print_output_is_valid_json(void)
{
    cJSON *original = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(original);

    cJSON_AddStringToObject(original, "name", "Alice");
    cJSON_AddNumberToObject(original, "age", 30.0);
    cJSON_AddTrueToObject(original, "active");

    char *printed = cJSON_Print(original);
    TEST_ASSERT_NOT_NULL(printed);

    cJSON *reparsed = cJSON_Parse(printed);
    TEST_ASSERT_NOT_NULL_MESSAGE(reparsed, "Re-parsed JSON should not be NULL");

    /* Verify the reparsed object has the same values */
    cJSON *name_item = cJSON_GetObjectItem(reparsed, "name");
    TEST_ASSERT_NOT_NULL(name_item);
    TEST_ASSERT_EQUAL_STRING("Alice", name_item->valuestring);

    cJSON *age_item = cJSON_GetObjectItem(reparsed, "age");
    TEST_ASSERT_NOT_NULL(age_item);
    TEST_ASSERT_EQUAL_DOUBLE(30.0, age_item->valuedouble);

    free(printed);
    cJSON_Delete(original);
    cJSON_Delete(reparsed);
}

/* Test: cJSON_Print on a raw JSON item */
void test_cJSON_Print_raw_json(void)
{
    cJSON *item = cJSON_CreateRaw("{\"raw\":true}");
    TEST_ASSERT_NOT_NULL(item);

    char *result = cJSON_Print(item);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(contains_substring(result, "{\"raw\":true}"));

    free(result);
    cJSON_Delete(item);
}

/* Test: cJSON_Print on a complex mixed object */
void test_cJSON_Print_complex_mixed_object(void)
{
    cJSON *root = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(root);

    cJSON_AddStringToObject(root, "name", "test");
    cJSON_AddNumberToObject(root, "value", 123.0);
    cJSON_AddFalseToObject(root, "enabled");
    cJSON_AddNullToObject(root, "data");

    cJSON *arr = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(arr);
    cJSON_AddItemToArray(arr, cJSON_CreateNumber(1.0));
    cJSON_AddItemToArray(arr, cJSON_CreateNumber(2.0));
    cJSON_AddItemToObject(root, "items", arr);

    char *result = cJSON_Print(root);
    TEST_ASSERT_NOT_NULL(result);

    TEST_ASSERT_TRUE(contains_substring(result, "\"name\""));
    TEST_ASSERT_TRUE(contains_substring(result, "\"test\""));
    TEST_ASSERT_TRUE(contains_substring(result, "123"));
    TEST_ASSERT_TRUE(contains_substring(result, "false"));
    TEST_ASSERT_TRUE(contains_substring(result, "null"));
    TEST_ASSERT_TRUE(contains_substring(result, "\"items\""));

    free(result);
    cJSON_Delete(root);
}

/* Test: cJSON_Print on a string with backslash */
void test_cJSON_Print_string_with_backslash(void)
{
    cJSON *item = cJSON_CreateString("path\\to\\file");
    TEST_ASSERT_NOT_NULL(item);

    char *result = cJSON_Print(item);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(contains_substring(result, "\\\\"));

    free(result);
    cJSON_Delete(item);
}

/* Test: cJSON_Print on a large array */
void test_cJSON_Print_large_array(void)
{
    cJSON *arr = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(arr);

    int i;
    for (i = 0; i < 100; i++)
    {
        cJSON_AddItemToArray(arr, cJSON_CreateNumber((double)i));
    }

    char *result = cJSON_Print(arr);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(contains_substring(result, "99"));

    free(result);
    cJSON_Delete(arr);
}

/* Test: cJSON_Print on a bool created with cJSON_CreateBool true */
void test_cJSON_Print_bool_true(void)
{
    cJSON *item = cJSON_CreateBool(1);
    TEST_ASSERT_NOT_NULL(item);

    char *result = cJSON_Print(item);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("true", result);

    free(result);
    cJSON_Delete(item);
}

/* Test: cJSON_Print on a bool created with cJSON_CreateBool false */
void test_cJSON_Print_bool_false(void)
{
    cJSON *item = cJSON_CreateBool(0);
    TEST_ASSERT_NOT_NULL(item);

    char *result = cJSON_Print(item);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("false", result);

    free(result);
    cJSON_Delete(item);
}

/* Test: cJSON_Print result is a newly allocated string (not the same pointer as internal) */
void test_cJSON_Print_returns_new_allocation(void)
{
    cJSON *item = cJSON_CreateString("test");
    TEST_ASSERT_NOT_NULL(item);

    char *result1 = cJSON_Print(item);
    char *result2 = cJSON_Print(item);

    TEST_ASSERT_NOT_NULL(result1);
    TEST_ASSERT_NOT_NULL(result2);

    /* Two calls should return different pointers */
    TEST_ASSERT_NOT_EQUAL(result1, result2);

    /* But same content */
    TEST_ASSERT_EQUAL_STRING(result1, result2);

    free(result1);
    free(result2);
    cJSON_Delete(item);
}

/* Test: cJSON_Print on an int array */
void test_cJSON_Print_int_array(void)
{
    int numbers[] = {10, 20, 30};
    cJSON *item = cJSON_CreateIntArray(numbers, 3);
    TEST_ASSERT_NOT_NULL(item);

    char *result = cJSON_Print(item);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(contains_substring(result, "10"));
    TEST_ASSERT_TRUE(contains_substring(result, "20"));
    TEST_ASSERT_TRUE(contains_substring(result, "30"));

    free(result);
    cJSON_Delete(item);
}

/* Test: cJSON_Print on a string array */
void test_cJSON_Print_string_array(void)
{
    const char *strings[] = {"alpha", "beta", "gamma"};
    cJSON *item = cJSON_CreateStringArray(strings, 3);
    TEST_ASSERT_NOT_NULL(item);

    char *result = cJSON_Print(item);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(contains_substring(result, "alpha"));
    TEST_ASSERT_TRUE(contains_substring(result, "beta"));
    TEST_ASSERT_TRUE(contains_substring(result, "gamma"));

    free(result);
    cJSON_Delete(item);
}

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_cJSON_Print_null_item_returns_null);
    RUN_TEST(test_cJSON_Print_null_value);
    RUN_TEST(test_cJSON_Print_true_value);
    RUN_TEST(test_cJSON_Print_false_value);
    RUN_TEST(test_cJSON_Print_number_integer);
    RUN_TEST(test_cJSON_Print_number_float);
    RUN_TEST(test_cJSON_Print_number_zero);
    RUN_TEST(test_cJSON_Print_number_negative);
    RUN_TEST(test_cJSON_Print_string_simple);
    RUN_TEST(test_cJSON_Print_string_empty);
    RUN_TEST(test_cJSON_Print_string_with_special_chars);
    RUN_TEST(test_cJSON_Print_string_with_quotes);
    RUN_TEST(test_cJSON_Print_empty_array);
    RUN_TEST(test_cJSON_Print_array_with_elements);
    RUN_TEST(test_cJSON_Print_empty_object);
    RUN_TEST(test_cJSON_Print_object_with_string_value);
    RUN_TEST(test_cJSON_Print_object_with_number_value);
    RUN_TEST(test_cJSON_Print_object_with_bool_value);
    RUN_TEST(test_cJSON_Print_object_with_null_value);
    RUN_TEST(test_cJSON_Print_produces_formatted_output);
    RUN_TEST(test_cJSON_Print_differs_from_unformatted);
    RUN_TEST(test_cJSON_Print_nested_object);
    RUN_TEST(test_cJSON_Print_nested_array);
    RUN_TEST(test_cJSON_Print_output_is_valid_json);
    RUN_TEST(test_cJSON_Print_raw_json);
    RUN_TEST(test_cJSON_Print_complex_mixed_object);
    RUN_TEST(test_cJSON_Print_string_with_backslash);
    RUN_TEST(test_cJSON_Print_large_array);
    RUN_TEST(test_cJSON_Print_bool_true);
    RUN_TEST(test_cJSON_Print_bool_false);
    RUN_TEST(test_cJSON_Print_returns_new_allocation);
    RUN_TEST(test_cJSON_Print_int_array);
    RUN_TEST(test_cJSON_Print_string_array);
    return UNITY_END();
}