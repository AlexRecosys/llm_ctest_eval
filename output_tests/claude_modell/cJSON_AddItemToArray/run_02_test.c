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

/* 6. Add single item → item's prev points to itself (circular tail sentinel) */
void test_AddItemToArray_SingleItem_PrevPointsToSelf(void)
{
    item = make_number(3.0);
    cJSON_AddItemToArray(array, item);
    /* In cJSON the last element's next is NULL and child->prev points to last */
    TEST_ASSERT_EQUAL_PTR(item, array->child->prev);
    item = NULL;
}

/* 7. Add single item → item's next is NULL */
void test_AddItemToArray_SingleItem_NextIsNull(void)
{
    item = make_number(5.0);
    cJSON_AddItemToArray(array, item);
    TEST_ASSERT_NULL(item->next);
    item = NULL;
}

/* 8. Add multiple items → size grows correctly */
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

/* 9. Add multiple items → values are stored in insertion order */
void test_AddItemToArray_MultipleItems_OrderPreserved(void)
{
    int i;
    for (i = 0; i < 3; i++)
    {
        cJSON_AddItemToArray(array, make_number((double)i));
    }
    for (i = 0; i < 3; i++)
    {
        cJSON *retrieved = cJSON_GetArrayItem(array, i);
        TEST_ASSERT_NOT_NULL(retrieved);
        TEST_ASSERT_EQUAL_DOUBLE((double)i, retrieved->valuedouble);
    }
}

/* 10. Add string item → stored correctly */
void test_AddItemToArray_StringItem_StoredCorrectly(void)
{
    cJSON *str = cJSON_CreateString("hello");
    cJSON_bool result = cJSON_AddItemToArray(array, str);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(1, cJSON_GetArraySize(array));
    cJSON *retrieved = cJSON_GetArrayItem(array, 0);
    TEST_ASSERT_NOT_NULL(retrieved);
    TEST_ASSERT_EQUAL_STRING("hello", retrieved->valuestring);
}

/* 11. Add null JSON item → stored correctly */
void test_AddItemToArray_NullJsonItem_StoredCorrectly(void)
{
    cJSON *null_item = cJSON_CreateNull();
    cJSON_bool result = cJSON_AddItemToArray(array, null_item);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(1, cJSON_GetArraySize(array));
    cJSON *retrieved = cJSON_GetArrayItem(array, 0);
    TEST_ASSERT_NOT_NULL(retrieved);
    TEST_ASSERT_TRUE(cJSON_IsNull(retrieved));
}

/* 12. Add boolean item → stored correctly */
void test_AddItemToArray_BoolItem_StoredCorrectly(void)
{
    cJSON *bool_item = cJSON_CreateTrue();
    cJSON_bool result = cJSON_AddItemToArray(array, bool_item);
    TEST_ASSERT_TRUE(result);
    cJSON *retrieved = cJSON_GetArrayItem(array, 0);
    TEST_ASSERT_NOT_NULL(retrieved);
    TEST_ASSERT_TRUE(cJSON_IsTrue(retrieved));
}

/* 13. Add nested array → stored correctly */
void test_AddItemToArray_NestedArray_StoredCorrectly(void)
{
    cJSON *inner = cJSON_CreateArray();
    cJSON_AddItemToArray(inner, make_number(99.0));
    cJSON_bool result = cJSON_AddItemToArray(array, inner);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(1, cJSON_GetArraySize(array));
    cJSON *retrieved = cJSON_GetArrayItem(array, 0);
    TEST_ASSERT_NOT_NULL(retrieved);
    TEST_ASSERT_TRUE(cJSON_IsArray(retrieved));
    TEST_ASSERT_EQUAL_INT(1, cJSON_GetArraySize(retrieved));
}

/* 14. Add nested object → stored correctly */
void test_AddItemToArray_NestedObject_StoredCorrectly(void)
{
    cJSON *obj = cJSON_CreateObject();
    cJSON_AddItemToObject(obj, "key", make_number(1.0));
    cJSON_bool result = cJSON_AddItemToArray(array, obj);
    TEST_ASSERT_TRUE(result);
    cJSON *retrieved = cJSON_GetArrayItem(array, 0);
    TEST_ASSERT_NOT_NULL(retrieved);
    TEST_ASSERT_TRUE(cJSON_IsObject(retrieved));
}

