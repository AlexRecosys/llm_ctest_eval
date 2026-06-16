#include "cJSON.c"
#include "unity.h"
#include "cJSON.h"
#include <stdlib.h>
#include <string.h>

/* File-scope fixtures */
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

/* Helper: create a simple number item */
static cJSON *make_number(double val)
{
    return cJSON_CreateNumber(val);
}

/* Helper: create a simple string item */
static cJSON *make_string(const char *val)
{
    return cJSON_CreateString(val);
}

/* ----------------------------- Test Cases ----------------------------- */

/* Replace an existing item with a new number item */
void test_replace_existing_item_with_number(void)
{
    cJSON_bool result;
    cJSON *found = NULL;

    cJSON_AddNumberToObject(test_object, "key", 1.0);

    cJSON *newitem = make_number(42.0);
    TEST_ASSERT_NOT_NULL(newitem);

    result = cJSON_ReplaceItemInObject(test_object, "key", newitem);
    TEST_ASSERT_TRUE(result);

    found = cJSON_GetObjectItem(test_object, "key");
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_DOUBLE(42.0, found->valuedouble);
}

/* Replace an existing item with a string item */
void test_replace_existing_item_with_string(void)
{
    cJSON_bool result;
    cJSON *found = NULL;

    cJSON_AddStringToObject(test_object, "name", "old_value");

    cJSON *newitem = make_string("new_value");
    TEST_ASSERT_NOT_NULL(newitem);

    result = cJSON_ReplaceItemInObject(test_object, "name", newitem);
    TEST_ASSERT_TRUE(result);

    found = cJSON_GetObjectItem(test_object, "name");
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_STRING("new_value", found->valuestring);
}

/* Replace with a key that does not exist should return false */
void test_replace_nonexistent_key_returns_false(void)
{
    cJSON_bool result;

    cJSON_AddNumberToObject(test_object, "existing", 1.0);

    cJSON *newitem = make_number(99.0);
    TEST_ASSERT_NOT_NULL(newitem);

    result = cJSON_ReplaceItemInObject(test_object, "nonexistent", newitem);
    TEST_ASSERT_FALSE(result);

    /* newitem was not consumed, we must free it */
    cJSON_Delete(newitem);
}

/* NULL object should return false */
void test_replace_null_object_returns_false(void)
{
    cJSON_bool result;
    cJSON *newitem = make_number(1.0);
    TEST_ASSERT_NOT_NULL(newitem);

    result = cJSON_ReplaceItemInObject(NULL, "key", newitem);
    TEST_ASSERT_FALSE(result);

    cJSON_Delete(newitem);
}

/* NULL string should return false */
void test_replace_null_string_returns_false(void)
{
    cJSON_bool result;
    cJSON_AddNumberToObject(test_object, "key", 1.0);

    cJSON *newitem = make_number(2.0);
    TEST_ASSERT_NOT_NULL(newitem);

    result = cJSON_ReplaceItemInObject(test_object, NULL, newitem);
    TEST_ASSERT_FALSE(result);

    cJSON_Delete(newitem);
}

/* NULL newitem should return false */
void test_replace_null_newitem_returns_false(void)
{
    cJSON_bool result;
    cJSON_AddNumberToObject(test_object, "key", 1.0);

    result = cJSON_ReplaceItemInObject(test_object, "key", NULL);
    TEST_ASSERT_FALSE(result);
}

/* After replacement, the object size should remain the same */
void test_replace_does_not_change_object_size(void)
{
    int size_before, size_after;

    cJSON_AddNumberToObject(test_object, "a", 1.0);
    cJSON_AddNumberToObject(test_object, "b", 2.0);
    cJSON_AddNumberToObject(test_object, "c", 3.0);

    size_before = cJSON_GetArraySize(test_object);

    cJSON *newitem = make_number(99.0);
    TEST_ASSERT_NOT_NULL(newitem);

    cJSON_bool result = cJSON_ReplaceItemInObject(test_object, "b", newitem);
    TEST_ASSERT_TRUE(result);

    size_after = cJSON_GetArraySize(test_object);
    TEST_ASSERT_EQUAL_INT(size_before, size_after);
}

