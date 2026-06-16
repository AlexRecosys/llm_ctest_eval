#include "cJSON.c"
#include "unity.h"

/* Helper macros and functions */
static cJSON_bool compare_double(double a, double b)
{
    const double epsilon = 1e-9;
    if (a == b)
    {
        return true;
    }
    if (a == 0.0 || b == 0.0)
    {
        return (a == 0.0 && b == 0.0);
    }
    return (cJSON_fabs(a - b) < epsilon * cJSON_fabs(a)) && (cJSON_fabs(a - b) < epsilon * cJSON_fabs(b));
}

/* Helper to create cJSON objects for testing */
static cJSON* create_test_number(double value)
{
    cJSON* num = cJSON_CreateNumber(value);
    TEST_ASSERT_NOT_NULL(num);
    return num;
}

static cJSON* create_test_string(const char* str)
{
    cJSON* s = cJSON_CreateString(str);
    TEST_ASSERT_NOT_NULL(s);
    return s;
}

static cJSON* create_test_array(void)
{
    cJSON* arr = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(arr);
    return arr;
}

static cJSON* create_test_object(void)
{
    cJSON* obj = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(obj);
    return obj;
}

/* Test fixtures */
static cJSON* test_a = NULL;
static cJSON* test_b = NULL;

void setUp(void)
{
    test_a = NULL;
    test_b = NULL;
}

void tearDown(void)
{
    if (test_a != NULL)
    {
        cJSON_Delete(test_a);
        test_a = NULL;
    }
    if (test_b != NULL)
    {
        cJSON_Delete(test_b);
        test_b = NULL;
    }
}

/* Test cases */

void test_cJSON_Compare_NULL_inputs(void)
{
    TEST_ASSERT_FALSE(cJSON_Compare(NULL, NULL, cJSON_True));
    TEST_ASSERT_FALSE(cJSON_Compare(test_a, NULL, cJSON_True));
    TEST_ASSERT_FALSE(cJSON_Compare(NULL, test_b, cJSON_True));
}

void test_cJSON_Compare_different_types(void)
{
    test_a = cJSON_CreateNumber(42);
    test_b = cJSON_CreateString("42");
    TEST_ASSERT_FALSE(cJSON_Compare(test_a, test_b, cJSON_True));
}

void test_cJSON_Compare_invalid_type(void)
{
    cJSON item_a = {0}, item_b = {0};
    item_a.type = 0xFF; /* invalid type */
    item_b.type = 0xFF;
    TEST_ASSERT_FALSE(cJSON_Compare(&item_a, &item_b, cJSON_True));
}

void test_cJSON_Compare_identical_pointers(void)
{
    test_a = cJSON_CreateTrue();
    TEST_ASSERT_TRUE(cJSON_Compare(test_a, test_a, cJSON_True));
}

void test_cJSON_Compare_booleans_true(void)
{
    test_a = cJSON_CreateTrue();
    test_b = cJSON_CreateTrue();
    TEST_ASSERT_TRUE(cJSON_Compare(test_a, test_b, cJSON_True));
}

void test_cJSON_Compare_booleans_false(void)
{
    test_a = cJSON_CreateFalse();
    test_b = cJSON_CreateFalse();
    TEST_ASSERT_TRUE(cJSON_Compare(test_a, test_b, cJSON_True));
}

void test_cJSON_Compare_booleans_mismatch(void)
{
    test_a = cJSON_CreateTrue();
    test_b = cJSON_CreateFalse();
    TEST_ASSERT_FALSE(cJSON_Compare(test_a, test_b, cJSON_True));
}

void test_cJSON_Compare_null(void)
{
    test_a = cJSON_CreateNull();
    test_b = cJSON_CreateNull();
    TEST_ASSERT_TRUE(cJSON_Compare(test_a, test_b, cJSON_True));
}

void test_cJSON_Compare_numbers_equal(void)
{
    test_a = create_test_number(123.456);
    test_b = create_test_number(123.456);
    TEST_ASSERT_TRUE(cJSON_Compare(test_a, test_b, cJSON_True));
}

void test_cJSON_Compare_numbers_different(void)
{
    test_a = create_test_number(123.456);
    test_b = create_test_number(789.012);
    TEST_ASSERT_FALSE(cJSON_Compare(test_a, test_b, cJSON_True));
}

void test_cJSON_Compare_numbers_zero(void)
{
    test_a = create_test_number(0.0);
    test_b = create_test_number(0.0);
    TEST_ASSERT_TRUE(cJSON_Compare(test_a, test_b, cJSON_True));
}

