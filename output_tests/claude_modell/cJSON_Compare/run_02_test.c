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
 * Test 1: NULL inputs and type mismatch return false
 * ---------------------------------------------------------------------- */
void test_compare_null_inputs_and_type_mismatch(void)
{
    /* Both NULL */
    TEST_ASSERT_FALSE_MESSAGE(cJSON_Compare(NULL, NULL, 1),
                              "Both NULL should return false");

    /* First NULL */
    item_b = cJSON_CreateTrue();
    TEST_ASSERT_NOT_NULL(item_b);
    TEST_ASSERT_FALSE_MESSAGE(cJSON_Compare(NULL, item_b, 1),
                              "First arg NULL should return false");
    cJSON_Delete(item_b);
    item_b = NULL;

    /* Second NULL */
    item_a = cJSON_CreateTrue();
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_FALSE_MESSAGE(cJSON_Compare(item_a, NULL, 1),
                              "Second arg NULL should return false");
    cJSON_Delete(item_a);
    item_a = NULL;

    /* Type mismatch: True vs False */
    item_a = cJSON_CreateTrue();
    item_b = cJSON_CreateFalse();
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);
    TEST_ASSERT_FALSE_MESSAGE(cJSON_Compare(item_a, item_b, 1),
                              "True vs False type mismatch should return false");
    cJSON_Delete(item_a);
    item_a = NULL;
    cJSON_Delete(item_b);
    item_b = NULL;

    /* Type mismatch: Number vs String */
    item_a = cJSON_CreateNumber(42.0);
    item_b = cJSON_CreateString("42");
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);
    TEST_ASSERT_FALSE_MESSAGE(cJSON_Compare(item_a, item_b, 1),
                              "Number vs String type mismatch should return false");
}

/* -------------------------------------------------------------------------
 * Test 2: Primitive types (null, true, false) compare equal when same type
 * ---------------------------------------------------------------------- */
void test_compare_primitive_types(void)
{
    /* NULL type */
    item_a = cJSON_CreateNull();
    item_b = cJSON_CreateNull();
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);
    TEST_ASSERT_TRUE_MESSAGE(cJSON_Compare(item_a, item_b, 1),
                             "Two null items should be equal");
    cJSON_Delete(item_a); item_a = NULL;
    cJSON_Delete(item_b); item_b = NULL;

    /* True type */
    item_a = cJSON_CreateTrue();
    item_b = cJSON_CreateTrue();
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);
    TEST_ASSERT_TRUE_MESSAGE(cJSON_Compare(item_a, item_b, 1),
                             "Two true items should be equal");
    cJSON_Delete(item_a); item_a = NULL;
    cJSON_Delete(item_b); item_b = NULL;

    /* False type */
    item_a = cJSON_CreateFalse();
    item_b = cJSON_CreateFalse();
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);
    TEST_ASSERT_TRUE_MESSAGE(cJSON_Compare(item_a, item_b, 1),
                             "Two false items should be equal");

    /* Identical pointer: same object compared to itself */
    TEST_ASSERT_TRUE_MESSAGE(cJSON_Compare(item_a, item_a, 1),
                             "Same pointer should be equal to itself");
}

/* -------------------------------------------------------------------------
 * Test 3: Number and String comparisons
 * ---------------------------------------------------------------------- */
void test_compare_numbers_and_strings(void)
{
    /* Equal numbers */
    item_a = cJSON_CreateNumber(3.14);
    item_b = cJSON_CreateNumber(3.14);
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);
    TEST_ASSERT_TRUE_MESSAGE(cJSON_Compare(item_a, item_b, 1),
                             "Equal numbers should compare true");
    cJSON_Delete(item_a); item_a = NULL;
    cJSON_Delete(item_b); item_b = NULL;

    /* Different numbers */
    item_a = cJSON_CreateNumber(1.0);
    item_b = cJSON_CreateNumber(2.0);
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);
    TEST_ASSERT_FALSE_MESSAGE(cJSON_Compare(item_a, item_b, 1),
                              "Different numbers should compare false");
    cJSON_Delete(item_a); item_a = NULL;
    cJSON_Delete(item_b); item_b = NULL;

    /* Equal strings */
    item_a = cJSON_CreateString("hello");
    item_b = cJSON_CreateString("hello");
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);
    TEST_ASSERT_TRUE_MESSAGE(cJSON_Compare(item_a, item_b, 1),
                             "Equal strings should compare true");
    cJSON_Delete(item_a); item_a = NULL;
    cJSON_Delete(item_b); item_b = NULL;

    /* Different strings */
    item_a = cJSON_CreateString("hello");
    item_b = cJSON_CreateString("world");
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);
    TEST_ASSERT_FALSE_MESSAGE(cJSON_Compare(item_a, item_b, 1),
                              "Different strings should compare false");
}

