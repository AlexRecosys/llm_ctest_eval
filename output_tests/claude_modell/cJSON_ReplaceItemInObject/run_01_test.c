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

/* -------------------------------------------------------------------------
 * Test cases
 * ---------------------------------------------------------------------- */

/* 1. NULL object → must return false (0) */
void test_replace_null_object_returns_false(void)
{
    cJSON *newitem = make_number(42.0);
    cJSON_bool result = cJSON_ReplaceItemInObject(NULL, "key", newitem);
    TEST_ASSERT_FALSE(result);
    cJSON_Delete(newitem);
}

/* 2. NULL string → must return false */
void test_replace_null_string_returns_false(void)
{
    cJSON_AddItemToObject(test_object, "key", make_number(1.0));
    cJSON *newitem = make_number(99.0);
    cJSON_bool result = cJSON_ReplaceItemInObject(test_object, NULL, newitem);
    TEST_ASSERT_FALSE(result);
    cJSON_Delete(newitem);
}

/* 3. NULL newitem → must return false */
void test_replace_null_newitem_returns_false(void)
{
    cJSON_AddItemToObject(test_object, "key", make_number(1.0));
    cJSON_bool result = cJSON_ReplaceItemInObject(test_object, "key", NULL);
    TEST_ASSERT_FALSE(result);
}

/* 4. Key not found → must return false */
void test_replace_key_not_found_returns_false(void)
{
    cJSON_AddItemToObject(test_object, "existing", make_number(1.0));
    cJSON *newitem = make_number(2.0);
    cJSON_bool result = cJSON_ReplaceItemInObject(test_object, "nonexistent", newitem);
    TEST_ASSERT_FALSE(result);
    cJSON_Delete(newitem);
}

/* 5. Successful replacement returns true */
void test_replace_existing_key_returns_true(void)
{
    cJSON_AddItemToObject(test_object, "key", make_number(1.0));
    cJSON *newitem = make_number(99.0);
    cJSON_bool result = cJSON_ReplaceItemInObject(test_object, "key", newitem);
    TEST_ASSERT_TRUE(result);
}

/* 6. After replacement the new value is retrievable */
void test_replace_existing_key_value_updated(void)
{
    cJSON_AddItemToObject(test_object, "key", make_number(1.0));
    cJSON *newitem = make_number(99.0);
    cJSON_ReplaceItemInObject(test_object, "key", newitem);

    cJSON *found = cJSON_GetObjectItem(test_object, "key");
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_DOUBLE(99.0, found->valuedouble);
}

/* 7. After replacement the old value is gone (only one item with that key) */
void test_replace_existing_key_old_value_gone(void)
{
    cJSON_AddItemToObject(test_object, "key", make_number(1.0));
    cJSON *newitem = make_number(99.0);
    cJSON_ReplaceItemInObject(test_object, "key", newitem);

    /* Array size should still be 1 */
    TEST_ASSERT_EQUAL_INT(1, cJSON_GetArraySize(test_object));
}

/* 8. Replacement preserves the key name on the new item */
void test_replace_preserves_key_name(void)
{
    cJSON_AddItemToObject(test_object, "mykey", make_number(1.0));
    cJSON *newitem = make_number(55.0);
    cJSON_ReplaceItemInObject(test_object, "mykey", newitem);

    cJSON *found = cJSON_GetObjectItem(test_object, "mykey");
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_STRING("mykey", found->string);
}

/* 9. Replace with a string item */
void test_replace_with_string_item(void)
{
    cJSON_AddItemToObject(test_object, "name", make_string("old"));
    cJSON *newitem = make_string("new");
    cJSON_bool result = cJSON_ReplaceItemInObject(test_object, "name", newitem);
    TEST_ASSERT_TRUE(result);

    cJSON *found = cJSON_GetObjectItem(test_object, "name");
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_STRING("new", found->valuestring);
}

