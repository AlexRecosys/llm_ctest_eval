#include "cJSON.c"
#include "unity.h"

/* Helper to create a cJSON object with a single key-value pair */
static cJSON* create_test_object(const char *key, cJSON *value)
{
    cJSON *obj = cJSON_CreateObject();
    if (obj && key && value) {
        cJSON_AddItemToObject(obj, key, value);
    }
    return obj;
}

/* Helper to create a cJSON object with multiple key-value pairs */
static cJSON* create_multi_key_object(void)
{
    cJSON *obj = cJSON_CreateObject();
    cJSON *num = cJSON_CreateNumber(42);
    cJSON *str = cJSON_CreateString("hello");
    cJSON *arr = cJSON_CreateArray();
    cJSON *bool_val = cJSON_CreateTrue();
    cJSON *null_val = cJSON_CreateNull();

    if (obj && num && str && arr && bool_val && null_val) {
        cJSON_AddItemToObject(obj, "number", num);
        cJSON_AddItemToObject(obj, "string", str);
        cJSON_AddItemToObject(obj, "array", arr);
        cJSON_AddItemToObject(obj, "boolean", bool_val);
        cJSON_AddItemToObject(obj, "null", null_val);
    } else {
        cJSON_Delete(obj);
        cJSON_Delete(num);
        cJSON_Delete(str);
        cJSON_Delete(arr);
        cJSON_Delete(bool_val);
        cJSON_Delete(null_val);
        obj = NULL;
    }
    return obj;
}

/* Test: Get existing object item by key (case-insensitive) */
void test_cJSON_GetObjectItem_ExistingKey(void)
{
    cJSON *obj = create_multi_key_object();
    TEST_ASSERT_NOT_NULL(obj);

    cJSON *item = cJSON_GetObjectItem(obj, "NUMBER");
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, item->type);
    TEST_ASSERT_EQUAL_DOUBLE(42.0, item->valuedouble);

    item = cJSON_GetObjectItem(obj, "String");
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_EQUAL_INT(cJSON_String, item->type);
    TEST_ASSERT_EQUAL_STRING("hello", item->valuestring);

    cJSON_Delete(obj);
}

/* Test: Get non-existing object item returns NULL */
void test_cJSON_GetObjectItem_NonExistingKey(void)
{
    cJSON *obj = create_multi_key_object();
    TEST_ASSERT_NOT_NULL(obj);

    cJSON *item = cJSON_GetObjectItem(obj, "nonexistent");
    TEST_ASSERT_NULL(item);

    cJSON_Delete(obj);
}

/* Test: NULL object returns NULL */
void test_cJSON_GetObjectItem_NullObject(void)
{
    cJSON *item = cJSON_GetObjectItem(NULL, "key");
    TEST_ASSERT_NULL(item);
}

/* Test: NULL key returns NULL */
void test_cJSON_GetObjectItem_NullKey(void)
{
    cJSON *obj = create_multi_key_object();
    TEST_ASSERT_NOT_NULL(obj);

    cJSON *item = cJSON_GetObjectItem(obj, NULL);
    TEST_ASSERT_NULL(item);

    cJSON_Delete(obj);
}

/* Test: Empty object returns NULL for any key */
void test_cJSON_GetObjectItem_EmptyObject(void)
{
    cJSON *obj = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(obj);

    cJSON *item = cJSON_GetObjectItem(obj, "anykey");
    TEST_ASSERT_NULL(item);

    cJSON_Delete(obj);
}

/* Test: Case-insensitive key matching */
void test_cJSON_GetObjectItem_CaseInsensitive(void)
{
    cJSON *obj = create_multi_key_object();
    TEST_ASSERT_NOT_NULL(obj);

    /* Test various case combinations */
    cJSON *item = cJSON_GetObjectItem(obj, "NUMBER");
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, item->type);

    item = cJSON_GetObjectItem(obj, "nUmBeR");
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, item->type);

    item = cJSON_GetObjectItem(obj, "STRING");
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_EQUAL_INT(cJSON_String, item->type);

    item = cJSON_GetObjectItem(obj, "sTrInG");
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_EQUAL_INT(cJSON_String, item->type);

    cJSON_Delete(obj);
}

/* Test: Get nested object item */
void test_cJSON_GetObjectItem_NestedObject(void)
{
    cJSON *inner_obj = cJSON_CreateObject();
    cJSON *inner_num = cJSON_CreateNumber(123);
    cJSON_AddItemToObject(inner_obj, "inner_number", inner_num);

    cJSON *outer_obj = cJSON_CreateObject();
    cJSON_AddItemToObject(outer_obj, "inner", inner_obj);

    cJSON *item = cJSON_GetObjectItem(outer_obj, "inner");
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_EQUAL_INT(cJSON_Object, item->type);

    cJSON *nested_item = cJSON_GetObjectItem(item, "INNER_NUMBER");
    TEST_ASSERT_NOT_NULL(nested_item);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, nested_item->type);
    TEST_ASSERT_EQUAL_DOUBLE(123.0, nested_item->valuedouble);

    cJSON_Delete(outer_obj);
}

