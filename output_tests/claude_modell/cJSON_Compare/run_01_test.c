#include "cJSON.c"
#include "unity.h"
#include "cJSON.h"
#include <stdlib.h>
#include <string.h>

/* File-scope variables */
static cJSON *item_a = NULL;
static cJSON *item_b = NULL;

/* setUp and tearDown */
void setUp(void)
{
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
}

/* ===== Helper macros ===== */
#define ASSERT_COMPARE_TRUE(a, b, cs)  TEST_ASSERT_TRUE(cJSON_Compare((a), (b), (cs)))
#define ASSERT_COMPARE_FALSE(a, b, cs) TEST_ASSERT_FALSE(cJSON_Compare((a), (b), (cs)))

/* ===== Test cases ===== */

/* NULL handling */
void test_compare_null_a_returns_false(void)
{
    item_b = cJSON_CreateNull();
    TEST_ASSERT_NOT_NULL(item_b);
    ASSERT_COMPARE_FALSE(NULL, item_b, true);
}

void test_compare_null_b_returns_false(void)
{
    item_a = cJSON_CreateNull();
    TEST_ASSERT_NOT_NULL(item_a);
    ASSERT_COMPARE_FALSE(item_a, NULL, true);
}

void test_compare_both_null_pointers_returns_false(void)
{
    ASSERT_COMPARE_FALSE(NULL, NULL, true);
}

/* Identical pointer */
void test_compare_same_pointer_returns_true(void)
{
    item_a = cJSON_CreateNumber(42.0);
    TEST_ASSERT_NOT_NULL(item_a);
    ASSERT_COMPARE_TRUE(item_a, item_a, true);
}

/* Type mismatch */
void test_compare_different_types_returns_false(void)
{
    item_a = cJSON_CreateNumber(1.0);
    item_b = cJSON_CreateString("1");
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);
    ASSERT_COMPARE_FALSE(item_a, item_b, true);
}

void test_compare_true_vs_false_returns_false(void)
{
    item_a = cJSON_CreateTrue();
    item_b = cJSON_CreateFalse();
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);
    ASSERT_COMPARE_FALSE(item_a, item_b, true);
}

void test_compare_null_vs_false_returns_false(void)
{
    item_a = cJSON_CreateNull();
    item_b = cJSON_CreateFalse();
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);
    ASSERT_COMPARE_FALSE(item_a, item_b, true);
}

/* cJSON_False */
void test_compare_false_false_returns_true(void)
{
    item_a = cJSON_CreateFalse();
    item_b = cJSON_CreateFalse();
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);
    ASSERT_COMPARE_TRUE(item_a, item_b, true);
}

/* cJSON_True */
void test_compare_true_true_returns_true(void)
{
    item_a = cJSON_CreateTrue();
    item_b = cJSON_CreateTrue();
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);
    ASSERT_COMPARE_TRUE(item_a, item_b, true);
}

/* cJSON_NULL */
void test_compare_null_null_returns_true(void)
{
    item_a = cJSON_CreateNull();
    item_b = cJSON_CreateNull();
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);
    ASSERT_COMPARE_TRUE(item_a, item_b, true);
}

/* cJSON_Number */
void test_compare_equal_numbers_returns_true(void)
{
    item_a = cJSON_CreateNumber(3.14);
    item_b = cJSON_CreateNumber(3.14);
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);
    ASSERT_COMPARE_TRUE(item_a, item_b, true);
}

void test_compare_different_numbers_returns_false(void)
{
    item_a = cJSON_CreateNumber(1.0);
    item_b = cJSON_CreateNumber(2.0);
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);
    ASSERT_COMPARE_FALSE(item_a, item_b, true);
}

void test_compare_zero_numbers_returns_true(void)
{
    item_a = cJSON_CreateNumber(0.0);
    item_b = cJSON_CreateNumber(0.0);
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);
    ASSERT_COMPARE_TRUE(item_a, item_b, true);
}

void test_compare_negative_numbers_returns_true(void)
{
    item_a = cJSON_CreateNumber(-99.5);
    item_b = cJSON_CreateNumber(-99.5);
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);
    ASSERT_COMPARE_TRUE(item_a, item_b, true);
}

