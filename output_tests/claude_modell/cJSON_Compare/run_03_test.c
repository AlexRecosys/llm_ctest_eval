#include "cJSON.c"
#include "unity.h"
#include "cJSON.h"
#include <stdlib.h>
#include <string.h>
#include <signal.h>

/* File-scope fixtures */
static cJSON *item_a = NULL;
static cJSON *item_b = NULL;

/* Signal handler for SIGSEGV */
static void sigsegv_handler(int sig)
{
    (void)sig;
    TEST_FAIL_MESSAGE("Segmentation fault (SIGSEGV) caught during test execution");
}

void setUp(void)
{
    signal(SIGSEGV, sigsegv_handler);
    item_a = NULL;
    item_b = NULL;
}

void tearDown(void)
{
    if (item_a != NULL)
    {
        cJSON_Delete(item_a);
        item_a = NULL;
    }
    if (item_b != NULL)
    {
        cJSON_Delete(item_b);
        item_b = NULL;
    }
    signal(SIGSEGV, SIG_DFL);
}

/* Test 1: NULL inputs return false */
void test_compare_null_inputs_return_false(void)
{
    item_a = cJSON_CreateNumber(42.0);
    TEST_ASSERT_NOT_NULL(item_a);

    /* Both NULL */
    TEST_ASSERT_FALSE_MESSAGE(cJSON_Compare(NULL, NULL, 1),
                              "Comparing NULL with NULL should return false");

    /* First NULL */
    TEST_ASSERT_FALSE_MESSAGE(cJSON_Compare(NULL, item_a, 1),
                              "Comparing NULL with valid item should return false");

    /* Second NULL */
    TEST_ASSERT_FALSE_MESSAGE(cJSON_Compare(item_a, NULL, 1),
                              "Comparing valid item with NULL should return false");
}

/* Test 2: Primitive types (True, False, NULL) equal and unequal */
void test_compare_primitive_types(void)
{
    cJSON *true_a  = cJSON_CreateTrue();
    cJSON *true_b  = cJSON_CreateTrue();
    cJSON *false_a = cJSON_CreateFalse();
    cJSON *false_b = cJSON_CreateFalse();
    cJSON *null_a  = cJSON_CreateNull();
    cJSON *null_b  = cJSON_CreateNull();

    TEST_ASSERT_NOT_NULL(true_a);
    TEST_ASSERT_NOT_NULL(true_b);
    TEST_ASSERT_NOT_NULL(false_a);
    TEST_ASSERT_NOT_NULL(false_b);
    TEST_ASSERT_NOT_NULL(null_a);
    TEST_ASSERT_NOT_NULL(null_b);

    /* Same type primitives are equal */
    TEST_ASSERT_TRUE_MESSAGE(cJSON_Compare(true_a, true_b, 1),
                             "Two cJSON_True items should be equal");
    TEST_ASSERT_TRUE_MESSAGE(cJSON_Compare(false_a, false_b, 1),
                             "Two cJSON_False items should be equal");
    TEST_ASSERT_TRUE_MESSAGE(cJSON_Compare(null_a, null_b, 1),
                             "Two cJSON_NULL items should be equal");

    /* Different type primitives are not equal */
    TEST_ASSERT_FALSE_MESSAGE(cJSON_Compare(true_a, false_a, 1),
                              "cJSON_True and cJSON_False should not be equal");
    TEST_ASSERT_FALSE_MESSAGE(cJSON_Compare(true_a, null_a, 1),
                              "cJSON_True and cJSON_NULL should not be equal");
    TEST_ASSERT_FALSE_MESSAGE(cJSON_Compare(false_a, null_a, 1),
                              "cJSON_False and cJSON_NULL should not be equal");

    cJSON_Delete(true_a);
    cJSON_Delete(true_b);
    cJSON_Delete(false_a);
    cJSON_Delete(false_b);
    cJSON_Delete(null_a);
    cJSON_Delete(null_b);
}

/* Test 3: Number and String comparisons */
void test_compare_numbers_and_strings(void)
{
    cJSON *num_a  = cJSON_CreateNumber(3.14);
    cJSON *num_b  = cJSON_CreateNumber(3.14);
    cJSON *num_c  = cJSON_CreateNumber(2.71);
    cJSON *str_a  = cJSON_CreateString("hello");
    cJSON *str_b  = cJSON_CreateString("hello");
    cJSON *str_c  = cJSON_CreateString("world");

    TEST_ASSERT_NOT_NULL(num_a);
    TEST_ASSERT_NOT_NULL(num_b);
    TEST_ASSERT_NOT_NULL(num_c);
    TEST_ASSERT_NOT_NULL(str_a);
    TEST_ASSERT_NOT_NULL(str_b);
    TEST_ASSERT_NOT_NULL(str_c);

    /* Equal numbers */
    TEST_ASSERT_TRUE_MESSAGE(cJSON_Compare(num_a, num_b, 1),
                             "Numbers with same value should be equal");

    /* Different numbers */
    TEST_ASSERT_FALSE_MESSAGE(cJSON_Compare(num_a, num_c, 1),
                              "Numbers with different values should not be equal");

    /* Equal strings */
    TEST_ASSERT_TRUE_MESSAGE(cJSON_Compare(str_a, str_b, 1),
                             "Strings with same value should be equal");

    /* Different strings */
    TEST_ASSERT_FALSE_MESSAGE(cJSON_Compare(str_a, str_c, 1),
                              "Strings with different values should not be equal");

    /* Number vs String (different types) */
    TEST_ASSERT_FALSE_MESSAGE(cJSON_Compare(num_a, str_a, 1),
                              "Number and String should not be equal");

    cJSON_Delete(num_a);
    cJSON_Delete(num_b);
    cJSON_Delete(num_c);
    cJSON_Delete(str_a);
    cJSON_Delete(str_b);
    cJSON_Delete(str_c);
}

