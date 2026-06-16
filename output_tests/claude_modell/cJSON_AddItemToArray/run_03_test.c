#include "cJSON.c"
#include "unity.h"
#include "cJSON.h"
#include <stdlib.h>
#include <string.h>

/* File-scope fixtures */
static cJSON *array = NULL;
static cJSON *item = NULL;

void setUp(void)
{
    array = cJSON_CreateArray();
    item = NULL;
}

void tearDown(void)
{
    if (array != NULL)
    {
        cJSON_Delete(array);
        array = NULL;
    }
    /* item is owned by array after successful add; only delete if not added */
}

/* -------------------------------------------------------------------------
 * Helper
 * ---------------------------------------------------------------------- */
static cJSON *make_number(double val)
{
    return cJSON_CreateNumber(val);
}

/* =========================================================================
 * Test cases
 * ========================================================================= */

/* 1. NULL array → must return false (0) */
void test_AddItemToArray_NullArray_ReturnsFalse(void)
{
    item = make_number(1.0);
    cJSON_bool result = cJSON_AddItemToArray(NULL, item);
    TEST_ASSERT_FALSE(result);
    /* item was not consumed; free it manually */
    cJSON_Delete(item);
    item = NULL;
}

/* 2. NULL item → must return false (0) */
void test_AddItemToArray_NullItem_ReturnsFalse(void)
{
    cJSON_bool result = cJSON_AddItemToArray(array, NULL);
    TEST_ASSERT_FALSE(result);
}

/* 3. Both NULL → must return false (0) */
void test_AddItemToArray_BothNull_ReturnsFalse(void)
{
    cJSON_bool result = cJSON_AddItemToArray(NULL, NULL);
    TEST_ASSERT_FALSE(result);
}

/* 4. Add single item → returns true, array size becomes 1 */
void test_AddItemToArray_SingleItem_ReturnsTrue(void)
{
    item = make_number(42.0);
    cJSON_bool result = cJSON_AddItemToArray(array, item);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(1, cJSON_GetArraySize(array));
    item = NULL; /* owned by array */
}

/* 5. Add single item → child pointer is set correctly */
void test_AddItemToArray_SingleItem_ChildIsItem(void)
{
    item = make_number(7.0);
    cJSON_AddItemToArray(array, item);
    TEST_ASSERT_EQUAL_PTR(item, array->child);
    item = NULL;
}

/* 6. Add multiple items → size grows correctly */
void test_AddItemToArray_MultipleItems_SizeGrows(void)
{
    int i;
    for (i = 0; i < 5; i++)
    {
        cJSON *n = make_number((double)i);
        cJSON_bool result = cJSON_AddItemToArray(array, n);
        TEST_ASSERT_TRUE(result);
    }
    TEST_ASSERT_EQUAL_INT(5, cJSON_GetArraySize(array));
}

/* 7. Items are appended in order (last added is last in list) */
void test_AddItemToArray_MultipleItems_OrderPreserved(void)
{
    cJSON *n0 = make_number(0.0);
    cJSON *n1 = make_number(1.0);
    cJSON *n2 = make_number(2.0);

    cJSON_AddItemToArray(array, n0);
    cJSON_AddItemToArray(array, n1);
    cJSON_AddItemToArray(array, n2);

    TEST_ASSERT_EQUAL_PTR(n0, cJSON_GetArrayItem(array, 0));
    TEST_ASSERT_EQUAL_PTR(n1, cJSON_GetArrayItem(array, 1));
    TEST_ASSERT_EQUAL_PTR(n2, cJSON_GetArrayItem(array, 2));
}

/* 8. The added item's value is retrievable */
void test_AddItemToArray_ItemValueRetrievable(void)
{
    item = make_number(3.14);
    cJSON_AddItemToArray(array, item);
    cJSON *retrieved = cJSON_GetArrayItem(array, 0);
    TEST_ASSERT_NOT_NULL(retrieved);
    TEST_ASSERT_EQUAL_DOUBLE(3.14, retrieved->valuedouble);
    item = NULL;
}

