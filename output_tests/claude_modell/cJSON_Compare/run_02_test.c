#include "cJSON.c"
#include "unity.h"
#include "cJSON.h"
#include <stdlib.h>
#include <string.h>

/* -------------------------------------------------------------------------
 * Helper macros / functions
 * ---------------------------------------------------------------------- */

static cJSON *make_string(const char *s)
{
    return cJSON_CreateString(s);
}

static cJSON *make_number(double d)
{
    return cJSON_CreateNumber(d);
}

/* -------------------------------------------------------------------------
 * setUp / tearDown
 * ---------------------------------------------------------------------- */

void setUp(void)
{
    /* nothing needed */
}

void tearDown(void)
{
    /* nothing needed */
}

/* =========================================================================
 * NULL / invalid-type tests
 * ====================================================================== */

void test_compare_both_null_returns_false(void)
{
    TEST_ASSERT_FALSE(cJSON_Compare(NULL, NULL, true));
}

void test_compare_first_null_returns_false(void)
{
    cJSON *b = cJSON_CreateNull();
    TEST_ASSERT_FALSE(cJSON_Compare(NULL, b, true));
    cJSON_Delete(b);
}

void test_compare_second_null_returns_false(void)
{
    cJSON *a = cJSON_CreateNull();
    TEST_ASSERT_FALSE(cJSON_Compare(a, NULL, true));
    cJSON_Delete(a);
}

