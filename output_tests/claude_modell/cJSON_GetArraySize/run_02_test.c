#include "cJSON.c"
#include "unity.h"
#include "cJSON.h"
#include <stddef.h>

/* File-scope fixtures */
static cJSON *array_fixture = NULL;

/* setUp and tearDown */
void setUp(void)
{
    array_fixture = NULL;
}

void tearDown(void)
{
    if (array_fixture != NULL)
    {
        cJSON_Delete(array_fixture);
        array_fixture = NULL;
    }
}

/* Helper: build an array with n number items */
static cJSON *build_array_of_size(int n)
{
    cJSON *arr = cJSON_CreateArray();
    int i;
    for (i = 0; i < n; i++)
    {
        cJSON_AddItemToArray(arr, cJSON_CreateNumber((double)i));
    }
    return arr;
}

/* ----------------------------- Test Cases ----------------------------- */

void test_GetArraySize_null_returns_zero(void)
{
    int result = cJSON_GetArraySize(NULL);
    TEST_ASSERT_EQUAL_INT(0, result);
}

void test_GetArraySize_empty_array_returns_zero(void)
{
    array_fixture = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(array_fixture);
    int result = cJSON_GetArraySize(array_fixture);
    TEST_ASSERT_EQUAL_INT(0, result);
}

void test_GetArraySize_single_element_returns_one(void)
{
    array_fixture = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(array_fixture);
    cJSON_AddItemToArray(array_fixture, cJSON_CreateNumber(42.0));
    int result = cJSON_GetArraySize(array_fixture);
    TEST_ASSERT_EQUAL_INT(1, result);
}

void test_GetArraySize_two_elements_returns_two(void)
{
    array_fixture = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(array_fixture);
    cJSON_AddItemToArray(array_fixture, cJSON_CreateNumber(1.0));
    cJSON_AddItemToArray(array_fixture, cJSON_CreateNumber(2.0));
    int result = cJSON_GetArraySize(array_fixture);
    TEST_ASSERT_EQUAL_INT(2, result);
}

void test_GetArraySize_five_elements_returns_five(void)
{
    array_fixture = build_array_of_size(5);
    TEST_ASSERT_NOT_NULL(array_fixture);
    int result = cJSON_GetArraySize(array_fixture);
    TEST_ASSERT_EQUAL_INT(5, result);
}

void test_GetArraySize_ten_elements_returns_ten(void)
{
    array_fixture = build_array_of_size(10);
    TEST_ASSERT_NOT_NULL(array_fixture);
    int result = cJSON_GetArraySize(array_fixture);
    TEST_ASSERT_EQUAL_INT(10, result);
}

void test_GetArraySize_hundred_elements_returns_hundred(void)
{
    array_fixture = build_array_of_size(100);
    TEST_ASSERT_NOT_NULL(array_fixture);
    int result = cJSON_GetArraySize(array_fixture);
    TEST_ASSERT_EQUAL_INT(100, result);
}

void test_GetArraySize_mixed_types_counts_all(void)
{
    array_fixture = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(array_fixture);
    cJSON_AddItemToArray(array_fixture, cJSON_CreateNumber(1.0));
    cJSON_AddItemToArray(array_fixture, cJSON_CreateString("hello"));
    cJSON_AddItemToArray(array_fixture, cJSON_CreateNull());
    cJSON_AddItemToArray(array_fixture, cJSON_CreateTrue());
    cJSON_AddItemToArray(array_fixture, cJSON_CreateFalse());
    int result = cJSON_GetArraySize(array_fixture);
    TEST_ASSERT_EQUAL_INT(5, result);
}

void test_GetArraySize_nested_array_counts_only_top_level(void)
{
    array_fixture = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(array_fixture);

    cJSON *inner = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(inner);
    cJSON_AddItemToArray(inner, cJSON_CreateNumber(1.0));
    cJSON_AddItemToArray(inner, cJSON_CreateNumber(2.0));
    cJSON_AddItemToArray(inner, cJSON_CreateNumber(3.0));

    cJSON_AddItemToArray(array_fixture, inner);
    cJSON_AddItemToArray(array_fixture, cJSON_CreateNumber(99.0));

    int result = cJSON_GetArraySize(array_fixture);
    TEST_ASSERT_EQUAL_INT(2, result);
}

void test_GetArraySize_parsed_array_correct_size(void)
{
    array_fixture = cJSON_Parse("[1, 2, 3, 4, 5]");
    TEST_ASSERT_NOT_NULL(array_fixture);
    int result = cJSON_GetArraySize(array_fixture);
    TEST_ASSERT_EQUAL_INT(5, result);
}

