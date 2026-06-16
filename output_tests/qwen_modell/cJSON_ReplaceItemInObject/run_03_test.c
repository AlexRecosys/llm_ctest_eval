#include "cJSON.c"
#include "unity.h"

/* Helper macros */
#define TEST_ASSERT_CJSON_BOOL_EQUAL(expected, actual) \
    TEST_ASSERT_EQUAL_INT((cJSON_bool)(expected), (cJSON_bool)(actual))

/* Global fixtures */
static cJSON *test_object = NULL;
static cJSON *item_to_replace = NULL;
static cJSON *replacement_item = NULL;

/* Helper functions */
static void setup_test_object(void)
{
    test_object = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(test_object);

    item_to_replace = cJSON_CreateString("original_value");
    TEST_ASSERT_NOT_NULL(item_to_replace);
    cJSON_AddItemToObject(test_object, "key", item_to_replace);
}

static void cleanup_test_objects(void)
{
    if (test_object != NULL)
    {
        cJSON_Delete(test_object);
        test_object = NULL;
    }
    if (replacement_item != NULL)
    {
        cJSON_Delete(replacement_item);
        replacement_item = NULL;
    }
}

/* Test Cases */

void test_cJSON_ReplaceItemInObject_ReplaceExistingKeyWithNewString(void)
{
    setup_test_object();

    replacement_item = cJSON_CreateString("new_value");
    TEST_ASSERT_NOT_NULL(replacement_item);

    cJSON_bool result = cJSON_ReplaceItemInObject(test_object, "key", replacement_item);
    TEST_ASSERT_CJSON_BOOL_EQUAL(cJSON_True, result);

    /* Verify the replacement */
    cJSON *retrieved = cJSON_GetObjectItem(test_object, "key");
    TEST_ASSERT_NOT_NULL(retrieved);
    TEST_ASSERT_EQUAL_STRING("new_value", cJSON_GetStringValue(retrieved));

    /* Original item should be detached and not part of the object */
    TEST_ASSERT_NULL(item_to_replace->parent);
    TEST_ASSERT_NULL(item_to_replace->next);
    TEST_ASSERT_NULL(item_to_replace->prev);

    cleanup_test_objects();
}

void test_cJSON_ReplaceItemInObject_ReplaceExistingKeyWithNumber(void)
{
    setup_test_object();

    replacement_item = cJSON_CreateNumber(42.0);
    TEST_ASSERT_NOT_NULL(replacement_item);

    cJSON_bool result = cJSON_ReplaceItemInObject(test_object, "key", replacement_item);
    TEST_ASSERT_CJSON_BOOL_EQUAL(cJSON_True, result);

    cJSON *retrieved = cJSON_GetObjectItem(test_object, "key");
    TEST_ASSERT_NOT_NULL(retrieved);
    TEST_ASSERT_EQUAL_DOUBLE(42.0, cJSON_GetNumberValue(retrieved));

    cleanup_test_objects();
}

void test_cJSON_ReplaceItemInObject_ReplaceExistingKeyWithObject(void)
{
    setup_test_object();

    replacement_item = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(replacement_item);
    cJSON_AddStringToObject(replacement_item, "nested", "value");

    cJSON_bool result = cJSON_ReplaceItemInObject(test_object, "key", replacement_item);
    TEST_ASSERT_CJSON_BOOL_EQUAL(cJSON_True, result);

    cJSON *retrieved = cJSON_GetObjectItem(test_object, "key");
    TEST_ASSERT_NOT_NULL(retrieved);
    TEST_ASSERT_EQUAL_INT(cJSON_Object, retrieved->type);

    cJSON *nested = cJSON_GetObjectItem(retrieved, "nested");
    TEST_ASSERT_NOT_NULL(nested);
    TEST_ASSERT_EQUAL_STRING("value", cJSON_GetStringValue(nested));

    cleanup_test_objects();
}

void test_cJSON_ReplaceItemInObject_ReplaceNonExistentKeyReturnsFalse(void)
{
    setup_test_object();

    replacement_item = cJSON_CreateString("new_value");
    TEST_ASSERT_NOT_NULL(replacement_item);

    cJSON_bool result = cJSON_ReplaceItemInObject(test_object, "nonexistent", replacement_item);
    TEST_ASSERT_CJSON_BOOL_EQUAL(cJSON_False, result);

    /* Original item should remain unchanged */
    cJSON *retrieved = cJSON_GetObjectItem(test_object, "key");
    TEST_ASSERT_NOT_NULL(retrieved);
    TEST_ASSERT_EQUAL_STRING("original_value", cJSON_GetStringValue(retrieved));

    cleanup_test_objects();
}

