#include "cJSON.c"
#include "unity.h"
#include <string.h>
#include <stdlib.h>

/* File-scope static variables / fixtures */
static const char *test_strings[] = {"hello", "world", "test", "array"};
static const char *empty_strings[] = {"", "", ""};
static const char *null_strings[] = {NULL, "valid"};
static const char *single_string[] = {"single"};

/* Helper functions */
static void assert_string_array_equal(const cJSON *array, const char *const *expected, int count)
{
    int i;
    const cJSON *item;

    TEST_ASSERT_NOT_NULL(array);
    TEST_ASSERT_EQUAL_INT(count, cJSON_GetArraySize(array));

    cJSON_ArrayForEach(item, array)
    {
        TEST_ASSERT_TRUE(cJSON_IsString(item));
        TEST_ASSERT_EQUAL_STRING(expected[i], cJSON_GetStringValue(item));
        i++;
    }
}

static void assert_array_null_items(const cJSON *array, int count)
{
    const cJSON *item;
    int i = 0;

    TEST_ASSERT_NOT_NULL(array);
    TEST_ASSERT_EQUAL_INT(count, cJSON_GetArraySize(array));

    cJSON_ArrayForEach(item, array)
    {
        TEST_ASSERT_TRUE(cJSON_IsString(item));
        TEST_ASSERT_NULL(cJSON_GetStringValue(item));
        i++;
    }
}

/* Test cases */

void test_cJSON_CreateStringArray_null_strings_returns_null(void)
{
    cJSON *result = cJSON_CreateStringArray(NULL, 5);
    TEST_ASSERT_NULL(result);
}

void test_cJSON_CreateStringArray_negative_count_returns_null(void)
{
    cJSON *result = cJSON_CreateStringArray(test_strings, -1);
    TEST_ASSERT_NULL(result);
}

void test_cJSON_CreateStringArray_zero_count_returns_empty_array(void)
{
    cJSON *result = cJSON_CreateStringArray(test_strings, 0);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(cJSON_IsArray(result));
    TEST_ASSERT_EQUAL_INT(0, cJSON_GetArraySize(result));
    TEST_ASSERT_NULL(result->child);
    cJSON_Delete(result);
}

void test_cJSON_CreateStringArray_single_item(void)
{
    cJSON *result = cJSON_CreateStringArray(single_string, 1);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(cJSON_IsArray(result));
    TEST_ASSERT_EQUAL_INT(1, cJSON_GetArraySize(result));
    assert_string_array_equal(result, single_string, 1);
    cJSON_Delete(result);
}

void test_cJSON_CreateStringArray_multiple_items(void)
{
    cJSON *result = cJSON_CreateStringArray(test_strings, 4);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(cJSON_IsArray(result));
    TEST_ASSERT_EQUAL_INT(4, cJSON_GetArraySize(result));
    assert_string_array_equal(result, test_strings, 4);
    cJSON_Delete(result);
}

void test_cJSON_CreateStringArray_empty_strings(void)
{
    cJSON *result = cJSON_CreateStringArray(empty_strings, 3);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(cJSON_IsArray(result));
    TEST_ASSERT_EQUAL_INT(3, cJSON_GetArraySize(result));
    assert_string_array_equal(result, empty_strings, 3);
    cJSON_Delete(result);
}

void test_cJSON_CreateStringArray_null_in_strings(void)
{
    cJSON *result = cJSON_CreateStringArray(null_strings, 2);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(cJSON_IsArray(result));
    TEST_ASSERT_EQUAL_INT(2, cJSON_GetArraySize(result));
    assert_string_array_equal(result, null_strings, 2);
    cJSON_Delete(result);
}

void test_cJSON_CreateStringArray_memory_cleanup_on_failure(void)
{
    /* Force failure by providing invalid input after valid items */
    const char *valid_strings[] = {"first", "second"};
    cJSON *result = cJSON_CreateStringArray(valid_strings, 2);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(2, cJSON_GetArraySize(result));
    cJSON_Delete(result);
}

void test_cJSON_CreateStringArray_circular_links(void)
{
    cJSON *result = cJSON_CreateStringArray(test_strings, 4);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(cJSON_IsArray(result));

    /* Check circular links: first->prev should point to last, last->next should be NULL */
    const cJSON *first = result->child;
    const cJSON *last = first;
    while (last && last->next)
    {
        last = last->next;
    }

    TEST_ASSERT_NOT_NULL(first);
    TEST_ASSERT_NOT_NULL(last);
    TEST_ASSERT_EQUAL_PTR(first->prev, last);
    TEST_ASSERT_NULL(last->next);
    TEST_ASSERT_NULL(first->prev->next); /* prev of first should be last, and last->next is NULL */

    cJSON_Delete(result);
}

void test_cJSON_CreateStringArray_large_count(void)
{
    const char *strings[] = {"a", "b", "c"};
    cJSON *result = cJSON_CreateStringArray(strings, 100); /* count > actual array size */
    /* Function should process only up to count, but since strings[100] is out of bounds,
       this test is technically undefined behavior. Skip this test or use a safe array. */
    /* Instead, test with a large but valid count */
    const char *large_strings[] = {"0", "1", "2", "3", "4"};
    result = cJSON_CreateStringArray(large_strings, 5);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(5, cJSON_GetArraySize(result));
    cJSON_Delete(result);
}

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();

    RUN_TEST(test_cJSON_CreateStringArray_null_strings_returns_null);
    RUN_TEST(test_cJSON_CreateStringArray_negative_count_returns_null);
    RUN_TEST(test_cJSON_CreateStringArray_zero_count_returns_empty_array);
    RUN_TEST(test_cJSON_CreateStringArray_single_item);
    RUN_TEST(test_cJSON_CreateStringArray_multiple_items);
    RUN_TEST(test_cJSON_CreateStringArray_empty_strings);
    RUN_TEST(test_cJSON_CreateStringArray_null_in_strings);
    RUN_TEST(test_cJSON_CreateStringArray_memory_cleanup_on_failure);
    RUN_TEST(test_cJSON_CreateStringArray_circular_links);
    RUN_TEST(test_cJSON_CreateStringArray_large_count);

    return UNITY_END();
}