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

/* Helper: parse JSON string and return cJSON object */
static cJSON *parse_json(const char *json_str)
{
    return cJSON_Parse(json_str);
}

/* Helper: free printed string */
static void free_printed(char *str)
{
    if (str != NULL)
    {
        free(str);
    }
}

/* ===== Test Cases ===== */

void test_cJSON_Print_null_item_returns_null(void)
{
    char *result = cJSON_Print(NULL);
    TEST_ASSERT_NULL(result);
}

void test_cJSON_Print_null_json_value(void)
{
    cJSON *item = cJSON_CreateNull();
    TEST_ASSERT_NOT_NULL(item);

    char *result = cJSON_Print(item);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("null", result);

    free_printed(result);
    cJSON_Delete(item);
}

void test_cJSON_Print_true_value(void)
{
    cJSON *item = cJSON_CreateTrue();
    TEST_ASSERT_NOT_NULL(item);

    char *result = cJSON_Print(item);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("true", result);

    free_printed(result);
    cJSON_Delete(item);
}

void test_cJSON_Print_false_value(void)
{
    cJSON *item = cJSON_CreateFalse();
    TEST_ASSERT_NOT_NULL(item);

    char *result = cJSON_Print(item);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("false", result);

    free_printed(result);
    cJSON_Delete(item);
}

void test_cJSON_Print_integer_number(void)
{
    cJSON *item = cJSON_CreateNumber(42.0);
    TEST_ASSERT_NOT_NULL(item);

    char *result = cJSON_Print(item);
    TEST_ASSERT_NOT_NULL(result);
    /* The number 42 should appear in the output */
    TEST_ASSERT_NOT_NULL(strstr(result, "42"));

    free_printed(result);
    cJSON_Delete(item);
}

void test_cJSON_Print_zero_number(void)
{
    cJSON *item = cJSON_CreateNumber(0.0);
    TEST_ASSERT_NOT_NULL(item);

    char *result = cJSON_Print(item);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("0", result);

    free_printed(result);
    cJSON_Delete(item);
}

void test_cJSON_Print_negative_number(void)
{
    cJSON *item = cJSON_CreateNumber(-3.14);
    TEST_ASSERT_NOT_NULL(item);

    char *result = cJSON_Print(item);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(strstr(result, "-3.14"));

    free_printed(result);
    cJSON_Delete(item);
}

void test_cJSON_Print_simple_string(void)
{
    cJSON *item = cJSON_CreateString("hello");
    TEST_ASSERT_NOT_NULL(item);

    char *result = cJSON_Print(item);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("\"hello\"", result);

    free_printed(result);
    cJSON_Delete(item);
}

void test_cJSON_Print_empty_string(void)
{
    cJSON *item = cJSON_CreateString("");
    TEST_ASSERT_NOT_NULL(item);

    char *result = cJSON_Print(item);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("\"\"", result);

    free_printed(result);
    cJSON_Delete(item);
}

void test_cJSON_Print_string_with_special_chars(void)
{
    cJSON *item = cJSON_CreateString("hello\nworld");
    TEST_ASSERT_NOT_NULL(item);

    char *result = cJSON_Print(item);
    TEST_ASSERT_NOT_NULL(result);
    /* Should contain escaped newline */
    TEST_ASSERT_NOT_NULL(strstr(result, "\\n"));

    free_printed(result);
    cJSON_Delete(item);
}

void test_cJSON_Print_string_with_quotes(void)
{
    cJSON *item = cJSON_CreateString("say \"hello\"");
    TEST_ASSERT_NOT_NULL(item);

    char *result = cJSON_Print(item);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(strstr(result, "\\\""));

    free_printed(result);
    cJSON_Delete(item);
}

void test_cJSON_Print_empty_array(void)
{
    cJSON *item = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(item);

    char *result = cJSON_Print(item);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("[]", result);

    free_printed(result);
    cJSON_Delete(item);
}

