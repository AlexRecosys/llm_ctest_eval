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

/* Test: key exists in object — should return 1 */
void test_HasObjectItem_existing_string_key_returns_true(void)
{
    cJSON *obj = create_populated_object();
    cJSON_bool result = cJSON_HasObjectItem(obj, "name");
    TEST_ASSERT_EQUAL_INT(1, result);
    cJSON_Delete(obj);
}

/* Test: numeric key exists — should return 1 */
void test_HasObjectItem_existing_number_key_returns_true(void)
{
    cJSON *obj = create_populated_object();
    cJSON_bool result = cJSON_HasObjectItem(obj, "age");
    TEST_ASSERT_EQUAL_INT(1, result);
    cJSON_Delete(obj);
}

/* Test: boolean key exists — should return 1 */
void test_HasObjectItem_existing_bool_key_returns_true(void)
{
    cJSON *obj = create_populated_object();
    cJSON_bool result = cJSON_HasObjectItem(obj, "active");
    TEST_ASSERT_EQUAL_INT(1, result);
    cJSON_Delete(obj);
}

/* Test: null-value key exists — should return 1 */
void test_HasObjectItem_existing_null_value_key_returns_true(void)
{
    cJSON *obj = create_populated_object();
    cJSON_bool result = cJSON_HasObjectItem(obj, "nickname");
    TEST_ASSERT_EQUAL_INT(1, result);
    cJSON_Delete(obj);
}

/* Test: key does not exist — should return 0 */
void test_HasObjectItem_nonexistent_key_returns_false(void)
{
    cJSON *obj = create_populated_object();
    cJSON_bool result = cJSON_HasObjectItem(obj, "missing_key");
    TEST_ASSERT_EQUAL_INT(0, result);
    cJSON_Delete(obj);
}

/* Test: empty object — should return 0 */
void test_HasObjectItem_empty_object_returns_false(void)
{
    cJSON_bool result = cJSON_HasObjectItem(test_object, "anything");
    TEST_ASSERT_EQUAL_INT(0, result);
}

/* Test: NULL object — should return 0 */
void test_HasObjectItem_null_object_returns_false(void)
{
    cJSON_bool result = cJSON_HasObjectItem(NULL, "key");
    TEST_ASSERT_EQUAL_INT(0, result);
}

/* Test: NULL string key — should return 0 */
void test_HasObjectItem_null_string_returns_false(void)
{
    cJSON *obj = create_populated_object();
    cJSON_bool result = cJSON_HasObjectItem(obj, NULL);
    TEST_ASSERT_EQUAL_INT(0, result);
    cJSON_Delete(obj);
}

/* Test: case-insensitive match — cJSON_HasObjectItem uses cJSON_GetObjectItem which is case-insensitive */
void test_HasObjectItem_case_insensitive_match_returns_true(void)
{
    cJSON *obj = create_populated_object();
    /* cJSON_GetObjectItem is case insensitive, so "NAME" should match "name" */
    cJSON_bool result = cJSON_HasObjectItem(obj, "NAME");
    TEST_ASSERT_EQUAL_INT(1, result);
    cJSON_Delete(obj);
}

/* Test: case-insensitive partial mismatch — different key entirely */
void test_HasObjectItem_different_key_returns_false(void)
{
    cJSON *obj = create_populated_object();
    cJSON_bool result = cJSON_HasObjectItem(obj, "nam");
    TEST_ASSERT_EQUAL_INT(0, result);
    cJSON_Delete(obj);
}

/* Test: object with single item, check that item */
void test_HasObjectItem_single_item_object_found(void)
{
    cJSON *obj = cJSON_CreateObject();
    cJSON_AddNumberToObject(obj, "only_key", 42);
    cJSON_bool result = cJSON_HasObjectItem(obj, "only_key");
    TEST_ASSERT_EQUAL_INT(1, result);
    cJSON_Delete(obj);
}

/* Test: object with single item, check a different key */
void test_HasObjectItem_single_item_object_not_found(void)
{
    cJSON *obj = cJSON_CreateObject();
    cJSON_AddNumberToObject(obj, "only_key", 42);
    cJSON_bool result = cJSON_HasObjectItem(obj, "other_key");
    TEST_ASSERT_EQUAL_INT(0, result);
    cJSON_Delete(obj);
}

/* Test: return value is exactly 1 (not just truthy) when found */
void test_HasObjectItem_return_value_is_exactly_one_when_found(void)
{
    cJSON *obj = create_populated_object();
    cJSON_bool result = cJSON_HasObjectItem(obj, "name");
    TEST_ASSERT_EQUAL_INT(1, result);
    cJSON_Delete(obj);
}