/* 10. Replace one key in an object with multiple keys; others unaffected */
void test_replace_one_of_multiple_keys(void)
{
    cJSON_AddItemToObject(test_object, "a", make_number(1.0));
    cJSON_AddItemToObject(test_object, "b", make_number(2.0));
    cJSON_AddItemToObject(test_object, "c", make_number(3.0));

    cJSON *newitem = make_number(200.0);
    cJSON_bool result = cJSON_ReplaceItemInObject(test_object, "b", newitem);
    TEST_ASSERT_TRUE(result);

    cJSON *a = cJSON_GetObjectItem(test_object, "a");
    cJSON *b = cJSON_GetObjectItem(test_object, "b");
    cJSON *c = cJSON_GetObjectItem(test_object, "c");

    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    TEST_ASSERT_NOT_NULL(c);

    TEST_ASSERT_EQUAL_DOUBLE(1.0, a->valuedouble);
    TEST_ASSERT_EQUAL_DOUBLE(200.0, b->valuedouble);
    TEST_ASSERT_EQUAL_DOUBLE(3.0, c->valuedouble);

    TEST_ASSERT_EQUAL_INT(3, cJSON_GetArraySize(test_object));
}

/* 11. Replace the first item in a multi-item object */
void test_replace_first_item_in_object(void)
{
    cJSON_AddItemToObject(test_object, "first", make_number(10.0));
    cJSON_AddItemToObject(test_object, "second", make_number(20.0));

    cJSON *newitem = make_number(100.0);
    cJSON_bool result = cJSON_ReplaceItemInObject(test_object, "first", newitem);
    TEST_ASSERT_TRUE(result);

    cJSON *first = cJSON_GetObjectItem(test_object, "first");
    TEST_ASSERT_NOT_NULL(first);
    TEST_ASSERT_EQUAL_DOUBLE(100.0, first->valuedouble);

    cJSON *second = cJSON_GetObjectItem(test_object, "second");
    TEST_ASSERT_NOT_NULL(second);
    TEST_ASSERT_EQUAL_DOUBLE(20.0, second->valuedouble);
}

/* 12. Replace the last item in a multi-item object */
void test_replace_last_item_in_object(void)
{
    cJSON_AddItemToObject(test_object, "first", make_number(10.0));
    cJSON_AddItemToObject(test_object, "last", make_number(20.0));

    cJSON *newitem = make_number(999.0);
    cJSON_bool result = cJSON_ReplaceItemInObject(test_object, "last", newitem);
    TEST_ASSERT_TRUE(result);

    cJSON *last = cJSON_GetObjectItem(test_object, "last");
    TEST_ASSERT_NOT_NULL(last);
    TEST_ASSERT_EQUAL_DOUBLE(999.0, last->valuedouble);
}

/* 13. Case-insensitive: replace with different case key should succeed */
void test_replace_case_insensitive_match(void)
{
    cJSON_AddItemToObject(test_object, "Key", make_number(1.0));
    cJSON *newitem = make_number(77.0);
    /* cJSON_ReplaceItemInObject uses case-insensitive matching */
    cJSON_bool result = cJSON_ReplaceItemInObject(test_object, "key", newitem);
    TEST_ASSERT_TRUE(result);

    /* GetObjectItem is also case-insensitive */
    cJSON *found = cJSON_GetObjectItem(test_object, "Key");
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_DOUBLE(77.0, found->valuedouble);
}

/* 14. Replace in an empty object returns false */
void test_replace_in_empty_object_returns_false(void)
{
    cJSON *newitem = make_number(1.0);
    cJSON_bool result = cJSON_ReplaceItemInObject(test_object, "key", newitem);
    TEST_ASSERT_FALSE(result);
    cJSON_Delete(newitem);
}

/* 15. Replace with a nested object */
void test_replace_with_nested_object(void)
{
    cJSON_AddItemToObject(test_object, "nested", make_number(0.0));

    cJSON *inner = cJSON_CreateObject();
    cJSON_AddItemToObject(inner, "x", make_number(42.0));

    cJSON_bool result = cJSON_ReplaceItemInObject(test_object, "nested", inner);
    TEST_ASSERT_TRUE(result);

    cJSON *found = cJSON_GetObjectItem(test_object, "nested");
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_TRUE(cJSON_IsObject(found));

    cJSON *x = cJSON_GetObjectItem(found, "x");
    TEST_ASSERT_NOT_NULL(x);
    TEST_ASSERT_EQUAL_DOUBLE(42.0, x->valuedouble);
}

