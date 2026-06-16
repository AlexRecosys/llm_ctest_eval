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

/* Test cases */

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
    cJSON *result = cJSON_GetObjectItem(test_object, "key");
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

void test_GetObjectItem_returns_null_for_nonexistent_key(void)
{
    cJSON *obj = build_simple_object();
    cJSON *result = cJSON_GetObjectItem(obj, "nonexistent");
    TEST_ASSERT_NULL(result);
    cJSON_Delete(obj);
}

void test_GetObjectItem_is_case_insensitive_uppercase(void)
{
    cJSON *obj = build_simple_object();
    /* cJSON_GetObjectItem is case insensitive */
    cJSON *result = cJSON_GetObjectItem(obj, "NAME");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("Alice", result->valuestring);
    cJSON_Delete(obj);
}

void test_GetObjectItem_is_case_insensitive_mixed_case(void)
{
    cJSON *obj = build_simple_object();
    cJSON *result = cJSON_GetObjectItem(obj, "NaMe");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("Alice", result->valuestring);
    cJSON_Delete(obj);
}

void test_GetObjectItem_is_case_insensitive_lowercase_key(void)
{
    cJSON *obj = cJSON_CreateObject();
    cJSON_AddStringToObject(obj, "MyKey", "value");
    cJSON *result = cJSON_GetObjectItem(obj, "mykey");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("value", result->valuestring);
    cJSON_Delete(obj);
}

void test_GetObjectItem_returns_first_match_with_duplicate_keys(void)
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
    cJSON_AddNumberToObject(obj, "only", 42);
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

void test_GetObjectItem_empty_string_key_not_found(void)
{
    cJSON *obj = build_simple_object();
    cJSON *result = cJSON_GetObjectItem(obj, "");
    TEST_ASSERT_NULL(result);
    cJSON_Delete(obj);
}

void test_GetObjectItem_does_not_modify_object(void)
{
    cJSON *obj = build_simple_object();
    int size_before = cJSON_GetArraySize(obj);
    cJSON_GetObjectItem(obj, "name");
    int size_after = cJSON_GetArraySize(obj);
    TEST_ASSERT_EQUAL_INT(size_before, size_after);
    cJSON_Delete(obj);
}

void test_GetObjectItem_returns_correct_type_string(void)
{
    cJSON *obj = build_simple_object();
    cJSON *result = cJSON_GetObjectItem(obj, "name");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(cJSON_IsString(result));
    cJSON_Delete(obj);
}

void test_GetObjectItem_returns_correct_type_number(void)
{
    cJSON *obj = build_simple_object();
    cJSON *result = cJSON_GetObjectItem(obj, "age");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(cJSON_IsNumber(result));
    cJSON_Delete(obj);
}

void test_GetObjectItem_nested_object(void)
{
    cJSON *outer = cJSON_CreateObject();
    cJSON *inner = cJSON_CreateObject();
    cJSON_AddStringToObject(inner, "city", "Wonderland");
    cJSON_AddItemToObject(outer, "address", inner);

    cJSON *result = cJSON_GetObjectItem(outer, "address");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(cJSON_IsObject(result));

    cJSON *city = cJSON_GetObjectItem(result, "city");
    TEST_ASSERT_NOT_NULL(city);
    TEST_ASSERT_EQUAL_STRING("Wonderland", city->valuestring);

    cJSON_Delete(outer);
}

void test_GetObjectItem_with_array_value(void)
{
    cJSON *obj = cJSON_CreateObject();
    cJSON *arr = cJSON_CreateArray();
    cJSON_AddItemToArray(arr, cJSON_CreateNumber(1));
    cJSON_AddItemToArray(arr, cJSON_CreateNumber(2));
    cJSON_AddItemToObject(obj, "numbers", arr);

    cJSON *result = cJSON_GetObjectItem(obj, "numbers");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(cJSON_IsArray(result));
    TEST_ASSERT_EQUAL_INT(2, cJSON_GetArraySize(result));

    cJSON_Delete(obj);
}

