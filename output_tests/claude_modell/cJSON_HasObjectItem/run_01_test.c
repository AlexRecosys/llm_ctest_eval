#include "cJSON.c"
#include "unity.h"
#include "cJSON.h"
#include <stdlib.h>
#include <string.h>

static cJSON *test_object = NULL;

void setUp(void)
{
    test_object = cJSON_CreateObject();
}

void tearDown(void)
{
    if (test_object != NULL)
    {
        cJSON_Delete(test_object);
        test_object = NULL;
    }
}

/* Helper: create a simple object with known keys */
static cJSON *create_populated_object(void)
{
    cJSON *obj = cJSON_CreateObject();
    cJSON_AddStringToObject(obj, "name", "Alice");
    cJSON_AddNumberToObject(obj, "age", 30);
    cJSON_AddTrueToObject(obj, "active");
    cJSON_AddNullToObject(obj, "nickname");
    return obj;
}

/* Test: item exists in object — should return 1 */
void test_HasObjectItem_existing_string_key_returns_true(void)
{
    cJSON_AddStringToObject(test_object, "key", "value");
    cJSON_bool result = cJSON_HasObjectItem(test_object, "key");
    TEST_ASSERT_EQUAL_INT(1, result);
}

/* Test: item does not exist — should return 0 */
void test_HasObjectItem_nonexistent_key_returns_false(void)
{
    cJSON_AddStringToObject(test_object, "key", "value");
    cJSON_bool result = cJSON_HasObjectItem(test_object, "missing");
    TEST_ASSERT_EQUAL_INT(0, result);
}

/* Test: NULL object — should return 0 */
void test_HasObjectItem_null_object_returns_false(void)
{
    cJSON_bool result = cJSON_HasObjectItem(NULL, "key");
    TEST_ASSERT_EQUAL_INT(0, result);
}

/* Test: NULL string — should return 0 */
void test_HasObjectItem_null_string_returns_false(void)
{
    cJSON_AddStringToObject(test_object, "key", "value");
    cJSON_bool result = cJSON_HasObjectItem(test_object, NULL);
    TEST_ASSERT_EQUAL_INT(0, result);
}

/* Test: both NULL — should return 0 */
void test_HasObjectItem_null_object_and_null_string_returns_false(void)
{
    cJSON_bool result = cJSON_HasObjectItem(NULL, NULL);
    TEST_ASSERT_EQUAL_INT(0, result);
}

/* Test: empty object — should return 0 */
void test_HasObjectItem_empty_object_returns_false(void)
{
    cJSON_bool result = cJSON_HasObjectItem(test_object, "key");
    TEST_ASSERT_EQUAL_INT(0, result);
}

/* Test: case insensitive match — cJSON_GetObjectItem is case insensitive */
void test_HasObjectItem_case_insensitive_match_returns_true(void)
{
    cJSON_AddStringToObject(test_object, "Name", "Alice");
    cJSON_bool result = cJSON_HasObjectItem(test_object, "name");
    TEST_ASSERT_EQUAL_INT(1, result);
}

/* Test: case insensitive match uppercase query */
void test_HasObjectItem_case_insensitive_uppercase_query_returns_true(void)
{
    cJSON_AddStringToObject(test_object, "name", "Alice");
    cJSON_bool result = cJSON_HasObjectItem(test_object, "NAME");
    TEST_ASSERT_EQUAL_INT(1, result);
}

/* Test: number item exists */
void test_HasObjectItem_number_item_exists_returns_true(void)
{
    cJSON_AddNumberToObject(test_object, "count", 42);
    cJSON_bool result = cJSON_HasObjectItem(test_object, "count");
    TEST_ASSERT_EQUAL_INT(1, result);
}

/* Test: boolean true item exists */
void test_HasObjectItem_bool_true_item_exists_returns_true(void)
{
    cJSON_AddTrueToObject(test_object, "flag");
    cJSON_bool result = cJSON_HasObjectItem(test_object, "flag");
    TEST_ASSERT_EQUAL_INT(1, result);
}

