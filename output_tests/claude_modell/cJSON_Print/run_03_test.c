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

/* -------------------------------------------------------------------------
 * Test cases
 * ---------------------------------------------------------------------- */

/* cJSON_Print(NULL) should return NULL */
void test_cJSON_Print_null_item_returns_null(void)
{
    char *result = cJSON_Print(NULL);
    TEST_ASSERT_NULL(result);
}

/* cJSON_Print on a null JSON value */
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

/* cJSON_Print on a true JSON value */
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

/* cJSON_Print on a false JSON value */
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

/* cJSON_Print on an integer number */
void test_cJSON_Print_integer_number(void)
{
    cJSON *item = cJSON_CreateNumber(42);
    TEST_ASSERT_NOT_NULL(item);

    char *result = cJSON_Print(item);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(contains_substring(result, "42"));

    free(result);
    cJSON_Delete(item);
}

/* cJSON_Print on a negative number */
void test_cJSON_Print_negative_number(void)
{
    cJSON *item = cJSON_CreateNumber(-7);
    TEST_ASSERT_NOT_NULL(item);

    char *result = cJSON_Print(item);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(contains_substring(result, "-7"));

    free(result);
    cJSON_Delete(item);
}

/* cJSON_Print on zero */
void test_cJSON_Print_zero(void)
{
    cJSON *item = cJSON_CreateNumber(0);
    TEST_ASSERT_NOT_NULL(item);

    char *result = cJSON_Print(item);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(contains_substring(result, "0"));

    free(result);
    cJSON_Delete(item);
}

/* cJSON_Print on a floating point number */
void test_cJSON_Print_float_number(void)
{
    cJSON *item = cJSON_CreateNumber(3.14);
    TEST_ASSERT_NOT_NULL(item);

    char *result = cJSON_Print(item);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(contains_substring(result, "3.14"));

    free(result);
    cJSON_Delete(item);
}

/* cJSON_Print on a simple string */
void test_cJSON_Print_simple_string(void)
{
    cJSON *item = cJSON_CreateString("hello");
    TEST_ASSERT_NOT_NULL(item);

    char *result = cJSON_Print(item);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("\"hello\"", result);

    free(result);
    cJSON_Delete(item);
}

/* cJSON_Print on an empty string */
void test_cJSON_Print_empty_string(void)
{
    cJSON *item = cJSON_CreateString("");
    TEST_ASSERT_NOT_NULL(item);

    char *result = cJSON_Print(item);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("\"\"", result);

    free(result);
    cJSON_Delete(item);
}

/* cJSON_Print on a string with special characters */
void test_cJSON_Print_string_with_special_chars(void)
{
    cJSON *item = cJSON_CreateString("hello\nworld");
    TEST_ASSERT_NOT_NULL(item);

    char *result = cJSON_Print(item);
    TEST_ASSERT_NOT_NULL(result);
    /* The newline should be escaped */
    TEST_ASSERT_TRUE(contains_substring(result, "\\n"));

    free(result);
    cJSON_Delete(item);
}

/* cJSON_Print on a string with quotes */
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

/* cJSON_Print on an empty array */
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

/* cJSON_Print on an array with elements */
void test_cJSON_Print_array_with_elements(void)
{
    cJSON *item = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(item);
    cJSON_AddItemToArray(item, cJSON_CreateNumber(1));
    cJSON_AddItemToArray(item, cJSON_CreateNumber(2));
    cJSON_AddItemToArray(item, cJSON_CreateNumber(3));

    char *result = cJSON_Print(item);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(contains_substring(result, "1"));
    TEST_ASSERT_TRUE(contains_substring(result, "2"));
    TEST_ASSERT_TRUE(contains_substring(result, "3"));

    free(result);
    cJSON_Delete(item);
}

/* cJSON_Print on an empty object */
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

/* cJSON_Print on an object with a key-value pair */
void test_cJSON_Print_object_with_key_value(void)
{
    cJSON *item = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(item);
    cJSON_AddStringToObject(item, "key", "value");

    char *result = cJSON_Print(item);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(contains_substring(result, "key"));
    TEST_ASSERT_TRUE(contains_substring(result, "value"));

    free(result);
    cJSON_Delete(item);
}

