#include "cJSON.c"
#include "unity.h"
#include "cJSON.h"
#include <stdlib.h>
#include <string.h>

/* File-scope fixtures */
static cJSON *test_object = NULL;

/* setUp and tearDown */
void setUp(void)
{
    test_object = cJSON_CreateObject();
}

void tearDown(void)
{
    cJSON_Delete(test_object);
    test_object = NULL;
}

/* Helper: build a simple object with known keys */
static cJSON *build_simple_object(void)
{
    cJSON *obj = cJSON_CreateObject();
    cJSON_AddStringToObject(obj, "name", "Alice");
    cJSON_AddNumberToObject(obj, "age", 30);
    cJSON_AddTrueToObject(obj, "active");
    cJSON_AddNullToObject(obj, "nickname");
    return obj;
}

/* ----------------------------- Test Cases ----------------------------- */

void test_GetObjectItem_returns_null_for_null_object(void)
{
    cJSON *result = cJSON_GetObjectItem(NULL, "key");
    TEST_ASSERT_NULL(result);
}

void test_GetObjectItem_returns_null_for_null_string(void)
{
    cJSON *obj = build_simple_object();
    cJSON *result = cJSON_GetObjectItem(obj, NULL);
    TEST_ASSERT_NULL(result);
    cJSON_Delete(obj);
}

void test_GetObjectItem_returns_null_for_empty_object(void)
{
    cJSON *result = cJSON_GetObjectItem(test_object, "missing");
    TEST_ASSERT_NULL(result);
}

void test_GetObjectItem_finds_existing_string_item(void)
{
    cJSON *obj = build_simple_object();
    cJSON *result = cJSON_GetObjectItem(obj, "name");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("Alice", result->valuestring);
    cJSON_Delete(obj);
}

void test_GetObjectItem_finds_existing_number_item(void)
{
    cJSON *obj = build_simple_object();
    cJSON *result = cJSON_GetObjectItem(obj, "age");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_DOUBLE(30.0, result->valuedouble);
    cJSON_Delete(obj);
}

void test_GetObjectItem_finds_existing_bool_item(void)
{
    cJSON *obj = build_simple_object();
    cJSON *result = cJSON_GetObjectItem(obj, "active");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(cJSON_IsTrue(result));
    cJSON_Delete(obj);
}

void test_GetObjectItem_finds_existing_null_item(void)
{
    cJSON *obj = build_simple_object();
    cJSON *result = cJSON_GetObjectItem(obj, "nickname");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(cJSON_IsNull(result));
    cJSON_Delete(obj);
}

void test_GetObjectItem_returns_null_for_missing_key(void)
{
    cJSON *obj = build_simple_object();
    cJSON *result = cJSON_GetObjectItem(obj, "nonexistent");
    TEST_ASSERT_NULL(result);
    cJSON_Delete(obj);
}

void test_GetObjectItem_is_case_insensitive(void)
{
    cJSON *obj = build_simple_object();
    /* cJSON_GetObjectItem is case insensitive */
    cJSON *result_upper = cJSON_GetObjectItem(obj, "NAME");
    cJSON *result_mixed = cJSON_GetObjectItem(obj, "NaMe");
    cJSON *result_lower = cJSON_GetObjectItem(obj, "name");
    TEST_ASSERT_NOT_NULL(result_upper);
    TEST_ASSERT_NOT_NULL(result_mixed);
    TEST_ASSERT_NOT_NULL(result_lower);
    TEST_ASSERT_EQUAL_STRING("Alice", result_upper->valuestring);
    TEST_ASSERT_EQUAL_STRING("Alice", result_mixed->valuestring);
    TEST_ASSERT_EQUAL_STRING("Alice", result_lower->valuestring);
    cJSON_Delete(obj);
}

void test_GetObjectItem_returns_first_match_when_duplicate_keys(void)
{
    cJSON *obj = cJSON_CreateObject();
    cJSON_AddStringToObject(obj, "key", "first");
    cJSON_AddStringToObject(obj, "key", "second");
    cJSON *result = cJSON_GetObjectItem(obj, "key");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("first", result->valuestring);
    cJSON_Delete(obj);
}