void test_cJSON_Print_array_with_numbers(void)
{
    cJSON *item = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(item);

    cJSON_AddItemToArray(item, cJSON_CreateNumber(1.0));
    cJSON_AddItemToArray(item, cJSON_CreateNumber(2.0));
    cJSON_AddItemToArray(item, cJSON_CreateNumber(3.0));

    char *result = cJSON_Print(item);
    TEST_ASSERT_NOT_NULL(result);
    /* Formatted output should contain the numbers */
    TEST_ASSERT_NOT_NULL(strstr(result, "1"));
    TEST_ASSERT_NOT_NULL(strstr(result, "2"));
    TEST_ASSERT_NOT_NULL(strstr(result, "3"));

    free_printed(result);
    cJSON_Delete(item);
}

void test_cJSON_Print_array_with_strings(void)
{
    cJSON *item = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(item);

    cJSON_AddItemToArray(item, cJSON_CreateString("foo"));
    cJSON_AddItemToArray(item, cJSON_CreateString("bar"));

    char *result = cJSON_Print(item);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(strstr(result, "\"foo\""));
    TEST_ASSERT_NOT_NULL(strstr(result, "\"bar\""));

    free_printed(result);
    cJSON_Delete(item);
}

void test_cJSON_Print_empty_object(void)
{
    cJSON *item = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(item);

    char *result = cJSON_Print(item);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("{}", result);

    free_printed(result);
    cJSON_Delete(item);
}

void test_cJSON_Print_object_with_string_value(void)
{
    cJSON *item = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(item);

    cJSON_AddStringToObject(item, "key", "value");

    char *result = cJSON_Print(item);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(strstr(result, "\"key\""));
    TEST_ASSERT_NOT_NULL(strstr(result, "\"value\""));

    free_printed(result);
    cJSON_Delete(item);
}

void test_cJSON_Print_object_with_number_value(void)
{
    cJSON *item = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(item);

    cJSON_AddNumberToObject(item, "count", 99.0);

    char *result = cJSON_Print(item);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(strstr(result, "\"count\""));
    TEST_ASSERT_NOT_NULL(strstr(result, "99"));

    free_printed(result);
    cJSON_Delete(item);
}

void test_cJSON_Print_object_with_bool_values(void)
{
    cJSON *item = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(item);

    cJSON_AddTrueToObject(item, "flag_true");
    cJSON_AddFalseToObject(item, "flag_false");

    char *result = cJSON_Print(item);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(strstr(result, "true"));
    TEST_ASSERT_NOT_NULL(strstr(result, "false"));

    free_printed(result);
    cJSON_Delete(item);
}

void test_cJSON_Print_object_with_null_value(void)
{
    cJSON *item = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(item);

    cJSON_AddNullToObject(item, "nothing");

    char *result = cJSON_Print(item);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(strstr(result, "null"));

    free_printed(result);
    cJSON_Delete(item);
}

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
    TEST_ASSERT_NOT_NULL(strstr(result, "outer_key"));
    TEST_ASSERT_NOT_NULL(strstr(result, "inner_key"));
    TEST_ASSERT_NOT_NULL(strstr(result, "inner_value"));

    free_printed(result);
    cJSON_Delete(root);
}

void test_cJSON_Print_nested_array_in_object(void)
{
    cJSON *root = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(root);

    cJSON *arr = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(arr);

    cJSON_AddItemToArray(arr, cJSON_CreateNumber(1.0));
    cJSON_AddItemToArray(arr, cJSON_CreateNumber(2.0));
    cJSON_AddItemToObject(root, "numbers", arr);

    char *result = cJSON_Print(root);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(strstr(result, "numbers"));
    TEST_ASSERT_NOT_NULL(strstr(result, "1"));
    TEST_ASSERT_NOT_NULL(strstr(result, "2"));

    free_printed(result);
    cJSON_Delete(root);
}

void test_cJSON_Print_produces_formatted_output(void)
{
    cJSON *item = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(item);

    cJSON_AddStringToObject(item, "name", "test");

    char *result = cJSON_Print(item);
    TEST_ASSERT_NOT_NULL(result);

    /* Formatted output should contain newline or tab for pretty printing */
    int has_formatting = (strchr(result, '\n') != NULL) || (strchr(result, '\t') != NULL);
    TEST_ASSERT_TRUE(has_formatting);

    free_printed(result);
    cJSON_Delete(item);
}

