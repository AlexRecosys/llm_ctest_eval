#include "cJSON.c"
#include "unity.h"
#include "cJSON.h"
#include <stddef.h>
#include <stdlib.h>

/* File-scope variables */
static cJSON *array = NULL;

/* Helper functions */
static void build_array_with_items(int count)
{
    int i;
    array = cJSON_CreateArray();
    for (i = 0; i < count; i++)
    {
        cJSON_AddItemToArray(array, cJSON_CreateNumber((double)i));
    }
}

static void verify_array_values(double *expected, int count)
{
    int i;
    cJSON *item = NULL;
    TEST_ASSERT_EQUAL_INT(count, cJSON_GetArraySize(array));
    for (i = 0; i < count; i++)
    {
        item = cJSON_GetArrayItem(array, i);
        TEST_ASSERT_NOT_NULL(item);
        TEST_ASSERT_EQUAL_DOUBLE(expected[i], item->valuedouble);
    }
}

/* setUp and tearDown */
void setUp(void)
{
    array = NULL;
}

void tearDown(void)
{
    if (array != NULL)
    {
        cJSON_Delete(array);
        array = NULL;
    }
}

/* Test cases */

/* Test: negative index returns false */
void test_insert_negative_index_returns_false(void)
{
    cJSON *newitem = NULL;
    cJSON_bool result;

    build_array_with_items(3);
    newitem = cJSON_CreateNumber(99.0);
    result = cJSON_InsertItemInArray(array, -1, newitem);
    TEST_ASSERT_FALSE(result);
    /* newitem was not inserted, must be freed manually */
    cJSON_Delete(newitem);
}

/* Test: NULL newitem returns false */
void test_insert_null_newitem_returns_false(void)
{
    cJSON_bool result;

    build_array_with_items(3);
    result = cJSON_InsertItemInArray(array, 0, NULL);
    TEST_ASSERT_FALSE(result);
}

/* Test: insert at index 0 in empty array (appends) */
void test_insert_at_index_0_in_empty_array(void)
{
    cJSON *newitem = NULL;
    cJSON_bool result;
    double expected[] = {42.0};

    array = cJSON_CreateArray();
    newitem = cJSON_CreateNumber(42.0);
    result = cJSON_InsertItemInArray(array, 0, newitem);
    TEST_ASSERT_TRUE(result);
    verify_array_values(expected, 1);
}

/* Test: insert at index beyond array size appends to end */
void test_insert_beyond_array_size_appends(void)
{
    cJSON *newitem = NULL;
    cJSON_bool result;
    double expected[] = {0.0, 1.0, 2.0, 99.0};

    build_array_with_items(3);
    newitem = cJSON_CreateNumber(99.0);
    result = cJSON_InsertItemInArray(array, 10, newitem);
    TEST_ASSERT_TRUE(result);
    verify_array_values(expected, 4);
}

/* Test: insert at index 0 in non-empty array shifts existing items */
void test_insert_at_index_0_shifts_items(void)
{
    cJSON *newitem = NULL;
    cJSON_bool result;
    double expected[] = {99.0, 0.0, 1.0, 2.0};

    build_array_with_items(3);
    newitem = cJSON_CreateNumber(99.0);
    result = cJSON_InsertItemInArray(array, 0, newitem);
    TEST_ASSERT_TRUE(result);
    verify_array_values(expected, 4);
}

/* Test: insert at index 0 updates array->child */
void test_insert_at_index_0_updates_child_pointer(void)
{
    cJSON *newitem = NULL;
    cJSON_bool result;

    build_array_with_items(3);
    newitem = cJSON_CreateNumber(99.0);
    result = cJSON_InsertItemInArray(array, 0, newitem);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_PTR(newitem, array->child);
}

/* Test: insert at middle index */
void test_insert_at_middle_index(void)
{
    cJSON *newitem = NULL;
    cJSON_bool result;
    double expected[] = {0.0, 1.0, 99.0, 2.0, 3.0, 4.0};

    build_array_with_items(5);
    newitem = cJSON_CreateNumber(99.0);
    result = cJSON_InsertItemInArray(array, 2, newitem);
    TEST_ASSERT_TRUE(result);
    verify_array_values(expected, 6);
}

/* Test: insert at last valid index */
void test_insert_at_last_valid_index(void)
{
    cJSON *newitem = NULL;
    cJSON_bool result;
    double expected[] = {0.0, 1.0, 99.0, 2.0};

    build_array_with_items(3);
    newitem = cJSON_CreateNumber(99.0);
    result = cJSON_InsertItemInArray(array, 2, newitem);
    TEST_ASSERT_TRUE(result);
    verify_array_values(expected, 4);
}

