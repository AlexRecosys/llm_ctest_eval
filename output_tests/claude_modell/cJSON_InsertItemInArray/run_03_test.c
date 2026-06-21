#include "cJSON.c"
#include "unity.h"
#include "cJSON.h"
#include <stdlib.h>
#include <string.h>

/* File-scope variables */
static cJSON *test_array = NULL;

/* Helper functions */
static cJSON *create_number_array(int count)
{
    cJSON *array = cJSON_CreateArray();
    int i;
    for (i = 0; i < count; i++)
    {
        cJSON_AddItemToArray(array, cJSON_CreateNumber((double)i));
    }
    return array;
}

static int get_array_item_value(cJSON *array, int index)
{
    cJSON *item = cJSON_GetArrayItem(array, index);
    if (item == NULL)
    {
        return -9999;
    }
    return (int)item->valuedouble;
}

void setUp(void)
{
    test_array = NULL;
}

void tearDown(void)
{
    if (test_array != NULL)
    {
        cJSON_Delete(test_array);
        test_array = NULL;
    }
}

/* Test: negative index returns false */
void test_InsertItemInArray_negative_index_returns_false(void)
{
    cJSON *newitem = cJSON_CreateNumber(99.0);
    test_array = cJSON_CreateArray();
    cJSON_AddItemToArray(test_array, cJSON_CreateNumber(0.0));

    cJSON_bool result = cJSON_InsertItemInArray(test_array, -1, newitem);

    TEST_ASSERT_FALSE(result);
    /* newitem was not inserted, must be freed manually */
    cJSON_Delete(newitem);
}

/* Test: NULL newitem returns false */
void test_InsertItemInArray_null_newitem_returns_false(void)
{
    test_array = cJSON_CreateArray();
    cJSON_AddItemToArray(test_array, cJSON_CreateNumber(0.0));

    cJSON_bool result = cJSON_InsertItemInArray(test_array, 0, NULL);

    TEST_ASSERT_FALSE(result);
}

/* Test: insert at index 0 in empty array (appends) */
void test_InsertItemInArray_empty_array_index_zero_appends(void)
{
    test_array = cJSON_CreateArray();
    cJSON *newitem = cJSON_CreateNumber(42.0);

    cJSON_bool result = cJSON_InsertItemInArray(test_array, 0, newitem);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(1, cJSON_GetArraySize(test_array));
    TEST_ASSERT_EQUAL_INT(42, get_array_item_value(test_array, 0));
}

/* Test: insert at index beyond array size appends to end */
void test_InsertItemInArray_index_beyond_size_appends(void)
{
    test_array = create_number_array(3); /* [0, 1, 2] */
    cJSON *newitem = cJSON_CreateNumber(99.0);

    cJSON_bool result = cJSON_InsertItemInArray(test_array, 10, newitem);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(4, cJSON_GetArraySize(test_array));
    TEST_ASSERT_EQUAL_INT(99, get_array_item_value(test_array, 3));
}

/* Test: insert at index 0 in non-empty array shifts existing items */
void test_InsertItemInArray_insert_at_index_zero_shifts_items(void)
{
    test_array = create_number_array(3); /* [0, 1, 2] */
    cJSON *newitem = cJSON_CreateNumber(99.0);

    cJSON_bool result = cJSON_InsertItemInArray(test_array, 0, newitem);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(4, cJSON_GetArraySize(test_array));
    TEST_ASSERT_EQUAL_INT(99, get_array_item_value(test_array, 0));
    TEST_ASSERT_EQUAL_INT(0, get_array_item_value(test_array, 1));
    TEST_ASSERT_EQUAL_INT(1, get_array_item_value(test_array, 2));
    TEST_ASSERT_EQUAL_INT(2, get_array_item_value(test_array, 3));
}

