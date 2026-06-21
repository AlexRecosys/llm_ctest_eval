#include "cJSON.c"
#include "unity.h"
#include <stdlib.h>
#include <string.h>

/* File-scope static variables / fixtures */
static cJSON *array = NULL;
static cJSON *item = NULL;

/* Helper functions */
static void setup_array(void)
{
    array = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(array);
}

static void setup_item(void)
{
    item = cJSON_CreateNull();
    TEST_ASSERT_NOT_NULL(item);
}

static void cleanup_array(void)
{
    if (array != NULL)
    {
        cJSON_Delete(array);
        array = NULL;
    }
}

static void cleanup_item(void)
{
    if (item != NULL)
    {
        cJSON_Delete(item);
        item = NULL;
    }
}

/* Test cases */

void test_cJSON_AddItemToArray_with_null_array_should_return_false(void)
{
    TEST_ASSERT_FALSE(cJSON_AddItemToArray(NULL, item));
}

void test_cJSON_AddItemToArray_with_null_item_should_return_false(void)
{
    TEST_ASSERT_FALSE(cJSON_AddItemToArray(array, NULL));
}

void test_cJSON_AddItemToArray_with_same_array_and_item_should_return_false(void)
{
    TEST_ASSERT_FALSE(cJSON_AddItemToArray(array, array));
}

void test_cJSON_AddItemToArray_to_empty_array_should_add_item_and_set_prev_to_itself(void)
{
    TEST_ASSERT_NULL(array->child);
    TEST_ASSERT_TRUE(cJSON_AddItemToArray(array, item));
    TEST_ASSERT_EQUAL_PTR(array->child, item);
    TEST_ASSERT_EQUAL_PTR(item->prev, item);
    TEST_ASSERT_NULL(item->next);
}

void test_cJSON_AddItemToArray_to_nonempty_array_should_append_to_end(void)
{
    cJSON *first_item = cJSON_CreateNull();
    cJSON *second_item = cJSON_CreateNull();
    TEST_ASSERT_NOT_NULL(first_item);
    TEST_ASSERT_NOT_NULL(second_item);

    TEST_ASSERT_TRUE(cJSON_AddItemToArray(array, first_item));
    TEST_ASSERT_TRUE(cJSON_AddItemToArray(array, second_item));

    TEST_ASSERT_EQUAL_PTR(array->child, first_item);
    TEST_ASSERT_EQUAL_PTR(first_item->next, second_item);
    TEST_ASSERT_EQUAL_PTR(second_item->prev, first_item);
    TEST_ASSERT_EQUAL_PTR(array->child->prev, second_item);
    TEST_ASSERT_NULL(second_item->next);

    cJSON_Delete(first_item);
    cJSON_Delete(second_item);
}

void test_cJSON_AddItemToArray_multiple_items_should_maintain_circular_prev_links(void)
{
    cJSON *items[3] = {0};
    size_t i;

    for (i = 0; i < 3; ++i)
    {
        items[i] = cJSON_CreateNull();
        TEST_ASSERT_NOT_NULL(items[i]);
        TEST_ASSERT_TRUE(cJSON_AddItemToArray(array, items[i]));
    }

    /* Check array->child points to first item */
    TEST_ASSERT_EQUAL_PTR(array->child, items[0]);

    /* Check next/prev chain */
    TEST_ASSERT_EQUAL_PTR(items[0]->next, items[1]);
    TEST_ASSERT_EQUAL_PTR(items[1]->next, items[2]);
    TEST_ASSERT_NULL(items[2]->next);

    TEST_ASSERT_EQUAL_PTR(items[0]->prev, items[2]);
    TEST_ASSERT_EQUAL_PTR(items[1]->prev, items[0]);
    TEST_ASSERT_EQUAL_PTR(items[2]->prev, items[1]);

    /* Check array->child->prev points to last item */
    TEST_ASSERT_EQUAL_PTR(array->child->prev, items[2]);

    for (i = 0; i < 3; ++i)
    {
        cJSON_Delete(items[i]);
    }
}

void test_cJSON_AddItemToArray_should_return_true_on_success(void)
{
    setup_item();
    TEST_ASSERT_TRUE(cJSON_AddItemToArray(array, item));
    cleanup_item();
}

void test_cJSON_AddItemToArray_should_not_modify_array_if_item_is_null(void)
{
    TEST_ASSERT_NULL(array->child);
    TEST_ASSERT_FALSE(cJSON_AddItemToArray(array, NULL));
    TEST_ASSERT_NULL(array->child);
}

void test_cJSON_AddItemToArray_should_not_modify_array_if_array_is_null(void)
{
    TEST_ASSERT_FALSE(cJSON_AddItemToArray(NULL, item));
}

/* main function */
int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();

    RUN_TEST(test_cJSON_AddItemToArray_with_null_array_should_return_false);
    RUN_TEST(test_cJSON_AddItemToArray_with_null_item_should_return_false);
    RUN_TEST(test_cJSON_AddItemToArray_with_same_array_and_item_should_return_false);
    RUN_TEST(test_cJSON_AddItemToArray_to_empty_array_should_add_item_and_set_prev_to_itself);
    RUN_TEST(test_cJSON_AddItemToArray_to_nonempty_array_should_append_to_end);
    RUN_TEST(test_cJSON_AddItemToArray_multiple_items_should_maintain_circular_prev_links);
    RUN_TEST(test_cJSON_AddItemToArray_should_return_true_on_success);
    RUN_TEST(test_cJSON_AddItemToArray_should_not_modify_array_if_item_is_null);
    RUN_TEST(test_cJSON_AddItemToArray_should_not_modify_array_if_array_is_null);

    return UNITY_END();
}