void test_cJSON_ReplaceItemInObject_ReplaceWithNullItem(void)
{
    setup_test_object();

    replacement_item = NULL;

    cJSON_bool result = cJSON_ReplaceItemInObject(test_object, "key", replacement_item);
    TEST_ASSERT_CJSON_BOOL_EQUAL(cJSON_False, result);

    /* Original item should remain unchanged */
    cJSON *retrieved = cJSON_GetObjectItem(test_object, "key");
    TEST_ASSERT_NOT_NULL(retrieved);
    TEST_ASSERT_EQUAL_STRING("original_value", cJSON_GetStringValue(retrieved));

    cleanup_test_objects();
}

void test_cJSON_ReplaceItemInObject_ReplaceWithNullObject(void)
{
    replacement_item = cJSON_CreateString("new_value");
    TEST_ASSERT_NOT_NULL(replacement_item);

    cJSON_bool result = cJSON_ReplaceItemInObject(NULL, "key", replacement_item);
    TEST_ASSERT_CJSON_BOOL_EQUAL(cJSON_False, result);

    cleanup_test_objects();
}

void test_cJSON_ReplaceItemInObject_ReplaceWithNullString(void)
{
    setup_test_object();

    replacement_item = cJSON_CreateString("new_value");
    TEST_ASSERT_NOT_NULL(replacement_item);

    cJSON_bool result = cJSON_ReplaceItemInObject(test_object, NULL, replacement_item);
    TEST_ASSERT_CJSON_BOOL_EQUAL(cJSON_False, result);

    /* Original item should remain unchanged */
    cJSON *retrieved = cJSON_GetObjectItem(test_object, "key");
    TEST_ASSERT_NOT_NULL(retrieved);
    TEST_ASSERT_EQUAL_STRING("original_value", cJSON_GetStringValue(retrieved));

    cleanup_test_objects();
}

void test_cJSON_ReplaceItemInObject_ReplaceWithEmptyStringKey(void)
{
    setup_test_object();

    replacement_item = cJSON_CreateString("new_value");
    TEST_ASSERT_NOT_NULL(replacement_item);

    cJSON_bool result = cJSON_ReplaceItemInObject(test_object, "", replacement_item);
    TEST_ASSERT_CJSON_BOOL_EQUAL(cJSON_False, result);

    /* Original item should remain unchanged */
    cJSON *retrieved = cJSON_GetObjectItem(test_object, "key");
    TEST_ASSERT_NOT_NULL(retrieved);
    TEST_ASSERT_EQUAL_STRING("original_value", cJSON_GetStringValue(retrieved));

    cleanup_test_objects();
}

void test_cJSON_ReplaceItemInObject_ReplaceWithCaseSensitiveKey(void)
{
    setup_test_object();

    replacement_item = cJSON_CreateString("new_value");
    TEST_ASSERT_NOT_NULL(replacement_item);

    /* cJSON_ReplaceItemInObject is case-insensitive */
    cJSON_bool result = cJSON_ReplaceItemInObject(test_object, "KEY", replacement_item);
    TEST_ASSERT_CJSON_BOOL_EQUAL(cJSON_True, result);

    /* Verify the replacement */
    cJSON *retrieved = cJSON_GetObjectItem(test_object, "key");
    TEST_ASSERT_NOT_NULL(retrieved);
    TEST_ASSERT_EQUAL_STRING("new_value", cJSON_GetStringValue(retrieved));

    cleanup_test_objects();
}

void test_cJSON_ReplaceItemInObject_ReplaceWithSameItem(void)
{
    setup_test_object();

    /* Replacing with the same item should succeed */
    cJSON_bool result = cJSON_ReplaceItemInObject(test_object, "key", item_to_replace);
    TEST_ASSERT_CJSON_BOOL_EQUAL(cJSON_True, result);

    /* Verify the item is still present */
    cJSON *retrieved = cJSON_GetObjectItem(test_object, "key");
    TEST_ASSERT_EQUAL_PTR(item_to_replace, retrieved);

    cleanup_test_objects();
}

void test_cJSON_ReplaceItemInObject_ReplaceWithObjectHavingChilds(void)
{
    setup_test_object();

    replacement_item = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(replacement_item);
    cJSON_AddNumberToObject(replacement_item, "a", 1.0);
    cJSON_AddStringToObject(replacement_item, "b", "test");

    cJSON_bool result = cJSON_ReplaceItemInObject(test_object, "key", replacement_item);
    TEST_ASSERT_CJSON_BOOL_EQUAL(cJSON_True, result);

    cJSON *retrieved = cJSON_GetObjectItem(test_object, "key");
    TEST_ASSERT_NOT_NULL(retrieved);
    TEST_ASSERT_EQUAL_INT(cJSON_Object, retrieved->type);
    TEST_ASSERT_NOT_NULL(cJSON_GetObjectItem(retrieved, "a"));
    TEST_ASSERT_NOT_NULL(cJSON_GetObjectItem(retrieved, "b"));

    cleanup_test_objects();
}