void test_cJSON_Print_result_is_valid_json(void)
{
    cJSON *original = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(original);

    cJSON_AddStringToObject(original, "key", "value");
    cJSON_AddNumberToObject(original, "num", 42.0);

    char *printed = cJSON_Print(original);
    TEST_ASSERT_NOT_NULL(printed);

    /* Re-parse the printed output to verify it's valid JSON */
    cJSON *reparsed = cJSON_Parse(printed);
    TEST_ASSERT_NOT_NULL(reparsed);

    /* Verify the reparsed object has the same structure */
    cJSON *key_item = cJSON_GetObjectItem(reparsed, "key");
    TEST_ASSERT_NOT_NULL(key_item);
    TEST_ASSERT_EQUAL_STRING("value", key_item->valuestring);

    cJSON *num_item = cJSON_GetObjectItem(reparsed, "num");
    TEST_ASSERT_NOT_NULL(num_item);
    TEST_ASSERT_EQUAL_DOUBLE(42.0, num_item->valuedouble);

    free_printed(printed);
    cJSON_Delete(original);
    cJSON_Delete(reparsed);
}

void test_cJSON_Print_raw_json(void)
{
    cJSON *item = cJSON_CreateRaw("{\"raw\":true}");
    TEST_ASSERT_NOT_NULL(item);

    char *result = cJSON_Print(item);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("{\"raw\":true}", result);

    free_printed(result);
    cJSON_Delete(item);
}

void test_cJSON_Print_complex_nested_structure(void)
{
    cJSON *root = parse_json("{\"name\":\"John\",\"age\":30,\"cars\":[\"Ford\",\"BMW\"],\"address\":{\"city\":\"New York\"}}");
    TEST_ASSERT_NOT_NULL(root);

    char *result = cJSON_Print(root);
    TEST_ASSERT_NOT_NULL(result);

    /* Re-parse to verify validity */
    cJSON *reparsed = cJSON_Parse(result);
    TEST_ASSERT_NOT_NULL(reparsed);

    cJSON *name = cJSON_GetObjectItem(reparsed, "name");
    TEST_ASSERT_NOT_NULL(name);
    TEST_ASSERT_EQUAL_STRING("John", name->valuestring);

    cJSON *age = cJSON_GetObjectItem(reparsed, "age");
    TEST_ASSERT_NOT_NULL(age);
    TEST_ASSERT_EQUAL_DOUBLE(30.0, age->valuedouble);

    free_printed(result);
    cJSON_Delete(root);
    cJSON_Delete(reparsed);
}

void test_cJSON_Print_array_with_mixed_types(void)
{
    cJSON *arr = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(arr);

    cJSON_AddItemToArray(arr, cJSON_CreateNull());
    cJSON_AddItemToArray(arr, cJSON_CreateTrue());
    cJSON_AddItemToArray(arr, cJSON_CreateFalse());
    cJSON_AddItemToArray(arr, cJSON_CreateNumber(3.14));
    cJSON_AddItemToArray(arr, cJSON_CreateString("text"));

    char *result = cJSON_Print(arr);
    TEST_ASSERT_NOT_NULL(result);

    TEST_ASSERT_NOT_NULL(strstr(result, "null"));
    TEST_ASSERT_NOT_NULL(strstr(result, "true"));
    TEST_ASSERT_NOT_NULL(strstr(result, "false"));
    TEST_ASSERT_NOT_NULL(strstr(result, "3.14"));
    TEST_ASSERT_NOT_NULL(strstr(result, "\"text\""));

    free_printed(result);
    cJSON_Delete(arr);
}

void test_cJSON_Print_returns_heap_allocated_string(void)
{
    cJSON *item = cJSON_CreateString("test");
    TEST_ASSERT_NOT_NULL(item);

    char *result = cJSON_Print(item);
    TEST_ASSERT_NOT_NULL(result);

    /* Modify the result to verify it's writable (heap allocated) */
    result[0] = result[0]; /* no-op but confirms writability */

    free_printed(result);
    cJSON_Delete(item);
}