void test_compare_negative_vs_positive_returns_false(void)
{
    item_a = cJSON_CreateNumber(-1.0);
    item_b = cJSON_CreateNumber(1.0);
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);
    ASSERT_COMPARE_FALSE(item_a, item_b, true);
}

/* cJSON_String */
void test_compare_equal_strings_returns_true(void)
{
    item_a = cJSON_CreateString("hello");
    item_b = cJSON_CreateString("hello");
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);
    ASSERT_COMPARE_TRUE(item_a, item_b, true);
}

void test_compare_different_strings_returns_false(void)
{
    item_a = cJSON_CreateString("hello");
    item_b = cJSON_CreateString("world");
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);
    ASSERT_COMPARE_FALSE(item_a, item_b, true);
}

void test_compare_empty_strings_returns_true(void)
{
    item_a = cJSON_CreateString("");
    item_b = cJSON_CreateString("");
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);
    ASSERT_COMPARE_TRUE(item_a, item_b, true);
}

void test_compare_string_case_sensitive_different_case_returns_false(void)
{
    item_a = cJSON_CreateString("Hello");
    item_b = cJSON_CreateString("hello");
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);
    ASSERT_COMPARE_FALSE(item_a, item_b, true);
}

/* cJSON_Raw */
void test_compare_equal_raw_returns_true(void)
{
    item_a = cJSON_CreateRaw("{\"key\":1}");
    item_b = cJSON_CreateRaw("{\"key\":1}");
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);
    ASSERT_COMPARE_TRUE(item_a, item_b, true);
}

void test_compare_different_raw_returns_false(void)
{
    item_a = cJSON_CreateRaw("{\"key\":1}");
    item_b = cJSON_CreateRaw("{\"key\":2}");
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);
    ASSERT_COMPARE_FALSE(item_a, item_b, true);
}

/* cJSON_Array */
void test_compare_empty_arrays_returns_true(void)
{
    item_a = cJSON_CreateArray();
    item_b = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);
    ASSERT_COMPARE_TRUE(item_a, item_b, true);
}

void test_compare_equal_arrays_returns_true(void)
{
    item_a = cJSON_CreateArray();
    item_b = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);

    cJSON_AddItemToArray(item_a, cJSON_CreateNumber(1.0));
    cJSON_AddItemToArray(item_a, cJSON_CreateString("two"));
    cJSON_AddItemToArray(item_a, cJSON_CreateTrue());

    cJSON_AddItemToArray(item_b, cJSON_CreateNumber(1.0));
    cJSON_AddItemToArray(item_b, cJSON_CreateString("two"));
    cJSON_AddItemToArray(item_b, cJSON_CreateTrue());

    ASSERT_COMPARE_TRUE(item_a, item_b, true);
}

void test_compare_different_arrays_returns_false(void)
{
    item_a = cJSON_CreateArray();
    item_b = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);

    cJSON_AddItemToArray(item_a, cJSON_CreateNumber(1.0));
    cJSON_AddItemToArray(item_b, cJSON_CreateNumber(2.0));

    ASSERT_COMPARE_FALSE(item_a, item_b, true);
}

void test_compare_arrays_different_length_a_longer_returns_false(void)
{
    item_a = cJSON_CreateArray();
    item_b = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);

    cJSON_AddItemToArray(item_a, cJSON_CreateNumber(1.0));
    cJSON_AddItemToArray(item_a, cJSON_CreateNumber(2.0));
    cJSON_AddItemToArray(item_b, cJSON_CreateNumber(1.0));

    ASSERT_COMPARE_FALSE(item_a, item_b, true);
}

void test_compare_arrays_different_length_b_longer_returns_false(void)
{
    item_a = cJSON_CreateArray();
    item_b = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);

    cJSON_AddItemToArray(item_a, cJSON_CreateNumber(1.0));
    cJSON_AddItemToArray(item_b, cJSON_CreateNumber(1.0));
    cJSON_AddItemToArray(item_b, cJSON_CreateNumber(2.0));

    ASSERT_COMPARE_FALSE(item_a, item_b, true);
}

