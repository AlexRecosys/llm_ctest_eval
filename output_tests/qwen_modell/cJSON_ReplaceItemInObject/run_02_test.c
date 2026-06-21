#include "cJSON.c"
#include "unity.h"

/* Global hooks for initialization */
static cJSON_Hooks test_hooks = {
    .malloc_fn = malloc,
    .free_fn = free
};

/* Helper: create a cJSON object with one key-value pair */
static cJSON* create_test_object(const char *key, cJSON *value)
{
    cJSON *obj = cJSON_CreateObject();
    if (obj == NULL) {
        return NULL;
    }
    if (!cJSON_AddItemToObject(obj, key, value)) {
        cJSON_Delete(obj);
        return NULL;
    }
    return obj;
}

/* Helper: compare two cJSON objects for structural equality (keys and children only) */
static cJSON_bool objects_equal(const cJSON *a, const cJSON *b)
{
    if (a == b) {
        return true;
    }
    if (a == NULL || b == NULL) {
        return false;
    }
    if (a->type != b->type) {
        return false;
    }
    if (a->type & (cJSON_Object | cJSON_Array)) {
        const cJSON *child_a = a->child;
        const cJSON *child_b = b->child;
        while (child_a != NULL && child_b != NULL) {
            if (strcmp(child_a->string, child_b->string) != 0) {
                return false;
            }
            if (!objects_equal(child_a, child_b)) {
                return false;
            }
            child_a = child_a->next;
            child_b = child_b->next;
        }
        if (child_a != NULL || child_b != NULL) {
            return false;
        }
    }
    return true;
}

/* Helper: get item by key for verification */
static cJSON* get_item(cJSON *obj, const char *key)
{
    return cJSON_GetObjectItem(obj, key);
}

/* Helper: free replacement item after test to avoid leaks */
static void cleanup_replacement(cJSON *replacement)
{
    if (replacement != NULL) {
        /* Ensure string is not const before freeing */
        replacement->type &= ~cJSON_StringIsConst;
        cJSON_free(replacement->string);
        replacement->string = NULL;
        cJSON_Delete(replacement);
    }
}

/* Test: Replace existing item in object */
void test_cJSON_ReplaceItemInObject_ReplaceExistingItem(void)
{
    cJSON *obj = NULL;
    cJSON *old_item = NULL;
    cJSON *new_item = NULL;
    cJSON_bool result;

    /* Setup */
    cJSON_InitHooks(&test_hooks);
    obj = create_test_object("key1", cJSON_CreateString("old_value"));
    TEST_ASSERT_NOT_NULL(obj);

    old_item = get_item(obj, "key1");
    TEST_ASSERT_NOT_NULL(old_item);
    TEST_ASSERT_EQUAL_STRING("old_value", cJSON_GetStringValue(old_item));

    new_item = cJSON_CreateString("new_value");
    TEST_ASSERT_NOT_NULL(new_item);

    /* Execute */
    result = cJSON_ReplaceItemInObject(obj, "key1", new_item);

    /* Verify */
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_PTR(new_item, get_item(obj, "key1"));
    TEST_ASSERT_NULL(get_item(obj, "key1")->prev);  /* Should be first child */
    TEST_ASSERT_EQUAL_STRING("new_value", cJSON_GetStringValue(get_item(obj, "key1")));
    TEST_ASSERT_NULL(old_item->next);  /* Old item detached */
    TEST_ASSERT_NULL(old_item->prev);
    TEST_ASSERT_NULL(old_item->child);
    TEST_ASSERT_NULL(old_item->string);  /* String freed */
    TEST_ASSERT_EQUAL_INT(0, old_item->type & cJSON_StringIsConst);

    /* Cleanup */
    cJSON_Delete(obj);
    cleanup_replacement(new_item);
}

/* Test: Replace non-existing item (should not add new item) */
void test_cJSON_ReplaceItemInObject_ReplaceNonExistingItem(void)
{
    cJSON *obj = NULL;
    cJSON *new_item = NULL;
    cJSON_bool result;

    /* Setup */
    cJSON_InitHooks(&test_hooks);
    obj = create_test_object("key1", cJSON_CreateString("value1"));
    TEST_ASSERT_NOT_NULL(obj);

    new_item = cJSON_CreateString("new_value");
    TEST_ASSERT_NOT_NULL(new_item);

    /* Execute */
    result = cJSON_ReplaceItemInObject(obj, "nonexistent", new_item);

    /* Verify */
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(get_item(obj, "nonexistent"));
    TEST_ASSERT_NOT_NULL(get_item(obj, "key1"));

    /* Cleanup */
    cJSON_Delete(obj);
    cleanup_replacement(new_item);
}