void test_cJSON_ReplaceItemInObject_ReplaceWithArray(void)
{
    setup_test_object();

    replacement_item = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(replacement_item);
    cJSON_AddNumberToObject(replacement_item, "0", 1.0);
    cJSON_AddStringToObject(replacement_item, "1", "test");

    cJSON_bool result = cJSON_ReplaceItemInObject(test_object, "key", replacement_item);
    TEST_ASSERT_CJSON_BOOL_EQUAL(cJSON_True, result);

    cJSON *retrieved = cJSON_GetObjectItem(test_object, "key");
    TEST_ASSERT_NOT_NULL(retrieved);
    TEST_ASSERT_EQUAL_INT(cJSON_Array, retrieved->type);
    TEST_ASSERT_EQUAL_INT(2, cJSON_GetArraySize(retrieved));

    cleanup_test_objects();
}

void test_cJSON_ReplaceItemInObject_ReplaceWithBool(void)
{
    setup_test_object();

    replacement_item = cJSON_CreateTrue();
    TEST_ASSERT_NOT_NULL(replacement_item);

    cJSON_bool result = cJSON_ReplaceItemInObject(test_object, "key", replacement_item);
    TEST_ASSERT_CJSON_BOOL_EQUAL(cJSON_True, result);

    cJSON *retrieved = cJSON_GetObjectItem(test_object, "key");
    TEST_ASSERT_NOT_NULL(retrieved);
    TEST_ASSERT_EQUAL_INT(cJSON_True, retrieved->type);

    cleanup_test_objects();
}

void test_cJSON_ReplaceItemInObject_ReplaceWithNull(void)
{
    setup_test_object();

    replacement_item = cJSON_CreateNull();
    TEST_ASSERT_NOT_NULL(replacement_item);

    cJSON_bool result = cJSON_ReplaceItemInObject(test_object, "key", replacement_item);
    TEST_ASSERT_CJSON_BOOL_EQUAL(cJSON_True, result);

    cJSON *retrieved = cJSON_GetObjectItem(test_object, "key");
    TEST_ASSERT_NOT_NULL(retrieved);
    TEST_ASSERT_EQUAL_INT(cJSON_NULL, retrieved->type);

    cleanup_test_objects();
}

void test_cJSON_ReplaceItemInObject_ReplaceWithRaw(void)
{
    setup_test_object();

    replacement_item = cJSON_CreateRaw("{\"raw\":\"json\"}");
    TEST_ASSERT_NOT_NULL(replacement_item);

    cJSON_bool result = cJSON_ReplaceItemInObject(test_object, "key", replacement_item);
    TEST_ASSERT_CJSON_BOOL_EQUAL(cJSON_True, result);

    cJSON *retrieved = cJSON_GetObjectItem(test_object, "key");
    TEST_ASSERT_NOT_NULL(retrieved);
    TEST_ASSERT_EQUAL_INT(cJSON_Raw, retrieved->type);
    TEST_ASSERT_EQUAL_STRING("{\"raw\":\"json\"}", cJSON_GetStringValue(retrieved));

    cleanup_test_objects();
}

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_cJSON_ReplaceItemInObject_ReplaceExistingKeyWithNewString);
    RUN_TEST(test_cJSON_ReplaceItemInObject_ReplaceExistingKeyWithNumber);
    RUN_TEST(test_cJSON_ReplaceItemInObject_ReplaceExistingKeyWithObject);
    RUN_TEST(test_cJSON_ReplaceItemInObject_ReplaceNonExistentKeyReturnsFalse);
    RUN_TEST(test_cJSON_ReplaceItemInObject_ReplaceWithNullItem);
    RUN_TEST(test_cJSON_ReplaceItemInObject_ReplaceWithNullObject);
    RUN_TEST(test_cJSON_ReplaceItemInObject_ReplaceWithNullString);
    RUN_TEST(test_cJSON_ReplaceItemInObject_ReplaceWithEmptyStringKey);
    RUN_TEST(test_cJSON_ReplaceItemInObject_ReplaceWithCaseSensitiveKey);
    RUN_TEST(test_cJSON_ReplaceItemInObject_ReplaceWithSameItem);
    RUN_TEST(test_cJSON_ReplaceItemInObject_ReplaceWithObjectHavingChilds);
    RUN_TEST(test_cJSON_ReplaceItemInObject_ReplaceWithArray);
    RUN_TEST(test_cJSON_ReplaceItemInObject_ReplaceWithBool);
    RUN_TEST(test_cJSON_ReplaceItemInObject_ReplaceWithNull);
    RUN_TEST(test_cJSON_ReplaceItemInObject_ReplaceWithRaw);
    return UNITY_END();
}