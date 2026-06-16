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

/* ------------------------------------------------------------------ */
/* Helper: build an array with n number items using the public API     */
/* ------------------------------------------------------------------ */
static cJSON *build_array(int n)
{
    cJSON *arr = cJSON_CreateArray();
    int i;
    for (i = 0; i < n; i++)
    {
        cJSON_AddItemToArray(arr, cJSON_CreateNumber((double)i));
    }
    return arr;
}

/* ================================================================== */
/* Test cases                                                           */
/* ================================================================== */

/* NULL input must return 0 */
void test_GetArraySize_NullInput_ReturnsZero(void)
{
    int result = cJSON_GetArraySize(NULL);
    TEST_ASSERT_EQUAL_INT(0, result);
}

/* Empty array (child == NULL) must return 0 */
void test_GetArraySize_EmptyArray_ReturnsZero(void)
{
    array_fixture = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(array_fixture);
    int result = cJSON_GetArraySize(array_fixture);
    TEST_ASSERT_EQUAL_INT(0, result);
}

/* Array with exactly one element */
void test_GetArraySize_OneElement_ReturnsOne(void)
{
    array_fixture = build_array(1);
    TEST_ASSERT_NOT_NULL(array_fixture);
    int result = cJSON_GetArraySize(array_fixture);
    TEST_ASSERT_EQUAL_INT(1, result);
}

/* Array with two elements */
void test_GetArraySize_TwoElements_ReturnsTwo(void)
{
    array_fixture = build_array(2);
    TEST_ASSERT_NOT_NULL(array_fixture);
    int result = cJSON_GetArraySize(array_fixture);
    TEST_ASSERT_EQUAL_INT(2, result);
}

/* Array with five elements */
void test_GetArraySize_FiveElements_ReturnsFive(void)
{
    array_fixture = build_array(5);
    TEST_ASSERT_NOT_NULL(array_fixture);
    int result = cJSON_GetArraySize(array_fixture);
    TEST_ASSERT_EQUAL_INT(5, result);
}

/* Array with ten elements */
void test_GetArraySize_TenElements_ReturnsTen(void)
{
    array_fixture = build_array(10);
    TEST_ASSERT_NOT_NULL(array_fixture);
    int result = cJSON_GetArraySize(array_fixture);
    TEST_ASSERT_EQUAL_INT(10, result);
}

/* Array with 100 elements */
void test_GetArraySize_HundredElements_ReturnsHundred(void)
{
    array_fixture = build_array(100);
    TEST_ASSERT_NOT_NULL(array_fixture);
    int result = cJSON_GetArraySize(array_fixture);
    TEST_ASSERT_EQUAL_INT(100, result);
}

/* Size decreases after removing an item */
void test_GetArraySize_AfterDeleteItem_DecreasesSize(void)
{
    array_fixture = build_array(5);
    TEST_ASSERT_NOT_NULL(array_fixture);
    TEST_ASSERT_EQUAL_INT(5, cJSON_GetArraySize(array_fixture));

    cJSON_DeleteItemFromArray(array_fixture, 0);
    TEST_ASSERT_EQUAL_INT(4, cJSON_GetArraySize(array_fixture));
}

/* Size increases after adding an item */
void test_GetArraySize_AfterAddItem_IncreasesSize(void)
{
    array_fixture = build_array(3);
    TEST_ASSERT_NOT_NULL(array_fixture);
    TEST_ASSERT_EQUAL_INT(3, cJSON_GetArraySize(array_fixture));

    cJSON_AddItemToArray(array_fixture, cJSON_CreateNumber(99.0));
    TEST_ASSERT_EQUAL_INT(4, cJSON_GetArraySize(array_fixture));
}

/* Works on an object (object items are also a linked list) */
void test_GetArraySize_ObjectWithThreeKeys_ReturnsThree(void)
{
    array_fixture = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(array_fixture);
    cJSON_AddNumberToObject(array_fixture, "a", 1.0);
    cJSON_AddNumberToObject(array_fixture, "b", 2.0);
    cJSON_AddNumberToObject(array_fixture, "c", 3.0);
    int result = cJSON_GetArraySize(array_fixture);
    TEST_ASSERT_EQUAL_INT(3, result);
}

/* Empty object returns 0 */
void test_GetArraySize_EmptyObject_ReturnsZero(void)
{
    array_fixture = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(array_fixture);
    int result = cJSON_GetArraySize(array_fixture);
    TEST_ASSERT_EQUAL_INT(0, result);
}