/* Test: insert at middle index */
void test_InsertItemInArray_insert_at_middle_index(void)
{
    test_array = create_number_array(4); /* [0, 1, 2, 3] */
    cJSON *newitem = cJSON_CreateNumber(99.0);

    cJSON_bool result = cJSON_InsertItemInArray(test_array, 2, newitem);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(5, cJSON_GetArraySize(test_array));
    TEST_ASSERT_EQUAL_INT(0, get_array_item_value(test_array, 0));
    TEST_ASSERT_EQUAL_INT(1, get_array_item_value(test_array, 1));
    TEST_ASSERT_EQUAL_INT(99, get_array_item_value(test_array, 2));
    TEST_ASSERT_EQUAL_INT(2, get_array_item_value(test_array, 3));
    TEST_ASSERT_EQUAL_INT(3, get_array_item_value(test_array, 4));
}

/* Test: insert at last valid index */
void test_InsertItemInArray_insert_at_last_valid_index(void)
{
    test_array = create_number_array(3); /* [0, 1, 2] */
    cJSON *newitem = cJSON_CreateNumber(99.0);

    cJSON_bool result = cJSON_InsertItemInArray(test_array, 2, newitem);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(4, cJSON_GetArraySize(test_array));
    TEST_ASSERT_EQUAL_INT(0, get_array_item_value(test_array, 0));
    TEST_ASSERT_EQUAL_INT(1, get_array_item_value(test_array, 1));
    TEST_ASSERT_EQUAL_INT(99, get_array_item_value(test_array, 2));
    TEST_ASSERT_EQUAL_INT(2, get_array_item_value(test_array, 3));
}

/* Test: insert at index 1 in two-element array */
void test_InsertItemInArray_insert_at_index_one_two_element_array(void)
{
    test_array = create_number_array(2); /* [0, 1] */
    cJSON *newitem = cJSON_CreateNumber(55.0);

    cJSON_bool result = cJSON_InsertItemInArray(test_array, 1, newitem);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(3, cJSON_GetArraySize(test_array));
    TEST_ASSERT_EQUAL_INT(0, get_array_item_value(test_array, 0));
    TEST_ASSERT_EQUAL_INT(55, get_array_item_value(test_array, 1));
    TEST_ASSERT_EQUAL_INT(1, get_array_item_value(test_array, 2));
}

/* Test: array child pointer updated when inserting at index 0 */
void test_InsertItemInArray_child_pointer_updated_on_insert_at_zero(void)
{
    test_array = create_number_array(2); /* [0, 1] */
    cJSON *newitem = cJSON_CreateNumber(77.0);

    cJSON_bool result = cJSON_InsertItemInArray(test_array, 0, newitem);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(test_array->child);
    TEST_ASSERT_EQUAL_PTR(newitem, test_array->child);
    TEST_ASSERT_EQUAL_INT(77, (int)test_array->child->valuedouble);
}

/* Test: prev pointer of first element is NULL after insert at 0 */
void test_InsertItemInArray_new_first_item_prev_is_null(void)
{
    test_array = create_number_array(2); /* [0, 1] */
    cJSON *newitem = cJSON_CreateNumber(77.0);

    cJSON_InsertItemInArray(test_array, 0, newitem);

    TEST_ASSERT_NULL(test_array->child->prev);
}

/* Test: next/prev linkage is correct after insert at middle */
void test_InsertItemInArray_linkage_correct_after_middle_insert(void)
{
    test_array = create_number_array(3); /* [0, 1, 2] */
    cJSON *newitem = cJSON_CreateNumber(99.0);

    cJSON_InsertItemInArray(test_array, 1, newitem);

    /* [0, 99, 1, 2] */
    cJSON *item0 = cJSON_GetArrayItem(test_array, 0);
    cJSON *item99 = cJSON_GetArrayItem(test_array, 1);
    cJSON *item1 = cJSON_GetArrayItem(test_array, 2);
    cJSON *item2 = cJSON_GetArrayItem(test_array, 3);

    TEST_ASSERT_NOT_NULL(item0);
    TEST_ASSERT_NOT_NULL(item99);
    TEST_ASSERT_NOT_NULL(item1);
    TEST_ASSERT_NOT_NULL(item2);

    TEST_ASSERT_EQUAL_PTR(item99, item0->next);
    TEST_ASSERT_EQUAL_PTR(item0, item99->prev);
    TEST_ASSERT_EQUAL_PTR(item1, item99->next);
    TEST_ASSERT_EQUAL_PTR(item99, item1->prev);
    TEST_ASSERT_EQUAL_PTR(item2, item1->next);
    TEST_ASSERT_EQUAL_PTR(item1, item2->prev);
}

