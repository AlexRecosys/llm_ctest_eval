#include "cJSON.c"
#include "unity.h"
#include "cJSON.h"
#include <stddef.h>

/* File-scope fixtures */
static cJSON *array_empty = NULL;
static cJSON *array_one   = NULL;
static cJSON *array_three = NULL;
static cJSON *object_two  = NULL;

/* ---------- helpers ---------- */

static cJSON *build_array_of_n(int n)
{
    cJSON *arr = cJSON_CreateArray();
    int i;
    for (i = 0; i < n; i++)
    {
        cJSON_AddItemToArray(arr, cJSON_CreateNumber((double)i));
    }
    return arr;
}

/* ---------- setUp / tearDown ---------- */

void setUp(void)
{
    array_empty = cJSON_CreateArray();
    array_one   = build_array_of_n(1);
    array_three = build_array_of_n(3);

    object_two = cJSON_CreateObject();
    cJSON_AddNumberToObject(object_two, "a", 1.0);
    cJSON_AddNumberToObject(object_two, "b", 2.0);
}

void tearDown(void)
{
    cJSON_Delete(array_empty);
    cJSON_Delete(array_one);
    cJSON_Delete(array_three);
    cJSON_Delete(object_two);

    array_empty = NULL;
    array_one   = NULL;
    array_three = NULL;
    object_two  = NULL;
}

/* ---------- test cases ---------- */

void test_GetArraySize_null_returns_zero(void)
{
    TEST_ASSERT_EQUAL_INT(0, cJSON_GetArraySize(NULL));
}

void test_GetArraySize_empty_array_returns_zero(void)
{
    TEST_ASSERT_EQUAL_INT(0, cJSON_GetArraySize(array_empty));
}

void test_GetArraySize_one_element_returns_one(void)
{
    TEST_ASSERT_EQUAL_INT(1, cJSON_GetArraySize(array_one));
}

void test_GetArraySize_three_elements_returns_three(void)
{
    TEST_ASSERT_EQUAL_INT(3, cJSON_GetArraySize(array_three));
}

void test_GetArraySize_object_with_two_keys_returns_two(void)
{
    /* cJSON_GetArraySize works on objects as well */
    TEST_ASSERT_EQUAL_INT(2, cJSON_GetArraySize(object_two));
}

void test_GetArraySize_after_add_item_increments(void)
{
    int before = cJSON_GetArraySize(array_three);
    cJSON_AddItemToArray(array_three, cJSON_CreateNull());
    int after = cJSON_GetArraySize(array_three);
    TEST_ASSERT_EQUAL_INT(before + 1, after);
}

void test_GetArraySize_after_delete_item_decrements(void)
{
    int before = cJSON_GetArraySize(array_three);
    cJSON_DeleteItemFromArray(array_three, 0);
    int after = cJSON_GetArraySize(array_three);
    TEST_ASSERT_EQUAL_INT(before - 1, after);
}

void test_GetArraySize_large_array(void)
{
    cJSON *large = build_array_of_n(100);
    TEST_ASSERT_EQUAL_INT(100, cJSON_GetArraySize(large));
    cJSON_Delete(large);
}

void test_GetArraySize_parsed_array(void)
{
    cJSON *parsed = cJSON_Parse("[1,2,3,4,5]");
    TEST_ASSERT_NOT_NULL(parsed);
    TEST_ASSERT_EQUAL_INT(5, cJSON_GetArraySize(parsed));
    cJSON_Delete(parsed);
}

void test_GetArraySize_parsed_empty_array(void)
{
    cJSON *parsed = cJSON_Parse("[]");
    TEST_ASSERT_NOT_NULL(parsed);
    TEST_ASSERT_EQUAL_INT(0, cJSON_GetArraySize(parsed));
    cJSON_Delete(parsed);
}

void test_GetArraySize_nested_array_counts_only_top_level(void)
{
    /* [[1,2],[3,4],[5,6]] — top level has 3 items */
    cJSON *parsed = cJSON_Parse("[[1,2],[3,4],[5,6]]");
    TEST_ASSERT_NOT_NULL(parsed);
    TEST_ASSERT_EQUAL_INT(3, cJSON_GetArraySize(parsed));
    cJSON_Delete(parsed);
}

void test_GetArraySize_non_array_node_with_no_child_returns_zero(void)
{
    /* A plain number node has no children */
    cJSON *num = cJSON_CreateNumber(42.0);
    TEST_ASSERT_EQUAL_INT(0, cJSON_GetArraySize(num));
    cJSON_Delete(num);
}

void test_GetArraySize_string_node_returns_zero(void)
{
    cJSON *str = cJSON_CreateString("hello");
    TEST_ASSERT_EQUAL_INT(0, cJSON_GetArraySize(str));
    cJSON_Delete(str);
}

void test_GetArraySize_single_element_object_returns_one(void)
{
    cJSON *obj = cJSON_CreateObject();
    cJSON_AddStringToObject(obj, "key", "value");
    TEST_ASSERT_EQUAL_INT(1, cJSON_GetArraySize(obj));
    cJSON_Delete(obj);
}

void test_GetArraySize_detach_reduces_count(void)
{
    cJSON *arr = build_array_of_n(4);
    TEST_ASSERT_EQUAL_INT(4, cJSON_GetArraySize(arr));

    cJSON *detached = cJSON_DetachItemFromArray(arr, 2);
    TEST_ASSERT_EQUAL_INT(3, cJSON_GetArraySize(arr));

    cJSON_Delete(detached);
    cJSON_Delete(arr);
}

void test_GetArraySize_array_of_mixed_types(void)
{
    cJSON *arr = cJSON_Parse("[1, \"two\", true, null, {\"k\":3}]");
    TEST_ASSERT_NOT_NULL(arr);
    TEST_ASSERT_EQUAL_INT(5, cJSON_GetArraySize(arr));
    cJSON_Delete(arr);
}

/* ---------- main ---------- */

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_GetArraySize_null_returns_zero);
    RUN_TEST(test_GetArraySize_empty_array_returns_zero);
    RUN_TEST(test_GetArraySize_one_element_returns_one);
    RUN_TEST(test_GetArraySize_three_elements_returns_three);
    RUN_TEST(test_GetArraySize_object_with_two_keys_returns_two);
    RUN_TEST(test_GetArraySize_after_add_item_increments);
    RUN_TEST(test_GetArraySize_after_delete_item_decrements);
    RUN_TEST(test_GetArraySize_large_array);
    RUN_TEST(test_GetArraySize_parsed_array);
    RUN_TEST(test_GetArraySize_parsed_empty_array);
    RUN_TEST(test_GetArraySize_nested_array_counts_only_top_level);
    RUN_TEST(test_GetArraySize_non_array_node_with_no_child_returns_zero);
    RUN_TEST(test_GetArraySize_string_node_returns_zero);
    RUN_TEST(test_GetArraySize_single_element_object_returns_one);
    RUN_TEST(test_GetArraySize_detach_reduces_count);
    RUN_TEST(test_GetArraySize_array_of_mixed_types);
    return UNITY_END();
}