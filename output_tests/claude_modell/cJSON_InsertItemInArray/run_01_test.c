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
void test_insert_negative_index_returns_false(void)
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
void test_insert_null_newitem_returns_false(void)
{
    test_array = cJSON_CreateArray();
    cJSON_AddItemToArray(test_array, cJSON_CreateNumber(0.0));

    cJSON_bool result = cJSON_InsertItemInArray(test_array, 0, NULL);
    TEST_ASSERT_FALSE(result);
}

/* Test: insert at index 0 in empty array appends item */
void test_insert_at_zero_in_empty_array(void)
{
    test_array = cJSON_CreateArray();
    cJSON *newitem = cJSON_CreateNumber(42.0);

    cJSON_bool result = cJSON_InsertItemInArray(test_array, 0, newitem);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(1, cJSON_GetArraySize(test_array));
    TEST_ASSERT_EQUAL_INT(42, get_array_item_value(test_array, 0));
}

/* Test: insert beyond array size appends item */
void test_insert_beyond_array_size_appends(void)
{
    test_array = create_number_array(3); /* [0, 1, 2] */
    cJSON *newitem = cJSON_CreateNumber(99.0);

    cJSON_bool result = cJSON_InsertItemInArray(test_array, 10, newitem);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(4, cJSON_GetArraySize(test_array));
    TEST_ASSERT_EQUAL_INT(99, get_array_item_value(test_array, 3));
}

/* Test: insert at index 0 shifts existing items right */
void test_insert_at_index_zero_shifts_items(void)
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
void test_insert_at_middle_index(void)
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
void test_insert_at_last_valid_index(void)
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

/* Test: array child pointer updated when inserting at index 0 */
void test_insert_at_zero_updates_child_pointer(void)
{
    test_array = create_number_array(2); /* [0, 1] */
    cJSON *newitem = cJSON_CreateNumber(55.0);

    cJSON_bool result = cJSON_InsertItemInArray(test_array, 0, newitem);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(test_array->child);
    TEST_ASSERT_EQUAL_PTR(newitem, test_array->child);
}

/* Test: inserted item's prev pointer is NULL when inserted at index 0 */
void test_insert_at_zero_newitem_prev_is_null(void)
{
    test_array = create_number_array(2); /* [0, 1] */
    cJSON *newitem = cJSON_CreateNumber(55.0);

    cJSON_bool result = cJSON_InsertItemInArray(test_array, 0, newitem);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NULL(newitem->prev);
}

/* Test: inserted item's next pointer is correct when inserted at index 0 */
void test_insert_at_zero_newitem_next_is_old_first(void)
{
    test_array = create_number_array(2); /* [0, 1] */
    cJSON *old_first = test_array->child;
    cJSON *newitem = cJSON_CreateNumber(55.0);

    cJSON_bool result = cJSON_InsertItemInArray(test_array, 0, newitem);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_PTR(old_first, newitem->next);
}

/* Test: after_inserted->prev updated to newitem when inserting at index 0 */
void test_insert_at_zero_after_inserted_prev_is_newitem(void)
{
    test_array = create_number_array(2); /* [0, 1] */
    cJSON *old_first = test_array->child;
    cJSON *newitem = cJSON_CreateNumber(55.0);

    cJSON_bool result = cJSON_InsertItemInArray(test_array, 0, newitem);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_PTR(newitem, old_first->prev);
}

/* Test: insert at middle - prev/next linkage is correct */
void test_insert_at_middle_linkage_correct(void)
{
    test_array = create_number_array(3); /* [0, 1, 2] */
    cJSON *item_at_1 = cJSON_GetArrayItem(test_array, 1);
    cJSON *item_at_0 = cJSON_GetArrayItem(test_array, 0);
    cJSON *newitem = cJSON_CreateNumber(77.0);

    cJSON_bool result = cJSON_InsertItemInArray(test_array, 1, newitem);
    TEST_ASSERT_TRUE(result);

    /* newitem->prev should be item_at_0 */
    TEST_ASSERT_EQUAL_PTR(item_at_0, newitem->prev);
    /* newitem->next should be item_at_1 */
    TEST_ASSERT_EQUAL_PTR(item_at_1, newitem->next);
    /* item_at_0->next should be newitem */
    TEST_ASSERT_EQUAL_PTR(newitem, item_at_0->next);
    /* item_at_1->prev should be newitem */
    TEST_ASSERT_EQUAL_PTR(newitem, item_at_1->prev);
}

