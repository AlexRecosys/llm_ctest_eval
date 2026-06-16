#include "cJSON.c"
#include "unity.h"

/* File-scope static variables / fixtures */
static cJSON *test_array = NULL;

/* Helper functions and macros */

/* Create a cJSON array with n items (cJSON_NULL) for testing */
static cJSON* create_test_array(size_t n) {
    cJSON *array = cJSON_CreateArray();
    if (array == NULL) {
        return NULL;
    }
    for (size_t i = 0; i < n; i++) {
        if (!cJSON_AddItemToArray(array, cJSON_CreateNull())) {
            cJSON_Delete(array);
            return NULL;
        }
    }
    return array;
}

/* Test cases */

void test_cJSON_GetArraySize_NullArray(void) {
    TEST_ASSERT_EQUAL_INT(0, cJSON_GetArraySize(NULL));
}

void test_cJSON_GetArraySize_EmptyArray(void) {
    cJSON *array = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(array);
    TEST_ASSERT_EQUAL_INT(0, cJSON_GetArraySize(array));
    cJSON_Delete(array);
}

void test_cJSON_GetArraySize_SingleItem(void) {
    cJSON *array = create_test_array(1);
    TEST_ASSERT_NOT_NULL(array);
    TEST_ASSERT_EQUAL_INT(1, cJSON_GetArraySize(array));
    cJSON_Delete(array);
}

void test_cJSON_GetArraySize_MultipleItems(void) {
    cJSON *array = create_test_array(5);
    TEST_ASSERT_NOT_NULL(array);
    TEST_ASSERT_EQUAL_INT(5, cJSON_GetArraySize(array));
    cJSON_Delete(array);
}

void test_cJSON_GetArraySize_ObjectAsArray(void) {
    cJSON *object = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(object);
    /* Objects also have child pointers, so GetArraySize should count them */
    TEST_ASSERT_EQUAL_INT(0, cJSON_GetArraySize(object));
    cJSON_Delete(object);
}

void test_cJSON_GetArraySize_ArrayWithMixedItems(void) {
    cJSON *array = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(array);

    TEST_ASSERT_TRUE(cJSON_AddItemToArray(array, cJSON_CreateNull()));
    TEST_ASSERT_TRUE(cJSON_AddItemToArray(array, cJSON_CreateTrue()));
    TEST_ASSERT_TRUE(cJSON_AddItemToArray(array, cJSON_CreateNumber(42)));
    TEST_ASSERT_TRUE(cJSON_AddItemToArray(array, cJSON_CreateString("test")));
    TEST_ASSERT_TRUE(cJSON_AddItemToArray(array, cJSON_CreateArray()));

    TEST_ASSERT_EQUAL_INT(5, cJSON_GetArraySize(array));
    cJSON_Delete(array);
}

void test_cJSON_GetArraySize_NestedArray(void) {
    cJSON *inner_array = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(inner_array);
    TEST_ASSERT_TRUE(cJSON_AddItemToArray(inner_array, cJSON_CreateNull()));
    TEST_ASSERT_TRUE(cJSON_AddItemToArray(inner_array, cJSON_CreateNull()));

    cJSON *outer_array = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(outer_array);
    TEST_ASSERT_TRUE(cJSON_AddItemToArray(outer_array, inner_array));
    TEST_ASSERT_TRUE(cJSON_AddItemToArray(outer_array, cJSON_CreateNull()));

    /* GetArraySize counts direct children only */
    TEST_ASSERT_EQUAL_INT(2, cJSON_GetArraySize(outer_array));
    cJSON_Delete(outer_array);
}

void test_cJSON_GetArraySize_ArrayWithNullChild(void) {
    /* This tests the edge case where array->child is NULL */
    cJSON *array = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(array);
    /* Ensure child is NULL (already is by default) */
    TEST_ASSERT_NULL(array->child);
    TEST_ASSERT_EQUAL_INT(0, cJSON_GetArraySize(array));
    cJSON_Delete(array);
}

void test_cJSON_GetArraySize_ArrayWithNextChain(void) {
    /* Manually construct a chain to test the while loop */
    cJSON *item1 = cJSON_CreateNull();
    cJSON *item2 = cJSON_CreateNull();
    cJSON *item3 = cJSON_CreateNull();

    item1->next = item2;
    item2->prev = item1;
    item2->next = item3;
    item3->prev = item2;

    cJSON *array = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(array);
    array->child = item1;
    /* item1, item2, item3 are now owned by the array */
    TEST_ASSERT_EQUAL_INT(3, cJSON_GetArraySize(array));

    /* Clean up */
    array->child = NULL; /* detach items from array to avoid double-free */
    cJSON_Delete(item1);
    cJSON_Delete(item2);
    cJSON_Delete(item3);
    cJSON_Delete(array);
}

int main(void) {
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_cJSON_GetArraySize_NullArray);
    RUN_TEST(test_cJSON_GetArraySize_EmptyArray);
    RUN_TEST(test_cJSON_GetArraySize_SingleItem);
    RUN_TEST(test_cJSON_GetArraySize_MultipleItems);
    RUN_TEST(test_cJSON_GetArraySize_ObjectAsArray);
    RUN_TEST(test_cJSON_GetArraySize_ArrayWithMixedItems);
    RUN_TEST(test_cJSON_GetArraySize_NestedArray);
    RUN_TEST(test_cJSON_GetArraySize_ArrayWithNullChild);
    RUN_TEST(test_cJSON_GetArraySize_ArrayWithNextChain);
    return UNITY_END();
}