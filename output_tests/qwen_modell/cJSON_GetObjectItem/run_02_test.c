#include "cJSON.c"
#include "unity.h"

/* File-scope static variables / fixtures */
static cJSON *test_object = NULL;

/* Helper functions */
static void create_test_object(void)
{
    cJSON *item1 = NULL;
    cJSON *item2 = NULL;
    cJSON *item3 = NULL;

    test_object = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(test_object);

    item1 = cJSON_CreateString("value1");
    TEST_ASSERT_NOT_NULL(item1);
    item1->string = cJSON_strdup("Key1");
    TEST_ASSERT_NOT_NULL(item1->string);
    cJSON_AddItemToObject(test_object, "Key1", item1);

    item2 = cJSON_CreateNumber(42.0);
    TEST_ASSERT_NOT_NULL(item2);
    item2->string = cJSON_strdup("Key2");
    TEST_ASSERT_NOT_NULL(item2->string);
    cJSON_AddItemToObject(test_object, "Key2", item2);

    item3 = cJSON_CreateNull();
    TEST_ASSERT_NOT_NULL(item3);
    item3->string = cJSON_strdup("Key3");
    TEST_ASSERT_NOT_NULL(item3->string);
    cJSON_AddItemToObject(test_object, "Key3", item3);
}

static void cleanup_test_object(void)
{
    if (test_object != NULL)
    {
        cJSON_Delete(test_object);
        test_object = NULL;
    }
}

/* Test cases */

void test_cJSON_GetObjectItem_returns_null_when_object_is_null(void)
{
    TEST_ASSERT_NULL(cJSON_GetObjectItem(NULL, "key"));
}

void test_cJSON_GetObjectItem_returns_null_when_name_is_null(void)
{
    TEST_ASSERT_NULL(cJSON_GetObjectItem(test_object, NULL));
}

void test_cJSON_GetObjectItem_returns_null_when_object_has_no_child(void)
{
    cJSON *empty_object = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(empty_object);
    TEST_ASSERT_NULL(cJSON_GetObjectItem(empty_object, "key"));
    cJSON_Delete(empty_object);
}

void test_cJSON_GetObjectItem_returns_null_when_key_not_found(void)
{
    TEST_ASSERT_NULL(cJSON_GetObjectItem(test_object, "NonExistentKey"));
}

void test_cJSON_GetObjectItem_returns_item_for_existing_key_case_insensitive(void)
{
    cJSON *item = NULL;

    item = cJSON_GetObjectItem(test_object, "key1");
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_EQUAL_STRING("value1", cJSON_GetStringValue(item));

    item = cJSON_GetObjectItem(test_object, "KEY1");
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_EQUAL_STRING("value1", cJSON_GetStringValue(item));

    item = cJSON_GetObjectItem(test_object, "KeY1");
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_EQUAL_STRING("value1", cJSON_GetStringValue(item));
}

void test_cJSON_GetObjectItem_returns_correct_item_for_second_key(void)
{
    cJSON *item = NULL;

    item = cJSON_GetObjectItem(test_object, "key2");
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_EQUAL_DOUBLE(42.0, cJSON_GetNumberValue(item), 0.0001);
}

void test_cJSON_GetObjectItem_returns_correct_item_for_null_value(void)
{
    cJSON *item = NULL;

    item = cJSON_GetObjectItem(test_object, "key3");
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_TRUE(cJSON_IsNull(item));
}

void test_cJSON_GetObjectItem_returns_same_item_pointer(void)
{
    cJSON *item1 = NULL;
    cJSON *item2 = NULL;

    item1 = cJSON_GetObjectItem(test_object, "key1");
    item2 = cJSON_GetObjectItem(test_object, "KEY1");
    TEST_ASSERT_EQUAL_PTR(item1, item2);
}

void test_cJSON_GetObjectItem_returns_null_when_string_member_is_null(void)
{
    cJSON *obj = NULL;
    cJSON *item = NULL;

    obj = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(obj);

    item = cJSON_CreateString("value");
    TEST_ASSERT_NOT_NULL(item);
    item->string = NULL;  /* Force string to be NULL */
    cJSON_AddItemToObject(obj, "temp", item);  /* Add with temp key, but override string to NULL */

    /* Now try to find it by key "temp" */
    TEST_ASSERT_NULL(cJSON_GetObjectItem(obj, "temp"));

    cJSON_Delete(obj);
}

void test_cJSON_GetObjectItem_returns_null_when_item_has_no_string_and_no_next(void)
{
    cJSON *obj = NULL;
    cJSON *item = NULL;

    obj = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(obj);

    item = cJSON_CreateString("value");
    TEST_ASSERT_NOT_NULL(item);
    item->string = NULL;
    cJSON_AddItemToObject(obj, "temp", item);

    /* The item has no string, so get_object_item should return NULL */
    TEST_ASSERT_NULL(cJSON_GetObjectItem(obj, "temp"));

    cJSON_Delete(obj);
}

void test_cJSON_GetObjectItem_handles_empty_string_key(void)
{
    cJSON *obj = NULL;
    cJSON *item = NULL;

    obj = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(obj);

    item = cJSON_CreateString("empty_key_value");
    TEST_ASSERT_NOT_NULL(item);
    item->string = cJSON_strdup("");
    TEST_ASSERT_NOT_NULL(item->string);
    cJSON_AddItemToObject(obj, "", item);

    TEST_ASSERT_NOT_NULL(cJSON_GetObjectItem(obj, ""));
    TEST_ASSERT_EQUAL_STRING("empty_key_value", cJSON_GetStringValue(cJSON_GetObjectItem(obj, "")));

    cJSON_Delete(obj);
}

void test_cJSON_GetObjectItem_returns_null_when_object_child_is_null_after_adding_and_removing(void)
{
    cJSON *obj = NULL;
    cJSON *item = NULL;

    obj = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(obj);

    item = cJSON_CreateString("value");
    TEST_ASSERT_NOT_NULL(item);
    item->string = cJSON_strdup("key");
    TEST_ASSERT_NOT_NULL(item->string);
    cJSON_AddItemToObject(obj, "key", item);

    /* Remove the item */
    cJSON_DeleteItemFromObject(obj, "key");

    TEST_ASSERT_NULL(cJSON_GetObjectItem(obj, "key"));

    cJSON_Delete(obj);
}

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();

    RUN_TEST(test_cJSON_GetObjectItem_returns_null_when_object_is_null);
    RUN_TEST(test_cJSON_GetObjectItem_returns_null_when_name_is_null);
    RUN_TEST(test_cJSON_GetObjectItem_returns_null_when_object_has_no_child);
    RUN_TEST(test_cJSON_GetObjectItem_returns_null_when_key_not_found);
    RUN_TEST(test_cJSON_GetObjectItem_returns_item_for_existing_key_case_insensitive);
    RUN_TEST(test_cJSON_GetObjectItem_returns_correct_item_for_second_key);
    RUN_TEST(test_cJSON_GetObjectItem_returns_correct_item_for_null_value);
    RUN_TEST(test_cJSON_GetObjectItem_returns_same_item_pointer);
    RUN_TEST(test_cJSON_GetObjectItem_returns_null_when_string_member_is_null);
    RUN_TEST(test_cJSON_GetObjectItem_returns_null_when_item_has_no_string_and_no_next);
    RUN_TEST(test_cJSON_GetObjectItem_handles_empty_string_key);
    RUN_TEST(test_cJSON_GetObjectItem_returns_null_when_object_child_is_null_after_adding_and_removing);

    return UNITY_END();
}