/* Test 4: Array comparisons including length mismatch */
void test_compare_arrays(void)
{
    /* Equal arrays: [1, 2, 3] vs [1, 2, 3] */
    item_a = cJSON_CreateArray();
    item_b = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);

    cJSON_AddItemToArray(item_a, cJSON_CreateNumber(1.0));
    cJSON_AddItemToArray(item_a, cJSON_CreateNumber(2.0));
    cJSON_AddItemToArray(item_a, cJSON_CreateNumber(3.0));

    cJSON_AddItemToArray(item_b, cJSON_CreateNumber(1.0));
    cJSON_AddItemToArray(item_b, cJSON_CreateNumber(2.0));
    cJSON_AddItemToArray(item_b, cJSON_CreateNumber(3.0));

    TEST_ASSERT_TRUE_MESSAGE(cJSON_Compare(item_a, item_b, 1),
                             "Arrays with same elements should be equal");

    /* Different element */
    cJSON_Delete(item_b);
    item_b = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(item_b);
    cJSON_AddItemToArray(item_b, cJSON_CreateNumber(1.0));
    cJSON_AddItemToArray(item_b, cJSON_CreateNumber(2.0));
    cJSON_AddItemToArray(item_b, cJSON_CreateNumber(99.0));

    TEST_ASSERT_FALSE_MESSAGE(cJSON_Compare(item_a, item_b, 1),
                              "Arrays with different elements should not be equal");

    /* Different lengths: [1,2,3] vs [1,2] */
    cJSON_Delete(item_b);
    item_b = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(item_b);
    cJSON_AddItemToArray(item_b, cJSON_CreateNumber(1.0));
    cJSON_AddItemToArray(item_b, cJSON_CreateNumber(2.0));

    TEST_ASSERT_FALSE_MESSAGE(cJSON_Compare(item_a, item_b, 1),
                              "Arrays with different lengths should not be equal");

    /* Empty arrays should be equal */
    cJSON_Delete(item_a);
    cJSON_Delete(item_b);
    item_a = cJSON_CreateArray();
    item_b = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);

    TEST_ASSERT_TRUE_MESSAGE(cJSON_Compare(item_a, item_b, 1),
                             "Two empty arrays should be equal");
}

/* Test 5: Object comparisons including case sensitivity */
void test_compare_objects_case_sensitivity(void)
{
    /* Equal objects */
    item_a = cJSON_CreateObject();
    item_b = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);

    cJSON_AddStringToObject(item_a, "key1", "value1");
    cJSON_AddNumberToObject(item_a, "key2", 42.0);

    cJSON_AddStringToObject(item_b, "key1", "value1");
    cJSON_AddNumberToObject(item_b, "key2", 42.0);

    TEST_ASSERT_TRUE_MESSAGE(cJSON_Compare(item_a, item_b, 1),
                             "Objects with same key-value pairs should be equal (case sensitive)");

    /* Object with extra key in b */
    cJSON_AddNullToObject(item_b, "key3");
    TEST_ASSERT_FALSE_MESSAGE(cJSON_Compare(item_a, item_b, 1),
                              "Objects with different number of keys should not be equal");

    /* Case insensitive key comparison */
    cJSON_Delete(item_a);
    cJSON_Delete(item_b);
    item_a = cJSON_CreateObject();
    item_b = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);

    cJSON_AddStringToObject(item_a, "Key", "value");
    cJSON_AddStringToObject(item_b, "key", "value");

    /* Case sensitive: "Key" != "key" => not equal */
    TEST_ASSERT_FALSE_MESSAGE(cJSON_Compare(item_a, item_b, 1),
                              "Objects with different-case keys should not be equal (case sensitive)");

    /* Case insensitive: "Key" == "key" => equal */
    TEST_ASSERT_TRUE_MESSAGE(cJSON_Compare(item_a, item_b, 0),
                             "Objects with different-case keys should be equal (case insensitive)");
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_compare_null_inputs_return_false);
    RUN_TEST(test_compare_primitive_types);
    RUN_TEST(test_compare_numbers_and_strings);
    RUN_TEST(test_compare_arrays);
    RUN_TEST(test_compare_objects_case_sensitivity);
    return UNITY_END();
}