void test_GetObjectItem_with_special_characters_in_key(void)
{
    cJSON *obj = cJSON_CreateObject();
    cJSON_AddStringToObject(obj, "key-with-dashes", "val1");
    cJSON_AddStringToObject(obj, "key_with_underscores", "val2");
    cJSON_AddStringToObject(obj, "key.with.dots", "val3");

    cJSON *r1 = cJSON_GetObjectItem(obj, "key-with-dashes");
    cJSON *r2 = cJSON_GetObjectItem(obj, "key_with_underscores");
    cJSON *r3 = cJSON_GetObjectItem(obj, "key.with.dots");

    TEST_ASSERT_NOT_NULL(r1);
    TEST_ASSERT_EQUAL_STRING("val1", r1->valuestring);
    TEST_ASSERT_NOT_NULL(r2);
    TEST_ASSERT_EQUAL_STRING("val2", r2->valuestring);
    TEST_ASSERT_NOT_NULL(r3);
    TEST_ASSERT_EQUAL_STRING("val3", r3->valuestring);

    cJSON_Delete(obj);
}

void test_GetObjectItem_parsed_from_json_string(void)
{
    const char *json = "{\"foo\":\"bar\",\"baz\":123}";
    cJSON *obj = cJSON_Parse(json);
    TEST_ASSERT_NOT_NULL(obj);

    cJSON *foo = cJSON_GetObjectItem(obj, "foo");
    TEST_ASSERT_NOT_NULL(foo);
    TEST_ASSERT_EQUAL_STRING("bar", foo->valuestring);

    cJSON *baz = cJSON_GetObjectItem(obj, "baz");
    TEST_ASSERT_NOT_NULL(baz);
    TEST_ASSERT_EQUAL_DOUBLE(123.0, baz->valuedouble);

    cJSON_Delete(obj);
}

void test_GetObjectItem_parsed_case_insensitive_lookup(void)
{
    const char *json = "{\"Hello\":\"world\"}";
    cJSON *obj = cJSON_Parse(json);
    TEST_ASSERT_NOT_NULL(obj);

    cJSON *result = cJSON_GetObjectItem(obj, "hello");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("world", result->valuestring);

    cJSON *result2 = cJSON_GetObjectItem(obj, "HELLO");
    TEST_ASSERT_NOT_NULL(result2);
    TEST_ASSERT_EQUAL_STRING("world", result2->valuestring);

    cJSON_Delete(obj);
}

void test_GetObjectItem_not_found_in_parsed_object(void)
{
    const char *json = "{\"a\":1,\"b\":2}";
    cJSON *obj = cJSON_Parse(json);
    TEST_ASSERT_NOT_NULL(obj);

    cJSON *result = cJSON_GetObjectItem(obj, "c");
    TEST_ASSERT_NULL(result);

    cJSON_Delete(obj);
}

void test_GetObjectItem_false_value(void)
{
    cJSON *obj = cJSON_CreateObject();
    cJSON_AddFalseToObject(obj, "flag");
    cJSON *result = cJSON_GetObjectItem(obj, "flag");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(cJSON_IsFalse(result));
    cJSON_Delete(obj);
}

void test_GetObjectItem_raw_value(void)
{
    cJSON *obj = cJSON_CreateObject();
    cJSON *raw = cJSON_CreateRaw("[1,2,3]");
    cJSON_AddItemToObject(obj, "rawdata", raw);
    cJSON *result = cJSON_GetObjectItem(obj, "rawdata");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(cJSON_IsRaw(result));
    cJSON_Delete(obj);
}

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
    RUN_TEST(test_GetObjectItem_returns_null_for_nonexistent_key);
    RUN_TEST(test_GetObjectItem_is_case_insensitive_uppercase);
    RUN_TEST(test_GetObjectItem_is_case_insensitive_mixed_case);
    RUN_TEST(test_GetObjectItem_is_case_insensitive_lowercase_key);
    RUN_TEST(test_GetObjectItem_returns_first_match_with_duplicate_keys);
    RUN_TEST(test_GetObjectItem_with_single_item_object);
    RUN_TEST(test_GetObjectItem_with_empty_string_key);
    RUN_TEST(test_GetObjectItem_empty_string_key_not_found);
    RUN_TEST(test_GetObjectItem_does_not_modify_object);
    RUN_TEST(test_GetObjectItem_returns_correct_type_string);
    RUN_TEST(test_GetObjectItem_returns_correct_type_number);
    RUN_TEST(test_GetObjectItem_nested_object);
    RUN_TEST(test_GetObjectItem_with_array_value);
    RUN_TEST(test_GetObjectItem_with_special_characters_in_key);
    RUN_TEST(test_GetObjectItem_parsed_from_json_string);
    RUN_TEST(test_GetObjectItem_parsed_case_insensitive_lookup);
    RUN_TEST(test_GetObjectItem_not_found_in_parsed_object);
    RUN_TEST(test_GetObjectItem_false_value);
    RUN_TEST(test_GetObjectItem_raw_value);
    return UNITY_END();
}