void test_compare_nested_arrays_equal_returns_true(void)
{
    item_a = cJSON_CreateArray();
    item_b = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);

    cJSON *inner_a = cJSON_CreateArray();
    cJSON *inner_b = cJSON_CreateArray();
    cJSON_AddItemToArray(inner_a, cJSON_CreateNumber(42.0));
    cJSON_AddItemToArray(inner_b, cJSON_CreateNumber(42.0));

    cJSON_AddItemToArray(item_a, inner_a);
    cJSON_AddItemToArray(item_b, inner_b);

    ASSERT_COMPARE_TRUE(item_a, item_b, true);
}

void test_compare_nested_arrays_different_returns_false(void)
{
    item_a = cJSON_CreateArray();
    item_b = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);

    cJSON *inner_a = cJSON_CreateArray();
    cJSON *inner_b = cJSON_CreateArray();
    cJSON_AddItemToArray(inner_a, cJSON_CreateNumber(1.0));
    cJSON_AddItemToArray(inner_b, cJSON_CreateNumber(2.0));

    cJSON_AddItemToArray(item_a, inner_a);
    cJSON_AddItemToArray(item_b, inner_b);

    ASSERT_COMPARE_FALSE(item_a, item_b, true);
}

/* cJSON_Object */
void test_compare_empty_objects_returns_true(void)
{
    item_a = cJSON_CreateObject();
    item_b = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);
    ASSERT_COMPARE_TRUE(item_a, item_b, true);
}

void test_compare_equal_objects_returns_true(void)
{
    item_a = cJSON_CreateObject();
    item_b = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);

    cJSON_AddItemToObject(item_a, "key1", cJSON_CreateNumber(1.0));
    cJSON_AddItemToObject(item_a, "key2", cJSON_CreateString("value"));

    cJSON_AddItemToObject(item_b, "key1", cJSON_CreateNumber(1.0));
    cJSON_AddItemToObject(item_b, "key2", cJSON_CreateString("value"));

    ASSERT_COMPARE_TRUE(item_a, item_b, true);
}

void test_compare_objects_different_values_returns_false(void)
{
    item_a = cJSON_CreateObject();
    item_b = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);

    cJSON_AddItemToObject(item_a, "key", cJSON_CreateNumber(1.0));
    cJSON_AddItemToObject(item_b, "key", cJSON_CreateNumber(2.0));

    ASSERT_COMPARE_FALSE(item_a, item_b, true);
}

void test_compare_objects_missing_key_in_b_returns_false(void)
{
    item_a = cJSON_CreateObject();
    item_b = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);

    cJSON_AddItemToObject(item_a, "key1", cJSON_CreateNumber(1.0));
    cJSON_AddItemToObject(item_a, "key2", cJSON_CreateNumber(2.0));
    cJSON_AddItemToObject(item_b, "key1", cJSON_CreateNumber(1.0));

    ASSERT_COMPARE_FALSE(item_a, item_b, true);
}

void test_compare_objects_missing_key_in_a_returns_false(void)
{
    item_a = cJSON_CreateObject();
    item_b = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);

    cJSON_AddItemToObject(item_a, "key1", cJSON_CreateNumber(1.0));
    cJSON_AddItemToObject(item_b, "key1", cJSON_CreateNumber(1.0));
    cJSON_AddItemToObject(item_b, "key2", cJSON_CreateNumber(2.0));

    ASSERT_COMPARE_FALSE(item_a, item_b, true);
}

void test_compare_objects_case_sensitive_key_mismatch_returns_false(void)
{
    item_a = cJSON_CreateObject();
    item_b = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);

    cJSON_AddItemToObject(item_a, "Key", cJSON_CreateNumber(1.0));
    cJSON_AddItemToObject(item_b, "key", cJSON_CreateNumber(1.0));

    ASSERT_COMPARE_FALSE(item_a, item_b, true);
}

