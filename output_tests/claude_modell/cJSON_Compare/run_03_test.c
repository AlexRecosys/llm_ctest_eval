#include "cJSON.c"
#include "unity.h"
#include "cJSON.h"
#include <stdlib.h>
#include <string.h>

/* setUp and tearDown */
void setUp(void)
{
}

void tearDown(void)
{
}

/* ------------------------------------------------------------------ */
/* Helper macros                                                        */
/* ------------------------------------------------------------------ */
#define ASSERT_COMPARE_TRUE(a, b, cs)  TEST_ASSERT_TRUE(cJSON_Compare((a), (b), (cs)))
#define ASSERT_COMPARE_FALSE(a, b, cs) TEST_ASSERT_FALSE(cJSON_Compare((a), (b), (cs)))

/* ------------------------------------------------------------------ */
/* Tests: NULL / invalid inputs                                         */
/* ------------------------------------------------------------------ */

void test_compare_null_a_returns_false(void)
{
    cJSON *b = cJSON_CreateNull();
    TEST_ASSERT_NOT_NULL(b);
    ASSERT_COMPARE_FALSE(NULL, b, true);
    cJSON_Delete(b);
}

void test_compare_null_b_returns_false(void)
{
    cJSON *a = cJSON_CreateNull();
    TEST_ASSERT_NOT_NULL(a);
    ASSERT_COMPARE_FALSE(a, NULL, true);
    cJSON_Delete(a);
}

void test_compare_both_null_pointers_returns_false(void)
{
    ASSERT_COMPARE_FALSE(NULL, NULL, true);
}