/* Test: insert at index equal to array size appends */
void test_insert_at_exact_size_appends(void)
{
    test_array = create_number_array(3); /* [0, 1, 2] */
    cJSON *newitem = cJSON_CreateNumber(88.0);

    cJSON_bool result = cJSON_InsertItemInArray(test_array, 3, newitem);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(4, cJSON_GetArraySize(test_array));
    TEST_ASSERT_EQUAL_INT(88, get_array_item_value(test_array, 3));
}

/* Test: insert into single-element array at index 0 */
void test_insert_at_zero_single_element_array(void)
{
    test_array = cJSON_CreateArray();
    cJSON_AddItemToArray(test_array, cJSON_CreateNumber(10.0));
    cJSON *newitem = cJSON_CreateNumber(20.0);

    cJSON_bool result = cJSON_InsertItemInArray(test_array, 0, newitem);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(2, cJSON_GetArraySize(test_array));
    TEST_ASSERT_EQUAL_INT(20, get_array_item_value(test_array, 0));
    TEST_ASSERT_EQUAL_INT(10, get_array_item_value(test_array, 1));
}

/* Test: insert into single-element array at index 1 appends */
void test_insert_at_one_single_element_array_appends(void)
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

/* Test: multiple inserts at index 0 maintain correct order */
void test_multiple_inserts_at_zero(void)
{
    test_array = cJSON_CreateArray();
    cJSON *item_a = cJSON_CreateNumber(1.0);
    cJSON *item_b = cJSON_CreateNumber(2.0);
    cJSON *item_c = cJSON_CreateNumber(3.0);

    cJSON_InsertItemInArray(test_array, 0, item_a); /* [1] */
    cJSON_InsertItemInArray(test_array, 0, item_b); /* [2, 1] */
    cJSON_InsertItemInArray(test_array, 0, item_c); /* [3, 2, 1] */

    TEST_ASSERT_EQUAL_INT(3, cJSON_GetArraySize(test_array));
    TEST_ASSERT_EQUAL_INT(3, get_array_item_value(test_array, 0));
    TEST_ASSERT_EQUAL_INT(2, get_array_item_value(test_array, 1));
    TEST_ASSERT_EQUAL_INT(1, get_array_item_value(test_array, 2));
}

/* Test: insert returns true on success at index 0 */
void test_insert_returns_true_on_success(void)
{
    test_array = create_number_array(3);
    cJSON *newitem = cJSON_CreateNumber(5.0);

    cJSON_bool result = cJSON_InsertItemInArray(test_array, 0, newitem);
    TEST_ASSERT_TRUE(result);
}

/* Test: array size increases by one after successful insert */
void test_array_size_increases_after_insert(void)
{
    test_array = create_number_array(5);
    int size_before = cJSON_GetArraySize(test_array);
    cJSON *newitem = cJSON_CreateNumber(100.0);

    cJSON_InsertItemInArray(test_array, 2, newitem);
    int size_after = cJSON_GetArraySize(test_array);

    TEST_ASSERT_EQUAL_INT(size_before + 1, size_after);
}

/* Test: insert at index 0 with large array */
void test_insert_at_zero_large_array(void)
{
    test_array = create_number_array(10); /* [0..9] */
    cJSON *newitem = cJSON_CreateNumber(999.0);

    cJSON_bool result = cJSON_InsertItemInArray(test_array, 0, newitem);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(11, cJSON_GetArraySize(test_array));
    TEST_ASSERT_EQUAL_INT(999, get_array_item_value(test_array, 0));
    TEST_ASSERT_EQUAL_INT(0, get_array_item_value(test_array, 1));
    TEST_ASSERT_EQUAL_INT(9, get_array_item_value(test_array, 10));
}