/* Array parsed from JSON string */
void test_GetArraySize_ParsedArrayFourElements_ReturnsFour(void)
{
    array_fixture = cJSON_Parse("[1, 2, 3, 4]");
    TEST_ASSERT_NOT_NULL(array_fixture);
    int result = cJSON_GetArraySize(array_fixture);
    TEST_ASSERT_EQUAL_INT(4, result);
}

/* Array parsed from JSON string — single element */
void test_GetArraySize_ParsedArraySingleElement_ReturnsOne(void)
{
    array_fixture = cJSON_Parse("[42]");
    TEST_ASSERT_NOT_NULL(array_fixture);
    int result = cJSON_GetArraySize(array_fixture);
    TEST_ASSERT_EQUAL_INT(1, result);
}

/* Array parsed from JSON string — empty */
void test_GetArraySize_ParsedEmptyArray_ReturnsZero(void)
{
    array_fixture = cJSON_Parse("[]");
    TEST_ASSERT_NOT_NULL(array_fixture);
    int result = cJSON_GetArraySize(array_fixture);
    TEST_ASSERT_EQUAL_INT(0, result);
}

/* Nested array: outer array has 2 elements (inner arrays count as 1 each) */
void test_GetArraySize_NestedArrays_CountsTopLevelOnly(void)
{
    array_fixture = cJSON_Parse("[[1,2,3],[4,5]]");
    TEST_ASSERT_NOT_NULL(array_fixture);
    int result = cJSON_GetArraySize(array_fixture);
    TEST_ASSERT_EQUAL_INT(2, result);
}

/* Array of mixed types */
void test_GetArraySize_MixedTypeArray_ReturnsCorrectCount(void)
{
    array_fixture = cJSON_Parse("[1, \"hello\", true, null, {}]");
    TEST_ASSERT_NOT_NULL(array_fixture);
    int result = cJSON_GetArraySize(array_fixture);
    TEST_ASSERT_EQUAL_INT(5, result);
}

/* Calling GetArraySize twice on the same array returns the same value */
void test_GetArraySize_CalledTwice_ReturnsSameValue(void)
{
    array_fixture = build_array(7);
    TEST_ASSERT_NOT_NULL(array_fixture);
    int first  = cJSON_GetArraySize(array_fixture);
    int second = cJSON_GetArraySize(array_fixture);
    TEST_ASSERT_EQUAL_INT(first, second);
    TEST_ASSERT_EQUAL_INT(7, first);
}

/* Non-array cJSON node with no child — returns 0 */
void test_GetArraySize_NumberNode_ReturnsZero(void)
{
    array_fixture = cJSON_CreateNumber(3.14);
    TEST_ASSERT_NOT_NULL(array_fixture);
    int result = cJSON_GetArraySize(array_fixture);
    TEST_ASSERT_EQUAL_INT(0, result);
}

/* ================================================================== */
/* main                                                                 */
/* ================================================================== */
int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_GetArraySize_NullInput_ReturnsZero);
    RUN_TEST(test_GetArraySize_EmptyArray_ReturnsZero);
    RUN_TEST(test_GetArraySize_OneElement_ReturnsOne);
    RUN_TEST(test_GetArraySize_TwoElements_ReturnsTwo);
    RUN_TEST(test_GetArraySize_FiveElements_ReturnsFive);
    RUN_TEST(test_GetArraySize_TenElements_ReturnsTen);
    RUN_TEST(test_GetArraySize_HundredElements_ReturnsHundred);
    RUN_TEST(test_GetArraySize_AfterDeleteItem_DecreasesSize);
    RUN_TEST(test_GetArraySize_AfterAddItem_IncreasesSize);
    RUN_TEST(test_GetArraySize_ObjectWithThreeKeys_ReturnsThree);
    RUN_TEST(test_GetArraySize_EmptyObject_ReturnsZero);
    RUN_TEST(test_GetArraySize_ParsedArrayFourElements_ReturnsFour);
    RUN_TEST(test_GetArraySize_ParsedArraySingleElement_ReturnsOne);
    RUN_TEST(test_GetArraySize_ParsedEmptyArray_ReturnsZero);
    RUN_TEST(test_GetArraySize_NestedArrays_CountsTopLevelOnly);
    RUN_TEST(test_GetArraySize_MixedTypeArray_ReturnsCorrectCount);
    RUN_TEST(test_GetArraySize_CalledTwice_ReturnsSameValue);
    RUN_TEST(test_GetArraySize_NumberNode_ReturnsZero);
    return UNITY_END();
}