void test_compare_different_types_returns_false(void)
{
    cJSON *a = cJSON_CreateNull();
    cJSON *b = cJSON_CreateTrue();
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    ASSERT_COMPARE_FALSE(a, b, true);
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_invalid_type_returns_false(void)
{
    /* Build two nodes with an invalid type (0) */
    cJSON a_node;
    cJSON b_node;
    memset(&a_node, 0, sizeof(a_node));
    memset(&b_node, 0, sizeof(b_node));
    a_node.type = cJSON_Invalid;
    b_node.type = cJSON_Invalid;
    ASSERT_COMPARE_FALSE(&a_node, &b_node, true);
}

/* ------------------------------------------------------------------ */
/* Tests: identical pointer                                             */
/* ------------------------------------------------------------------ */

void test_compare_same_pointer_returns_true(void)
{
    cJSON *a = cJSON_CreateNumber(42.0);
    TEST_ASSERT_NOT_NULL(a);
    ASSERT_COMPARE_TRUE(a, a, true);
    cJSON_Delete(a);
}

/* ------------------------------------------------------------------ */
/* Tests: cJSON_False / cJSON_True / cJSON_NULL                        */
/* ------------------------------------------------------------------ */

void test_compare_false_false_returns_true(void)
{
    cJSON *a = cJSON_CreateFalse();
    cJSON *b = cJSON_CreateFalse();
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    ASSERT_COMPARE_TRUE(a, b, true);
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_true_true_returns_true(void)
{
    cJSON *a = cJSON_CreateTrue();
    cJSON *b = cJSON_CreateTrue();
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    ASSERT_COMPARE_TRUE(a, b, true);
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_null_null_returns_true(void)
{
    cJSON *a = cJSON_CreateNull();
    cJSON *b = cJSON_CreateNull();
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    ASSERT_COMPARE_TRUE(a, b, true);
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_true_false_returns_false(void)
{
    cJSON *a = cJSON_CreateTrue();
    cJSON *b = cJSON_CreateFalse();
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    ASSERT_COMPARE_FALSE(a, b, true);
    cJSON_Delete(a);
    cJSON_Delete(b);
}

/* ------------------------------------------------------------------ */
/* Tests: Number                                                        */
/* ------------------------------------------------------------------ */

void test_compare_equal_numbers_returns_true(void)
{
    cJSON *a = cJSON_CreateNumber(3.14);
    cJSON *b = cJSON_CreateNumber(3.14);
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    ASSERT_COMPARE_TRUE(a, b, true);
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_different_numbers_returns_false(void)
{
    cJSON *a = cJSON_CreateNumber(1.0);
    cJSON *b = cJSON_CreateNumber(2.0);
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    ASSERT_COMPARE_FALSE(a, b, true);
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_zero_numbers_returns_true(void)
{
    cJSON *a = cJSON_CreateNumber(0.0);
    cJSON *b = cJSON_CreateNumber(0.0);
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    ASSERT_COMPARE_TRUE(a, b, true);
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_negative_numbers_returns_true(void)
{
    cJSON *a = cJSON_CreateNumber(-99.5);
    cJSON *b = cJSON_CreateNumber(-99.5);
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    ASSERT_COMPARE_TRUE(a, b, true);
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_negative_vs_positive_returns_false(void)
{
    cJSON *a = cJSON_CreateNumber(-1.0);
    cJSON *b = cJSON_CreateNumber(1.0);
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    ASSERT_COMPARE_FALSE(a, b, true);
    cJSON_Delete(a);
    cJSON_Delete(b);
}

/* ------------------------------------------------------------------ */
/* Tests: String                                                        */
/* ------------------------------------------------------------------ */

void test_compare_equal_strings_returns_true(void)
{
    cJSON *a = cJSON_CreateString("hello");
    cJSON *b = cJSON_CreateString("hello");
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    ASSERT_COMPARE_TRUE(a, b, true);
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_different_strings_returns_false(void)
{
    cJSON *a = cJSON_CreateString("hello");
    cJSON *b = cJSON_CreateString("world");
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    ASSERT_COMPARE_FALSE(a, b, true);
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_empty_strings_returns_true(void)
{
    cJSON *a = cJSON_CreateString("");
    cJSON *b = cJSON_CreateString("");
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    ASSERT_COMPARE_TRUE(a, b, true);
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_string_case_sensitive_different_case_returns_false(void)
{
    cJSON *a = cJSON_CreateString("Hello");
    cJSON *b = cJSON_CreateString("hello");
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    /* strcmp is used for string values, so case matters regardless of case_sensitive flag */
    ASSERT_COMPARE_FALSE(a, b, true);
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_string_with_null_valuestring_returns_false(void)
{
    cJSON *a = cJSON_CreateString("hello");
    cJSON *b = cJSON_CreateString("hello");
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    /* Manually set one valuestring to NULL to simulate the edge case */
    free(b->valuestring);
    b->valuestring = NULL;
    ASSERT_COMPARE_FALSE(a, b, true);
    cJSON_Delete(a);
    cJSON_Delete(b);
}

/* ------------------------------------------------------------------ */
/* Tests: Raw                                                           */
/* ------------------------------------------------------------------ */

void test_compare_equal_raw_returns_true(void)
{
    cJSON *a = cJSON_CreateRaw("{\"key\":1}");
    cJSON *b = cJSON_CreateRaw("{\"key\":1}");
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    ASSERT_COMPARE_TRUE(a, b, true);
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_different_raw_returns_false(void)
{
    cJSON *a = cJSON_CreateRaw("{\"key\":1}");
    cJSON *b = cJSON_CreateRaw("{\"key\":2}");
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    ASSERT_COMPARE_FALSE(a, b, true);
    cJSON_Delete(a);
    cJSON_Delete(b);
}

/* ------------------------------------------------------------------ */
/* Tests: Array                                                         */
/* ------------------------------------------------------------------ */

void test_compare_empty_arrays_returns_true(void)
{
    cJSON *a = cJSON_CreateArray();
    cJSON *b = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    ASSERT_COMPARE_TRUE(a, b, true);
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_equal_arrays_returns_true(void)
{
    cJSON *a = cJSON_CreateArray();
    cJSON *b = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    cJSON_AddItemToArray(a, cJSON_CreateNumber(1.0));
    cJSON_AddItemToArray(a, cJSON_CreateString("two"));
    cJSON_AddItemToArray(a, cJSON_CreateTrue());
    cJSON_AddItemToArray(b, cJSON_CreateNumber(1.0));
    cJSON_AddItemToArray(b, cJSON_CreateString("two"));
    cJSON_AddItemToArray(b, cJSON_CreateTrue());
    ASSERT_COMPARE_TRUE(a, b, true);
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_arrays_different_values_returns_false(void)
{
    cJSON *a = cJSON_CreateArray();
    cJSON *b = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    cJSON_AddItemToArray(a, cJSON_CreateNumber(1.0));
    cJSON_AddItemToArray(b, cJSON_CreateNumber(2.0));
    ASSERT_COMPARE_FALSE(a, b, true);
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_arrays_different_lengths_a_longer_returns_false(void)
{
    cJSON *a = cJSON_CreateArray();
    cJSON *b = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    cJSON_AddItemToArray(a, cJSON_CreateNumber(1.0));
    cJSON_AddItemToArray(a, cJSON_CreateNumber(2.0));
    cJSON_AddItemToArray(b, cJSON_CreateNumber(1.0));
    ASSERT_COMPARE_FALSE(a, b, true);
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_arrays_different_lengths_b_longer_returns_false(void)
{
    cJSON *a = cJSON_CreateArray();
    cJSON *b = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    cJSON_AddItemToArray(a, cJSON_CreateNumber(1.0));
    cJSON_AddItemToArray(b, cJSON_CreateNumber(1.0));
    cJSON_AddItemToArray(b, cJSON_CreateNumber(2.0));
    ASSERT_COMPARE_FALSE(a, b, true);
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_nested_arrays_equal_returns_true(void)
{
    cJSON *a = cJSON_CreateArray();
    cJSON *b = cJSON_CreateArray();
    cJSON *inner_a = cJSON_CreateArray();
    cJSON *inner_b = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    TEST_ASSERT_NOT_NULL(inner_a);
    TEST_ASSERT_NOT_NULL(inner_b);
    cJSON_AddItemToArray(inner_a, cJSON_CreateNumber(42.0));
    cJSON_AddItemToArray(inner_b, cJSON_CreateNumber(42.0));
    cJSON_AddItemToArray(a, inner_a);
    cJSON_AddItemToArray(b, inner_b);
    ASSERT_COMPARE_TRUE(a, b, true);
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_nested_arrays_different_returns_false(void)
{
    cJSON *a = cJSON_CreateArray();
    cJSON *b = cJSON_CreateArray();
    cJSON *inner_a = cJSON_CreateArray();
    cJSON *inner_b = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    TEST_ASSERT_NOT_NULL(inner_a);
    TEST_ASSERT_NOT_NULL(inner_b);
    cJSON_AddItemToArray(inner_a, cJSON_CreateNumber(1.0));
    cJSON_AddItemToArray(inner_b, cJSON_CreateNumber(2.0));
    cJSON_AddItemToArray(a, inner_a);
    cJSON_AddItemToArray(b, inner_b);
    ASSERT_COMPARE_FALSE(a, b, true);
    cJSON_Delete(a);
    cJSON_Delete(b);
}

/* ------------------------------------------------------------------ */
/* Tests: Object                                                        */
/* ------------------------------------------------------------------ */

void test_compare_empty_objects_returns_true(void)
{
    cJSON *a = cJSON_CreateObject();
    cJSON *b = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    ASSERT_COMPARE_TRUE(a, b, true);
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_equal_objects_returns_true(void)
{
    cJSON *a = cJSON_CreateObject();
    cJSON *b = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    cJSON_AddNumberToObject(a, "x", 1.0);
    cJSON_AddStringToObject(a, "y", "hello");
    cJSON_AddNumberToObject(b, "x", 1.0);
    cJSON_AddStringToObject(b, "y", "hello");
    ASSERT_COMPARE_TRUE(a, b, true);
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_objects_different_values_returns_false(void)
{
    cJSON *a = cJSON_CreateObject();
    cJSON *b = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    cJSON_AddNumberToObject(a, "x", 1.0);
    cJSON_AddNumberToObject(b, "x", 2.0);
    ASSERT_COMPARE_FALSE(a, b, true);
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_objects_missing_key_in_b_returns_false(void)
{
    cJSON *a = cJSON_CreateObject();
    cJSON *b = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    cJSON_AddNumberToObject(a, "x", 1.0);
    cJSON_AddNumberToObject(a, "y", 2.0);
    cJSON_AddNumberToObject(b, "x", 1.0);
    ASSERT_COMPARE_FALSE(a, b, true);
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_objects_missing_key_in_a_returns_false(void)
{
    cJSON *a = cJSON_CreateObject();
    cJSON *b = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    cJSON_AddNumberToObject(a, "x", 1.0);
    cJSON_AddNumberToObject(b, "x", 1.0);
    cJSON_AddNumberToObject(b, "y", 2.0);
    ASSERT_COMPARE_FALSE(a, b, true);
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_objects_case_sensitive_key_match_returns_true(void)
{
    cJSON *a = cJSON_CreateObject();
    cJSON *b = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    cJSON_AddNumberToObject(a, "Key", 1.0);
    cJSON_AddNumberToObject(b, "Key", 1.0);
    ASSERT_COMPARE_TRUE(a, b, true);
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_objects_case_sensitive_key_mismatch_returns_false(void)
{
    cJSON *a = cJSON_CreateObject();
    cJSON *b = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    cJSON_AddNumberToObject(a, "Key", 1.0);
    cJSON_AddNumberToObject(b, "key", 1.0);
    /* case_sensitive = true: "Key" != "key" */
    ASSERT_COMPARE_FALSE(a, b, true);
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_objects_case_insensitive_key_match_returns_true(void)
{
    cJSON *a = cJSON_CreateObject();
    cJSON *b = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    cJSON_AddNumberToObject(a, "Key", 1.0);
    cJSON_AddNumberToObject(b, "key", 1.0);
    /* case_sensitive = false: "Key" == "key" */
    ASSERT_COMPARE_TRUE(a, b, false);
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_objects_different_order_same_content_returns_true(void)
{
    cJSON *a = cJSON_CreateObject();
    cJSON *b = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    cJSON_AddNumberToObject(a, "x", 1.0);
    cJSON_AddStringToObject(a, "y", "val");
    /* Different insertion order */
    cJSON_AddStringToObject(b, "y", "val");
    cJSON_AddNumberToObject(b, "x", 1.0);
    ASSERT_COMPARE_TRUE(a, b, true);
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_nested_objects_equal_returns_true(void)
{
    cJSON *a = cJSON_CreateObject();
    cJSON *b = cJSON_CreateObject();
    cJSON *inner_a = cJSON_CreateObject();
    cJSON *inner_b = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    TEST_ASSERT_NOT_NULL(inner_a);
    TEST_ASSERT_NOT_NULL(inner_b);
    cJSON_AddNumberToObject(inner_a, "n", 7.0);
    cJSON_AddNumberToObject(inner_b, "n", 7.0);
    cJSON_AddItemToObject(a, "inner", inner_a);
    cJSON_AddItemToObject(b, "inner", inner_b);
    ASSERT_COMPARE_TRUE(a, b, true);
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_nested_objects_different_returns_false(void)
{
    cJSON *a = cJSON_CreateObject();
    cJSON *b = cJSON_CreateObject();
    cJSON *inner_a = cJSON_CreateObject();
    cJSON *inner_b = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    TEST_ASSERT_NOT_NULL(inner_a);
    TEST_ASSERT_NOT_NULL(inner_b);
    cJSON_AddNumberToObject(inner_a, "n", 7.0);
    cJSON_AddNumberToObject(inner_b, "n", 8.0);
    cJSON_AddItemToObject(a, "inner", inner_a);
    cJSON_AddItemToObject(b, "inner", inner_b);
    ASSERT_COMPARE_FALSE(a, b, true);
    cJSON_Delete(a);
    cJSON_Delete(b);
}

/* ------------------------------------------------------------------ */
/* Tests: Mixed / complex structures                                    */
/* ------------------------------------------------------------------ */

void test_compare_complex_equal_structures_returns_true(void)
{
    cJSON *a = cJSON_Parse("{\"name\":\"Alice\",\"age\":30,\"active\":true,\"scores\":[10,20,30]}");
    cJSON *b = cJSON_Parse("{\"name\":\"Alice\",\"age\":30,\"active\":true,\"scores\":[10,20,30]}");
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    ASSERT_COMPARE_TRUE(a, b, true);
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_complex_different_structures_returns_false(void)
{
    cJSON *a = cJSON_Parse("{\"name\":\"Alice\",\"age\":30}");
    cJSON *b = cJSON_Parse("{\"name\":\"Alice\",\"age\":31}");
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    ASSERT_COMPARE_FALSE(a, b, true);
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_array_vs_object_returns_false(void)
{
    cJSON *a = cJSON_CreateArray();
    cJSON *b = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    ASSERT_COMPARE_FALSE(a, b, true);
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_number_vs_string_returns_false(void)
{
    cJSON *a = cJSON_CreateNumber(1.0);
    cJSON *b = cJSON_CreateString("1");
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    ASSERT_COMPARE_FALSE(a, b, true);
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_null_vs_false_returns_false(void)
{
    cJSON *a = cJSON_CreateNull();
    cJSON *b = cJSON_CreateFalse();
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    ASSERT_COMPARE_FALSE(a, b, true);
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_array_with_mixed_types_equal_returns_true(void)
{
    cJSON *a = cJSON_CreateArray();
    cJSON *b = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    cJSON_AddItemToArray(a, cJSON_CreateNull());
    cJSON_AddItemToArray(a, cJSON_CreateTrue());
    cJSON_AddItemToArray(a, cJSON_CreateFalse());
    cJSON_AddItemToArray(a, cJSON_CreateNumber(0.0));
    cJSON_AddItemToArray(b, cJSON_CreateNull());
    cJSON_AddItemToArray(b, cJSON_CreateTrue());
    cJSON_AddItemToArray(b, cJSON_CreateFalse());
    cJSON_AddItemToArray(b, cJSON_CreateNumber(0.0));
    ASSERT_COMPARE_TRUE(a, b, true);
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_object_with_array_value_equal_returns_true(void)
{
    cJSON *a = cJSON_CreateObject();
    cJSON *b = cJSON_CreateObject();
    cJSON *arr_a = cJSON_CreateArray();
    cJSON *arr_b = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    TEST_ASSERT_NOT_NULL(arr_a);
    TEST_ASSERT_NOT_NULL(arr_b);
    cJSON_AddItemToArray(arr_a, cJSON_CreateNumber(1.0));
    cJSON_AddItemToArray(arr_a, cJSON_CreateNumber(2.0));
    cJSON_AddItemToArray(arr_b, cJSON_CreateNumber(1.0));
    cJSON_AddItemToArray(arr_b, cJSON_CreateNumber(2.0));
    cJSON_AddItemToObject(a, "list", arr_a);
    cJSON_AddItemToObject(b, "list", arr_b);
    ASSERT_COMPARE_TRUE(a, b, true);
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_large_equal_numbers_returns_true(void)
{
    cJSON *a = cJSON_CreateNumber(1e300);
    cJSON *b = cJSON_CreateNumber(1e300);
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    ASSERT_COMPARE_TRUE(a, b, true);
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_object_subset_is_not_equal(void)
{
    /* a is a subset of b — should return false */
    cJSON *a = cJSON_CreateObject();
    cJSON *b = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    cJSON_AddNumberToObject(a, "x", 1.0);
    cJSON_AddNumberToObject(b, "x", 1.0);
    cJSON_AddNumberToObject(b, "y", 2.0);
    ASSERT_COMPARE_FALSE(a, b, true);
    cJSON_Delete(a);
    cJSON_Delete(b);
}

/* ------------------------------------------------------------------ */
/* main                                                                 */
/* ------------------------------------------------------------------ */

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();

    RUN_TEST(test_compare_null_a_returns_false);
    RUN_TEST(test_compare_null_b_returns_false);
    RUN_TEST(test_compare_both_null_pointers_returns_false);
    RUN_TEST(test_compare_different_types_returns_false);
    RUN_TEST(test_compare_invalid_type_returns_false);
    RUN_TEST(test_compare_same_pointer_returns_true);
    RUN_TEST(test_compare_false_false_returns_true);
    RUN_TEST(test_compare_true_true_returns_true);
    RUN_TEST(test_compare_null_null_returns_true);
    RUN_TEST(test_compare_true_false_returns_false);
    RUN_TEST(test_compare_equal_numbers_returns_true);
    RUN_TEST(test_compare_different_numbers_returns_false);
    RUN_TEST(test_compare_zero_numbers_returns_true);
    RUN_TEST(test_compare_negative_numbers_returns_true);
    RUN_TEST(test_compare_negative_vs_positive_returns_false);
    RUN_TEST(test_compare_equal_strings_returns_true);
    RUN_TEST(test_compare_different_strings_returns_false);
    RUN_TEST(test_compare_empty_strings_returns_true);
    RUN_TEST(test_compare_string_case_sensitive_different_case_returns_false);
    RUN_TEST(test_compare_string_with_null_valuestring_returns_false);
    RUN_TEST(test_compare_equal_raw_returns_true);
    RUN_TEST(test_compare_different_raw_returns_false);
    RUN_TEST(test_compare_empty_arrays_returns_true);
    RUN_TEST(test_compare_equal_arrays_returns_true);
    RUN_TEST(test_compare_arrays_different_values_returns_false);
    RUN_TEST(test_compare_arrays_different_lengths_a_longer_returns_false);
    RUN_TEST(test_compare_arrays_different_lengths_b_longer_returns_false);
    RUN_TEST(test_compare_nested_arrays_equal_returns_true);
    RUN_TEST(test_compare_nested_arrays_different_returns_false);
    RUN_TEST(test_compare_empty_objects_returns_true);
    RUN_TEST(test_compare_equal_objects_returns_true);
    RUN_TEST(test_compare_objects_different_values_returns_false);
    RUN_TEST(test_compare_objects_missing_key_in_b_returns_false);
    RUN_TEST(test_compare_objects_missing_key_in_a_returns_false);
    RUN_TEST(test_compare_objects_case_sensitive_key_match_returns_true);
    RUN_TEST(test_compare_objects_case_sensitive_key_mismatch_returns_false);
    RUN_TEST(test_compare_objects_case_insensitive_key_match_returns_true);
    RUN_TEST(test_compare_objects_different_order_same_content_returns_true);
    RUN_TEST(test_compare_nested_objects_equal_returns_true);
    RUN_TEST(test_compare_nested_objects_different_returns_false);
    RUN_TEST(test_compare_complex_equal_structures_returns_true);
    RUN_TEST(test_compare_complex_different_structures_returns_false);
    RUN_TEST(test_compare_array_vs_object_returns_false);
    RUN_TEST(test_compare_number_vs_string_returns_false);
    RUN_TEST(test_compare_null_vs_false_returns_false);
    RUN_TEST(test_compare_array_with_mixed_types_equal_returns_true);
    RUN_TEST(test_compare_object_with_array_value_equal_returns_true);
    RUN_TEST(test_compare_large_equal_numbers_returns_true);
    RUN_TEST(test_compare_object_subset_is_not_equal);

    return UNITY_END();
}