/* 15. After adding two items, last item's next is NULL */
void test_AddItemToArray_TwoItems_LastNextIsNull(void)
{
    cJSON *first = make_number(1.0);
    cJSON *second = make_number(2.0);
    cJSON_AddItemToArray(array, first);
    cJSON_AddItemToArray(array, second);
    TEST_ASSERT_NULL(second->next);
}

/* 16. After adding two items, first->next == second */
void test_AddItemToArray_TwoItems_LinkedCorrectly(void)
{
    cJSON *first = make_number(1.0);
    cJSON *second = make_number(2.0);
    cJSON_AddItemToArray(array, first);
    cJSON_AddItemToArray(array, second);
    TEST_ASSERT_EQUAL_PTR(second, first->next);
}

/* 17. After adding two items, second->prev == first */
void test_AddItemToArray_TwoItems_PrevLinkedCorrectly(void)
{
    cJSON *first = make_number(1.0);
    cJSON *second = make_number(2.0);
    cJSON_AddItemToArray(array, first);
    cJSON_AddItemToArray(array, second);
    TEST_ASSERT_EQUAL_PTR(first, second->prev);
}

/* 18. child->prev points to last element after multiple inserts */
void test_AddItemToArray_MultipleItems_ChildPrevIsLast(void)
{
    cJSON *last = NULL;
    int i;
    for (i = 0; i < 4; i++)
    {
        last = make_number((double)i);
        cJSON_AddItemToArray(array, last);
    }
    TEST_ASSERT_EQUAL_PTR(last, array->child->prev);
}

/* 19. Adding to a non-array cJSON object (cJSON_Object) also succeeds */
void test_AddItemToArray_ObjectContainer_ReturnsTrue(void)
{
    cJSON *obj_container = cJSON_CreateObject();
    item = make_number(1.0);
    cJSON_bool result = cJSON_AddItemToArray(obj_container, item);
    TEST_ASSERT_TRUE(result);
    cJSON_Delete(obj_container);
    item = NULL;
}

/* 20. Large number of items → size is correct */
void test_AddItemToArray_LargeNumberOfItems_SizeCorrect(void)
{
    int count = 100;
    int i;
    for (i = 0; i < count; i++)
    {
        cJSON_bool result = cJSON_AddItemToArray(array, make_number((double)i));
        TEST_ASSERT_TRUE(result);
    }
    TEST_ASSERT_EQUAL_INT(count, cJSON_GetArraySize(array));
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
    RUN_TEST(test_AddItemToArray_SingleItem_PrevPointsToSelf);
    RUN_TEST(test_AddItemToArray_SingleItem_NextIsNull);
    RUN_TEST(test_AddItemToArray_MultipleItems_SizeGrows);
    RUN_TEST(test_AddItemToArray_MultipleItems_OrderPreserved);
    RUN_TEST(test_AddItemToArray_StringItem_StoredCorrectly);
    RUN_TEST(test_AddItemToArray_NullJsonItem_StoredCorrectly);
    RUN_TEST(test_AddItemToArray_BoolItem_StoredCorrectly);
    RUN_TEST(test_AddItemToArray_NestedArray_StoredCorrectly);
    RUN_TEST(test_AddItemToArray_NestedObject_StoredCorrectly);
    RUN_TEST(test_AddItemToArray_TwoItems_LastNextIsNull);
    RUN_TEST(test_AddItemToArray_TwoItems_LinkedCorrectly);
    RUN_TEST(test_AddItemToArray_TwoItems_PrevLinkedCorrectly);
    RUN_TEST(test_AddItemToArray_MultipleItems_ChildPrevIsLast);
    RUN_TEST(test_AddItemToArray_ObjectContainer_ReturnsTrue);
    RUN_TEST(test_AddItemToArray_LargeNumberOfItems_SizeCorrect);
    return UNITY_END();
}