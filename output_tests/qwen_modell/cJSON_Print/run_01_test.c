#include "cJSON.c"

#include "unity.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* File-scope static variables / fixtures */
static cJSON_Hooks original_hooks = {0};
static cJSON_Hooks test_hooks = {0};
static void* test_malloc_count = 0;
static void* test_free_count = 0;

/* Helper functions and macros */
static void* test_malloc(size_t sz)
{
    test_malloc_count = (void*)((size_t)test_malloc_count + 1);
    return malloc(sz);
}

static void test_free(void* ptr)
{
    test_free_count = (void*)((size_t)test_free_count + 1);
    free(ptr);
}

static void setup_hooks(void)
{
    /* Save original hooks */
    cJSON_GetHooks(&original_hooks);
    /* Install test hooks */
    test_hooks.malloc_fn = test_malloc;
    test_hooks.free_fn = test_free;
    cJSON_InitHooks(&test_hooks);
}

static void restore_hooks(void)
{
    /* Restore original hooks */
    cJSON_InitHooks(&original_hooks);
    test_malloc_count = 0;
    test_free_count = 0;
}

/* Test cases */

void test_cJSON_Print_null_returns_null(void)
{
    char* result = cJSON_Print(NULL);
    TEST_ASSERT_NULL(result);
}

void test_cJSON_Print_empty_object(void)
{
    cJSON* item = cJSON_CreateObject();
    char* result = cJSON_Print(item);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("{\"\"}", result);
    cJSON_Delete(item);
    free(result);
}

void test_cJSON_Print_object_with_single_key_value(void)
{
    cJSON* item = cJSON_CreateObject();
    cJSON_AddStringToObject(item, "key", "value");
    char* result = cJSON_Print(item);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("{\"key\":\"value\"}", result);
    cJSON_Delete(item);
    free(result);
}

void test_cJSON_Print_array_with_elements(void)
{
    cJSON* item = cJSON_CreateArray();
    cJSON_AddItemToArray(item, cJSON_CreateNumber(1));
    cJSON_AddItemToArray(item, cJSON_CreateNumber(2));
    cJSON_AddItemToArray(item, cJSON_CreateNumber(3));
    char* result = cJSON_Print(item);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("[1,2,3]", result);
    cJSON_Delete(item);
    free(result);
}

void test_cJSON_Print_nested_structure(void)
{
    cJSON* root = cJSON_CreateObject();
    cJSON* array = cJSON_CreateArray();
    cJSON* obj = cJSON_CreateObject();

    cJSON_AddNumberToObject(obj, "a", 1);
    cJSON_AddStringToObject(obj, "b", "test");
    cJSON_AddItemToArray(array, obj);
    cJSON_AddItemToObject(root, "arr", array);

    char* result = cJSON_Print(root);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("{\"arr\":[{\"a\":1,\"b\":\"test\"}]}", result);
    cJSON_Delete(root);
    free(result);
}

void test_cJSON_Print_string_with_special_characters(void)
{
    cJSON* item = cJSON_CreateString("hello\nworld\t\"quoted\"");
    char* result = cJSON_Print(item);
    TEST_ASSERT_NOT_NULL(result);
    /* cJSON_Print escapes special characters properly */
    TEST_ASSERT_EQUAL_STRING("\"hello\\nworld\\t\\\"quoted\\\"\"", result);
    cJSON_Delete(item);
    free(result);
}

void test_cJSON_Print_raw_type(void)
{
    cJSON* item = cJSON_CreateRaw("{\"raw\":\"json\"}");
    char* result = cJSON_Print(item);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("{\"raw\":\"json\"}", result);
    cJSON_Delete(item);
    free(result);
}

void test_cJSON_Print_number(void)
{
    cJSON* item = cJSON_CreateNumber(123.456);
    char* result = cJSON_Print(item);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("123.456", result);
    cJSON_Delete(item);
    free(result);
}

void test_cJSON_Print_boolean_true(void)
{
    cJSON* item = cJSON_CreateTrue();
    char* result = cJSON_Print(item);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("true", result);
    cJSON_Delete(item);
    free(result);
}

void test_cJSON_Print_boolean_false(void)
{
    cJSON* item = cJSON_CreateFalse();
    char* result = cJSON_Print(item);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("false", result);
    cJSON_Delete(item);
    free(result);
}

void test_cJSON_Print_null_value(void)
{
    cJSON* item = cJSON_CreateNull();
    char* result = cJSON_Print(item);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("null", result);
    cJSON_Delete(item);
    free(result);
}

void test_cJSON_Print_memory_management(void)
{
    cJSON* item = cJSON_CreateObject();
    char* result = cJSON_Print(item);
    TEST_ASSERT_NOT_NULL(result);
    /* Verify result is heap-allocated and can be freed with free() */
    free(result);
    cJSON_Delete(item);
}

void test_cJSON_Print_with_hooks(void)
{
    setup_hooks();
    cJSON* item = cJSON_CreateObject();
    cJSON_AddStringToObject(item, "key", "value");
    char* result = cJSON_Print(item);
    TEST_ASSERT_NOT_NULL(result);
    /* Verify hooks were used */
    TEST_ASSERT_EQUAL_INT(1, (size_t)test_malloc_count);
    TEST_ASSERT_EQUAL_INT(0, (size_t)test_free_count);
    free(result);
    /* After free, both should be 1 */
    TEST_ASSERT_EQUAL_INT(1, (size_t)test_free_count);
    cJSON_Delete(item);
    restore_hooks();
}

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_cJSON_Print_null_returns_null);
    RUN_TEST(test_cJSON_Print_empty_object);
    RUN_TEST(test_cJSON_Print_object_with_single_key_value);
    RUN_TEST(test_cJSON_Print_array_with_elements);
    RUN_TEST(test_cJSON_Print_nested_structure);
    RUN_TEST(test_cJSON_Print_string_with_special_characters);
    RUN_TEST(test_cJSON_Print_raw_type);
    RUN_TEST(test_cJSON_Print_number);
    RUN_TEST(test_cJSON_Print_boolean_true);
    RUN_TEST(test_cJSON_Print_boolean_false);
    RUN_TEST(test_cJSON_Print_null_value);
    RUN_TEST(test_cJSON_Print_memory_management);
    RUN_TEST(test_cJSON_Print_with_hooks);
    return UNITY_END();
}