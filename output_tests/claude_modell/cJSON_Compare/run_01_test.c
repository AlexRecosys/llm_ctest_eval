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

/* -------------------------------------------------------------------------
 * Test 1: NULL inputs return false
 * ---------------------------------------------------------------------- */
void test_compare_null_inputs_return_false(void)
{
    item_a = cJSON_CreateNumber(42.0);
    TEST_ASSERT_NOT_NULL(item_a);

    /* Both NULL */
    TEST_ASSERT_FALSE_MESSAGE(cJSON_Compare(NULL, NULL, 1),
                              "Both NULL should return false");

    /* First NULL */
    TEST_ASSERT_FALSE_MESSAGE(cJSON_Compare(NULL, item_a, 1),
                              "First arg NULL should return false");

    /* Second NULL */
    TEST_ASSERT_FALSE_MESSAGE(cJSON_Compare(item_a, NULL, 1),
                              "Second arg NULL should return false");
}

/* -------------------------------------------------------------------------
 * Test 2: Primitive types (True, False, NULL) — equal and unequal
 * ---------------------------------------------------------------------- */
void test_compare_primitive_types(void)
{
    cJSON *t1 = cJSON_CreateTrue();
    cJSON *t2 = cJSON_CreateTrue();
    cJSON *f1 = cJSON_CreateFalse();
    cJSON *f2 = cJSON_CreateFalse();
    cJSON *n1 = cJSON_CreateNull();
    cJSON *n2 = cJSON_CreateNull();

    TEST_ASSERT_NOT_NULL(t1);
    TEST_ASSERT_NOT_NULL(t2);
    TEST_ASSERT_NOT_NULL(f1);
    TEST_ASSERT_NOT_NULL(f2);
    TEST_ASSERT_NOT_NULL(n1);
    TEST_ASSERT_NOT_NULL(n2);

    /* Same type primitives are equal */
    TEST_ASSERT_TRUE_MESSAGE(cJSON_Compare(t1, t2, 1),
                             "Two true values should be equal");
    TEST_ASSERT_TRUE_MESSAGE(cJSON_Compare(f1, f2, 1),
                             "Two false values should be equal");
    TEST_ASSERT_TRUE_MESSAGE(cJSON_Compare(n1, n2, 1),
                             "Two null values should be equal");

    /* Different type primitives are not equal */
    TEST_ASSERT_FALSE_MESSAGE(cJSON_Compare(t1, f1, 1),
                              "True vs False should not be equal");
    TEST_ASSERT_FALSE_MESSAGE(cJSON_Compare(t1, n1, 1),
                              "True vs Null should not be equal");
    TEST_ASSERT_FALSE_MESSAGE(cJSON_Compare(f1, n1, 1),
                              "False vs Null should not be equal");

    cJSON_Delete(t1);
    cJSON_Delete(t2);
    cJSON_Delete(f1);
    cJSON_Delete(f2);
    cJSON_Delete(n1);
    cJSON_Delete(n2);
}

/* -------------------------------------------------------------------------
 * Test 3: Number and String comparisons
 * ---------------------------------------------------------------------- */
void test_compare_numbers_and_strings(void)
{
    cJSON *num1 = cJSON_CreateNumber(3.14);
    cJSON *num2 = cJSON_CreateNumber(3.14);
    cJSON *num3 = cJSON_CreateNumber(2.71);
    cJSON *str1 = cJSON_CreateString("hello");
    cJSON *str2 = cJSON_CreateString("hello");
    cJSON *str3 = cJSON_CreateString("world");

    TEST_ASSERT_NOT_NULL(num1);
    TEST_ASSERT_NOT_NULL(num2);
    TEST_ASSERT_NOT_NULL(num3);
    TEST_ASSERT_NOT_NULL(str1);
    TEST_ASSERT_NOT_NULL(str2);
    TEST_ASSERT_NOT_NULL(str3);

    /* Equal numbers */
    TEST_ASSERT_TRUE_MESSAGE(cJSON_Compare(num1, num2, 1),
                             "Equal numbers should compare as equal");

    /* Different numbers */
    TEST_ASSERT_FALSE_MESSAGE(cJSON_Compare(num1, num3, 1),
                              "Different numbers should not be equal");

    /* Equal strings */
    TEST_ASSERT_TRUE_MESSAGE(cJSON_Compare(str1, str2, 1),
                             "Equal strings should compare as equal");

    /* Different strings */
    TEST_ASSERT_FALSE_MESSAGE(cJSON_Compare(str1, str3, 1),
                              "Different strings should not be equal");

    /* Number vs String — different types */
    TEST_ASSERT_FALSE_MESSAGE(cJSON_Compare(num1, str1, 1),
                              "Number vs String should not be equal");

    cJSON_Delete(num1);
    cJSON_Delete(num2);
    cJSON_Delete(num3);
    cJSON_Delete(str1);
    cJSON_Delete(str2);
    cJSON_Delete(str3);
}