/* Test: boolean false item exists */
void test_HasObjectItem_bool_false_item_exists_returns_true(void)
{
    cJSON_AddFalseToObject(test_object, "disabled");
    cJSON_bool result = cJSON_HasObjectItem(test_object, "disabled");
    TEST_ASSERT_EQUAL_INT(1, result);
}

/* Test: null item exists */
void test_HasObjectItem_null_item_exists_returns_true(void)
{
    cJSON_AddNullToObject(test_object, "nothing");
    cJSON_bool result = cJSON_HasObjectItem(test_object, "nothing");
    TEST_ASSERT_EQUAL_INT(1, result);
}

/* Test: nested object item exists */
void test_HasObjectItem_nested_object_key_exists_returns_true(void)
{
    cJSON *nested = cJSON_CreateObject();
    cJSON_AddStringToObject(nested, "inner", "value");
    cJSON_AddItemToObject(test_object, "nested", nested);
    cJSON_bool result = cJSON_HasObjectItem(test_object, "nested");
    TEST_ASSERT_EQUAL_INT(1, result);
}

/* Test: array item exists */
void test_HasObjectItem_array_item_exists_returns_true(void)
{
    cJSON *arr = cJSON_CreateArray();
    cJSON_AddItemToObject(test_object, "list", arr);
    cJSON_bool result = cJSON_HasObjectItem(test_object, "list");
    TEST_ASSERT_EQUAL_INT(1, result);
}

/* Test: multiple keys, check first key */
void test_HasObjectItem_multiple_keys_first_key_found(void)
{
    cJSON *obj = create_populated_object();
    cJSON_bool result = cJSON_HasObjectItem(obj, "name");
    TEST_ASSERT_EQUAL_INT(1, result);
    cJSON_Delete(obj);
}

/* Test: multiple keys, check last key */
void test_HasObjectItem_multiple_keys_last_key_found(void)
{
    cJSON *obj = create_populated_object();
    cJSON_bool result = cJSON_HasObjectItem(obj, "nickname");
    TEST_ASSERT_EQUAL_INT(1, result);
    cJSON_Delete(obj);
}

/* Test: multiple keys, check middle key */
void test_HasObjectItem_multiple_keys_middle_key_found(void)
{
    cJSON *obj = create_populated_object();
    cJSON_bool result = cJSON_HasObjectItem(obj, "age");
    TEST_ASSERT_EQUAL_INT(1, result);
    cJSON_Delete(obj);
}

/* Test: multiple keys, check nonexistent key */
void test_HasObjectItem_multiple_keys_nonexistent_key_returns_false(void)
{
    cJSON *obj = create_populated_object();
    cJSON_bool result = cJSON_HasObjectItem(obj, "email");
    TEST_ASSERT_EQUAL_INT(0, result);
    cJSON_Delete(obj);
}

/* Test: empty string key added and searched */
void test_HasObjectItem_empty_string_key_exists_returns_true(void)
{
    cJSON_AddStringToObject(test_object, "", "empty_key_value");
    cJSON_bool result = cJSON_HasObjectItem(test_object, "");
    TEST_ASSERT_EQUAL_INT(1, result);
}

/* Test: empty string key not present */
void test_HasObjectItem_empty_string_key_not_present_returns_false(void)
{
    cJSON_AddStringToObject(test_object, "key", "value");
    cJSON_bool result = cJSON_HasObjectItem(test_object, "");
    TEST_ASSERT_EQUAL_INT(0, result);
}

/* Test: return value is exactly 1 (not just truthy) when found */
void test_HasObjectItem_return_value_is_exactly_one_when_found(void)
{
    cJSON_AddStringToObject(test_object, "present", "yes");
    cJSON_bool result = cJSON_HasObjectItem(test_object, "present");
    TEST_ASSERT_EQUAL_INT(1, result);
}