/* Test: inserted item's next pointer is correct */
void test_insert_sets_next_pointer_correctly(void)
{
    cJSON *newitem = NULL;
    cJSON *original_first = NULL;
    cJSON_bool result;

    build_array_with_items(3);
    original_first = array->child;
    newitem = cJSON_CreateNumber(99.0);
    result = cJSON_InsertItemInArray(array, 0, newitem);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_PTR(original_first, newitem->next);
}

/* Test: inserted item's prev pointer is correct when inserted at index 0 */
void test_insert_at_index_0_prev_is_null(void)
{
    cJSON *newitem = NULL;
    cJSON_bool result;

    build_array_with_items(3);
    newitem = cJSON_CreateNumber(99.0);
    result = cJSON_InsertItemInArray(array, 0, newitem);
    TEST_ASSERT_TRUE(result);
    /* The new child's prev should point to the last element (circular) or NULL depending on implementation */
    /* In cJSON, array->child->prev points to the last element */
    TEST_ASSERT_EQUAL_PTR(newitem, array->child);
}

/* Test: inserted item at middle has correct prev pointer */
void test_insert_at_middle_prev_pointer(void)
{
    cJSON *newitem = NULL;
    cJSON *item_at_1 = NULL;
    cJSON_bool result;

    build_array_with_items(3);
    item_at_1 = cJSON_GetArrayItem(array, 1);
    newitem = cJSON_CreateNumber(99.0);
    result = cJSON_InsertItemInArray(array, 2, newitem);
    TEST_ASSERT_TRUE(result);
    /* newitem->prev should be item_at_1 */
    TEST_ASSERT_EQUAL_PTR(item_at_1, newitem->prev);
}

/* Test: inserted item at middle has correct next pointer */
void test_insert_at_middle_next_pointer(void)
{
    cJSON *newitem = NULL;
    cJSON *item_at_2 = NULL;
    cJSON_bool result;

    build_array_with_items(3);
    item_at_2 = cJSON_GetArrayItem(array, 2);
    newitem = cJSON_CreateNumber(99.0);
    result = cJSON_InsertItemInArray(array, 2, newitem);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_PTR(item_at_2, newitem->next);
}

/* Test: array size increases by 1 after insert */
void test_insert_increases_array_size(void)
{
    cJSON *newitem = NULL;
    cJSON_bool result;
    int size_before;
    int size_after;

    build_array_with_items(5);
    size_before = cJSON_GetArraySize(array);
    newitem = cJSON_CreateNumber(99.0);
    result = cJSON_InsertItemInArray(array, 2, newitem);
    TEST_ASSERT_TRUE(result);
    size_after = cJSON_GetArraySize(array);
    TEST_ASSERT_EQUAL_INT(size_before + 1, size_after);
}

/* Test: insert at index 1 in single-element array */
void test_insert_at_index_1_in_single_element_array_appends(void)
{
    cJSON *newitem = NULL;
    cJSON_bool result;
    double expected[] = {0.0, 99.0};

    build_array_with_items(1);
    newitem = cJSON_CreateNumber(99.0);
    result = cJSON_InsertItemInArray(array, 1, newitem);
    TEST_ASSERT_TRUE(result);
    verify_array_values(expected, 2);
}

/* Test: insert at index 0 in single-element array */
void test_insert_at_index_0_in_single_element_array(void)
{
    cJSON *newitem = NULL;
    cJSON_bool result;
    double expected[] = {99.0, 0.0};

    build_array_with_items(1);
    newitem = cJSON_CreateNumber(99.0);
    result = cJSON_InsertItemInArray(array, 0, newitem);
    TEST_ASSERT_TRUE(result);
    verify_array_values(expected, 2);
}

/* Test: multiple inserts maintain correct order */
void test_multiple_inserts_maintain_order(void)
{
    cJSON *item1 = NULL;
    cJSON *item2 = NULL;
    cJSON_bool result1;
    cJSON_bool result2;
    double expected[] = {10.0, 0.0, 20.0, 1.0, 2.0};

    build_array_with_items(3);
    item1 = cJSON_CreateNumber(10.0);
    result1 = cJSON_InsertItemInArray(array, 0, item1);
    TEST_ASSERT_TRUE(result1);

    item2 = cJSON_CreateNumber(20.0);
    result2 = cJSON_InsertItemInArray(array, 2, item2);
    TEST_ASSERT_TRUE(result2);

    verify_array_values(expected, 5);
}