/* 9. Add a string item */
void test_AddItemToArray_StringItem_AddedCorrectly(void)
{
    cJSON *str = cJSON_CreateString("hello");
    cJSON_bool result = cJSON_AddItemToArray(array, str);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(1, cJSON_GetArraySize(array));
    cJSON *retrieved = cJSON_GetArrayItem(array, 0);
    TEST_ASSERT_NOT_NULL(retrieved);
    TEST_ASSERT_EQUAL_STRING("hello", retrieved->valuestring);
}

/* 10. Add a null JSON item */
void test_AddItemToArray_NullJsonItem_AddedCorrectly(void)
{
    cJSON *null_item = cJSON_CreateNull();
    cJSON_bool result = cJSON_AddItemToArray(array, null_item);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(1, cJSON_GetArraySize(array));
    cJSON *retrieved = cJSON_GetArrayItem(array, 0);
    TEST_ASSERT_NOT_NULL(retrieved);
    TEST_ASSERT_TRUE(cJSON_IsNull(retrieved));
}

/* 11. Add a boolean item */
void test_AddItemToArray_BoolItem_AddedCorrectly(void)
{
    cJSON *bool_item = cJSON_CreateTrue();
    cJSON_bool result = cJSON_AddItemToArray(array, bool_item);
    TEST_ASSERT_TRUE(result);
    cJSON *retrieved = cJSON_GetArrayItem(array, 0);
    TEST_ASSERT_NOT_NULL(retrieved);
    TEST_ASSERT_TRUE(cJSON_IsTrue(retrieved));
}

/* 12. Add a nested array */
void test_AddItemToArray_NestedArray_AddedCorrectly(void)
{
    cJSON *inner = cJSON_CreateArray();
    cJSON_AddItemToArray(inner, make_number(1.0));
    cJSON_bool result = cJSON_AddItemToArray(array, inner);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(1, cJSON_GetArraySize(array));
    cJSON *retrieved = cJSON_GetArrayItem(array, 0);
    TEST_ASSERT_NOT_NULL(retrieved);
    TEST_ASSERT_TRUE(cJSON_IsArray(retrieved));
    TEST_ASSERT_EQUAL_INT(1, cJSON_GetArraySize(retrieved));
}

/* 13. Add a nested object */
void test_AddItemToArray_NestedObject_AddedCorrectly(void)
{
    cJSON *obj = cJSON_CreateObject();
    cJSON_AddItemToObject(obj, "key", make_number(99.0));
    cJSON_bool result = cJSON_AddItemToArray(array, obj);
    TEST_ASSERT_TRUE(result);
    cJSON *retrieved = cJSON_GetArrayItem(array, 0);
    TEST_ASSERT_NOT_NULL(retrieved);
    TEST_ASSERT_TRUE(cJSON_IsObject(retrieved));
}

/* 14. After adding, prev pointer of first item is the last item (circular) */
void test_AddItemToArray_TwoItems_PrevOfFirstIsLast(void)
{
    cJSON *n0 = make_number(0.0);
    cJSON *n1 = make_number(1.0);
    cJSON_AddItemToArray(array, n0);
    cJSON_AddItemToArray(array, n1);
    /* In cJSON the child->prev points to the last element */
    TEST_ASSERT_EQUAL_PTR(n1, array->child->prev);
}

/* 15. After adding two items, next/prev linkage is correct */
void test_AddItemToArray_TwoItems_NextPrevLinkage(void)
{
    cJSON *n0 = make_number(10.0);
    cJSON *n1 = make_number(20.0);
    cJSON_AddItemToArray(array, n0);
    cJSON_AddItemToArray(array, n1);

    TEST_ASSERT_EQUAL_PTR(n1, n0->next);
    TEST_ASSERT_EQUAL_PTR(n0, n1->prev);
    TEST_ASSERT_NULL(n1->next);
}