void test_cJSON_Print_multiple_calls_independent(void)
{
    cJSON *item = cJSON_CreateString("hello");
    TEST_ASSERT_NOT_NULL(item);

    char *result1 = cJSON_Print(item);
    char *result2 = cJSON_Print(item);

    TEST_ASSERT_NOT_NULL(result1);
    TEST_ASSERT_NOT_NULL(result2);

    /* Results should be equal in content but different pointers */
    TEST_ASSERT_EQUAL_STRING(result1, result2);
    TEST_ASSERT_NOT_EQUAL(result1, result2);

    free_printed(result1);
    free_printed(result2);
    cJSON_Delete(item);
}

void test_cJSON_Print_unicode_string(void)
{
    cJSON *item = cJSON_CreateString("caf\xc3\xa9");
    TEST_ASSERT_NOT_NULL(item);

    char *result = cJSON_Print(item);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(strstr(result, "caf"));

    free_printed(result);
    cJSON_Delete(item);
}

void test_cJSON_Print_large_number(void)
{
    cJSON *item = cJSON_CreateNumber(1e100);
    TEST_ASSERT_NOT_NULL(item);

    char *result = cJSON_Print(item);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_NOT_NULL(result[0] != '\0');

    free_printed(result);
    cJSON_Delete(item);
}

void test_cJSON_Print_object_multiple_keys(void)
{
    cJSON *obj = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(obj);

    cJSON_AddStringToObject(obj, "a", "alpha");
    cJSON_AddStringToObject(obj, "b", "beta");
    cJSON_AddStringToObject(obj, "c", "gamma");

    char *result = cJSON_Print(obj);
    TEST_ASSERT_NOT_NULL(result);

    TEST_ASSERT_NOT_NULL(strstr(result, "\"a\""));
    TEST_ASSERT_NOT_NULL(strstr(result, "\"alpha\""));
    TEST_ASSERT_NOT_NULL(strstr(result, "\"b\""));
    TEST_ASSERT_NOT_NULL(strstr(result, "\"beta\""));
    TEST_ASSERT_NOT_NULL(strstr(result, "\"c\""));
    TEST_ASSERT_NOT_NULL(strstr(result, "\"gamma\""));

    free_printed(result);
    cJSON_Delete(obj);
}

/* ===== main ===== */
int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_cJSON_Print_null_item_returns_null);
    RUN_TEST(test_cJSON_Print_null_json_value);
    RUN_TEST(test_cJSON_Print_true_value);
    RUN_TEST(test_cJSON_Print_false_value);
    RUN_TEST(test_cJSON_Print_integer_number);
    RUN_TEST(test_cJSON_Print_zero_number);
    RUN_TEST(test_cJSON_Print_negative_number);
    RUN_TEST(test_cJSON_Print_simple_string);
    RUN_TEST(test_cJSON_Print_empty_string);
    RUN_TEST(test_cJSON_Print_string_with_special_chars);
    RUN_TEST(test_cJSON_Print_string_with_quotes);
    RUN_TEST(test_cJSON_Print_empty_array);
    RUN_TEST(test_cJSON_Print_array_with_numbers);
    RUN_TEST(test_cJSON_Print_array_with_strings);
    RUN_TEST(test_cJSON_Print_empty_object);
    RUN_TEST(test_cJSON_Print_object_with_string_value);
    RUN_TEST(test_cJSON_Print_object_with_number_value);
    RUN_TEST(test_cJSON_Print_object_with_bool_values);
    RUN_TEST(test_cJSON_Print_object_with_null_value);
    RUN_TEST(test_cJSON_Print_nested_object);
    RUN_TEST(test_cJSON_Print_nested_array_in_object);
    RUN_TEST(test_cJSON_Print_produces_formatted_output);
    RUN_TEST(test_cJSON_Print_result_is_valid_json);
    RUN_TEST(test_cJSON_Print_raw_json);
    RUN_TEST(test_cJSON_Print_complex_nested_structure);
    RUN_TEST(test_cJSON_Print_array_with_mixed_types);
    RUN_TEST(test_cJSON_Print_returns_heap_allocated_string);
    RUN_TEST(test_cJSON_Print_multiple_calls_independent);
    RUN_TEST(test_cJSON_Print_unicode_string);
    RUN_TEST(test_cJSON_Print_large_number);
    RUN_TEST(test_cJSON_Print_object_multiple_keys);
    return UNITY_END();
}