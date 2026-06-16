#include "cJSON.c"
#include "unity.h"
#include <stdlib.h>
#include <string.h>

/* File-scope static variables / fixtures */
static cJSON *array = NULL;
static cJSON *item1 = NULL;
static cJSON *item2 = NULL;

/* Helper functions and macros */

/* Setup and teardown functions */
void setUp(void)
{
    array = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(array);
    item1 = cJSON_CreateNumber(42);
    TEST_ASSERT_NOT_NULL(item1);
    item2 = cJSON_CreateString("test");
    TEST_ASSERT_NOT_NULL(item2);
}

void tearDown(void)
{
    cJSON_Delete(array);
    cJSON_Delete(item1);
    cJSON_Delete(item2);
    array = NULL;
    item1 = NULL;
    item2 = NULL;
}

/* Test cases */

void test_cJSON_AddItemToArray_should_add_item_to_empty_array(void)
{
    cJSON_bool result = cJSON_AddItemToArray(array, item1);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(1, cJSON_GetArraySize(array));
    TEST_ASSERT_EQUAL_PTR(item1, cJSON_GetArrayItem(array, 0));
}

void test_cJSON_AddItemToArray_should_append_item_to_nonempty_array(void)
{
    /* Pre-populate array */
    TEST_ASSERT_TRUE(cJSON_AddItemToArray(array, item1));
    TEST_ASSERT_EQUAL_INT(1, cJSON_GetArraySize(array));

    /* Add second item */
    cJSON_bool result = cJSON_AddItemToArray(array, item2);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(2, cJSON_GetArraySize(array));
    TEST_ASSERT_EQUAL_PTR(item1, cJSON_GetArrayItem(array, 0));
    TEST_ASSERT_EQUAL_PTR(item2, cJSON_GetArrayItem(array, 1));
}

void test_cJSON_AddItemToArray_should_fail_when_array_is_NULL(void)
{
    cJSON_bool result = cJSON_AddItemToArray(NULL, item1);
    TEST_ASSERT_FALSE(result);
}

void test_cJSON_AddItemToArray_should_fail_when_item_is_NULL(void)
{
    cJSON_bool result = cJSON_AddItemToArray(array, NULL);
    TEST_ASSERT_FALSE(result);
}

void test_cJSON_AddItemToArray_should_not_steal_reference(void)
{
    /* Add item to array */
    TEST_ASSERT_TRUE(cJSON_AddItemToArray(array, item1));

    /* Verify item is still accessible independently */
    TEST_ASSERT_EQUAL_INT(42, (int)item1->valuedouble);

    /* Verify array contains same value */
    cJSON *array_item = cJSON_GetArrayItem(array, 0);
    TEST_ASSERT_EQUAL_PTR(item1, array_item);
    TEST_ASSERT_EQUAL_INT(42, (int)array_item->valuedouble);
}

void test_cJSON_AddItemToArray_should_maintain_order(void)
{
    cJSON *item3 = cJSON_CreateBool(1);
    TEST_ASSERT_NOT_NULL(item3);

    TEST_ASSERT_TRUE(cJSON_AddItemToArray(array, item1));
    TEST_ASSERT_TRUE(cJSON_AddItemToArray(array, item2));
    TEST_ASSERT_TRUE(cJSON_AddItemToArray(array, item3));

    TEST_ASSERT_EQUAL_INT(3, cJSON_GetArraySize(array));
    TEST_ASSERT_EQUAL_PTR(item1, cJSON_GetArrayItem(array, 0));
    TEST_ASSERT_EQUAL_PTR(item2, cJSON_GetArrayItem(array, 1));
    TEST_ASSERT_EQUAL_PTR(item3, cJSON_GetArrayItem(array, 2));

    cJSON_Delete(item3);
}

void test_cJSON_AddItemToArray_should_handle_multiple_adds(void)
{
    int i;
    for (i = 0; i < 10; i++) {
        cJSON *item = cJSON_CreateNumber(i);
        TEST_ASSERT_NOT_NULL(item);
        TEST_ASSERT_TRUE(cJSON_AddItemToArray(array, item));
    }

    TEST_ASSERT_EQUAL_INT(10, cJSON_GetArraySize(array));
    for (i = 0; i < 10; i++) {
        cJSON *item = cJSON_GetArrayItem(array, i);
        TEST_ASSERT_NOT_NULL(item);
        TEST_ASSERT_EQUAL_INT(i, (int)item->valuedouble);
    }
}

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_cJSON_AddItemToArray_should_add_item_to_empty_array);
    RUN_TEST(test_cJSON_AddItemToArray_should_append_item_to_nonempty_array);
    RUN_TEST(test_cJSON_AddItemToArray_should_fail_when_array_is_NULL);
    RUN_TEST(test_cJSON_AddItemToArray_should_fail_when_item_is_NULL);
    RUN_TEST(test_cJSON_AddItemToArray_should_not_steal_reference);
    RUN_TEST(test_cJSON_AddItemToArray_should_maintain_order);
    RUN_TEST(test_cJSON_AddItemToArray_should_handle_multiple_adds);
    return UNITY_END();
}