/* 16. Adding to an object (not array) also succeeds */
void test_AddItemToArray_ObjectTarget_ReturnsTrue(void)
{
    cJSON *obj = cJSON_CreateObject();
    cJSON *num = make_number(5.0);
    cJSON_bool result = cJSON_AddItemToArray(obj, num);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(1, cJSON_GetArraySize(obj));
    cJSON_Delete(obj);
}

/* 17. Large number of items */
void test_AddItemToArray_ManyItems_AllAdded(void)
{
    int count = 100;
    int i;
    for (i = 0; i < count; i++)
    {
        cJSON *n = make_number((double)i);
        cJSON_bool result = cJSON_AddItemToArray(array, n);
        TEST_ASSERT_TRUE_MESSAGE(result, "Failed to add item");
    }
    TEST_ASSERT_EQUAL_INT(count, cJSON_GetArraySize(array));
    for (i = 0; i < count; i++)
    {
        cJSON *retrieved = cJSON_GetArrayItem(array, i);
        TEST_ASSERT_NOT_NULL(retrieved);
        TEST_ASSERT_EQUAL_DOUBLE((double)i, retrieved->valuedouble);
    }
}

/* 18. Return value is exactly 1 (true) on success */
void test_AddItemToArray_ReturnValueIsOne(void)
{
    item = make_number(0.0);
    cJSON_bool result = cJSON_AddItemToArray(array, item);
    TEST_ASSERT_EQUAL_INT(1, result);
    item = NULL;
}

/* 19. Add raw JSON item */
void test_AddItemToArray_RawItem_AddedCorrectly(void)
{
    cJSON *raw = cJSON_CreateRaw("{\"a\":1}");
    cJSON_bool result = cJSON_AddItemToArray(array, raw);
    TEST_ASSERT_TRUE(result);
    cJSON *retrieved = cJSON_GetArrayItem(array, 0);
    TEST_ASSERT_NOT_NULL(retrieved);
    TEST_ASSERT_TRUE(cJSON_IsRaw(retrieved));
}

/* 20. Array size is 0 before any add */
void test_AddItemToArray_EmptyArraySizeIsZero(void)
{
    TEST_ASSERT_EQUAL_INT(0, cJSON_GetArraySize(array));
}

/* =========================================================================
 * main
 * ========================================================================= */
int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_AddItemToArray_NullArray_ReturnsFalse);
    RUN_TEST(test_AddItemToArray_NullItem_ReturnsFalse);
    RUN_TEST(test_AddItemToArray_BothNull_ReturnsFalse);
    RUN_TEST(test_AddItemToArray_SingleItem_ReturnsTrue);
    RUN_TEST(test_AddItemToArray_SingleItem_ChildIsItem);
    RUN_TEST(test_AddItemToArray_MultipleItems_SizeGrows);
    RUN_TEST(test_AddItemToArray_MultipleItems_OrderPreserved);
    RUN_TEST(test_AddItemToArray_ItemValueRetrievable);
    RUN_TEST(test_AddItemToArray_StringItem_AddedCorrectly);
    RUN_TEST(test_AddItemToArray_NullJsonItem_AddedCorrectly);
    RUN_TEST(test_AddItemToArray_BoolItem_AddedCorrectly);
    RUN_TEST(test_AddItemToArray_NestedArray_AddedCorrectly);
    RUN_TEST(test_AddItemToArray_NestedObject_AddedCorrectly);
    RUN_TEST(test_AddItemToArray_TwoItems_PrevOfFirstIsLast);
    RUN_TEST(test_AddItemToArray_TwoItems_NextPrevLinkage);
    RUN_TEST(test_AddItemToArray_ObjectTarget_ReturnsTrue);
    RUN_TEST(test_AddItemToArray_ManyItems_AllAdded);
    RUN_TEST(test_AddItemToArray_ReturnValueIsOne);
    RUN_TEST(test_AddItemToArray_RawItem_AddedCorrectly);
    RUN_TEST(test_AddItemToArray_EmptyArraySizeIsZero);
    return UNITY_END();
}