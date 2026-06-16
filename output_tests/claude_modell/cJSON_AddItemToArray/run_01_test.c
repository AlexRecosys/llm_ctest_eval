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

/* 1. Adding a single item to an empty array succeeds */
void test_AddItemToArray_single_item_returns_true(void)
{
    item = make_number(1.0);
    TEST_ASSERT_NOT_NULL(item);

    cJSON_bool result = cJSON_AddItemToArray(array, item);
    TEST_ASSERT_TRUE(result);
}

/* 2. After adding one item the array size is 1 */
void test_AddItemToArray_single_item_size_is_one(void)
{
    item = make_number(42.0);
    cJSON_AddItemToArray(array, item);

    TEST_ASSERT_EQUAL_INT(1, cJSON_GetArraySize(array));
}

/* 3. The added item can be retrieved at index 0 */
void test_AddItemToArray_item_retrievable_at_index_zero(void)
{
    item = make_number(7.0);
    cJSON_AddItemToArray(array, item);

    cJSON *retrieved = cJSON_GetArrayItem(array, 0);
    TEST_ASSERT_NOT_NULL(retrieved);
    TEST_ASSERT_EQUAL_DOUBLE(7.0, retrieved->valuedouble);
}

/* 4. Adding multiple items increases the size correctly */
void test_AddItemToArray_multiple_items_correct_size(void)
{
    int i;
    for (i = 0; i < 5; i++)
    {
        cJSON *n = make_number((double)i);
        TEST_ASSERT_NOT_NULL(n);
        cJSON_bool r = cJSON_AddItemToArray(array, n);
        TEST_ASSERT_TRUE(r);
    }
    TEST_ASSERT_EQUAL_INT(5, cJSON_GetArraySize(array));
}

/* 5. Items are appended in order */
void test_AddItemToArray_items_in_order(void)
{
    int i;
    for (i = 0; i < 3; i++)
    {
        cJSON *n = make_number((double)i);
        cJSON_AddItemToArray(array, n);
    }

    for (i = 0; i < 3; i++)
    {
        cJSON *retrieved = cJSON_GetArrayItem(array, i);
        TEST_ASSERT_NOT_NULL(retrieved);
        TEST_ASSERT_EQUAL_DOUBLE((double)i, retrieved->valuedouble);
    }
}

/* 6. Passing NULL as array returns false */
void test_AddItemToArray_null_array_returns_false(void)
{
    item = make_number(1.0);
    TEST_ASSERT_NOT_NULL(item);

    cJSON_bool result = cJSON_AddItemToArray(NULL, item);
    TEST_ASSERT_FALSE(result);

    /* item was not consumed; free it manually */
    cJSON_Delete(item);
    item = NULL;
}

/* 7. Passing NULL as item returns false */
void test_AddItemToArray_null_item_returns_false(void)
{
    cJSON_bool result = cJSON_AddItemToArray(array, NULL);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL_INT(0, cJSON_GetArraySize(array));
}

/* 8. Both NULL returns false */
void test_AddItemToArray_both_null_returns_false(void)
{
    cJSON_bool result = cJSON_AddItemToArray(NULL, NULL);
    TEST_ASSERT_FALSE(result);
}

/* 9. Adding a string item works */
void test_AddItemToArray_string_item(void)
{
    item = cJSON_CreateString("hello");
    TEST_ASSERT_NOT_NULL(item);

    cJSON_bool result = cJSON_AddItemToArray(array, item);
    TEST_ASSERT_TRUE(result);

    cJSON *retrieved = cJSON_GetArrayItem(array, 0);
    TEST_ASSERT_NOT_NULL(retrieved);
    TEST_ASSERT_EQUAL_STRING("hello", retrieved->valuestring);
}

/* 10. Adding a NULL JSON value item works */
void test_AddItemToArray_null_json_item(void)
{
    item = cJSON_CreateNull();
    TEST_ASSERT_NOT_NULL(item);

    cJSON_bool result = cJSON_AddItemToArray(array, item);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(1, cJSON_GetArraySize(array));

    cJSON *retrieved = cJSON_GetArrayItem(array, 0);
    TEST_ASSERT_NOT_NULL(retrieved);
    TEST_ASSERT_TRUE(cJSON_IsNull(retrieved));
}

/* 11. Adding a boolean item works */
void test_AddItemToArray_bool_item(void)
{
    item = cJSON_CreateTrue();
    TEST_ASSERT_NOT_NULL(item);

    cJSON_bool result = cJSON_AddItemToArray(array, item);
    TEST_ASSERT_TRUE(result);

    cJSON *retrieved = cJSON_GetArrayItem(array, 0);
    TEST_ASSERT_NOT_NULL(retrieved);
    TEST_ASSERT_TRUE(cJSON_IsTrue(retrieved));
}

/* 12. Adding a nested array item works */
void test_AddItemToArray_nested_array(void)
{
    cJSON *inner = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(inner);

    cJSON *n = make_number(99.0);
    cJSON_AddItemToArray(inner, n);

    cJSON_bool result = cJSON_AddItemToArray(array, inner);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(1, cJSON_GetArraySize(array));

    cJSON *retrieved = cJSON_GetArrayItem(array, 0);
    TEST_ASSERT_NOT_NULL(retrieved);
    TEST_ASSERT_TRUE(cJSON_IsArray(retrieved));
    TEST_ASSERT_EQUAL_INT(1, cJSON_GetArraySize(retrieved));
}

/* 13. Adding a nested object item works */
void test_AddItemToArray_nested_object(void)
{
    cJSON *obj = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(obj);

    cJSON_bool result = cJSON_AddItemToArray(array, obj);
    TEST_ASSERT_TRUE(result);

    cJSON *retrieved = cJSON_GetArrayItem(array, 0);
    TEST_ASSERT_NOT_NULL(retrieved);
    TEST_ASSERT_TRUE(cJSON_IsObject(retrieved));
}

