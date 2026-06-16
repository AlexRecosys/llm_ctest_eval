#include "cJSON.c"
#include "unity.h"
#include "cJSON.h"
#include <string.h>
#include <stdlib.h>

/* File-scope variables */
static cJSON *test_item = NULL;

/* Helper: create a cJSON String item with a heap-allocated valuestring */
static cJSON *make_string_item(const char *value)
{
    cJSON *item = cJSON_CreateString(value);
    return item;
}

void setUp(void)
{
    test_item = NULL;
}

void tearDown(void)
{
    if (test_item != NULL)
    {
        cJSON_Delete(test_item);
        test_item = NULL;
    }
}

/* Test: NULL object returns NULL */
void test_SetValuestring_null_object_returns_null(void)
{
    char *result = cJSON_SetValuestring(NULL, "hello");
    TEST_ASSERT_NULL(result);
}

/* Test: NULL valuestring argument returns NULL */
void test_SetValuestring_null_valuestring_returns_null(void)
{
    test_item = make_string_item("original");
    TEST_ASSERT_NOT_NULL(test_item);
    char *result = cJSON_SetValuestring(test_item, NULL);
    TEST_ASSERT_NULL(result);
}

/* Test: object type is not cJSON_String returns NULL */
void test_SetValuestring_non_string_type_returns_null(void)
{
    test_item = cJSON_CreateNumber(42.0);
    TEST_ASSERT_NOT_NULL(test_item);
    char *result = cJSON_SetValuestring(test_item, "hello");
    TEST_ASSERT_NULL(result);
}

/* Test: object with cJSON_IsReference flag returns NULL */
void test_SetValuestring_is_reference_returns_null(void)
{
    test_item = make_string_item("original");
    TEST_ASSERT_NOT_NULL(test_item);
    test_item->type |= cJSON_IsReference;
    char *result = cJSON_SetValuestring(test_item, "hello");
    TEST_ASSERT_NULL(result);
}

/* Test: object with NULL valuestring returns NULL */
void test_SetValuestring_object_valuestring_null_returns_null(void)
{
    test_item = make_string_item("original");
    TEST_ASSERT_NOT_NULL(test_item);
    /* Manually free and set to NULL to simulate corrupted object */
    cJSON_free(test_item->valuestring);
    test_item->valuestring = NULL;
    char *result = cJSON_SetValuestring(test_item, "hello");
    TEST_ASSERT_NULL(result);
}

/* Test: new valuestring shorter than existing — in-place copy */
void test_SetValuestring_shorter_new_value_inplace(void)
{
    test_item = make_string_item("hello world");
    TEST_ASSERT_NOT_NULL(test_item);
    char *original_ptr = test_item->valuestring;

    char *result = cJSON_SetValuestring(test_item, "hi");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("hi", result);
    /* Should be in-place (same pointer) */
    TEST_ASSERT_EQUAL_PTR(original_ptr, result);
    TEST_ASSERT_EQUAL_STRING("hi", test_item->valuestring);
}

/* Test: new valuestring same length as existing — in-place copy */
void test_SetValuestring_same_length_value_inplace(void)
{
    test_item = make_string_item("hello");
    TEST_ASSERT_NOT_NULL(test_item);
    char *original_ptr = test_item->valuestring;

    char *result = cJSON_SetValuestring(test_item, "world");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("world", result);
    TEST_ASSERT_EQUAL_PTR(original_ptr, result);
    TEST_ASSERT_EQUAL_STRING("world", test_item->valuestring);
}

/* Test: new valuestring longer than existing — new allocation */
void test_SetValuestring_longer_new_value_new_alloc(void)
{
    test_item = make_string_item("hi");
    TEST_ASSERT_NOT_NULL(test_item);
    char *original_ptr = test_item->valuestring;

    char *result = cJSON_SetValuestring(test_item, "hello world");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("hello world", result);
    /* Should be a new allocation */
    TEST_ASSERT_NOT_EQUAL(original_ptr, result);
    TEST_ASSERT_EQUAL_PTR(result, test_item->valuestring);
}

/* Test: set to empty string (shorter than any non-empty) — in-place */
void test_SetValuestring_empty_string_inplace(void)
{
    test_item = make_string_item("hello");
    TEST_ASSERT_NOT_NULL(test_item);
    char *original_ptr = test_item->valuestring;

    char *result = cJSON_SetValuestring(test_item, "");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("", result);
    TEST_ASSERT_EQUAL_PTR(original_ptr, result);
}

/* Test: set from empty string to non-empty — new allocation */
void test_SetValuestring_from_empty_to_nonempty_new_alloc(void)
{
    test_item = make_string_item("");
    TEST_ASSERT_NOT_NULL(test_item);
    char *original_ptr = test_item->valuestring;

    char *result = cJSON_SetValuestring(test_item, "hello");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("hello", result);
    TEST_ASSERT_NOT_EQUAL(original_ptr, result);
    TEST_ASSERT_EQUAL_PTR(result, test_item->valuestring);
}

/* Test: set to same value (same length) — in-place */
void test_SetValuestring_same_value_inplace(void)
{
    test_item = make_string_item("test");
    TEST_ASSERT_NOT_NULL(test_item);
    char *original_ptr = test_item->valuestring;

    char *result = cJSON_SetValuestring(test_item, "test");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("test", result);
    TEST_ASSERT_EQUAL_PTR(original_ptr, result);
}