/* Test: Replace with NULL replacement (should fail) */
void test_cJSON_ReplaceItemInObject_NullReplacement(void)
{
    cJSON *obj = NULL;
    cJSON_bool result;

    /* Setup */
    cJSON_InitHooks(&test_hooks);
    obj = create_test_object("key1", cJSON_CreateString("value1"));
    TEST_ASSERT_NOT_NULL(obj);

    /* Execute */
    result = cJSON_ReplaceItemInObject(obj, "key1", NULL);

    /* Verify */
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NOT_NULL(get_item(obj, "key1"));
    TEST_ASSERT_EQUAL_STRING("value1", cJSON_GetStringValue(get_item(obj, "key1")));

    /* Cleanup */
    cJSON_Delete(obj);
}

/* Test: Replace with NULL string (should fail) */
void test_cJSON_ReplaceItemInObject_NullString(void)
{
    cJSON *obj = NULL;
    cJSON *new_item = NULL;
    cJSON_bool result;

    /* Setup */
    cJSON_InitHooks(&test_hooks);
    obj = create_test_object("key1", cJSON_CreateString("value1"));
    TEST_ASSERT_NOT_NULL(obj);

    new_item = cJSON_CreateString("new_value");
    TEST_ASSERT_NOT_NULL(new_item);

    /* Execute */
    result = cJSON_ReplaceItemInObject(obj, NULL, new_item);

    /* Verify */
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NOT_NULL(get_item(obj, "key1"));
    TEST_ASSERT_EQUAL_STRING("value1", cJSON_GetStringValue(get_item(obj, "key1")));

    /* Cleanup */
    cJSON_Delete(obj);
    cleanup_replacement(new_item);
}

/* Test: Replace item with existing string that has non-const flag (should free old string) */
void test_cJSON_ReplaceItemInObject_FreeOldString(void)
{
    cJSON *obj = NULL;
    cJSON *new_item = NULL;
    cJSON_bool result;

    /* Setup */
    cJSON_InitHooks(&test_hooks);
    obj = create_test_object("key1", cJSON_CreateString("old_value"));
    TEST_ASSERT_NOT_NULL(obj);

    new_item = cJSON_CreateString("new_value");
    TEST_ASSERT_NOT_NULL(new_item);

    /* Verify old string exists */
    TEST_ASSERT_NOT_NULL(get_item(obj, "key1"));
    TEST_ASSERT_EQUAL_STRING("old_value", cJSON_GetStringValue(get_item(obj, "key1")));

    /* Execute */
    result = cJSON_ReplaceItemInObject(obj, "key1", new_item);

    /* Verify */
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("new_value", cJSON_GetStringValue(get_item(obj, "key1")));

    /* Cleanup */
    cJSON_Delete(obj);
    cleanup_replacement(new_item);
}

/* Test: Replace with const string (should not free replacement->string) */
void test_cJSON_ReplaceItemInObject_ConstStringReplacement(void)
{
    cJSON *obj = NULL;
    cJSON *new_item = NULL;
    cJSON_bool result;
    const char *const_string = "const_value";

    /* Setup */
    cJSON_InitHooks(&test_hooks);
    obj = create_test_object("key1", cJSON_CreateString("old_value"));
    TEST_ASSERT_NOT_NULL(obj);

    new_item = cJSON_CreateString(const_string);
    TEST_ASSERT_NOT_NULL(new_item);

    /* Execute */
    result = cJSON_ReplaceItemInObject(obj, "key1", new_item);

    /* Verify */
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING(const_string, cJSON_GetStringValue(get_item(obj, "key1")));
    /* After replacement, string should be non-const (cJSON duplicates it) */
    TEST_ASSERT_EQUAL_INT(0, get_item(obj, "key1")->type & cJSON_StringIsConst);

    /* Cleanup */
    cJSON_Delete(obj);
    cleanup_replacement(new_item);
}

/* Test: Replace item in object with multiple items */
void test_cJSON_ReplaceItemInObject_MultipleItems(void)
{
    cJSON *obj = NULL;
    cJSON *new_item = NULL;
    cJSON_bool result;

    /* Setup */
    cJSON_InitHooks(&test_hooks);
    obj = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(obj);
    TEST_ASSERT_TRUE(cJSON_AddItemToObject(obj, "key1", cJSON_CreateString("value1")));
    TEST_ASSERT_TRUE(cJSON_AddItemToObject(obj, "key2", cJSON_CreateString("value2")));
    TEST_ASSERT_TRUE(cJSON_AddItemToObject(obj, "key3", cJSON_CreateString("value3")));

    new_item = cJSON_CreateString("replaced_value2");
    TEST_ASSERT_NOT_NULL(new_item);

    /* Execute */
    result = cJSON_ReplaceItemInObject(obj, "key2", new_item);

    /* Verify */
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("value1", cJSON_GetStringValue(get_item(obj, "key1")));
    TEST_ASSERT_EQUAL_STRING("replaced_value2", cJSON_GetStringValue(get_item(obj, "key2")));
    TEST_ASSERT_EQUAL_STRING("value3", cJSON_GetStringValue(get_item(obj, "key3")));
    TEST_ASSERT_EQUAL_INT(3, cJSON_GetArraySize(obj));

    /* Cleanup */
    cJSON_Delete(obj);
    cleanup_replacement(new_item);
}