void test_compare_objects_case_insensitive_key_match_returns_true(void)
{
    item_a = cJSON_CreateObject();
    item_b = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);

    cJSON_AddItemToObject(item_a, "Key", cJSON_CreateNumber(1.0));
    cJSON_AddItemToObject(item_b, "key", cJSON_CreateNumber(1.0));

    ASSERT_COMPARE_TRUE(item_a, item_b, false);
}

void test_compare_objects_different_order_same_content_returns_true(void)
{
    item_a = cJSON_CreateObject();
    item_b = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);

    cJSON_AddItemToObject(item_a, "alpha", cJSON_CreateNumber(1.0));
    cJSON_AddItemToObject(item_a, "beta",  cJSON_CreateNumber(2.0));

    cJSON_AddItemToObject(item_b, "beta",  cJSON_CreateNumber(2.0));
    cJSON_AddItemToObject(item_b, "alpha", cJSON_CreateNumber(1.0));

    ASSERT_COMPARE_TRUE(item_a, item_b, true);
}

void test_compare_nested_objects_equal_returns_true(void)
{
    item_a = cJSON_CreateObject();
    item_b = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);

    cJSON *inner_a = cJSON_CreateObject();
    cJSON *inner_b = cJSON_CreateObject();
    cJSON_AddItemToObject(inner_a, "x", cJSON_CreateNumber(10.0));
    cJSON_AddItemToObject(inner_b, "x", cJSON_CreateNumber(10.0));

    cJSON_AddItemToObject(item_a, "nested", inner_a);
    cJSON_AddItemToObject(item_b, "nested", inner_b);

    ASSERT_COMPARE_TRUE(item_a, item_b, true);
}

void test_compare_nested_objects_different_returns_false(void)
{
    item_a = cJSON_CreateObject();
    item_b = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);

    cJSON *inner_a = cJSON_CreateObject();
    cJSON *inner_b = cJSON_CreateObject();
    cJSON_AddItemToObject(inner_a, "x", cJSON_CreateNumber(10.0));
    cJSON_AddItemToObject(inner_b, "x", cJSON_CreateNumber(99.0));

    cJSON_AddItemToObject(item_a, "nested", inner_a);
    cJSON_AddItemToObject(item_b, "nested", inner_b);

    ASSERT_COMPARE_FALSE(item_a, item_b, true);
}

/* Mixed nested structures */
void test_compare_object_with_array_value_equal_returns_true(void)
{
    item_a = cJSON_CreateObject();
    item_b = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);

    cJSON *arr_a = cJSON_CreateArray();
    cJSON *arr_b = cJSON_CreateArray();
    cJSON_AddItemToArray(arr_a, cJSON_CreateNumber(1.0));
    cJSON_AddItemToArray(arr_a, cJSON_CreateNumber(2.0));
    cJSON_AddItemToArray(arr_b, cJSON_CreateNumber(1.0));
    cJSON_AddItemToArray(arr_b, cJSON_CreateNumber(2.0));

    cJSON_AddItemToObject(item_a, "arr", arr_a);
    cJSON_AddItemToObject(item_b, "arr", arr_b);

    ASSERT_COMPARE_TRUE(item_a, item_b, true);
}

void test_compare_object_with_array_value_different_returns_false(void)
{
    item_a = cJSON_CreateObject();
    item_b = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);

    cJSON *arr_a = cJSON_CreateArray();
    cJSON *arr_b = cJSON_CreateArray();
    cJSON_AddItemToArray(arr_a, cJSON_CreateNumber(1.0));
    cJSON_AddItemToArray(arr_b, cJSON_CreateNumber(9.0));

    cJSON_AddItemToObject(item_a, "arr", arr_a);
    cJSON_AddItemToObject(item_b, "arr", arr_b);

    ASSERT_COMPARE_FALSE(item_a, item_b, true);
}

/* Bool created via cJSON_CreateBool */
void test_compare_bool_true_created_via_create_bool_returns_true(void)
{
    item_a = cJSON_CreateBool(1);
    item_b = cJSON_CreateBool(1);
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);
    ASSERT_COMPARE_TRUE(item_a, item_b, true);
}