/* Test: object type cJSON_NULL (not cJSON_String) returns NULL */
void test_SetValuestring_null_type_returns_null(void)
{
    test_item = cJSON_CreateNull();
    TEST_ASSERT_NOT_NULL(test_item);
    char *result = cJSON_SetValuestring(test_item, "hello");
    TEST_ASSERT_NULL(result);
}

/* Test: object type cJSON_Array returns NULL */
void test_SetValuestring_array_type_returns_null(void)
{
    test_item = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(test_item);
    char *result = cJSON_SetValuestring(test_item, "hello");
    TEST_ASSERT_NULL(result);
}

/* Test: object type cJSON_Object returns NULL */
void test_SetValuestring_object_type_returns_null(void)
{
    test_item = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(test_item);
    char *result = cJSON_SetValuestring(test_item, "hello");
    TEST_ASSERT_NULL(result);
}

/* Test: object type cJSON_False returns NULL */
void test_SetValuestring_false_type_returns_null(void)
{
    test_item = cJSON_CreateFalse();
    TEST_ASSERT_NOT_NULL(test_item);
    char *result = cJSON_SetValuestring(test_item, "hello");
    TEST_ASSERT_NULL(result);
}

/* Test: returned pointer matches object->valuestring after longer replacement */
void test_SetValuestring_returned_ptr_matches_valuestring_after_longer(void)
{
    test_item = make_string_item("ab");
    TEST_ASSERT_NOT_NULL(test_item);

    char *result = cJSON_SetValuestring(test_item, "abcdefgh");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_PTR(test_item->valuestring, result);
    TEST_ASSERT_EQUAL_STRING("abcdefgh", test_item->valuestring);
}

/* Test: multiple sequential calls work correctly */
void test_SetValuestring_multiple_sequential_calls(void)
{
    test_item = make_string_item("initial value here");
    TEST_ASSERT_NOT_NULL(test_item);

    /* First call: shorter, in-place */
    char *result1 = cJSON_SetValuestring(test_item, "short");
    TEST_ASSERT_NOT_NULL(result1);
    TEST_ASSERT_EQUAL_STRING("short", test_item->valuestring);

    /* Second call: longer, new alloc */
    char *result2 = cJSON_SetValuestring(test_item, "a much longer string now");
    TEST_ASSERT_NOT_NULL(result2);
    TEST_ASSERT_EQUAL_STRING("a much longer string now", test_item->valuestring);

    /* Third call: shorter again, in-place */
    char *result3 = cJSON_SetValuestring(test_item, "ok");
    TEST_ASSERT_NOT_NULL(result3);
    TEST_ASSERT_EQUAL_STRING("ok", test_item->valuestring);
}

/* Test: cJSON_IsReference combined with cJSON_String returns NULL */
void test_SetValuestring_string_with_isreference_returns_null(void)
{
    test_item = make_string_item("value");
    TEST_ASSERT_NOT_NULL(test_item);
    /* Add IsReference flag */
    test_item->type = cJSON_String | cJSON_IsReference;
    char *result = cJSON_SetValuestring(test_item, "new_value");
    TEST_ASSERT_NULL(result);
}

/* Test: long string replacement with longer string */
void test_SetValuestring_long_string_replacement(void)
{
    const char *long_original = "abcdefghijklmnopqrstuvwxyz0123456789";
    const char *longer_new    = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_extra_chars_here";

    test_item = make_string_item(long_original);
    TEST_ASSERT_NOT_NULL(test_item);

    char *result = cJSON_SetValuestring(test_item, longer_new);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING(longer_new, result);
    TEST_ASSERT_EQUAL_STRING(longer_new, test_item->valuestring);
}

/* Test: long string replacement with shorter string */
void test_SetValuestring_long_string_replacement_shorter(void)
{
    const char *long_original = "abcdefghijklmnopqrstuvwxyz0123456789";
    const char *shorter_new   = "xyz";

    test_item = make_string_item(long_original);
    TEST_ASSERT_NOT_NULL(test_item);
    char *original_ptr = test_item->valuestring;

    char *result = cJSON_SetValuestring(test_item, shorter_new);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING(shorter_new, result);
    TEST_ASSERT_EQUAL_PTR(original_ptr, result);
}

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_SetValuestring_null_object_returns_null);
    RUN_TEST(test_SetValuestring_null_valuestring_returns_null);
    RUN_TEST(test_SetValuestring_non_string_type_returns_null);
    RUN_TEST(test_SetValuestring_is_reference_returns_null);
    RUN_TEST(test_SetValuestring_object_valuestring_null_returns_null);
    RUN_TEST(test_SetValuestring_shorter_new_value_inplace);
    RUN_TEST(test_SetValuestring_same_length_value_inplace);
    RUN_TEST(test_SetValuestring_longer_new_value_new_alloc);
    RUN_TEST(test_SetValuestring_empty_string_inplace);
    RUN_TEST(test_SetValuestring_from_empty_to_nonempty_new_alloc);
    RUN_TEST(test_SetValuestring_same_value_inplace);
    RUN_TEST(test_SetValuestring_null_type_returns_null);
    RUN_TEST(test_SetValuestring_array_type_returns_null);
    RUN_TEST(test_SetValuestring_object_type_returns_null);
    RUN_TEST(test_SetValuestring_false_type_returns_null);
    RUN_TEST(test_SetValuestring_returned_ptr_matches_valuestring_after_longer);
    RUN_TEST(test_SetValuestring_multiple_sequential_calls);
    RUN_TEST(test_SetValuestring_string_with_isreference_returns_null);
    RUN_TEST(test_SetValuestring_long_string_replacement);
    RUN_TEST(test_SetValuestring_long_string_replacement_shorter);
    return UNITY_END();
}