void test_GetObjectItem_with_single_item_object(void)
{
    cJSON *obj = cJSON_CreateObject();
    cJSON_AddNumberToObject(obj, "only", 42.0);
    cJSON *result = cJSON_GetObjectItem(obj, "only");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_DOUBLE(42.0, result->valuedouble);
    cJSON_Delete(obj);
}

void test_GetObjectItem_with_empty_string_key(void)
{
    cJSON *obj = cJSON_CreateObject();
    cJSON_AddStringToObject(obj, "", "empty_key_value");
    cJSON *result = cJSON_GetObjectItem(obj, "");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("empty_key_value", result->valuestring);
    cJSON_Delete(obj);
}

void test_GetObjectItem_does_not_find_empty_string_key_when_not_present(void)
{
    cJSON *obj = build_simple_object();
    cJSON *result = cJSON_GetObjectItem(obj, "");
    TEST_ASSERT_NULL(result);
    cJSON_Delete(obj);
}

void test_GetObjectItem_with_nested_object(void)
{
    cJSON *outer = cJSON_CreateObject();
    cJSON *inner = cJSON_CreateObject();
    cJSON_AddStringToObject(inner, "inner_key", "inner_value");
    cJSON_AddItemToObject(outer, "nested", inner);

    cJSON *result = cJSON_GetObjectItem(outer, "nested");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(cJSON_IsObject(result));

    cJSON *inner_result = cJSON_GetObjectItem(result, "inner_key");
    TEST_ASSERT_NOT_NULL(inner_result);
    TEST_ASSERT_EQUAL_STRING("inner_value", inner_result->valuestring);

    cJSON_Delete(outer);
}

void test_GetObjectItem_with_array_value(void)
{
    cJSON *obj = cJSON_CreateObject();
    cJSON *arr = cJSON_CreateArray();
    cJSON_AddItemToArray(arr, cJSON_CreateNumber(1));
    cJSON_AddItemToArray(arr, cJSON_CreateNumber(2));
    cJSON_AddItemToObject(obj, "myarray", arr);

    cJSON *result = cJSON_GetObjectItem(obj, "myarray");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(cJSON_IsArray(result));
    TEST_ASSERT_EQUAL_INT(2, cJSON_GetArraySize(result));

    cJSON_Delete(obj);
}

void test_GetObjectItem_returns_correct_item_among_many(void)
{
    cJSON *obj = cJSON_CreateObject();
    int i;
    char key[32];
    for (i = 0; i < 20; i++)
    {
        snprintf(key, sizeof(key), "key%d", i);
        cJSON_AddNumberToObject(obj, key, (double)i);
    }

    cJSON *result = cJSON_GetObjectItem(obj, "key10");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_DOUBLE(10.0, result->valuedouble);

    result = cJSON_GetObjectItem(obj, "key0");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_DOUBLE(0.0, result->valuedouble);

    result = cJSON_GetObjectItem(obj, "key19");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_DOUBLE(19.0, result->valuedouble);

    cJSON_Delete(obj);
}

void test_GetObjectItem_parsed_from_json_string(void)
{
    const char *json = "{\"foo\": 123, \"bar\": \"hello\", \"baz\": true}";
    cJSON *obj = cJSON_Parse(json);
    TEST_ASSERT_NOT_NULL(obj);

    cJSON *foo = cJSON_GetObjectItem(obj, "foo");
    TEST_ASSERT_NOT_NULL(foo);
    TEST_ASSERT_EQUAL_DOUBLE(123.0, foo->valuedouble);

    cJSON *bar = cJSON_GetObjectItem(obj, "bar");
    TEST_ASSERT_NOT_NULL(bar);
    TEST_ASSERT_EQUAL_STRING("hello", bar->valuestring);

    cJSON *baz = cJSON_GetObjectItem(obj, "baz");
    TEST_ASSERT_NOT_NULL(baz);
    TEST_ASSERT_TRUE(cJSON_IsTrue(baz));

    cJSON *missing = cJSON_GetObjectItem(obj, "qux");
    TEST_ASSERT_NULL(missing);

    cJSON_Delete(obj);
}