/* -------------------------------------------------------------------------
 * Test 4: Array comparisons (equal, different length, different content)
 * ---------------------------------------------------------------------- */
void test_compare_arrays(void)
{
    /* Build array a: [1, 2, 3] */
    cJSON *arr_a = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(arr_a);
    cJSON_AddItemToArray(arr_a, cJSON_CreateNumber(1.0));
    cJSON_AddItemToArray(arr_a, cJSON_CreateNumber(2.0));
    cJSON_AddItemToArray(arr_a, cJSON_CreateNumber(3.0));

    /* Build array b: [1, 2, 3] — identical content */
    cJSON *arr_b = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(arr_b);
    cJSON_AddItemToArray(arr_b, cJSON_CreateNumber(1.0));
    cJSON_AddItemToArray(arr_b, cJSON_CreateNumber(2.0));
    cJSON_AddItemToArray(arr_b, cJSON_CreateNumber(3.0));

    /* Build array c: [1, 2] — shorter */
    cJSON *arr_c = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(arr_c);
    cJSON_AddItemToArray(arr_c, cJSON_CreateNumber(1.0));
    cJSON_AddItemToArray(arr_c, cJSON_CreateNumber(2.0));

    /* Build array d: [1, 2, 99] — same length, different content */
    cJSON *arr_d = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(arr_d);
    cJSON_AddItemToArray(arr_d, cJSON_CreateNumber(1.0));
    cJSON_AddItemToArray(arr_d, cJSON_CreateNumber(2.0));
    cJSON_AddItemToArray(arr_d, cJSON_CreateNumber(99.0));

    TEST_ASSERT_TRUE_MESSAGE(cJSON_Compare(arr_a, arr_b, 1),
                             "Identical arrays should be equal");
    TEST_ASSERT_FALSE_MESSAGE(cJSON_Compare(arr_a, arr_c, 1),
                              "Arrays of different length should not be equal");
    TEST_ASSERT_FALSE_MESSAGE(cJSON_Compare(arr_a, arr_d, 1),
                              "Arrays with different content should not be equal");

    /* Self-comparison */
    TEST_ASSERT_TRUE_MESSAGE(cJSON_Compare(arr_a, arr_a, 1),
                             "Array compared to itself should be equal");

    cJSON_Delete(arr_a);
    cJSON_Delete(arr_b);
    cJSON_Delete(arr_c);
    cJSON_Delete(arr_d);
}

/* -------------------------------------------------------------------------
 * Test 5: Object comparisons — case-sensitive and case-insensitive keys
 * ---------------------------------------------------------------------- */
void test_compare_objects_case_sensitivity(void)
{
    /* Object a: {"key": 1, "Value": 2} */
    cJSON *obj_a = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(obj_a);
    cJSON_AddItemToObject(obj_a, "key", cJSON_CreateNumber(1.0));
    cJSON_AddItemToObject(obj_a, "Value", cJSON_CreateNumber(2.0));

    /* Object b: identical to a */
    cJSON *obj_b = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(obj_b);
    cJSON_AddItemToObject(obj_b, "key", cJSON_CreateNumber(1.0));
    cJSON_AddItemToObject(obj_b, "Value", cJSON_CreateNumber(2.0));

    /* Object c: {"key": 1, "value": 2} — "value" differs in case from "Value" */
    cJSON *obj_c = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(obj_c);
    cJSON_AddItemToObject(obj_c, "key", cJSON_CreateNumber(1.0));
    cJSON_AddItemToObject(obj_c, "value", cJSON_CreateNumber(2.0));

    /* Object d: {"key": 1} — missing a field */
    cJSON *obj_d = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(obj_d);
    cJSON_AddItemToObject(obj_d, "key", cJSON_CreateNumber(1.0));

    /* Identical objects are equal (case-sensitive) */
    TEST_ASSERT_TRUE_MESSAGE(cJSON_Compare(obj_a, obj_b, 1),
                             "Identical objects should be equal (case-sensitive)");

    /* "Value" vs "value" — case-sensitive: not equal */
    TEST_ASSERT_FALSE_MESSAGE(cJSON_Compare(obj_a, obj_c, 1),
                              "Objects with different-case keys should not be equal (case-sensitive)");

    /* "Value" vs "value" — case-insensitive: equal */
    TEST_ASSERT_TRUE_MESSAGE(cJSON_Compare(obj_a, obj_c, 0),
                             "Objects with same-case-insensitive keys should be equal (case-insensitive)");

    /* Missing field: not equal */
    TEST_ASSERT_FALSE_MESSAGE(cJSON_Compare(obj_a, obj_d, 1),
                              "Objects with different number of keys should not be equal");

    cJSON_Delete(obj_a);
    cJSON_Delete(obj_b);
    cJSON_Delete(obj_c);
    cJSON_Delete(obj_d);
}

/* -------------------------------------------------------------------------
 * main
 * ---------------------------------------------------------------------- */
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