/* Test: Replace item with object (type change) */
void test_cJSON_ReplaceItemInObject_ReplaceWithTypeChange(void)
{
    cJSON *obj = NULL;
    cJSON *new_item = NULL;
    cJSON_bool result;

    /* Setup */
    cJSON_InitHooks(&test_hooks);
    obj = create_test_object("key1", cJSON_CreateString("old_value"));
    TEST_ASSERT_NOT_NULL(obj);

    new_item = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(new_item);
    TEST_ASSERT_TRUE(cJSON_AddItemToObject(new_item, "nested", cJSON_CreateString("nested_value")));

    /* Execute */
    result = cJSON_ReplaceItemInObject(obj, "key1", new_item);

    /* Verify */
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_PTR(new_item, get_item(obj, "key1"));
    TEST_ASSERT_EQUAL_INT(cJSON_Object, get_item(obj, "key1")->type & ~cJSON_StringIsConst);
    TEST_ASSERT_NOT_NULL(get_item(obj, "key1")->child);
    TEST_ASSERT_EQUAL_STRING("nested", get_item(obj, "key1")->child->string);

    /* Cleanup */
    cJSON_Delete(obj);
    cleanup_replacement(new_item);
}

/* Test: Replace item with array (type change) */
void test_cJSON_ReplaceItemInObject_ReplaceWithArray(void)
{
    cJSON *obj = NULL;
    cJSON *new_item = NULL;
    cJSON_bool result;

    /* Setup */
    cJSON_InitHooks(&test_hooks);
    obj = create_test_object("key1", cJSON_CreateString("old_value"));
    TEST_ASSERT_NOT_NULL(obj);

    new_item = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(new_item);
    TEST_ASSERT_TRUE(cJSON_AddItemToArray(new_item, cJSON_CreateString("array_value")));

    /* Execute */
    result = cJSON_ReplaceItemInObject(obj, "key1", new_item);

    /* Verify */
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_PTR(new_item, get_item(obj, "key1"));
    TEST_ASSERT_EQUAL_INT(cJSON_Array, get_item(obj, "key1")->type & ~cJSON_StringIsConst);
    TEST_ASSERT_NOT_NULL(get_item(obj, "key1")->child);

    /* Cleanup */
    cJSON_Delete(obj);
    cleanup_replacement(new_item);
}

/* Test: Replace item with NULL object (should fail) */
void test_cJSON_ReplaceItemInObject_NullObject(void)
{
    cJSON *new_item = NULL;
    cJSON_bool result;

    /* Setup */
    cJSON_InitHooks(&test_hooks);

    new_item = cJSON_CreateString("new_value");
    TEST_ASSERT_NOT_NULL(new_item);

    /* Execute */
    result = cJSON_ReplaceItemInObject(NULL, "key1", new_item);

    /* Verify */
    TEST_ASSERT_FALSE(result);

    /* Cleanup */
    cleanup_replacement(new_item);
}

/* Test: Replace item with empty string key (should fail) */
void test_cJSON_ReplaceItemInObject_EmptyStringKey(void)
{
    cJSON *obj = NULL;
    cJSON *new_item = NULL;
    cJSON_bool result;

    /* Setup */
    cJSON_InitHooks(&test_hooks);
    obj = create_test_object("key1", cJSON_CreateString("value1"));
    TEST_ASSERT_NOT_NULL(obj);

    new_item = cJSON_CreateString("new_value");
    TEST_ASSERT_NOT_NULL(new_item);

    /* Execute */
    result = cJSON_ReplaceItemInObject(obj, "", new_item);

    /* Verify */
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NOT_NULL(get_item(obj, "key1"));

    /* Cleanup */
    cJSON_Delete(obj);
    cleanup_replacement(new_item);
}

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();

    RUN_TEST(test_cJSON_ReplaceItemInObject_ReplaceExistingItem);
    RUN_TEST(test_cJSON_ReplaceItemInObject_ReplaceNonExistingItem);
    RUN_TEST(test_cJSON_ReplaceItemInObject_NullReplacement);
    RUN_TEST(test_cJSON_ReplaceItemInObject_NullString);
    RUN_TEST(test_cJSON_ReplaceItemInObject_FreeOldString);
    RUN_TEST(test_cJSON_ReplaceItemInObject_ConstStringReplacement);
    RUN_TEST(test_cJSON_ReplaceItemInObject_MultipleItems);
    RUN_TEST(test_cJSON_ReplaceItemInObject_ReplaceWithTypeChange);
    RUN_TEST(test_cJSON_ReplaceItemInObject_ReplaceWithArray);
    RUN_TEST(test_cJSON_ReplaceItemInObject_NullObject);
    RUN_TEST(test_cJSON_ReplaceItemInObject_EmptyStringKey);

    return UNITY_END();
}