void test_compare_bool_false_created_via_create_bool_returns_true(void)
{
    item_a = cJSON_CreateBool(0);
    item_b = cJSON_CreateBool(0);
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);
    ASSERT_COMPARE_TRUE(item_a, item_b, true);
}

/* Array of booleans */
void test_compare_array_of_booleans_equal_returns_true(void)
{
    item_a = cJSON_CreateArray();
    item_b = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);

    cJSON_AddItemToArray(item_a, cJSON_CreateTrue());
    cJSON_AddItemToArray(item_a, cJSON_CreateFalse());
    cJSON_AddItemToArray(item_a, cJSON_CreateNull());

    cJSON_AddItemToArray(item_b, cJSON_CreateTrue());
    cJSON_AddItemToArray(item_b, cJSON_CreateFalse());
    cJSON_AddItemToArray(item_b, cJSON_CreateNull());

    ASSERT_COMPARE_TRUE(item_a, item_b, true);
}

void test_compare_array_of_booleans_different_returns_false(void)
{
    item_a = cJSON_CreateArray();
    item_b = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);

    cJSON_AddItemToArray(item_a, cJSON_CreateTrue());
    cJSON_AddItemToArray(item_b, cJSON_CreateFalse());

    ASSERT_COMPARE_FALSE(item_a, item_b, true);
}

/* Large number comparison */
void test_compare_large_equal_numbers_returns_true(void)
{
    item_a = cJSON_CreateNumber(1e300);
    item_b = cJSON_CreateNumber(1e300);
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);
    ASSERT_COMPARE_TRUE(item_a, item_b, true);
}

void test_compare_large_different_numbers_returns_false(void)
{
    item_a = cJSON_CreateNumber(1e300);
    item_b = cJSON_CreateNumber(2e300);
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);
    ASSERT_COMPARE_FALSE(item_a, item_b, true);
}

/* String with special characters */
void test_compare_strings_with_special_chars_equal_returns_true(void)
{
    item_a = cJSON_CreateString("hello\nworld\t!");
    item_b = cJSON_CreateString("hello\nworld\t!");
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);
    ASSERT_COMPARE_TRUE(item_a, item_b, true);
}

void test_compare_strings_with_special_chars_different_returns_false(void)
{
    item_a = cJSON_CreateString("hello\nworld");
    item_b = cJSON_CreateString("hello world");
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);
    ASSERT_COMPARE_FALSE(item_a, item_b, true);
}

/* Object with multiple types of values */
void test_compare_complex_objects_equal_returns_true(void)
{
    item_a = cJSON_CreateObject();
    item_b = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);

    cJSON_AddItemToObject(item_a, "num",  cJSON_CreateNumber(3.14));
    cJSON_AddItemToObject(item_a, "str",  cJSON_CreateString("test"));
    cJSON_AddItemToObject(item_a, "bool", cJSON_CreateTrue());
    cJSON_AddItemToObject(item_a, "null", cJSON_CreateNull());

    cJSON_AddItemToObject(item_b, "num",  cJSON_CreateNumber(3.14));
    cJSON_AddItemToObject(item_b, "str",  cJSON_CreateString("test"));
    cJSON_AddItemToObject(item_b, "bool", cJSON_CreateTrue());
    cJSON_AddItemToObject(item_b, "null", cJSON_CreateNull());

    ASSERT_COMPARE_TRUE(item_a, item_b, true);
}

void test_compare_complex_objects_one_field_different_returns_false(void)
{
    item_a = cJSON_CreateObject();
    item_b = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);

    cJSON_AddItemToObject(item_a, "num",  cJSON_CreateNumber(3.14));
    cJSON_AddItemToObject(item_a, "str",  cJSON_CreateString("test"));

    cJSON_AddItemToObject(item_b, "num",  cJSON_CreateNumber(3.14));
    cJSON_AddItemToObject(item_b, "str",  cJSON_CreateString("different"));

    ASSERT_COMPARE_FALSE(item_a, item_b, true);
}