/* Test: insert at middle of large array */
void test_insert_at_middle_large_array(void)
{
    test_array = create_number_array(10); /* [0..9] */
    cJSON *newitem = cJSON_CreateNumber(999.0);

    cJSON_bool result = cJSON_InsertItemInArray(test_array, 5, newitem);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(11, cJSON_GetArraySize(test_array));
    TEST_ASSERT_EQUAL_INT(4, get_array_item_value(test_array, 4));
    TEST_ASSERT_EQUAL_INT(999, get_array_item_value(test_array, 5));
    TEST_ASSERT_EQUAL_INT(5, get_array_item_value(test_array, 6));
}

/* Test: insert with index -100 returns false */
void test_insert_very_negative_index_returns_false(void)
{
    test_array = create_number_array(3);
    cJSON *newitem = cJSON_CreateNumber(1.0);

    cJSON_bool result = cJSON_InsertItemInArray(test_array, -100, newitem);
    TEST_ASSERT_FALSE(result);

    cJSON_Delete(newitem);
}

/* Test: insert into empty array at index 0 sets child */
void test_insert_empty_array_sets_child(void)
{
    test_array = cJSON_CreateArray();
    TEST_ASSERT_NULL(test_array->child);

    cJSON *newitem = cJSON_CreateNumber(7.0);
    cJSON_bool result = cJSON_InsertItemInArray(test_array, 0, newitem);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(test_array->child);
}

/* Test: insert at index 1 in two-element array */
void test_insert_at_index_one_two_element_array(void)
{
    test_array = create_number_array(2); /* [0, 1] */
    cJSON *newitem = cJSON_CreateNumber(50.0);

    cJSON_bool result = cJSON_InsertItemInArray(test_array, 1, newitem);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(3, cJSON_GetArraySize(test_array));
    TEST_ASSERT_EQUAL_INT(0, get_array_item_value(test_array, 0));
    TEST_ASSERT_EQUAL_INT(50, get_array_item_value(test_array, 1));
    TEST_ASSERT_EQUAL_INT(1, get_array_item_value(test_array, 2));
}

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_insert_negative_index_returns_false);
    RUN_TEST(test_insert_null_newitem_returns_false);
    RUN_TEST(test_insert_at_zero_in_empty_array);
    RUN_TEST(test_insert_beyond_array_size_appends);
    RUN_TEST(test_insert_at_index_zero_shifts_items);
    RUN_TEST(test_insert_at_middle_index);
    RUN_TEST(test_insert_at_last_valid_index);
    RUN_TEST(test_insert_at_zero_updates_child_pointer);
    RUN_TEST(test_insert_at_zero_newitem_prev_is_null);
    RUN_TEST(test_insert_at_zero_newitem_next_is_old_first);
    RUN_TEST(test_insert_at_zero_after_inserted_prev_is_newitem);
    RUN_TEST(test_insert_at_middle_linkage_correct);
    RUN_TEST(test_insert_at_exact_size_appends);
    RUN_TEST(test_insert_at_zero_single_element_array);
    RUN_TEST(test_insert_at_one_single_element_array_appends);
    RUN_TEST(test_multiple_inserts_at_zero);
    RUN_TEST(test_insert_returns_true_on_success);
    RUN_TEST(test_array_size_increases_after_insert);
    RUN_TEST(test_insert_at_zero_large_array);
    RUN_TEST(test_insert_at_middle_large_array);
    RUN_TEST(test_insert_very_negative_index_returns_false);
    RUN_TEST(test_insert_empty_array_sets_child);
    RUN_TEST(test_insert_at_index_one_two_element_array);
    return UNITY_END();
}