void test_cJSON_Compare_numbers_negative_zero(void)
{
    test_a = create_test_number(-0.0);
    test_b = create_test_number(0.0);
    TEST_ASSERT_TRUE(cJSON_Compare(test_a, test_b, cJSON_True));
}

void test_cJSON_Compare_numbers_very_close(void)
{
    test_a = create_test_number(1.0);
    test_b = create_test_number(1.0 + 1e-10);
    TEST_ASSERT_TRUE(cJSON_Compare(test_a, test_b, cJSON_True));
}

void test_cJSON_Compare_strings_equal(void)
{
    test_a = create_test_string("hello");
    test_b = create_test_string("hello");
    TEST_ASSERT_TRUE(cJSON_Compare(test_a, test_b, cJSON_True));
}

void test_cJSON_Compare_strings_different(void)
{
    test_a = create_test_string("hello");
    test_b = create_test_string("world");
    TEST_ASSERT_FALSE(cJSON_Compare(test_a, test_b, cJSON_True));
}

void test_cJSON_Compare_strings_one_null(void)
{
    test_a = create_test_string("hello");
    test_b = cJSON_CreateNull();
    TEST_ASSERT_FALSE(cJSON_Compare(test_a, test_b, cJSON_True));
}

void test_cJSON_Compare_strings_case_sensitive(void)
{
    test_a = create_test_string("Hello");
    test_b = create_test_string("hello");
    TEST_ASSERT_FALSE(cJSON_Compare(test_a, test_b, cJSON_True));
}

void test_cJSON_Compare_strings_case_insensitive(void)
{
    test_a = create_test_string("Hello");
    test_b = create_test_string("hello");
    TEST_ASSERT_FALSE(cJSON_Compare(test_a, test_b, cJSON_False));
}

void test_cJSON_Compare_raw_equal(void)
{
    test_a = cJSON_CreateRaw("raw json");
    test_b = cJSON_CreateRaw("raw json");
    TEST_ASSERT_TRUE(cJSON_Compare(test_a, test_b, cJSON_True));
}

void test_cJSON_Compare_raw_different(void)
{
    test_a = cJSON_CreateRaw("raw json a");
    test_b = cJSON_CreateRaw("raw json b");
    TEST_ASSERT_FALSE(cJSON_Compare(test_a, test_b, cJSON_True));
}

void test_cJSON_Compare_arrays_empty(void)
{
    test_a = cJSON_CreateArray();
    test_b = cJSON_CreateArray();
    TEST_ASSERT_TRUE(cJSON_Compare(test_a, test_b, cJSON_True));
}

void test_cJSON_Compare_arrays_identical(void)
{
    test_a = cJSON_CreateArray();
    test_b = cJSON_CreateArray();
    cJSON_AddItemToArray(test_a, cJSON_CreateNumber(1));
    cJSON_AddItemToArray(test_a, cJSON_CreateString("two"));
    cJSON_AddItemToArray(test_b, cJSON_CreateNumber(1));
    cJSON_AddItemToArray(test_b, cJSON_CreateString("two"));
    TEST_ASSERT_TRUE(cJSON_Compare(test_a, test_b, cJSON_True));
}

void test_cJSON_Compare_arrays_different_length(void)
{
    test_a = cJSON_CreateArray();
    test_b = cJSON_CreateArray();
    cJSON_AddItemToArray(test_a, cJSON_CreateNumber(1));
    cJSON_AddItemToArray(test_b, cJSON_CreateNumber(1));
    cJSON_AddItemToArray(test_b, cJSON_CreateNumber(2));
    TEST_ASSERT_FALSE(cJSON_Compare(test_a, test_b, cJSON_True));
}

void test_cJSON_Compare_arrays_different_content(void)
{
    test_a = cJSON_CreateArray();
    test_b = cJSON_CreateArray();
    cJSON_AddItemToArray(test_a, cJSON_CreateNumber(1));
    cJSON_AddItemToArray(test_b, cJSON_CreateNumber(2));
    TEST_ASSERT_FALSE(cJSON_Compare(test_a, test_b, cJSON_True));
}