/* Parsed JSON comparison */
void test_compare_parsed_equal_json_returns_true(void)
{
    item_a = cJSON_Parse("{\"a\":1,\"b\":\"hello\",\"c\":true}");
    item_b = cJSON_Parse("{\"a\":1,\"b\":\"hello\",\"c\":true}");
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);
    ASSERT_COMPARE_TRUE(item_a, item_b, true);
}

void test_compare_parsed_different_json_returns_false(void)
{
    item_a = cJSON_Parse("{\"a\":1,\"b\":\"hello\"}");
    item_b = cJSON_Parse("{\"a\":1,\"b\":\"world\"}");
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);
    ASSERT_COMPARE_FALSE(item_a, item_b, true);
}

void test_compare_parsed_array_equal_returns_true(void)
{
    item_a = cJSON_Parse("[1,2,3,\"four\",true,null]");
    item_b = cJSON_Parse("[1,2,3,\"four\",true,null]");
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);
    ASSERT_COMPARE_TRUE(item_a, item_b, true);
}

void test_compare_parsed_array_different_returns_false(void)
{
    item_a = cJSON_Parse("[1,2,3]");
    item_b = cJSON_Parse("[1,2,4]");
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);
    ASSERT_COMPARE_FALSE(item_a, item_b, true);
}

/* Subset check: a is subset of b should return false */
void test_compare_object_subset_returns_false(void)
{
    item_a = cJSON_CreateObject();
    item_b = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);

    cJSON_AddItemToObject(item_a, "key1", cJSON_CreateNumber(1.0));

    cJSON_AddItemToObject(item_b, "key1", cJSON_CreateNumber(1.0));
    cJSON_AddItemToObject(item_b, "key2", cJSON_CreateNumber(2.0));

    ASSERT_COMPARE_FALSE(item_a, item_b, true);
}

/* Case insensitive object comparison */
void test_compare_objects_case_insensitive_equal_returns_true(void)
{
    item_a = cJSON_CreateObject();
    item_b = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);

    cJSON_AddItemToObject(item_a, "MyKey", cJSON_CreateNumber(42.0));
    cJSON_AddItemToObject(item_b, "mykey", cJSON_CreateNumber(42.0));

    ASSERT_COMPARE_TRUE(item_a, item_b, false);
}

void test_compare_objects_case_sensitive_different_case_returns_false(void)
{
    item_a = cJSON_CreateObject();
    item_b = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);

    cJSON_AddItemToObject(item_a, "MyKey", cJSON_CreateNumber(42.0));
    cJSON_AddItemToObject(item_b, "mykey", cJSON_CreateNumber(42.0));

    ASSERT_COMPARE_FALSE(item_a, item_b, true);
}

/* Array with null element type mismatch */
void test_compare_array_with_null_and_false_elements_returns_false(void)
{
    item_a = cJSON_CreateArray();
    item_b = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);

    cJSON_AddItemToArray(item_a, cJSON_CreateNull());
    cJSON_AddItemToArray(item_b, cJSON_CreateFalse());

    ASSERT_COMPARE_FALSE(item_a, item_b, true);
}

/* Single element arrays */
void test_compare_single_element_arrays_equal_returns_true(void)
{
    item_a = cJSON_CreateArray();
    item_b = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);

    cJSON_AddItemToArray(item_a, cJSON_CreateString("only"));
    cJSON_AddItemToArray(item_b, cJSON_CreateString("only"));

    ASSERT_COMPARE_TRUE(item_a, item_b, true);
}

void test_compare_single_element_arrays_different_returns_false(void)
{
    item_a = cJSON_CreateArray();
    item_b = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(item_a);
    TEST_ASSERT_NOT_NULL(item_b);

    cJSON_AddItemToArray(item_a, cJSON_CreateString("one"));
    cJSON_AddItemToArray(item_b, cJSON_CreateString("two"));

    ASSERT_COMPARE_FALSE(item_a, item_b, true);
}