/* Test: insert into single-element array at index 0 */
void test_InsertItemInArray_single_element_insert_at_zero(void)
{
    test_array = cJSON_CreateArray();
    cJSON_AddItemToArray(test_array, cJSON_CreateNumber(10.0));
    cJSON *newitem = cJSON_CreateNumber(5.0);

    cJSON_bool result = cJSON_InsertItemInArray(test_array, 0, newitem);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(2, cJSON_GetArraySize(test_array));
    TEST_ASSERT_EQUAL_INT(5, get_array_item_value(test_array, 0));
    TEST_ASSERT_EQUAL_INT(10, get_array_item_value(test_array, 1));
}

/* Test: insert into single-element array at index 1 (appends) */
void test_InsertItemInArray_single_element_insert_at_one_appends(void)
{
    test_array = cJSON_CreateArray();
    cJSON_AddItemToArray(test_array, cJSON_CreateNumber(10.0));
    cJSON *newitem = cJSON_CreateNumber(20.0);

    cJSON_bool result = cJSON_InsertItemInArray(test_array, 1, newitem);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(2, cJSON_GetArraySize(test_array));
    TEST_ASSERT_EQUAL_INT(10, get_array_item_value(test_array, 0));
    TEST_ASSERT_EQUAL_INT(20, get_array_item_value(test_array, 1));
}

/* Test: return value is true on successful insert at index 0 */
void test_InsertItemInArray_returns_true_on_success(void)
{
    test_array = create_number_array(3);
    cJSON *newitem = cJSON_CreateNumber(100.0);

    cJSON_bool result = cJSON_InsertItemInArray(test_array, 0, newitem);

    TEST_ASSERT_TRUE(result);
}

/* Test: array size increases by one after successful insert */
void test_InsertItemInArray_size_increases_by_one(void)
{
    test_array = create_number_array(5);
    int original_size = cJSON_GetArraySize(test_array);
    cJSON *newitem = cJSON_CreateNumber(100.0);

    cJSON_InsertItemInArray(test_array, 2, newitem);

    TEST_ASSERT_EQUAL_INT(original_size + 1, cJSON_GetArraySize(test_array));
}

/* Test: insert string item at index 0 */
void test_InsertItemInArray_insert_string_item_at_zero(void)
{
    test_array = cJSON_CreateArray();
    cJSON_AddItemToArray(test_array, cJSON_CreateString("world"));
    cJSON *newitem = cJSON_CreateString("hello");

    cJSON_bool result = cJSON_InsertItemInArray(test_array, 0, newitem);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(2, cJSON_GetArraySize(test_array));
    cJSON *first = cJSON_GetArrayItem(test_array, 0);
    cJSON *second = cJSON_GetArrayItem(test_array, 1);
    TEST_ASSERT_NOT_NULL(first);
    TEST_ASSERT_NOT_NULL(second);
    TEST_ASSERT_EQUAL_STRING("hello", first->valuestring);
    TEST_ASSERT_EQUAL_STRING("world", second->valuestring);
}