/* cJSON_Print on a nested object */
void test_cJSON_Print_nested_object(void)
{
    cJSON *root = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(root);

    cJSON *nested = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(nested);
    cJSON_AddNumberToObject(nested, "x", 10);
    cJSON_AddItemToObject(root, "nested", nested);

    char *result = cJSON_Print(root);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(contains_substring(result, "nested"));
    TEST_ASSERT_TRUE(contains_substring(result, "x"));
    TEST_ASSERT_TRUE(contains_substring(result, "10"));

    free(result);
    cJSON_Delete(root);
}

/* cJSON_Print on a nested array */
void test_cJSON_Print_nested_array(void)
{
    cJSON *root = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(root);

    cJSON *inner = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(inner);
    cJSON_AddItemToArray(inner, cJSON_CreateNumber(99));
    cJSON_AddItemToArray(root, inner);

    char *result = cJSON_Print(root);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(contains_substring(result, "99"));

    free(result);
    cJSON_Delete(root);
}

/* cJSON_Print produces formatted output (contains newlines or spaces) */
void test_cJSON_Print_produces_formatted_output(void)
{
    cJSON *item = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(item);
    cJSON_AddNumberToObject(item, "a", 1);

    char *result = cJSON_Print(item);
    TEST_ASSERT_NOT_NULL(result);

    /* Formatted output should contain whitespace (tab or newline) */
    int has_whitespace = (strchr(result, '\n') != NULL) || (strchr(result, '\t') != NULL) || (strchr(result, ' ') != NULL);
    TEST_ASSERT_TRUE(has_whitespace);

    free(result);
    cJSON_Delete(item);
}

/* cJSON_Print result differs from cJSON_PrintUnformatted (formatted has more chars) */
void test_cJSON_Print_vs_unformatted(void)
{
    cJSON *item = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(item);
    cJSON_AddNumberToObject(item, "key", 123);

    char *formatted = cJSON_Print(item);
    char *unformatted = cJSON_PrintUnformatted(item);

    TEST_ASSERT_NOT_NULL(formatted);
    TEST_ASSERT_NOT_NULL(unformatted);

    /* Formatted output should be longer than unformatted */
    TEST_ASSERT_GREATER_THAN(strlen(unformatted), strlen(formatted));

    free(formatted);
    free(unformatted);
    cJSON_Delete(item);
}

/* cJSON_Print on a raw JSON item */
void test_cJSON_Print_raw_item(void)
{
    cJSON *item = cJSON_CreateRaw("{\"raw\":true}");
    TEST_ASSERT_NOT_NULL(item);

    char *result = cJSON_Print(item);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(contains_substring(result, "raw"));

    free(result);
    cJSON_Delete(item);
}

/* cJSON_Print on a bool (true) created via cJSON_CreateBool */
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

/* cJSON_Print on a bool (false) created via cJSON_CreateBool */
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

/* cJSON_Print on a complex mixed object */
void test_cJSON_Print_complex_object(void)
{
    cJSON *root = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(root);

    cJSON_AddStringToObject(root, "name", "test");
    cJSON_AddNumberToObject(root, "value", 42);
    cJSON_AddBoolToObject(root, "active", 1);
    cJSON_AddNullToObject(root, "nothing");

    cJSON *arr = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(arr);
    cJSON_AddItemToArray(arr, cJSON_CreateNumber(1));
    cJSON_AddItemToArray(arr, cJSON_CreateNumber(2));
    cJSON_AddItemToObject(root, "list", arr);

    char *result = cJSON_Print(root);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(contains_substring(result, "name"));
    TEST_ASSERT_TRUE(contains_substring(result, "test"));
    TEST_ASSERT_TRUE(contains_substring(result, "42"));
    TEST_ASSERT_TRUE(contains_substring(result, "true"));
    TEST_ASSERT_TRUE(contains_substring(result, "null"));
    TEST_ASSERT_TRUE(contains_substring(result, "list"));

    free(result);
    cJSON_Delete(root);
}