/* 14. Linked list prev pointer of second item points to first */
void test_AddItemToArray_prev_pointer_correct(void)
{
    cJSON *first = make_number(1.0);
    cJSON *second = make_number(2.0);

    cJSON_AddItemToArray(array, first);
    cJSON_AddItemToArray(array, second);

    cJSON *child = array->child;
    TEST_ASSERT_NOT_NULL(child);
    TEST_ASSERT_NOT_NULL(child->next);
    TEST_ASSERT_EQUAL_PTR(child, child->next->prev);
}

/* 15. Linked list next pointer of last item is NULL */
void test_AddItemToArray_last_item_next_is_null(void)
{
    cJSON *first = make_number(10.0);
    cJSON *second = make_number(20.0);

    cJSON_AddItemToArray(array, first);
    cJSON_AddItemToArray(array, second);

    cJSON *child = array->child;
    TEST_ASSERT_NOT_NULL(child);
    TEST_ASSERT_NOT_NULL(child->next);
    TEST_ASSERT_NULL(child->next->next);
}

/* 16. Adding to a non-array (object) also succeeds via the same function */
void test_AddItemToArray_works_on_object_type(void)
{
    cJSON *obj = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(obj);

    cJSON *n = make_number(5.0);
    TEST_ASSERT_NOT_NULL(n);

    cJSON_bool result = cJSON_AddItemToArray(obj, n);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(1, cJSON_GetArraySize(obj));

    cJSON_Delete(obj);
}

/* 17. Large number of items can be added */
void test_AddItemToArray_large_number_of_items(void)
{
    int count = 100;
    int i;
    for (i = 0; i < count; i++)
    {
        cJSON *n = make_number((double)i);
        TEST_ASSERT_NOT_NULL(n);
        cJSON_bool r = cJSON_AddItemToArray(array, n);
        TEST_ASSERT_TRUE(r);
    }
    TEST_ASSERT_EQUAL_INT(count, cJSON_GetArraySize(array));

    for (i = 0; i < count; i++)
    {
        cJSON *retrieved = cJSON_GetArrayItem(array, i);
        TEST_ASSERT_NOT_NULL(retrieved);
        TEST_ASSERT_EQUAL_DOUBLE((double)i, retrieved->valuedouble);
    }
}

/* 18. First child pointer of array is set after first add */
void test_AddItemToArray_child_pointer_set_after_first_add(void)
{
    TEST_ASSERT_NULL(array->child);

    item = make_number(3.0);
    cJSON_AddItemToArray(array, item);

    TEST_ASSERT_NOT_NULL(array->child);
    TEST_ASSERT_EQUAL_PTR(item, array->child);
}

/* 19. Adding a raw JSON item works */
void test_AddItemToArray_raw_item(void)
{
    item = cJSON_CreateRaw("{\"key\":1}");
    TEST_ASSERT_NOT_NULL(item);

    cJSON_bool result = cJSON_AddItemToArray(array, item);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(1, cJSON_GetArraySize(array));

    cJSON *retrieved = cJSON_GetArrayItem(array, 0);
    TEST_ASSERT_NOT_NULL(retrieved);
    TEST_ASSERT_TRUE(cJSON_IsRaw(retrieved));
}

/* 20. Circular prev pointer: last item's prev points to second-to-last */
void test_AddItemToArray_three_items_prev_chain(void)
{
    cJSON *a = make_number(1.0);
    cJSON *b = make_number(2.0);
    cJSON *c = make_number(3.0);

    cJSON_AddItemToArray(array, a);
    cJSON_AddItemToArray(array, b);
    cJSON_AddItemToArray(array, c);

    /* Walk forward */
    cJSON *child = array->child;
    TEST_ASSERT_EQUAL_PTR(a, child);
    TEST_ASSERT_EQUAL_PTR(b, child->next);
    TEST_ASSERT_EQUAL_PTR(c, child->next->next);
    TEST_ASSERT_NULL(child->next->next->next);

    /* Walk backward via prev */
    TEST_ASSERT_EQUAL_PTR(b, c->prev);
    TEST_ASSERT_EQUAL_PTR(a, b->prev);
}

/* =========================================================================
 * main
 * ========================================================================= */
int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_AddItemToArray_single_item_returns_true);
    RUN_TEST(test_AddItemToArray_single_item_size_is_one);
    RUN_TEST(test_AddItemToArray_item_retrievable_at_index_zero);
    RUN_TEST(test_AddItemToArray_multiple_items_correct_size);
    RUN_TEST(test_AddItemToArray_items_in_order);
    RUN_TEST(test_AddItemToArray_null_array_returns_false);
    RUN_TEST(test_AddItemToArray_null_item_returns_false);
    RUN_TEST(test_AddItemToArray_both_null_returns_false);
    RUN_TEST(test_AddItemToArray_string_item);
    RUN_TEST(test_AddItemToArray_null_json_item);
    RUN_TEST(test_AddItemToArray_bool_item);
    RUN_TEST(test_AddItemToArray_nested_array);
    RUN_TEST(test_AddItemToArray_nested_object);
    RUN_TEST(test_AddItemToArray_prev_pointer_correct);
    RUN_TEST(test_AddItemToArray_last_item_next_is_null);
    RUN_TEST(test_AddItemToArray_works_on_object_type);
    RUN_TEST(test_AddItemToArray_large_number_of_items);
    RUN_TEST(test_AddItemToArray_child_pointer_set_after_first_add);
    RUN_TEST(test_AddItemToArray_raw_item);
    RUN_TEST(test_AddItemToArray_three_items_prev_chain);
    return UNITY_END();
}