/* Test: return value is exactly 0 (not just falsy) when not found */
void test_HasObjectItem_return_value_is_exactly_zero_when_not_found(void)
{
    cJSON_bool result = cJSON_HasObjectItem(test_object, "absent");
    TEST_ASSERT_EQUAL_INT(0, result);
}

/* Test: object parsed from JSON string */
void test_HasObjectItem_parsed_json_object_key_found(void)
{
    cJSON *parsed = cJSON_Parse("{\"foo\": 1, \"bar\": 2}");
    TEST_ASSERT_NOT_NULL(parsed);
    TEST_ASSERT_EQUAL_INT(1, cJSON_HasObjectItem(parsed, "foo"));
    TEST_ASSERT_EQUAL_INT(1, cJSON_HasObjectItem(parsed, "bar"));
    cJSON_Delete(parsed);
}

/* Test: object parsed from JSON string, key not found */
void test_HasObjectItem_parsed_json_object_key_not_found(void)
{
    cJSON *parsed = cJSON_Parse("{\"foo\": 1, \"bar\": 2}");
    TEST_ASSERT_NOT_NULL(parsed);
    TEST_ASSERT_EQUAL_INT(0, cJSON_HasObjectItem(parsed, "baz"));
    cJSON_Delete(parsed);
}

/* Test: raw item exists */
void test_HasObjectItem_raw_item_exists_returns_true(void)
{
    cJSON *raw = cJSON_CreateRaw("{\"raw\":true}");
    cJSON_AddItemToObject(test_object, "rawkey", raw);
    cJSON_bool result = cJSON_HasObjectItem(test_object, "rawkey");
    TEST_ASSERT_EQUAL_INT(1, result);
}

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_HasObjectItem_existing_string_key_returns_true);
    RUN_TEST(test_HasObjectItem_nonexistent_key_returns_false);
    RUN_TEST(test_HasObjectItem_null_object_returns_false);
    RUN_TEST(test_HasObjectItem_null_string_returns_false);
    RUN_TEST(test_HasObjectItem_null_object_and_null_string_returns_false);
    RUN_TEST(test_HasObjectItem_empty_object_returns_false);
    RUN_TEST(test_HasObjectItem_case_insensitive_match_returns_true);
    RUN_TEST(test_HasObjectItem_case_insensitive_uppercase_query_returns_true);
    RUN_TEST(test_HasObjectItem_number_item_exists_returns_true);
    RUN_TEST(test_HasObjectItem_bool_true_item_exists_returns_true);
    RUN_TEST(test_HasObjectItem_bool_false_item_exists_returns_true);
    RUN_TEST(test_HasObjectItem_null_item_exists_returns_true);
    RUN_TEST(test_HasObjectItem_nested_object_key_exists_returns_true);
    RUN_TEST(test_HasObjectItem_array_item_exists_returns_true);
    RUN_TEST(test_HasObjectItem_multiple_keys_first_key_found);
    RUN_TEST(test_HasObjectItem_multiple_keys_last_key_found);
    RUN_TEST(test_HasObjectItem_multiple_keys_middle_key_found);
    RUN_TEST(test_HasObjectItem_multiple_keys_nonexistent_key_returns_false);
    RUN_TEST(test_HasObjectItem_empty_string_key_exists_returns_true);
    RUN_TEST(test_HasObjectItem_empty_string_key_not_present_returns_false);
    RUN_TEST(test_HasObjectItem_return_value_is_exactly_one_when_found);
    RUN_TEST(test_HasObjectItem_return_value_is_exactly_zero_when_not_found);
    RUN_TEST(test_HasObjectItem_parsed_json_object_key_found);
    RUN_TEST(test_HasObjectItem_parsed_json_object_key_not_found);
    RUN_TEST(test_HasObjectItem_raw_item_exists_returns_true);
    return UNITY_END();
}