/* Replace the first item in a multi-item object */
void test_replace_first_item_in_object(void)
{
    cJSON_AddNumberToObject(test_object, "first", 1.0);
    cJSON_AddNumberToObject(test_object, "second", 2.0);
    cJSON_AddNumberToObject(test_object, "third", 3.0);

    cJSON *newitem = make_number(100.0);
    TEST_ASSERT_NOT_NULL(newitem);

    cJSON_bool result = cJSON_ReplaceItemInObject(test_object, "first", newitem);
    TEST_ASSERT_TRUE(result);

    cJSON *found = cJSON_GetObjectItem(test_object, "first");
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_DOUBLE(100.0, found->valuedouble);

    /* Other items should still be present */
    TEST_ASSERT_NOT_NULL(cJSON_GetObjectItem(test_object, "second"));
    TEST_ASSERT_NOT_NULL(cJSON_GetObjectItem(test_object, "third"));
}

/* Replace the last item in a multi-item object */
void test_replace_last_item_in_object(void)
{
    cJSON_AddNumberToObject(test_object, "first", 1.0);
    cJSON_AddNumberToObject(test_object, "second", 2.0);
    cJSON_AddNumberToObject(test_object, "last", 3.0);

    cJSON *newitem = make_number(200.0);
    TEST_ASSERT_NOT_NULL(newitem);

    cJSON_bool result = cJSON_ReplaceItemInObject(test_object, "last", newitem);
    TEST_ASSERT_TRUE(result);

    cJSON *found = cJSON_GetObjectItem(test_object, "last");
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_DOUBLE(200.0, found->valuedouble);

    TEST_ASSERT_NOT_NULL(cJSON_GetObjectItem(test_object, "first"));
    TEST_ASSERT_NOT_NULL(cJSON_GetObjectItem(test_object, "second"));
}

/* Replace is case-insensitive: key "Key" should match "key" */
void test_replace_is_case_insensitive(void)
{
    cJSON_AddNumberToObject(test_object, "Key", 1.0);

    cJSON *newitem = make_number(55.0);
    TEST_ASSERT_NOT_NULL(newitem);

    /* cJSON_ReplaceItemInObject uses case-insensitive matching */
    cJSON_bool result = cJSON_ReplaceItemInObject(test_object, "key", newitem);
    TEST_ASSERT_TRUE(result);

    /* The item should be retrievable (case-insensitive get) */
    cJSON *found = cJSON_GetObjectItem(test_object, "Key");
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_DOUBLE(55.0, found->valuedouble);
}

/* Replace with a null-type item */
void test_replace_with_null_type_item(void)
{
    cJSON_AddNumberToObject(test_object, "nullable", 1.0);

    cJSON *newitem = cJSON_CreateNull();
    TEST_ASSERT_NOT_NULL(newitem);

    cJSON_bool result = cJSON_ReplaceItemInObject(test_object, "nullable", newitem);
    TEST_ASSERT_TRUE(result);

    cJSON *found = cJSON_GetObjectItem(test_object, "nullable");
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_TRUE(cJSON_IsNull(found));
}

/* Replace with a boolean item */
void test_replace_with_bool_item(void)
{
    cJSON_AddNumberToObject(test_object, "flag", 0.0);

    cJSON *newitem = cJSON_CreateTrue();
    TEST_ASSERT_NOT_NULL(newitem);

    cJSON_bool result = cJSON_ReplaceItemInObject(test_object, "flag", newitem);
    TEST_ASSERT_TRUE(result);

    cJSON *found = cJSON_GetObjectItem(test_object, "flag");
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_TRUE(cJSON_IsTrue(found));
}

/* Replace with an array item */
void test_replace_with_array_item(void)
{
    cJSON_AddNumberToObject(test_object, "data", 0.0);

    cJSON *arr = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(arr);
    cJSON_AddItemToArray(arr, cJSON_CreateNumber(1.0));
    cJSON_AddItemToArray(arr, cJSON_CreateNumber(2.0));

    cJSON_bool result = cJSON_ReplaceItemInObject(test_object, "data", arr);
    TEST_ASSERT_TRUE(result);

    cJSON *found = cJSON_GetObjectItem(test_object, "data");
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_TRUE(cJSON_IsArray(found));
    TEST_ASSERT_EQUAL_INT(2, cJSON_GetArraySize(found));
}