/* Test: return value is exactly 0 (not just falsy) when not found */
void test_HasObjectItem_return_value_is_exactly_zero_when_not_found(void)
{
    cJSON *obj = create_populated_object();
    cJSON_bool result = cJSON_HasObjectItem(obj, "nonexistent");
    TEST_ASSERT_EQUAL_INT(0, result);
    cJSON_Delete(obj);
}

/* Test: nested object — check key at top level only */
void test_HasObjectItem_nested_object_top_level_key_found(void)
{
    cJSON *obj = cJSON_CreateObject();
    cJSON *nested = cJSON_CreateObject();
    cJSON_AddStringToObject(nested, "inner_key", "inner_value");
    cJSON_AddItemToObject(obj, "nested", nested);
    cJSON_bool result = cJSON_HasObjectItem(obj, "nested");
    TEST_ASSERT_EQUAL_INT(1, result);
    cJSON_Delete(obj);
}

/* Test: nested object — inner key not visible at top level */
void test_HasObjectItem_nested_object_inner_key_not_found_at_top(void)
{
    cJSON *obj = cJSON_CreateObject();
    cJSON *nested = cJSON_CreateObject();
    cJSON_AddStringToObject(nested, "inner_key", "inner_value");
    cJSON_AddItemToObject(obj, "nested", nested);
    cJSON_bool result = cJSON_HasObjectItem(obj, "inner_key");
    TEST_ASSERT_EQUAL_INT(0, result);
    cJSON_Delete(obj);
}

/* Test: object parsed from JSON string */
void test_HasObjectItem_parsed_object_key_found(void)
{
    cJSON *obj = cJSON_Parse("{\"foo\": 1, \"bar\": 2, \"baz\": 3}");
    TEST_ASSERT_NOT_NULL(obj);
    TEST_ASSERT_EQUAL_INT(1, cJSON_HasObjectItem(obj, "foo"));
    TEST_ASSERT_EQUAL_INT(1, cJSON_HasObjectItem(obj, "bar"));
    TEST_ASSERT_EQUAL_INT(1, cJSON_HasObjectItem(obj, "baz"));
    cJSON_Delete(obj);
}

/* Test: object parsed from JSON string — missing key */
void test_HasObjectItem_parsed_object_key_not_found(void)
{
    cJSON *obj = cJSON_Parse("{\"foo\": 1, \"bar\": 2}");
    TEST_ASSERT_NOT_NULL(obj);
    TEST_ASSERT_EQUAL_INT(0, cJSON_HasObjectItem(obj, "qux"));
    cJSON_Delete(obj);
}

/* Test: empty string key */
void test_HasObjectItem_empty_string_key_not_found(void)
{
    cJSON *obj = create_populated_object();
    cJSON_bool result = cJSON_HasObjectItem(obj, "");
    TEST_ASSERT_EQUAL_INT(0, result);
    cJSON_Delete(obj);
}

/* Test: empty string key that actually exists */
void test_HasObjectItem_empty_string_key_found(void)
{
    cJSON *obj = cJSON_CreateObject();
    cJSON_AddNumberToObject(obj, "", 99);
    cJSON_bool result = cJSON_HasObjectItem(obj, "");
    TEST_ASSERT_EQUAL_INT(1, result);
    cJSON_Delete(obj);
}

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_HasObjectItem_existing_string_key_returns_true);
    RUN_TEST(test_HasObjectItem_existing_number_key_returns_true);
    RUN_TEST(test_HasObjectItem_existing_bool_key_returns_true);
    RUN_TEST(test_HasObjectItem_existing_null_value_key_returns_true);
    RUN_TEST(test_HasObjectItem_nonexistent_key_returns_false);
    RUN_TEST(test_HasObjectItem_empty_object_returns_false);
    RUN_TEST(test_HasObjectItem_null_object_returns_false);
    RUN_TEST(test_HasObjectItem_null_string_returns_false);
    RUN_TEST(test_HasObjectItem_case_insensitive_match_returns_true);
    RUN_TEST(test_HasObjectItem_different_key_returns_false);
    RUN_TEST(test_HasObjectItem_single_item_object_found);
    RUN_TEST(test_HasObjectItem_single_item_object_not_found);
    RUN_TEST(test_HasObjectItem_return_value_is_exactly_one_when_found);
    RUN_TEST(test_HasObjectItem_return_value_is_exactly_zero_when_not_found);
    RUN_TEST(test_HasObjectItem_nested_object_top_level_key_found);
    RUN_TEST(test_HasObjectItem_nested_object_inner_key_not_found_at_top);
    RUN_TEST(test_HasObjectItem_parsed_object_key_found);
    RUN_TEST(test_HasObjectItem_parsed_object_key_not_found);
    RUN_TEST(test_HasObjectItem_empty_string_key_not_found);
    RUN_TEST(test_HasObjectItem_empty_string_key_found);
    return UNITY_END();
}