void test_GetObjectItem_case_insensitive_parsed_json(void)
{
    const char *json = "{\"Hello\": 1}";
    cJSON *obj = cJSON_Parse(json);
    TEST_ASSERT_NOT_NULL(obj);

    cJSON *result_lower = cJSON_GetObjectItem(obj, "hello");
    cJSON *result_upper = cJSON_GetObjectItem(obj, "HELLO");
    cJSON *result_exact = cJSON_GetObjectItem(obj, "Hello");

    TEST_ASSERT_NOT_NULL(result_lower);
    TEST_ASSERT_NOT_NULL(result_upper);
    TEST_ASSERT_NOT_NULL(result_exact);

    cJSON_Delete(obj);
}

void test_GetObjectItem_does_not_search_array_children(void)
{
    /* An array is not an object; searching it by key should return NULL */
    cJSON *arr = cJSON_CreateArray();
    cJSON_AddItemToArray(arr, cJSON_CreateString("value"));
    cJSON *result = cJSON_GetObjectItem(arr, "0");
    TEST_ASSERT_NULL(result);
    cJSON_Delete(arr);
}

void test_GetObjectItem_with_special_characters_in_key(void)
{
    cJSON *obj = cJSON_CreateObject();
    cJSON_AddStringToObject(obj, "key with spaces", "spaced");
    cJSON_AddStringToObject(obj, "key/with/slashes", "slashed");
    cJSON_AddStringToObject(obj, "key.with.dots", "dotted");

    cJSON *r1 = cJSON_GetObjectItem(obj, "key with spaces");
    cJSON *r2 = cJSON_GetObjectItem(obj, "key/with/slashes");
    cJSON *r3 = cJSON_GetObjectItem(obj, "key.with.dots");

    TEST_ASSERT_NOT_NULL(r1);
    TEST_ASSERT_EQUAL_STRING("spaced", r1->valuestring);
    TEST_ASSERT_NOT_NULL(r2);
    TEST_ASSERT_EQUAL_STRING("slashed", r2->valuestring);
    TEST_ASSERT_NOT_NULL(r3);
    TEST_ASSERT_EQUAL_STRING("dotted", r3->valuestring);

    cJSON_Delete(obj);
}

/* ----------------------------- main ----------------------------- */

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_GetObjectItem_returns_null_for_null_object);
    RUN_TEST(test_GetObjectItem_returns_null_for_null_string);
    RUN_TEST(test_GetObjectItem_returns_null_for_empty_object);
    RUN_TEST(test_GetObjectItem_finds_existing_string_item);
    RUN_TEST(test_GetObjectItem_finds_existing_number_item);
    RUN_TEST(test_GetObjectItem_finds_existing_bool_item);
    RUN_TEST(test_GetObjectItem_finds_existing_null_item);
    RUN_TEST(test_GetObjectItem_returns_null_for_missing_key);
    RUN_TEST(test_GetObjectItem_is_case_insensitive);
    RUN_TEST(test_GetObjectItem_returns_first_match_when_duplicate_keys);
    RUN_TEST(test_GetObjectItem_with_single_item_object);
    RUN_TEST(test_GetObjectItem_with_empty_string_key);
    RUN_TEST(test_GetObjectItem_does_not_find_empty_string_key_when_not_present);
    RUN_TEST(test_GetObjectItem_with_nested_object);
    RUN_TEST(test_GetObjectItem_with_array_value);
    RUN_TEST(test_GetObjectItem_returns_correct_item_among_many);
    RUN_TEST(test_GetObjectItem_parsed_from_json_string);
    RUN_TEST(test_GetObjectItem_case_insensitive_parsed_json);
    RUN_TEST(test_GetObjectItem_does_not_search_array_children);
    RUN_TEST(test_GetObjectItem_with_special_characters_in_key);
    return UNITY_END();
}