void test_cJSON_Compare_arrays_nested(void)
{
    test_a = cJSON_CreateArray();
    test_b = cJSON_CreateArray();
    cJSON* inner_a = cJSON_CreateObject();
    cJSON* inner_b = cJSON_CreateObject();
    cJSON_AddStringToObject(inner_a, "key", "value");
    cJSON_AddStringToObject(inner_b, "key", "value");
    cJSON_AddItemToArray(test_a, inner_a);
    cJSON_AddItemToArray(test_b, inner_b);
    TEST_ASSERT_TRUE(cJSON_Compare(test_a, test_b, cJSON_True));
}

void test_cJSON_Compare_objects_empty(void)
{
    test_a = cJSON_CreateObject();
    test_b = cJSON_CreateObject();
    TEST_ASSERT_TRUE(cJSON_Compare(test_a, test_b, cJSON_True));
}

void test_cJSON_Compare_objects_identical(void)
{
    test_a = cJSON_CreateObject();
    test_b = cJSON_CreateObject();
    cJSON_AddNumberToObject(test_a, "num", 42);
    cJSON_AddStringToObject(test_a, "str", "hello");
    cJSON_AddNumberToObject(test_b, "num", 42);
    cJSON_AddStringToObject(test_b, "str", "hello");
    TEST_ASSERT_TRUE(cJSON_Compare(test_a, test_b, cJSON_True));
}

void test_cJSON_Compare_objects_different_keys(void)
{
    test_a = cJSON_CreateObject();
    test_b = cJSON_CreateObject();
    cJSON_AddNumberToObject(test_a, "key1", 1);
    cJSON_AddNumberToObject(test_b, "key2", 1);
    TEST_ASSERT_FALSE(cJSON_Compare(test_a, test_b, cJSON_True));
}

void test_cJSON_Compare_objects_different_values(void)
{
    test_a = cJSON_CreateObject();
    test_b = cJSON_CreateObject();
    cJSON_AddNumberToObject(test_a, "key", 1);
    cJSON_AddNumberToObject(test_b, "key", 2);
    TEST_ASSERT_FALSE(cJSON_Compare(test_a, test_b, cJSON_True));
}

void test_cJSON_Compare_objects_different_size(void)
{
    test_a = cJSON_CreateObject();
    test_b = cJSON_CreateObject();
    cJSON_AddNumberToObject(test_a, "key1", 1);
    cJSON_AddNumberToObject(test_b, "key1", 1);
    cJSON_AddNumberToObject(test_b, "key2", 2);
    TEST_ASSERT_FALSE(cJSON_Compare(test_a, test_b, cJSON_True));
}

void test_cJSON_Compare_objects_nested(void)
{
    test_a = cJSON_CreateObject();
    test_b = cJSON_CreateObject();
    cJSON* inner_a = cJSON_CreateObject();
    cJSON* inner_b = cJSON_CreateObject();
    cJSON_AddNumberToObject(inner_a, "nested", 123);
    cJSON_AddNumberToObject(inner_b, "nested", 123);
    cJSON_AddItemToObject(test_a, "obj", inner_a);
    cJSON_AddItemToObject(test_b, "obj", inner_b);
    TEST_ASSERT_TRUE(cJSON_Compare(test_a, test_b, cJSON_True));
}

void test_cJSON_Compare_objects_case_insensitive(void)
{
    test_a = cJSON_CreateObject();
    test_b = cJSON_CreateObject();
    cJSON_AddNumberToObject(test_a, "Key", 1);
    cJSON_AddNumberToObject(test_b, "key", 1);
    TEST_ASSERT_TRUE(cJSON_Compare(test_a, test_b, cJSON_False));
}

void test_cJSON_Compare_objects_case_sensitive(void)
{
    test_a = cJSON_CreateObject();
    test_b = cJSON_CreateObject();
    cJSON_AddNumberToObject(test_a, "Key", 1);
    cJSON_AddNumberToObject(test_b, "key", 1);
    TEST_ASSERT_FALSE(cJSON_Compare(test_a, test_b, cJSON_True));
}

void test_cJSON_Compare_arrays_case_insensitive(void)
{
    /* Arrays don't have keys, so case sensitivity only affects object items */
    test_a = cJSON_CreateArray();
    test_b = cJSON_CreateArray();
    cJSON* obj_a = cJSON_CreateObject();
    cJSON* obj_b = cJSON_CreateObject();
    cJSON_AddNumberToObject(obj_a, "Key", 1);
    cJSON_AddNumberToObject(obj_b, "key", 1);
    cJSON_AddItemToArray(test_a, obj_a);
    cJSON_AddItemToArray(test_b, obj_b);
    TEST_ASSERT_TRUE(cJSON_Compare(test_a, test_b, cJSON_False));
}