/* cJSON_Print result can be re-parsed to produce an equivalent tree */
void test_cJSON_Print_output_is_valid_json(void)
{
    cJSON *original = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(original);
    cJSON_AddStringToObject(original, "hello", "world");
    cJSON_AddNumberToObject(original, "num", 7);

    char *printed = cJSON_Print(original);
    TEST_ASSERT_NOT_NULL(printed);

    cJSON *reparsed = cJSON_Parse(printed);
    TEST_ASSERT_NOT_NULL_MESSAGE(reparsed, "Re-parsed JSON should not be NULL");

    /* Compare the two trees */
    TEST_ASSERT_TRUE(cJSON_Compare(original, reparsed, 1));

    free(printed);
    cJSON_Delete(original);
    cJSON_Delete(reparsed);
}

/* cJSON_Print on a large array */
void test_cJSON_Print_large_array(void)
{
    cJSON *arr = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(arr);

    int i;
    for (i = 0; i < 100; i++)
    {
        cJSON_AddItemToArray(arr, cJSON_CreateNumber(i));
    }

    char *result = cJSON_Print(arr);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(contains_substring(result, "99"));

    free(result);
    cJSON_Delete(arr);
}

/* cJSON_Print on a string with unicode escape */
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

/* cJSON_Print on an object with multiple keys */
void test_cJSON_Print_object_multiple_keys(void)
{
    cJSON *obj = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(obj);
    cJSON_AddNumberToObject(obj, "a", 1);
    cJSON_AddNumberToObject(obj, "b", 2);
    cJSON_AddNumberToObject(obj, "c", 3);

    char *result = cJSON_Print(obj);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(contains_substring(result, "\"a\""));
    TEST_ASSERT_TRUE(contains_substring(result, "\"b\""));
    TEST_ASSERT_TRUE(contains_substring(result, "\"c\""));

    free(result);
    cJSON_Delete(obj);
}

/* -------------------------------------------------------------------------
 * main
 * ---------------------------------------------------------------------- */
int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_cJSON_Print_null_item_returns_null);
    RUN_TEST(test_cJSON_Print_null_value);
    RUN_TEST(test_cJSON_Print_true_value);
    RUN_TEST(test_cJSON_Print_false_value);
    RUN_TEST(test_cJSON_Print_integer_number);
    RUN_TEST(test_cJSON_Print_negative_number);
    RUN_TEST(test_cJSON_Print_zero);
    RUN_TEST(test_cJSON_Print_float_number);
    RUN_TEST(test_cJSON_Print_simple_string);
    RUN_TEST(test_cJSON_Print_empty_string);
    RUN_TEST(test_cJSON_Print_string_with_special_chars);
    RUN_TEST(test_cJSON_Print_string_with_quotes);
    RUN_TEST(test_cJSON_Print_empty_array);
    RUN_TEST(test_cJSON_Print_array_with_elements);
    RUN_TEST(test_cJSON_Print_empty_object);
    RUN_TEST(test_cJSON_Print_object_with_key_value);
    RUN_TEST(test_cJSON_Print_nested_object);
    RUN_TEST(test_cJSON_Print_nested_array);
    RUN_TEST(test_cJSON_Print_produces_formatted_output);
    RUN_TEST(test_cJSON_Print_vs_unformatted);
    RUN_TEST(test_cJSON_Print_raw_item);
    RUN_TEST(test_cJSON_Print_bool_true);
    RUN_TEST(test_cJSON_Print_bool_false);
    RUN_TEST(test_cJSON_Print_complex_object);
    RUN_TEST(test_cJSON_Print_output_is_valid_json);
    RUN_TEST(test_cJSON_Print_large_array);
    RUN_TEST(test_cJSON_Print_string_with_backslash);
    RUN_TEST(test_cJSON_Print_object_multiple_keys);
    return UNITY_END();
}