void test_GetArraySize_parsed_empty_array_returns_zero(void)
{
    array_fixture = cJSON_Parse("[]");
    TEST_ASSERT_NOT_NULL(array_fixture);
    int result = cJSON_GetArraySize(array_fixture);
    TEST_ASSERT_EQUAL_INT(0, result);
}

void test_GetArraySize_object_with_keys_returns_key_count(void)
{
    array_fixture = cJSON_Parse("{\"a\":1, \"b\":2, \"c\":3}");
    TEST_ASSERT_NOT_NULL(array_fixture);
    int result = cJSON_GetArraySize(array_fixture);
    TEST_ASSERT_EQUAL_INT(3, result);
}

void test_GetArraySize_empty_object_returns_zero(void)
{
    array_fixture = cJSON_Parse("{}");
    TEST_ASSERT_NOT_NULL(array_fixture);
    int result = cJSON_GetArraySize(array_fixture);
    TEST_ASSERT_EQUAL_INT(0, result);
}

void test_GetArraySize_after_delete_item_decrements(void)
{
    array_fixture = build_array_of_size(4);
    TEST_ASSERT_NOT_NULL(array_fixture);
    TEST_ASSERT_EQUAL_INT(4, cJSON_GetArraySize(array_fixture));

    cJSON_DeleteItemFromArray(array_fixture, 0);
    TEST_ASSERT_EQUAL_INT(3, cJSON_GetArraySize(array_fixture));
}

void test_GetArraySize_after_add_item_increments(void)
{
    array_fixture = build_array_of_size(3);
    TEST_ASSERT_NOT_NULL(array_fixture);
    TEST_ASSERT_EQUAL_INT(3, cJSON_GetArraySize(array_fixture));

    cJSON_AddItemToArray(array_fixture, cJSON_CreateNumber(99.0));
    TEST_ASSERT_EQUAL_INT(4, cJSON_GetArraySize(array_fixture));
}

void test_GetArraySize_non_array_node_with_no_child_returns_zero(void)
{
    /* A plain number node has no child, so size should be 0 */
    array_fixture = cJSON_CreateNumber(3.14);
    TEST_ASSERT_NOT_NULL(array_fixture);
    int result = cJSON_GetArraySize(array_fixture);
    TEST_ASSERT_EQUAL_INT(0, result);
}

void test_GetArraySize_string_node_returns_zero(void)
{
    array_fixture = cJSON_CreateString("test");
    TEST_ASSERT_NOT_NULL(array_fixture);
    int result = cJSON_GetArraySize(array_fixture);
    TEST_ASSERT_EQUAL_INT(0, result);
}

void test_GetArraySize_parsed_single_element_array(void)
{
    array_fixture = cJSON_Parse("[42]");
    TEST_ASSERT_NOT_NULL(array_fixture);
    int result = cJSON_GetArraySize(array_fixture);
    TEST_ASSERT_EQUAL_INT(1, result);
}

void test_GetArraySize_array_of_arrays_counts_top_level_only(void)
{
    array_fixture = cJSON_Parse("[[1,2],[3,4],[5,6,7]]");
    TEST_ASSERT_NOT_NULL(array_fixture);
    int result = cJSON_GetArraySize(array_fixture);
    TEST_ASSERT_EQUAL_INT(3, result);
}

/* ----------------------------- main ----------------------------- */

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_GetArraySize_null_returns_zero);
    RUN_TEST(test_GetArraySize_empty_array_returns_zero);
    RUN_TEST(test_GetArraySize_single_element_returns_one);
    RUN_TEST(test_GetArraySize_two_elements_returns_two);
    RUN_TEST(test_GetArraySize_five_elements_returns_five);
    RUN_TEST(test_GetArraySize_ten_elements_returns_ten);
    RUN_TEST(test_GetArraySize_hundred_elements_returns_hundred);
    RUN_TEST(test_GetArraySize_mixed_types_counts_all);
    RUN_TEST(test_GetArraySize_nested_array_counts_only_top_level);
    RUN_TEST(test_GetArraySize_parsed_array_correct_size);
    RUN_TEST(test_GetArraySize_parsed_empty_array_returns_zero);
    RUN_TEST(test_GetArraySize_object_with_keys_returns_key_count);
    RUN_TEST(test_GetArraySize_empty_object_returns_zero);
    RUN_TEST(test_GetArraySize_after_delete_item_decrements);
    RUN_TEST(test_GetArraySize_after_add_item_increments);
    RUN_TEST(test_GetArraySize_non_array_node_with_no_child_returns_zero);
    RUN_TEST(test_GetArraySize_string_node_returns_zero);
    RUN_TEST(test_GetArraySize_parsed_single_element_array);
    RUN_TEST(test_GetArraySize_array_of_arrays_counts_top_level_only);
    return UNITY_END();
}