/* -------------------------------------------------------------------------
 * Test 4: Array comparisons (equal, different length, different content)
 * ---------------------------------------------------------------------- */
void test_compare_arrays(void)
{
    cJSON *elem = NULL;

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
    cJSON_Delete(item_a); item_a = NULL;
    cJSON_Delete(item_b); item_b = NULL;

    /* Different length arrays: [1, 2] vs [1, 2, 3] */
    item_a = cJSON_CreateArray();
    item_b = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);

    cJSON_AddItemToArray(item_a, cJSON_CreateNumber(1.0));
    cJSON_AddItemToArray(item_a, cJSON_CreateNumber(2.0));

    cJSON_AddItemToArray(item_b, cJSON_CreateNumber(1.0));
    cJSON_AddItemToArray(item_b, cJSON_CreateNumber(2.0));
    cJSON_AddItemToArray(item_b, cJSON_CreateNumber(3.0));

    TEST_ASSERT_FALSE_MESSAGE(cJSON_Compare(item_a, item_b, 1),
                              "Arrays with different lengths should not be equal");
    cJSON_Delete(item_a); item_a = NULL;
    cJSON_Delete(item_b); item_b = NULL;

    /* Different content arrays: [1, 2] vs [1, 99] */
    item_a = cJSON_CreateArray();
    item_b = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);

    cJSON_AddItemToArray(item_a, cJSON_CreateNumber(1.0));
    cJSON_AddItemToArray(item_a, cJSON_CreateNumber(2.0));

    cJSON_AddItemToArray(item_b, cJSON_CreateNumber(1.0));
    cJSON_AddItemToArray(item_b, cJSON_CreateNumber(99.0));

    TEST_ASSERT_FALSE_MESSAGE(cJSON_Compare(item_a, item_b, 1),
                              "Arrays with different content should not be equal");

    /* Suppress unused variable warning */
    (void)elem;
}

/* -------------------------------------------------------------------------
 * Test 5: Object comparisons (equal, missing key, case sensitivity)
 * ---------------------------------------------------------------------- */
void test_compare_objects(void)
{
    /* Equal objects */
    item_a = cJSON_CreateObject();
    item_b = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);

    cJSON_AddNumberToObject(item_a, "x", 10.0);
    cJSON_AddStringToObject(item_a, "name", "Alice");

    cJSON_AddNumberToObject(item_b, "x", 10.0);
    cJSON_AddStringToObject(item_b, "name", "Alice");

    TEST_ASSERT_TRUE_MESSAGE(cJSON_Compare(item_a, item_b, 1),
                             "Objects with same keys and values should be equal");
    cJSON_Delete(item_a); item_a = NULL;
    cJSON_Delete(item_b); item_b = NULL;

    /* Object with missing key in b */
    item_a = cJSON_CreateObject();
    item_b = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);

    cJSON_AddNumberToObject(item_a, "x", 10.0);
    cJSON_AddNumberToObject(item_a, "y", 20.0);

    cJSON_AddNumberToObject(item_b, "x", 10.0);
    /* "y" is missing from item_b */

    TEST_ASSERT_FALSE_MESSAGE(cJSON_Compare(item_a, item_b, 1),
                              "Objects with different keys should not be equal");
    cJSON_Delete(item_a); item_a = NULL;
    cJSON_Delete(item_b); item_b = NULL;

    /* Case sensitivity: "Key" vs "key" with case_sensitive=1 should be unequal */
    item_a = cJSON_CreateObject();
    item_b = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);

    cJSON_AddNumberToObject(item_a, "Key", 1.0);
    cJSON_AddNumberToObject(item_b, "key", 1.0);

    TEST_ASSERT_FALSE_MESSAGE(cJSON_Compare(item_a, item_b, 1),
                              "Case-sensitive compare: 'Key' vs 'key' should be unequal");
    cJSON_Delete(item_a); item_a = NULL;
    cJSON_Delete(item_b); item_b = NULL;

    /* Case insensitivity: "Key" vs "key" with case_sensitive=0 should be equal */
    item_a = cJSON_CreateObject();
    item_b = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);

    cJSON_AddNumberToObject(item_a, "Key", 1.0);
    cJSON_AddNumberToObject(item_b, "key", 1.0);

    TEST_ASSERT_TRUE_MESSAGE(cJSON_Compare(item_a, item_b, 0),
                             "Case-insensitive compare: 'Key' vs 'key' should be equal");
}

/* -------------------------------------------------------------------------
 * main
 * ---------------------------------------------------------------------- */
int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_compare_null_inputs_and_type_mismatch);
    RUN_TEST(test_compare_primitive_types);
    RUN_TEST(test_compare_numbers_and_strings);
    RUN_TEST(test_compare_arrays);
    RUN_TEST(test_compare_objects);
    return UNITY_END();
}