/* main */
int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();

    RUN_TEST(test_compare_null_a_returns_false);
    RUN_TEST(test_compare_null_b_returns_false);
    RUN_TEST(test_compare_both_null_pointers_returns_false);
    RUN_TEST(test_compare_same_pointer_returns_true);
    RUN_TEST(test_compare_different_types_returns_false);
    RUN_TEST(test_compare_true_vs_false_returns_false);
    RUN_TEST(test_compare_null_vs_false_returns_false);
    RUN_TEST(test_compare_false_false_returns_true);
    RUN_TEST(test_compare_true_true_returns_true);
    RUN_TEST(test_compare_null_null_returns_true);
    RUN_TEST(test_compare_equal_numbers_returns_true);
    RUN_TEST(test_compare_different_numbers_returns_false);
    RUN_TEST(test_compare_zero_numbers_returns_true);
    RUN_TEST(test_compare_negative_numbers_returns_true);
    RUN_TEST(test_compare_negative_vs_positive_returns_false);
    RUN_TEST(test_compare_equal_strings_returns_true);
    RUN_TEST(test_compare_different_strings_returns_false);
    RUN_TEST(test_compare_empty_strings_returns_true);
    RUN_TEST(test_compare_string_case_sensitive_different_case_returns_false);
    RUN_TEST(test_compare_equal_raw_returns_true);
    RUN_TEST(test_compare_different_raw_returns_false);
    RUN_TEST(test_compare_empty_arrays_returns_true);
    RUN_TEST(test_compare_equal_arrays_returns_true);
    RUN_TEST(test_compare_different_arrays_returns_false);
    RUN_TEST(test_compare_arrays_different_length_a_longer_returns_false);
    RUN_TEST(test_compare_arrays_different_length_b_longer_returns_false);
    RUN_TEST(test_compare_nested_arrays_equal_returns_true);
    RUN_TEST(test_compare_nested_arrays_different_returns_false);
    RUN_TEST(test_compare_empty_objects_returns_true);
    RUN_TEST(test_compare_equal_objects_returns_true);
    RUN_TEST(test_compare_objects_different_values_returns_false);
    RUN_TEST(test_compare_objects_missing_key_in_b_returns_false);
    RUN_TEST(test_compare_objects_missing_key_in_a_returns_false);
    RUN_TEST(test_compare_objects_case_sensitive_key_mismatch_returns_false);
    RUN_TEST(test_compare_objects_case_insensitive_key_match_returns_true);
    RUN_TEST(test_compare_objects_different_order_same_content_returns_true);
    RUN_TEST(test_compare_nested_objects_equal_returns_true);
    RUN_TEST(test_compare_nested_objects_different_returns_false);
    RUN_TEST(test_compare_object_with_array_value_equal_returns_true);
    RUN_TEST(test_compare_object_with_array_value_different_returns_false);
    RUN_TEST(test_compare_bool_true_created_via_create_bool_returns_true);
    RUN_TEST(test_compare_bool_false_created_via_create_bool_returns_true);
    RUN_TEST(test_compare_array_of_booleans_equal_returns_true);
    RUN_TEST(test_compare_array_of_booleans_different_returns_false);
    RUN_TEST(test_compare_large_equal_numbers_returns_true);
    RUN_TEST(test_compare_large_different_numbers_returns_false);
    RUN_TEST(test_compare_strings_with_special_chars_equal_returns_true);
    RUN_TEST(test_compare_strings_with_special_chars_different_returns_false);
    RUN_TEST(test_compare_complex_objects_equal_returns_true);
    RUN_TEST(test_compare_complex_objects_one_field_different_returns_false);
    RUN_TEST(test_compare_parsed_equal_json_returns_true);
    RUN_TEST(test_compare_parsed_different_json_returns_false);
    RUN_TEST(test_compare_parsed_array_equal_returns_true);
    RUN_TEST(test_compare_parsed_array_different_returns_false);
    RUN_TEST(test_compare_object_subset_returns_false);
    RUN_TEST(test_compare_objects_case_insensitive_equal_returns_true);
    RUN_TEST(test_compare_objects_case_sensitive_different_case_returns_false);
    RUN_TEST(test_compare_array_with_null_and_false_elements_returns_false);
    RUN_TEST(test_compare_single_element_arrays_equal_returns_true);
    RUN_TEST(test_compare_single_element_arrays_different_returns_false);

    return UNITY_END();
}