/* Test: Get array item */
void test_cJSON_GetObjectItem_Array(void)
{
    cJSON *obj = create_multi_key_object();
    TEST_ASSERT_NOT_NULL(obj);

    cJSON *item = cJSON_GetObjectItem(obj, "array");
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_EQUAL_INT(cJSON_Array, item->type);
    TEST_ASSERT_NULL(item->child); /* empty array */

    cJSON_Delete(obj);
}

/* Test: Get boolean item */
void test_cJSON_GetObjectItem_Boolean(void)
{
    cJSON *obj = create_multi_key_object();
    TEST_ASSERT_NOT_NULL(obj);

    cJSON *item = cJSON_GetObjectItem(obj, "boolean");
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_EQUAL_INT(cJSON_True, item->type);

    cJSON_Delete(obj);
}

/* Test: Get null item */
void test_cJSON_GetObjectItem_NullItem(void)
{
    cJSON *obj = create_multi_key_object();
    TEST_ASSERT_NOT_NULL(obj);

    cJSON *item = cJSON_GetObjectItem(obj, "null");
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_EQUAL_INT(cJSON_NULL, item->type);

    cJSON_Delete(obj);
}

/* Test: Get raw item */
void test_cJSON_GetObjectItem_Raw(void)
{
    cJSON *obj = cJSON_CreateObject();
    cJSON *raw = cJSON_CreateRaw("{\"key\":\"value\"}");
    cJSON_AddItemToObject(obj, "raw", raw);

    cJSON *item = cJSON_GetObjectItem(obj, "RAW");
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_EQUAL_INT(cJSON_Raw, item->type);
    TEST_ASSERT_EQUAL_STRING("{\"key\":\"value\"}", item->valuestring);

    cJSON_Delete(obj);
}

/* Test: Get string reference item */
void test_cJSON_GetObjectItem_StringReference(void)
{
    cJSON *obj = cJSON_CreateObject();
    cJSON *str_ref = cJSON_CreateStringReference("constant_string");
    cJSON_AddItemToObject(obj, "string_ref", str_ref);

    cJSON *item = cJSON_GetObjectItem(obj, "STRING_REF");
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_EQUAL_INT(cJSON_String | cJSON_StringIsConst, item->type);
    TEST_ASSERT_EQUAL_STRING("constant_string", item->valuestring);

    cJSON_Delete(obj);
}

/* Test: Get object reference item */
void test_cJSON_GetObjectItem_ObjectReference(void)
{
    cJSON *original = cJSON_CreateObject();
    cJSON *num = cJSON_CreateNumber(999);
    cJSON_AddItemToObject(original, "value", num);

    cJSON *obj = cJSON_CreateObject();
    cJSON *obj_ref = cJSON_CreateObjectReference(original);
    cJSON_AddItemToObject(obj, "obj_ref", obj_ref);

    cJSON *item = cJSON_GetObjectItem(obj, "OBJ_REF");
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_EQUAL_INT(cJSON_Object | cJSON_IsReference, item->type);

    cJSON *ref_item = cJSON_GetObjectItem(item, "value");
    TEST_ASSERT_NOT_NULL(ref_item);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, ref_item->type);
    TEST_ASSERT_EQUAL_DOUBLE(999.0, ref_item->valuedouble);

    cJSON_Delete(obj);
    /* Note: original is not deleted here to preserve reference integrity */
}

/* Test: Get array reference item */
void test_cJSON_GetObjectItem_ArrayReference(void)
{
    cJSON *original = cJSON_CreateArray();
    cJSON_AddItemToArray(original, cJSON_CreateNumber(7));

    cJSON *obj = cJSON_CreateObject();
    cJSON *arr_ref = cJSON_CreateArrayReference(original);
    cJSON_AddItemToObject(obj, "arr_ref", arr_ref);

    cJSON *item = cJSON_GetObjectItem(obj, "ARR_REF");
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_EQUAL_INT(cJSON_Array | cJSON_IsReference, item->type);

    cJSON *ref_item = cJSON_GetArrayItem(item, 0);
    TEST_ASSERT_NOT_NULL(ref_item);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, ref_item->type);
    TEST_ASSERT_EQUAL_DOUBLE(7.0, ref_item->valuedouble);

    cJSON_Delete(obj);
    /* Note: original is not deleted here to preserve reference integrity */
}

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_cJSON_GetObjectItem_ExistingKey);
    RUN_TEST(test_cJSON_GetObjectItem_NonExistingKey);
    RUN_TEST(test_cJSON_GetObjectItem_NullObject);
    RUN_TEST(test_cJSON_GetObjectItem_NullKey);
    RUN_TEST(test_cJSON_GetObjectItem_EmptyObject);
    RUN_TEST(test_cJSON_GetObjectItem_CaseInsensitive);
    RUN_TEST(test_cJSON_GetObjectItem_NestedObject);
    RUN_TEST(test_cJSON_GetObjectItem_Array);
    RUN_TEST(test_cJSON_GetObjectItem_Boolean);
    RUN_TEST(test_cJSON_GetObjectItem_NullItem);
    RUN_TEST(test_cJSON_GetObjectItem_Raw);
    RUN_TEST(test_cJSON_GetObjectItem_StringReference);
    RUN_TEST(test_cJSON_GetObjectItem_ObjectReference);
    RUN_TEST(test_cJSON_GetObjectItem_ArrayReference);
    return UNITY_END();
}