/* 16. Replace with a null JSON item (cJSON_NULL type) */
void test_replace_with_null_json_item(void)
{
    cJSON_AddItemToObject(test_object, "key", make_number(1.0));
    cJSON *newitem = cJSON_CreateNull();
    cJSON_bool result = cJSON_ReplaceItemInObject(test_object, "key", newitem);
    TEST_ASSERT_TRUE(result);

    cJSON *found = cJSON_GetObjectItem(test_object, "key");
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_TRUE(cJSON_IsNull(found));
}

/* 17. Replace with a boolean item */
void test_replace_with_bool_item(void)
{
    cJSON_AddItemToObject(test_object, "flag", cJSON_CreateFalse());
    cJSON *newitem = cJSON_CreateTrue();
    cJSON_bool result = cJSON_ReplaceItemInObject(test_object, "flag", newitem);
    TEST_ASSERT_TRUE(result);

    cJSON *found = cJSON_GetObjectItem(test_object, "flag");
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_TRUE(cJSON_IsTrue(found));
}

/* 18. Replace with an array item */
void test_replace_with_array_item(void)
{
    cJSON_AddItemToObject(test_object, "arr", make_number(0.0));
    cJSON *arr = cJSON_CreateArray();
    cJSON_AddItemToArray(arr, make_number(1.0));
    cJSON_AddItemToArray(arr, make_number(2.0));

    cJSON_bool result = cJSON_ReplaceItemInObject(test_object, "arr", arr);
    TEST_ASSERT_TRUE(result);

    cJSON *found = cJSON_GetObjectItem(test_object, "arr");
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_TRUE(cJSON_IsArray(found));
    TEST_ASSERT_EQUAL_INT(2, cJSON_GetArraySize(found));
}

/* 19. Object size does not change after replacement */
void test_replace_does_not_change_object_size(void)
{
    cJSON_AddItemToObject(test_object, "a", make_number(1.0));
    cJSON_AddItemToObject(test_object, "b", make_number(2.0));

    int size_before = cJSON_GetArraySize(test_object);
    cJSON_ReplaceItemInObject(test_object, "a", make_number(100.0));
    int size_after = cJSON_GetArraySize(test_object);

    TEST_ASSERT_EQUAL_INT(size_before, size_after);
}

/* 20. Replace same key twice works correctly */
void test_replace_same_key_twice(void)
{
    cJSON_AddItemToObject(test_object, "key", make_number(1.0));

    cJSON_bool r1 = cJSON_ReplaceItemInObject(test_object, "key", make_number(2.0));
    TEST_ASSERT_TRUE(r1);

    cJSON_bool r2 = cJSON_ReplaceItemInObject(test_object, "key", make_number(3.0));
    TEST_ASSERT_TRUE(r2);

    cJSON *found = cJSON_GetObjectItem(test_object, "key");
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_DOUBLE(3.0, found->valuedouble);
    TEST_ASSERT_EQUAL_INT(1, cJSON_GetArraySize(test_object));
}

/* -------------------------------------------------------------------------
 * main
 * ---------------------------------------------------------------------- */
int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_replace_null_object_returns_false);
    RUN_TEST(test_replace_null_string_returns_false);
    RUN_TEST(test_replace_null_newitem_returns_false);
    RUN_TEST(test_replace_key_not_found_returns_false);
    RUN_TEST(test_replace_existing_key_returns_true);
    RUN_TEST(test_replace_existing_key_value_updated);
    RUN_TEST(test_replace_existing_key_old_value_gone);
    RUN_TEST(test_replace_preserves_key_name);
    RUN_TEST(test_replace_with_string_item);
    RUN_TEST(test_replace_one_of_multiple_keys);
    RUN_TEST(test_replace_first_item_in_object);
    RUN_TEST(test_replace_last_item_in_object);
    RUN_TEST(test_replace_case_insensitive_match);
    RUN_TEST(test_replace_in_empty_object_returns_false);
    RUN_TEST(test_replace_with_nested_object);
    RUN_TEST(test_replace_with_null_json_item);
    RUN_TEST(test_replace_with_bool_item);
    RUN_TEST(test_replace_with_array_item);
    RUN_TEST(test_replace_does_not_change_object_size);
    RUN_TEST(test_replace_same_key_twice);
    return UNITY_END();
}