/* Test: multiple inserts at index 0 maintain correct order */
void test_InsertItemInArray_multiple_inserts_at_zero_maintain_order(void)
{
    test_array = cJSON_CreateArray();
    cJSON_AddItemToArray(test_array, cJSON_CreateNumber(3.0));

    cJSON_InsertItemInArray(test_array, 0, cJSON_CreateNumber(2.0));
    cJSON_InsertItemInArray(test_array, 0, cJSON_CreateNumber(1.0));
    cJSON_InsertItemInArray(test_array, 0, cJSON_CreateNumber(0.0));

    TEST_ASSERT_EQUAL_INT(4, cJSON_GetArraySize(test_array));
    TEST_ASSERT_EQUAL_INT(0, get_array_item_value(test_array, 0));
    TEST_ASSERT_EQUAL_INT(1, get_array_item_value(test_array, 1));
    TEST_ASSERT_EQUAL_INT(2, get_array_item_value(test_array, 2));
    TEST_ASSERT_EQUAL_INT(3, get_array_item_value(test_array, 3));
}

/* Test: insert at index equal to array size appends */
void test_InsertItemInArray_insert_at_exact_size_appends(void)
{
    test_array = create_number_array(3); /* [0, 1, 2], size=3 */
    cJSON *newitem = cJSON_CreateNumber(99.0);

    cJSON_bool result = cJSON_InsertItemInArray(test_array, 3, newitem);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(4, cJSON_GetArraySize(test_array));
    TEST_ASSERT_EQUAL_INT(99, get_array_item_value(test_array, 3));
}

/* Test: corrupted array item (prev is NULL but not child) returns false */
void test_InsertItemInArray_corrupted_item_returns_false(void)
{
    test_array = create_number_array(3); /* [0, 1, 2] */
    cJSON *newitem = cJSON_CreateNumber(99.0);

    /* Corrupt the second item by setting its prev to NULL */
    cJSON *second = cJSON_GetArrayItem(test_array, 1);
    TEST_ASSERT_NOT_NULL(second);
    second->prev = NULL;

    cJSON_bool result = cJSON_InsertItemInArray(test_array, 1, newitem);

    TEST_ASSERT_FALSE(result);
    /* newitem was not inserted, must be freed manually */
    cJSON_Delete(newitem);
    /* Restore linkage to allow proper cleanup */
    cJSON *first = cJSON_GetArrayItem(test_array, 0);
    if (first != NULL)
    {
        second->prev = first;
    }
}

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_InsertItemInArray_negative_index_returns_false);
    RUN_TEST(test_InsertItemInArray_null_newitem_returns_false);
    RUN_TEST(test_InsertItemInArray_empty_array_index_zero_appends);
    RUN_TEST(test_InsertItemInArray_index_beyond_size_appends);
    RUN_TEST(test_InsertItemInArray_insert_at_index_zero_shifts_items);
    RUN_TEST(test_InsertItemInArray_insert_at_middle_index);
    RUN_TEST(test_InsertItemInArray_insert_at_last_valid_index);
    RUN_TEST(test_InsertItemInArray_insert_at_index_one_two_element_array);
    RUN_TEST(test_InsertItemInArray_child_pointer_updated_on_insert_at_zero);
    RUN_TEST(test_InsertItemInArray_new_first_item_prev_is_null);
    RUN_TEST(test_InsertItemInArray_linkage_correct_after_middle_insert);
    RUN_TEST(test_InsertItemInArray_single_element_insert_at_zero);
    RUN_TEST(test_InsertItemInArray_single_element_insert_at_one_appends);
    RUN_TEST(test_InsertItemInArray_returns_true_on_success);
    RUN_TEST(test_InsertItemInArray_size_increases_by_one);
    RUN_TEST(test_InsertItemInArray_insert_string_item_at_zero);
    RUN_TEST(test_InsertItemInArray_multiple_inserts_at_zero_maintain_order);
    RUN_TEST(test_InsertItemInArray_insert_at_exact_size_appends);
    RUN_TEST(test_InsertItemInArray_corrupted_item_returns_false);
    return UNITY_END();
}