void test_cJSON_Compare_raw_vs_string(void)
{
    test_a = cJSON_CreateRaw("raw");
    test_b = cJSON_CreateString("raw");
    TEST_ASSERT_FALSE(cJSON_Compare(test_a, test_b, cJSON_True));
}

void test_cJSON_Compare_with_subtrees(void)
{
    test_a = cJSON_Parse("[1, {\"a\": 2}, \"three\"]");
    test_b = cJSON_Parse("[1, {\"a\": 2}, \"three\"]");
    TEST_ASSERT_TRUE(cJSON_Compare(test_a, test_b, cJSON_True));
}

void test_cJSON_Compare_with_subtrees_different(void)
{
    test_a = cJSON_Parse("[1, {\"a\": 2}, \"three\"]");
    test_b = cJSON_Parse("[1, {\"a\": 3}, \"three\"]");
    TEST_ASSERT_FALSE(cJSON_Compare(test_a, test_b, cJSON_True));
}

void test_cJSON_Compare_with_subtrees_case_insensitive(void)
{
    test_a = cJSON_Parse("{\"A\": 1}");
    test_b = cJSON_Parse("{\"a\": 1}");
    TEST_ASSERT_TRUE(cJSON_Compare(test_a, test_b, cJSON_False));
}

void test_cJSON_Compare_with_subtrees_case_sensitive(void)
{
    test_a = cJSON_Parse("{\"A\": 1}");
    test_b = cJSON_Parse("{\"a\": 1}");
    TEST_ASSERT_FALSE(cJSON_Compare(test_a, test_b, cJSON_True));
}

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_cJSON_Compare_NULL_inputs);
    RUN_TEST(test_cJSON_Compare_different_types);
    RUN_TEST(test_cJSON_Compare_invalid_type);
    RUN_TEST(test_cJSON_Compare_identical_pointers);
    RUN_TEST(test_cJSON_Compare_booleans_true);
    RUN_TEST(test_cJSON_Compare_booleans_false);
    RUN_TEST(test_cJSON_Compare_booleans_mismatch);
    RUN_TEST(test_cJSON_Compare_null);
    RUN_TEST(test_cJSON_Compare_numbers_equal);
    RUN_TEST(test_cJSON_Compare_numbers_different);
    RUN_TEST(test_cJSON_Compare_numbers_zero);
    RUN_TEST(test_cJSON_Compare_numbers_negative_zero);
    RUN_TEST(test_cJSON_Compare_numbers_very_close);
    RUN_TEST(test_cJSON_Compare_strings_equal);
    RUN_TEST(test_cJSON_Compare_strings_different);
    RUN_TEST(test_cJSON_Compare_strings_one_null);
    RUN_TEST(test_cJSON_Compare_strings_case_sensitive);
    RUN_TEST(test_cJSON_Compare_strings_case_insensitive);
    RUN_TEST(test_cJSON_Compare_raw_equal);
    RUN_TEST(test_cJSON_Compare_raw_different);
    RUN_TEST(test_cJSON_Compare_arrays_empty);
    RUN_TEST(test_cJSON_Compare_arrays_identical);
    RUN_TEST(test_cJSON_Compare_arrays_different_length);
    RUN_TEST(test_cJSON_Compare_arrays_different_content);
    RUN_TEST(test_cJSON_Compare_arrays_nested);
    RUN_TEST(test_cJSON_Compare_objects_empty);
    RUN_TEST(test_cJSON_Compare_objects_identical);
    RUN_TEST(test_cJSON_Compare_objects_different_keys);
    RUN_TEST(test_cJSON_Compare_objects_different_values);
    RUN_TEST(test_cJSON_Compare_objects_different_size);
    RUN_TEST(test_cJSON_Compare_objects_nested);
    RUN_TEST(test_cJSON_Compare_objects_case_insensitive);
    RUN_TEST(test_cJSON_Compare_objects_case_sensitive);
    RUN_TEST(test_cJSON_Compare_arrays_case_insensitive);
    RUN_TEST(test_cJSON_Compare_raw_vs_string);
    RUN_TEST(test_cJSON_Compare_with_subtrees);
    RUN_TEST(test_cJSON_Compare_with_subtrees_different);
    RUN_TEST(test_cJSON_Compare_with_subtrees_case_insensitive);
    RUN_TEST(test_cJSON_Compare_with_subtrees_case_sensitive);
    return UNITY_END();
}