/* Test: insert into NULL array - should not crash (behavior depends on get_array_item) */
void test_insert_null_array_returns_false(void)
{
    cJSON *newitem = NULL;
    cJSON_bool result;

    newitem = cJSON_CreateNumber(42.0);
    /* Passing NULL array - get_array_item will return NULL, then add_item_to_array is called */
    /* add_item_to_array with NULL array should return false */
    result = cJSON_InsertItemInArray(NULL, 0, newitem);
    TEST_ASSERT_FALSE(result);
    cJSON_Delete(newitem);
}

/* Test: insert at index exactly equal to array size appends */
void test_insert_at_exact_size_appends(void)
{
    cJSON *newitem = NULL;
    cJSON_bool result;
    double expected[] = {0.0, 1.0, 2.0, 99.0};

    build_array_with_items(3);
    newitem = cJSON_CreateNumber(99.0);
    result = cJSON_InsertItemInArray(array, 3, newitem);
    TEST_ASSERT_TRUE(result);
    verify_array_values(expected, 4);
}

/* Test: verify linked list integrity after insert at beginning */
void test_linked_list_integrity_after_insert_at_beginning(void)
{
    cJSON *newitem = NULL;
    cJSON *current = NULL;
    int count = 0;

    build_array_with_items(4);
    newitem = cJSON_CreateNumber(99.0);
    cJSON_InsertItemInArray(array, 0, newitem);

    /* Walk forward through the list */
    current = array->child;
    while (current != NULL)
    {
        count++;
        if (current->next != NULL)
        {
            TEST_ASSERT_EQUAL_PTR(current, current->next->prev);
        }
        current = current->next;
    }
    TEST_ASSERT_EQUAL_INT(5, count);
}

/* Test: verify linked list integrity after insert at middle */
void test_linked_list_integrity_after_insert_at_middle(void)
{
    cJSON *newitem = NULL;
    cJSON *current = NULL;
    int count = 0;

    build_array_with_items(4);
    newitem = cJSON_CreateNumber(99.0);
    cJSON_InsertItemInArray(array, 2, newitem);

    current = array->child;
    while (current != NULL)
    {
        count++;
        if (current->next != NULL)
        {
            TEST_ASSERT_EQUAL_PTR(current, current->next->prev);
        }
        current = current->next;
    }
    TEST_ASSERT_EQUAL_INT(5, count);
}

/* Test: insert returns true on success */
void test_insert_returns_true_on_success(void)
{
    cJSON *newitem = NULL;
    cJSON_bool result;

    build_array_with_items(3);
    newitem = cJSON_CreateNumber(42.0);
    result = cJSON_InsertItemInArray(array, 1, newitem);
    TEST_ASSERT_TRUE(result);
}

/* Test: insert at index 0 in array with many elements */
void test_insert_at_index_0_large_array(void)
{
    cJSON *newitem = NULL;
    cJSON_bool result;
    int i;
    double expected[11];

    expected[0] = 999.0;
    for (i = 1; i <= 10; i++)
    {
        expected[i] = (double)(i - 1);
    }

    build_array_with_items(10);
    newitem = cJSON_CreateNumber(999.0);
    result = cJSON_InsertItemInArray(array, 0, newitem);
    TEST_ASSERT_TRUE(result);
    verify_array_values(expected, 11);
}

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_insert_negative_index_returns_false);
    RUN_TEST(test_insert_null_newitem_returns_false);
    RUN_TEST(test_insert_at_index_0_in_empty_array);
    RUN_TEST(test_insert_beyond_array_size_appends);
    RUN_TEST(test_insert_at_index_0_shifts_items);
    RUN_TEST(test_insert_at_index_0_updates_child_pointer);
    RUN_TEST(test_insert_at_middle_index);
    RUN_TEST(test_insert_at_last_valid_index);
    RUN_TEST(test_insert_sets_next_pointer_correctly);
    RUN_TEST(test_insert_at_index_0_prev_is_null);
    RUN_TEST(test_insert_at_middle_prev_pointer);
    RUN_TEST(test_insert_at_middle_next_pointer);
    RUN_TEST(test_insert_increases_array_size);
    RUN_TEST(test_insert_at_index_1_in_single_element_array_appends);
    RUN_TEST(test_insert_at_index_0_in_single_element_array);
    RUN_TEST(test_multiple_inserts_maintain_order);
    RUN_TEST(test_insert_null_array_returns_false);
    RUN_TEST(test_insert_at_exact_size_appends);
    RUN_TEST(test_linked_list_integrity_after_insert_at_beginning);
    RUN_TEST(test_linked_list_integrity_after_insert_at_middle);
    RUN_TEST(test_insert_returns_true_on_success);
    RUN_TEST(test_insert_at_index_0_large_array);
    return UNITY_END();
}