void test_compare_different_types_returns_false(void)
{
    cJSON *a = cJSON_CreateTrue();
    cJSON *b = cJSON_CreateNumber(1.0);
    TEST_ASSERT_FALSE(cJSON_Compare(a, b, true));
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_invalid_type_returns_false(void)
{
    /* Build two nodes with type == 0 (cJSON_Invalid) */
    cJSON a_node, b_node;
    memset(&a_node, 0, sizeof(a_node));
    memset(&b_node, 0, sizeof(b_node));
    a_node.type = cJSON_Invalid;
    b_node.type = cJSON_Invalid;
    TEST_ASSERT_FALSE(cJSON_Compare(&a_node, &b_node, true));
}

/* =========================================================================
 * Identical pointer
 * ====================================================================== */

void test_compare_same_pointer_returns_true(void)
{
    cJSON *a = cJSON_CreateNumber(42.0);
    TEST_ASSERT_TRUE(cJSON_Compare(a, a, true));
    cJSON_Delete(a);
}

/* =========================================================================
 * cJSON_False / cJSON_True / cJSON_NULL
 * ====================================================================== */

void test_compare_false_false_returns_true(void)
{
    cJSON *a = cJSON_CreateFalse();
    cJSON *b = cJSON_CreateFalse();
    TEST_ASSERT_TRUE(cJSON_Compare(a, b, true));
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_true_true_returns_true(void)
{
    cJSON *a = cJSON_CreateTrue();
    cJSON *b = cJSON_CreateTrue();
    TEST_ASSERT_TRUE(cJSON_Compare(a, b, true));
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_null_null_returns_true(void)
{
    cJSON *a = cJSON_CreateNull();
    cJSON *b = cJSON_CreateNull();
    TEST_ASSERT_TRUE(cJSON_Compare(a, b, true));
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_true_false_returns_false(void)
{
    cJSON *a = cJSON_CreateTrue();
    cJSON *b = cJSON_CreateFalse();
    TEST_ASSERT_FALSE(cJSON_Compare(a, b, true));
    cJSON_Delete(a);
    cJSON_Delete(b);
}

/* =========================================================================
 * cJSON_Number
 * ====================================================================== */

void test_compare_equal_numbers_returns_true(void)
{
    cJSON *a = make_number(3.14);
    cJSON *b = make_number(3.14);
    TEST_ASSERT_TRUE(cJSON_Compare(a, b, true));
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_different_numbers_returns_false(void)
{
    cJSON *a = make_number(1.0);
    cJSON *b = make_number(2.0);
    TEST_ASSERT_FALSE(cJSON_Compare(a, b, true));
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_zero_numbers_returns_true(void)
{
    cJSON *a = make_number(0.0);
    cJSON *b = make_number(0.0);
    TEST_ASSERT_TRUE(cJSON_Compare(a, b, true));
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_negative_numbers_returns_true(void)
{
    cJSON *a = make_number(-99.5);
    cJSON *b = make_number(-99.5);
    TEST_ASSERT_TRUE(cJSON_Compare(a, b, true));
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_large_numbers_returns_true(void)
{
    cJSON *a = make_number(1e300);
    cJSON *b = make_number(1e300);
    TEST_ASSERT_TRUE(cJSON_Compare(a, b, true));
    cJSON_Delete(a);
    cJSON_Delete(b);
}

/* =========================================================================
 * cJSON_String
 * ====================================================================== */

void test_compare_equal_strings_returns_true(void)
{
    cJSON *a = make_string("hello");
    cJSON *b = make_string("hello");
    TEST_ASSERT_TRUE(cJSON_Compare(a, b, true));
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_different_strings_returns_false(void)
{
    cJSON *a = make_string("hello");
    cJSON *b = make_string("world");
    TEST_ASSERT_FALSE(cJSON_Compare(a, b, true));
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_empty_strings_returns_true(void)
{
    cJSON *a = make_string("");
    cJSON *b = make_string("");
    TEST_ASSERT_TRUE(cJSON_Compare(a, b, true));
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_string_case_sensitive_different_case_returns_false(void)
{
    cJSON *a = make_string("Hello");
    cJSON *b = make_string("hello");
    TEST_ASSERT_FALSE(cJSON_Compare(a, b, true));
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_string_null_valuestring_returns_false(void)
{
    cJSON *a = make_string("hello");
    cJSON *b = make_string("hello");
    /* Force one valuestring to NULL */
    free(b->valuestring);
    b->valuestring = NULL;
    TEST_ASSERT_FALSE(cJSON_Compare(a, b, true));
    cJSON_Delete(a);
    cJSON_Delete(b);
}

/* =========================================================================
 * cJSON_Raw
 * ====================================================================== */

void test_compare_equal_raw_returns_true(void)
{
    cJSON *a = cJSON_CreateRaw("true");
    cJSON *b = cJSON_CreateRaw("true");
    TEST_ASSERT_TRUE(cJSON_Compare(a, b, true));
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_different_raw_returns_false(void)
{
    cJSON *a = cJSON_CreateRaw("true");
    cJSON *b = cJSON_CreateRaw("false");
    TEST_ASSERT_FALSE(cJSON_Compare(a, b, true));
    cJSON_Delete(a);
    cJSON_Delete(b);
}

/* =========================================================================
 * cJSON_Array
 * ====================================================================== */

void test_compare_empty_arrays_returns_true(void)
{
    cJSON *a = cJSON_CreateArray();
    cJSON *b = cJSON_CreateArray();
    TEST_ASSERT_TRUE(cJSON_Compare(a, b, true));
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_equal_arrays_returns_true(void)
{
    cJSON *a = cJSON_CreateArray();
    cJSON *b = cJSON_CreateArray();
    cJSON_AddItemToArray(a, cJSON_CreateNumber(1.0));
    cJSON_AddItemToArray(a, cJSON_CreateString("x"));
    cJSON_AddItemToArray(b, cJSON_CreateNumber(1.0));
    cJSON_AddItemToArray(b, cJSON_CreateString("x"));
    TEST_ASSERT_TRUE(cJSON_Compare(a, b, true));
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_arrays_different_values_returns_false(void)
{
    cJSON *a = cJSON_CreateArray();
    cJSON *b = cJSON_CreateArray();
    cJSON_AddItemToArray(a, cJSON_CreateNumber(1.0));
    cJSON_AddItemToArray(b, cJSON_CreateNumber(2.0));
    TEST_ASSERT_FALSE(cJSON_Compare(a, b, true));
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_arrays_different_lengths_a_longer_returns_false(void)
{
    cJSON *a = cJSON_CreateArray();
    cJSON *b = cJSON_CreateArray();
    cJSON_AddItemToArray(a, cJSON_CreateNumber(1.0));
    cJSON_AddItemToArray(a, cJSON_CreateNumber(2.0));
    cJSON_AddItemToArray(b, cJSON_CreateNumber(1.0));
    TEST_ASSERT_FALSE(cJSON_Compare(a, b, true));
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_arrays_different_lengths_b_longer_returns_false(void)
{
    cJSON *a = cJSON_CreateArray();
    cJSON *b = cJSON_CreateArray();
    cJSON_AddItemToArray(a, cJSON_CreateNumber(1.0));
    cJSON_AddItemToArray(b, cJSON_CreateNumber(1.0));
    cJSON_AddItemToArray(b, cJSON_CreateNumber(2.0));
    TEST_ASSERT_FALSE(cJSON_Compare(a, b, true));
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_nested_arrays_equal_returns_true(void)
{
    cJSON *a = cJSON_CreateArray();
    cJSON *b = cJSON_CreateArray();
    cJSON *inner_a = cJSON_CreateArray();
    cJSON *inner_b = cJSON_CreateArray();
    cJSON_AddItemToArray(inner_a, cJSON_CreateNumber(42.0));
    cJSON_AddItemToArray(inner_b, cJSON_CreateNumber(42.0));
    cJSON_AddItemToArray(a, inner_a);
    cJSON_AddItemToArray(b, inner_b);
    TEST_ASSERT_TRUE(cJSON_Compare(a, b, true));
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_nested_arrays_different_returns_false(void)
{
    cJSON *a = cJSON_CreateArray();
    cJSON *b = cJSON_CreateArray();
    cJSON *inner_a = cJSON_CreateArray();
    cJSON *inner_b = cJSON_CreateArray();
    cJSON_AddItemToArray(inner_a, cJSON_CreateNumber(1.0));
    cJSON_AddItemToArray(inner_b, cJSON_CreateNumber(2.0));
    cJSON_AddItemToArray(a, inner_a);
    cJSON_AddItemToArray(b, inner_b);
    TEST_ASSERT_FALSE(cJSON_Compare(a, b, true));
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_array_with_mixed_types_equal_returns_true(void)
{
    cJSON *a = cJSON_CreateArray();
    cJSON *b = cJSON_CreateArray();
    cJSON_AddItemToArray(a, cJSON_CreateNull());
    cJSON_AddItemToArray(a, cJSON_CreateTrue());
    cJSON_AddItemToArray(a, cJSON_CreateFalse());
    cJSON_AddItemToArray(b, cJSON_CreateNull());
    cJSON_AddItemToArray(b, cJSON_CreateTrue());
    cJSON_AddItemToArray(b, cJSON_CreateFalse());
    TEST_ASSERT_TRUE(cJSON_Compare(a, b, true));
    cJSON_Delete(a);
    cJSON_Delete(b);
}

/* =========================================================================
 * cJSON_Object
 * ====================================================================== */

void test_compare_empty_objects_returns_true(void)
{
    cJSON *a = cJSON_CreateObject();
    cJSON *b = cJSON_CreateObject();
    TEST_ASSERT_TRUE(cJSON_Compare(a, b, true));
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_equal_objects_returns_true(void)
{
    cJSON *a = cJSON_CreateObject();
    cJSON *b = cJSON_CreateObject();
    cJSON_AddItemToObject(a, "key", cJSON_CreateNumber(1.0));
    cJSON_AddItemToObject(b, "key", cJSON_CreateNumber(1.0));
    TEST_ASSERT_TRUE(cJSON_Compare(a, b, true));
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_objects_different_values_returns_false(void)
{
    cJSON *a = cJSON_CreateObject();
    cJSON *b = cJSON_CreateObject();
    cJSON_AddItemToObject(a, "key", cJSON_CreateNumber(1.0));
    cJSON_AddItemToObject(b, "key", cJSON_CreateNumber(2.0));
    TEST_ASSERT_FALSE(cJSON_Compare(a, b, true));
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_objects_missing_key_in_b_returns_false(void)
{
    cJSON *a = cJSON_CreateObject();
    cJSON *b = cJSON_CreateObject();
    cJSON_AddItemToObject(a, "key1", cJSON_CreateNumber(1.0));
    cJSON_AddItemToObject(a, "key2", cJSON_CreateNumber(2.0));
    cJSON_AddItemToObject(b, "key1", cJSON_CreateNumber(1.0));
    TEST_ASSERT_FALSE(cJSON_Compare(a, b, true));
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_objects_missing_key_in_a_returns_false(void)
{
    cJSON *a = cJSON_CreateObject();
    cJSON *b = cJSON_CreateObject();
    cJSON_AddItemToObject(a, "key1", cJSON_CreateNumber(1.0));
    cJSON_AddItemToObject(b, "key1", cJSON_CreateNumber(1.0));
    cJSON_AddItemToObject(b, "key2", cJSON_CreateNumber(2.0));
    TEST_ASSERT_FALSE(cJSON_Compare(a, b, true));
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_objects_case_sensitive_different_key_case_returns_false(void)
{
    cJSON *a = cJSON_CreateObject();
    cJSON *b = cJSON_CreateObject();
    cJSON_AddItemToObject(a, "Key", cJSON_CreateNumber(1.0));
    cJSON_AddItemToObject(b, "key", cJSON_CreateNumber(1.0));
    TEST_ASSERT_FALSE(cJSON_Compare(a, b, true));
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_objects_case_insensitive_same_key_different_case_returns_true(void)
{
    cJSON *a = cJSON_CreateObject();
    cJSON *b = cJSON_CreateObject();
    cJSON_AddItemToObject(a, "Key", cJSON_CreateNumber(1.0));
    cJSON_AddItemToObject(b, "key", cJSON_CreateNumber(1.0));
    TEST_ASSERT_TRUE(cJSON_Compare(a, b, false));
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_objects_multiple_keys_equal_returns_true(void)
{
    cJSON *a = cJSON_CreateObject();
    cJSON *b = cJSON_CreateObject();
    cJSON_AddItemToObject(a, "name", cJSON_CreateString("Alice"));
    cJSON_AddItemToObject(a, "age",  cJSON_CreateNumber(30.0));
    cJSON_AddItemToObject(b, "name", cJSON_CreateString("Alice"));
    cJSON_AddItemToObject(b, "age",  cJSON_CreateNumber(30.0));
    TEST_ASSERT_TRUE(cJSON_Compare(a, b, true));
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_objects_subset_a_of_b_returns_false(void)
{
    /* a is a strict subset of b — must return false */
    cJSON *a = cJSON_CreateObject();
    cJSON *b = cJSON_CreateObject();
    cJSON_AddItemToObject(a, "x", cJSON_CreateNumber(1.0));
    cJSON_AddItemToObject(b, "x", cJSON_CreateNumber(1.0));
    cJSON_AddItemToObject(b, "y", cJSON_CreateNumber(2.0));
    TEST_ASSERT_FALSE(cJSON_Compare(a, b, true));
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_nested_objects_equal_returns_true(void)
{
    cJSON *a = cJSON_CreateObject();
    cJSON *b = cJSON_CreateObject();
    cJSON *inner_a = cJSON_CreateObject();
    cJSON *inner_b = cJSON_CreateObject();
    cJSON_AddItemToObject(inner_a, "val", cJSON_CreateNumber(7.0));
    cJSON_AddItemToObject(inner_b, "val", cJSON_CreateNumber(7.0));
    cJSON_AddItemToObject(a, "inner", inner_a);
    cJSON_AddItemToObject(b, "inner", inner_b);
    TEST_ASSERT_TRUE(cJSON_Compare(a, b, true));
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_nested_objects_different_returns_false(void)
{
    cJSON *a = cJSON_CreateObject();
    cJSON *b = cJSON_CreateObject();
    cJSON *inner_a = cJSON_CreateObject();
    cJSON *inner_b = cJSON_CreateObject();
    cJSON_AddItemToObject(inner_a, "val", cJSON_CreateNumber(7.0));
    cJSON_AddItemToObject(inner_b, "val", cJSON_CreateNumber(8.0));
    cJSON_AddItemToObject(a, "inner", inner_a);
    cJSON_AddItemToObject(b, "inner", inner_b);
    TEST_ASSERT_FALSE(cJSON_Compare(a, b, true));
    cJSON_Delete(a);
    cJSON_Delete(b);
}

/* =========================================================================
 * Mixed / complex structures
 * ====================================================================== */

void test_compare_complex_equal_structures_returns_true(void)
{
    cJSON *a = cJSON_Parse("{\"name\":\"Bob\",\"scores\":[1,2,3],\"active\":true}");
    cJSON *b = cJSON_Parse("{\"name\":\"Bob\",\"scores\":[1,2,3],\"active\":true}");
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    TEST_ASSERT_TRUE(cJSON_Compare(a, b, true));
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_complex_different_structures_returns_false(void)
{
    cJSON *a = cJSON_Parse("{\"name\":\"Bob\",\"scores\":[1,2,3]}");
    cJSON *b = cJSON_Parse("{\"name\":\"Bob\",\"scores\":[1,2,4]}");
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    TEST_ASSERT_FALSE(cJSON_Compare(a, b, true));
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_array_vs_object_returns_false(void)
{
    cJSON *a = cJSON_CreateArray();
    cJSON *b = cJSON_CreateObject();
    TEST_ASSERT_FALSE(cJSON_Compare(a, b, true));
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_number_vs_string_returns_false(void)
{
    cJSON *a = make_number(1.0);
    cJSON *b = make_string("1");
    TEST_ASSERT_FALSE(cJSON_Compare(a, b, true));
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_bool_vs_null_returns_false(void)
{
    cJSON *a = cJSON_CreateTrue();
    cJSON *b = cJSON_CreateNull();
    TEST_ASSERT_FALSE(cJSON_Compare(a, b, true));
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_deeply_nested_equal_returns_true(void)
{
    cJSON *a = cJSON_Parse("{\"a\":{\"b\":{\"c\":42}}}");
    cJSON *b = cJSON_Parse("{\"a\":{\"b\":{\"c\":42}}}");
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    TEST_ASSERT_TRUE(cJSON_Compare(a, b, true));
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_deeply_nested_different_returns_false(void)
{
    cJSON *a = cJSON_Parse("{\"a\":{\"b\":{\"c\":42}}}");
    cJSON *b = cJSON_Parse("{\"a\":{\"b\":{\"c\":99}}}");
    TEST_ASSERT_NOT_NULL(a);
    TEST_ASSERT_NOT_NULL(b);
    TEST_ASSERT_FALSE(cJSON_Compare(a, b, true));
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_object_with_array_value_equal_returns_true(void)
{
    cJSON *a = cJSON_CreateObject();
    cJSON *b = cJSON_CreateObject();
    cJSON *arr_a = cJSON_CreateArray();
    cJSON *arr_b = cJSON_CreateArray();
    cJSON_AddItemToArray(arr_a, cJSON_CreateString("foo"));
    cJSON_AddItemToArray(arr_b, cJSON_CreateString("foo"));
    cJSON_AddItemToObject(a, "list", arr_a);
    cJSON_AddItemToObject(b, "list", arr_b);
    TEST_ASSERT_TRUE(cJSON_Compare(a, b, true));
    cJSON_Delete(a);
    cJSON_Delete(b);
}

void test_compare_object_with_null_value_equal_returns_true(void)
{
    cJSON *a = cJSON_CreateObject();
    cJSON *b = cJSON_CreateObject();
    cJSON_AddItemToObject(a, "n", cJSON_CreateNull());
    cJSON_AddItemToObject(b, "n", cJSON_CreateNull());
    TEST_ASSERT_TRUE(cJSON_Compare(a, b, true));
    cJSON_Delete(a);
    cJSON_Delete(b);
}

/* =========================================================================
 * main
 * ====================================================================== */

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();

    RUN_TEST(test_compare_both_null_returns_false);
    RUN_TEST(test_compare_first_null_returns_false);
    RUN_TEST(test_compare_second_null_returns_false);
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
    RUN_TEST(test_compare_large_numbers_returns_true);

    RUN_TEST(test_compare_equal_strings_returns_true);
    RUN_TEST(test_compare_different_strings_returns_false);
    RUN_TEST(test_compare_empty_strings_returns_true);
    RUN_TEST(test_compare_string_case_sensitive_different_case_returns_false);
    RUN_TEST(test_compare_string_null_valuestring_returns_false);

    RUN_TEST(test_compare_equal_raw_returns_true);
    RUN_TEST(test_compare_different_raw_returns_false);

    RUN_TEST(test_compare_empty_arrays_returns_true);
    RUN_TEST(test_compare_equal_arrays_returns_true);
    RUN_TEST(test_compare_arrays_different_values_returns_false);
    RUN_TEST(test_compare_arrays_different_lengths_a_longer_returns_false);
    RUN_TEST(test_compare_arrays_different_lengths_b_longer_returns_false);
    RUN_TEST(test_compare_nested_arrays_equal_returns_true);
    RUN_TEST(test_compare_nested_arrays_different_returns_false);
    RUN_TEST(test_compare_array_with_mixed_types_equal_returns_true);

    RUN_TEST(test_compare_empty_objects_returns_true);
    RUN_TEST(test_compare_equal_objects_returns_true);
    RUN_TEST(test_compare_objects_different_values_returns_false);
    RUN_TEST(test_compare_objects_missing_key_in_b_returns_false);
    RUN_TEST(test_compare_objects_missing_key_in_a_returns_false);
    RUN_TEST(test_compare_objects_case_sensitive_different_key_case_returns_false);
    RUN_TEST(test_compare_objects_case_insensitive_same_key_different_case_returns_true);
    RUN_TEST(test_compare_objects_multiple_keys_equal_returns_true);
    RUN_TEST(test_compare_objects_subset_a_of_b_returns_false);
    RUN_TEST(test_compare_nested_objects_equal_returns_true);
    RUN_TEST(test_compare_nested_objects_different_returns_false);

    RUN_TEST(test_compare_complex_equal_structures_returns_true);
    RUN_TEST(test_compare_complex_different_structures_returns_false);
    RUN_TEST(test_compare_array_vs_object_returns_false);
    RUN_TEST(test_compare_number_vs_string_returns_false);
    RUN_TEST(test_compare_bool_vs_null_returns_false);
    RUN_TEST(test_compare_deeply_nested_equal_returns_true);
    RUN_TEST(test_compare_deeply_nested_different_returns_false);
    RUN_TEST(test_compare_object_with_array_value_equal_returns_true);
    RUN_TEST(test_compare_object_with_null_value_equal_returns_true);

    return UNITY_END();
}