/* Replace with a nested object */
void test_replace_with_nested_object(void)
{
    cJSON_AddStringToObject(test_object, "nested", "old");

    cJSON *inner = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(inner);
    cJSON_AddNumberToObject(inner, "x", 10.0);

    cJSON_bool result = cJSON_ReplaceItemInObject(test_object, "nested", inner);
    TEST_ASSERT_TRUE(result);

    cJSON *found = cJSON_GetObjectItem(test_object, "nested");
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_TRUE(cJSON_IsObject(found));

    cJSON *x = cJSON_GetObjectItem(found, "x");
    TEST_ASSERT_NOT_NULL(x);
    TEST_ASSERT_EQUAL_DOUBLE(10.0, x->valuedouble);
}

/* Replace on an empty object should return false */
void test_replace_on_empty_object_returns_false(void)
{
    cJSON *newitem = make_number(1.0);
    TEST_ASSERT_NOT_NULL(newitem);

    cJSON_bool result = cJSON_ReplaceItemInObject(test_object, "key", newitem);
    TEST_ASSERT_FALSE(result);

    cJSON_Delete(newitem);
}

/* Replace preserves other items' values */
void test_replace_preserves_other_items(void)
{
    cJSON_AddNumberToObject(test_object, "alpha", 1.0);
    cJSON_AddNumberToObject(test_object, "beta", 2.0);
    cJSON_AddNumberToObject(test_object, "gamma", 3.0);

    cJSON *newitem = make_number(999.0);
    TEST_ASSERT_NOT_NULL(newitem);

    cJSON_bool result = cJSON_ReplaceItemInObject(test_object, "beta", newitem);
    TEST_ASSERT_TRUE(result);

    cJSON *alpha = cJSON_GetObjectItem(test_object, "alpha");
    TEST_ASSERT_NOT_NULL(alpha);
    TEST_ASSERT_EQUAL_DOUBLE(1.0, alpha->valuedouble);

    cJSON *gamma = cJSON_GetObjectItem(test_object, "gamma");
    TEST_ASSERT_NOT_NULL(gamma);
    TEST_ASSERT_EQUAL_DOUBLE(3.0, gamma->valuedouble);

    cJSON *beta = cJSON_GetObjectItem(test_object, "beta");
    TEST_ASSERT_NOT_NULL(beta);
    TEST_ASSERT_EQUAL_DOUBLE(999.0, beta->valuedouble);
}

/* Replace the only item in a single-item object */
void test_replace_only_item_in_object(void)
{
    cJSON_AddStringToObject(test_object, "solo", "original");

    cJSON *newitem = make_string("replaced");
    TEST_ASSERT_NOT_NULL(newitem);

    cJSON_bool result = cJSON_ReplaceItemInObject(test_object, "solo", newitem);
    TEST_ASSERT_TRUE(result);

    TEST_ASSERT_EQUAL_INT(1, cJSON_GetArraySize(test_object));

    cJSON *found = cJSON_GetObjectItem(test_object, "solo");
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_STRING("replaced", found->valuestring);
}

/* ----------------------------- main ----------------------------- */

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_replace_existing_item_with_number);
    RUN_TEST(test_replace_existing_item_with_string);
    RUN_TEST(test_replace_nonexistent_key_returns_false);
    RUN_TEST(test_replace_null_object_returns_false);
    RUN_TEST(test_replace_null_string_returns_false);
    RUN_TEST(test_replace_null_newitem_returns_false);
    RUN_TEST(test_replace_does_not_change_object_size);
    RUN_TEST(test_replace_first_item_in_object);
    RUN_TEST(test_replace_last_item_in_object);
    RUN_TEST(test_replace_is_case_insensitive);
    RUN_TEST(test_replace_with_null_type_item);
    RUN_TEST(test_replace_with_bool_item);
    RUN_TEST(test_replace_with_array_item);
    RUN_TEST(test_replace_with_nested_object);
    RUN_TEST(test_replace_on_empty_object_returns_false);
    RUN_TEST(test_replace_preserves_other_items);
    RUN_TEST(test_replace_only_item_in_object);
    return UNITY_END();
}