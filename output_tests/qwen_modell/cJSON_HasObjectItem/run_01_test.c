#include "cJSON.c"
#include "unity.h"
#include "cJSON.h"

/* Helper function to create a cJSON object with a single key-value pair */
static cJSON* create_test_object(const char *key, cJSON *value)
{
    cJSON *obj = cJSON_CreateObject();
    if (obj && key && value) {
        cJSON_AddItemToObject(obj, key, value);
    }
    return obj;
}

/* Helper function to create a cJSON object with multiple key-value pairs */
static cJSON* create_multi_key_object(void)
{
    cJSON *obj = cJSON_CreateObject();
    cJSON *num = cJSON_CreateNumber(42);
    cJSON *str = cJSON_CreateString("hello");
    cJSON *arr = cJSON_CreateArray();
    cJSON *null = cJSON_CreateNull();

    if (obj && num && str && arr && null) {
        cJSON_AddItemToObject(obj, "number", num);
        cJSON_AddItemToObject(obj, "string", str);
        cJSON_AddItemToObject(obj, "array", arr);
        cJSON_AddItemToObject(obj, "null", null);
    } else {
        cJSON_Delete(obj);
        cJSON_Delete(num);
        cJSON_Delete(str);
        cJSON_Delete(arr);
        cJSON_Delete(null);
        obj = NULL;
    }
    return obj;
}

void test_cJSON_HasObjectItem_WithExistingKey_ShouldReturnTrue(void)
{
    cJSON *obj = create_multi_key_object();
    TEST_ASSERT_NOT_NULL(obj);

    TEST_ASSERT_TRUE(cJSON_HasObjectItem(obj, "number"));
    TEST_ASSERT_TRUE(cJSON_HasObjectItem(obj, "string"));
    TEST_ASSERT_TRUE(cJSON_HasObjectItem(obj, "array"));
    TEST_ASSERT_TRUE(cJSON_HasObjectItem(obj, "null"));

    cJSON_Delete(obj);
}

void test_cJSON_HasObjectItem_WithNonExistingKey_ShouldReturnFalse(void)
{
    cJSON *obj = create_multi_key_object();
    TEST_ASSERT_NOT_NULL(obj);

    TEST_ASSERT_FALSE(cJSON_HasObjectItem(obj, "nonexistent"));
    TEST_ASSERT_FALSE(cJSON_HasObjectItem(obj, "NUMBER")); /* case insensitive */
    TEST_ASSERT_FALSE(cJSON_HasObjectItem(obj, "String")); /* case insensitive */

    cJSON_Delete(obj);
}

void test_cJSON_HasObjectItem_WithNullObject_ShouldReturnFalse(void)
{
    TEST_ASSERT_FALSE(cJSON_HasObjectItem(NULL, "anykey"));
}

void test_cJSON_HasObjectItem_WithNullKey_ShouldReturnFalse(void)
{
    cJSON *obj = create_multi_key_object();
    TEST_ASSERT_NOT_NULL(obj);

    TEST_ASSERT_FALSE(cJSON_HasObjectItem(obj, NULL));

    cJSON_Delete(obj);
}

void test_cJSON_HasObjectItem_WithEmptyObject_ShouldReturnFalse(void)
{
    cJSON *obj = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(obj);

    TEST_ASSERT_FALSE(cJSON_HasObjectItem(obj, "anykey"));

    cJSON_Delete(obj);
}

void test_cJSON_HasObjectItem_WithSingleKeyObject_ShouldReturnTrueForExistingKey(void)
{
    cJSON *obj = create_test_object("testkey", cJSON_CreateNumber(123));
    TEST_ASSERT_NOT_NULL(obj);

    TEST_ASSERT_TRUE(cJSON_HasObjectItem(obj, "testkey"));
    TEST_ASSERT_FALSE(cJSON_HasObjectItem(obj, "otherkey"));

    cJSON_Delete(obj);
}

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_cJSON_HasObjectItem_WithExistingKey_ShouldReturnTrue);
    RUN_TEST(test_cJSON_HasObjectItem_WithNonExistingKey_ShouldReturnFalse);
    RUN_TEST(test_cJSON_HasObjectItem_WithNullObject_ShouldReturnFalse);
    RUN_TEST(test_cJSON_HasObjectItem_WithNullKey_ShouldReturnFalse);
    RUN_TEST(test_cJSON_HasObjectItem_WithEmptyObject_ShouldReturnFalse);
    RUN_TEST(test_cJSON_HasObjectItem_WithSingleKeyObject_ShouldReturnTrueForExistingKey);
    return UNITY_END();
}