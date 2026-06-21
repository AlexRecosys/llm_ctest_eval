#include "cJSON.c"
#include "unity.h"
#include "cJSON.h"
#include <stddef.h>
#include <string.h>

/* File-scope variables */
static cJSON *result = NULL;

/* setUp and tearDown */
void setUp(void)
{
    result = NULL;
}

void tearDown(void)
{
    if (result != NULL)
    {
        cJSON_Delete(result);
        result = NULL;
    }
}

/* Helper: get nth item in array by walking child->next chain */
static cJSON *get_array_item_at(cJSON *array, int index)
{
    cJSON *current = NULL;
    int i = 0;

    if (array == NULL || array->child == NULL)
    {
        return NULL;
    }

    current = array->child;
    for (i = 0; i < index && current != NULL; i++)
    {
        current = current->next;
    }
    return current;
}

/* Helper: count items in array by walking child->next chain */
static int count_array_items(cJSON *array)
{
    cJSON *current = NULL;
    int count = 0;

    if (array == NULL || array->child == NULL)
    {
        return 0;
    }

    current = array->child;
    while (current != NULL)
    {
        count++;
        current = current->next;
    }
    return count;
}

/* Test: NULL strings pointer returns NULL */
void test_CreateStringArray_NullStrings_ReturnsNull(void)
{
    result = cJSON_CreateStringArray(NULL, 3);
    TEST_ASSERT_NULL(result);
}

/* Test: negative count returns NULL */
void test_CreateStringArray_NegativeCount_ReturnsNull(void)
{
    const char *strings[] = {"hello", "world"};
    result = cJSON_CreateStringArray(strings, -1);
    TEST_ASSERT_NULL(result);
}

/* Test: zero count returns empty array */
void test_CreateStringArray_ZeroCount_ReturnsEmptyArray(void)
{
    const char *strings[] = {"hello", "world"};
    result = cJSON_CreateStringArray(strings, 0);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(cJSON_IsArray(result));
    TEST_ASSERT_NULL(result->child);
}

/* Test: single string creates array with one element */
void test_CreateStringArray_SingleString_ReturnsArrayWithOneElement(void)
{
    const char *strings[] = {"hello"};
    cJSON *item = NULL;

    result = cJSON_CreateStringArray(strings, 1);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(cJSON_IsArray(result));
    TEST_ASSERT_EQUAL_INT(1, count_array_items(result));

    item = result->child;
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_TRUE(cJSON_IsString(item));
    TEST_ASSERT_EQUAL_STRING("hello", item->valuestring);
}

/* Test: multiple strings creates array with correct count */
void test_CreateStringArray_MultipleStrings_ReturnsCorrectCount(void)
{
    const char *strings[] = {"alpha", "beta", "gamma"};
    result = cJSON_CreateStringArray(strings, 3);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(cJSON_IsArray(result));
    TEST_ASSERT_EQUAL_INT(3, count_array_items(result));
}

/* Test: multiple strings have correct values */
void test_CreateStringArray_MultipleStrings_CorrectValues(void)
{
    const char *strings[] = {"alpha", "beta", "gamma"};
    cJSON *item0 = NULL;
    cJSON *item1 = NULL;
    cJSON *item2 = NULL;

    result = cJSON_CreateStringArray(strings, 3);
    TEST_ASSERT_NOT_NULL(result);

    item0 = get_array_item_at(result, 0);
    item1 = get_array_item_at(result, 1);
    item2 = get_array_item_at(result, 2);

    TEST_ASSERT_NOT_NULL(item0);
    TEST_ASSERT_NOT_NULL(item1);
    TEST_ASSERT_NOT_NULL(item2);

    TEST_ASSERT_EQUAL_STRING("alpha", item0->valuestring);
    TEST_ASSERT_EQUAL_STRING("beta", item1->valuestring);
    TEST_ASSERT_EQUAL_STRING("gamma", item2->valuestring);
}

/* Test: all items are of type cJSON_String */
void test_CreateStringArray_AllItemsAreStrings(void)
{
    const char *strings[] = {"one", "two", "three", "four"};
    cJSON *item = NULL;
    int i = 0;

    result = cJSON_CreateStringArray(strings, 4);
    TEST_ASSERT_NOT_NULL(result);

    item = result->child;
    for (i = 0; i < 4; i++)
    {
        TEST_ASSERT_NOT_NULL_MESSAGE(item, "Array item should not be NULL");
        TEST_ASSERT_TRUE_MESSAGE(cJSON_IsString(item), "Array item should be a string");
        item = item->next;
    }
}

/* Test: linked list next pointers are correct */
void test_CreateStringArray_NextPointersCorrect(void)
{
    const char *strings[] = {"a", "b", "c"};
    cJSON *first = NULL;
    cJSON *second = NULL;
    cJSON *third = NULL;

    result = cJSON_CreateStringArray(strings, 3);
    TEST_ASSERT_NOT_NULL(result);

    first = result->child;
    TEST_ASSERT_NOT_NULL(first);

    second = first->next;
    TEST_ASSERT_NOT_NULL(second);

    third = second->next;
    TEST_ASSERT_NOT_NULL(third);

    TEST_ASSERT_NULL(third->next);
}

/* Test: linked list prev pointers are correct */
void test_CreateStringArray_PrevPointersCorrect(void)
{
    const char *strings[] = {"a", "b", "c"};
    cJSON *first = NULL;
    cJSON *second = NULL;
    cJSON *third = NULL;

    result = cJSON_CreateStringArray(strings, 3);
    TEST_ASSERT_NOT_NULL(result);

    first = result->child;
    TEST_ASSERT_NOT_NULL(first);

    second = first->next;
    TEST_ASSERT_NOT_NULL(second);
    TEST_ASSERT_EQUAL_PTR(first, second->prev);

    third = second->next;
    TEST_ASSERT_NOT_NULL(third);
    TEST_ASSERT_EQUAL_PTR(second, third->prev);
}

/* Test: first child's prev points to last element (circular prev) */
void test_CreateStringArray_FirstChildPrevPointsToLast(void)
{
    const char *strings[] = {"x", "y", "z"};
    cJSON *first = NULL;
    cJSON *last = NULL;

    result = cJSON_CreateStringArray(strings, 3);
    TEST_ASSERT_NOT_NULL(result);

    first = result->child;
    TEST_ASSERT_NOT_NULL(first);

    last = get_array_item_at(result, 2);
    TEST_ASSERT_NOT_NULL(last);

    /* The function sets a->child->prev = n (last element) */
    TEST_ASSERT_EQUAL_PTR(last, first->prev);
}

/* Test: result type is cJSON_Array */
void test_CreateStringArray_ResultTypeIsArray(void)
{
    const char *strings[] = {"test"};
    result = cJSON_CreateStringArray(strings, 1);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(cJSON_Array, result->type);
}

/* Test: empty string in array */
void test_CreateStringArray_EmptyStringInArray(void)
{
    const char *strings[] = {""};
    cJSON *item = NULL;

    result = cJSON_CreateStringArray(strings, 1);
    TEST_ASSERT_NOT_NULL(result);

    item = result->child;
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_TRUE(cJSON_IsString(item));
    TEST_ASSERT_EQUAL_STRING("", item->valuestring);
}

/* Test: array with mixed empty and non-empty strings */
void test_CreateStringArray_MixedEmptyAndNonEmptyStrings(void)
{
    const char *strings[] = {"", "hello", "", "world"};
    cJSON *item0 = NULL;
    cJSON *item1 = NULL;
    cJSON *item2 = NULL;
    cJSON *item3 = NULL;

    result = cJSON_CreateStringArray(strings, 4);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(4, count_array_items(result));

    item0 = get_array_item_at(result, 0);
    item1 = get_array_item_at(result, 1);
    item2 = get_array_item_at(result, 2);
    item3 = get_array_item_at(result, 3);

    TEST_ASSERT_EQUAL_STRING("", item0->valuestring);
    TEST_ASSERT_EQUAL_STRING("hello", item1->valuestring);
    TEST_ASSERT_EQUAL_STRING("", item2->valuestring);
    TEST_ASSERT_EQUAL_STRING("world", item3->valuestring);
}

/* Test: NULL strings pointer with zero count still returns NULL */
void test_CreateStringArray_NullStringsZeroCount_ReturnsNull(void)
{
    result = cJSON_CreateStringArray(NULL, 0);
    TEST_ASSERT_NULL(result);
}

/* Test: cJSON_GetArraySize matches count */
void test_CreateStringArray_GetArraySizeMatchesCount(void)
{
    const char *strings[] = {"one", "two", "three", "four", "five"};
    result = cJSON_CreateStringArray(strings, 5);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(5, cJSON_GetArraySize(result));
}

/* Test: cJSON_GetArrayItem returns correct items */
void test_CreateStringArray_GetArrayItemReturnsCorrectItems(void)
{
    const char *strings[] = {"first", "second", "third"};
    cJSON *item = NULL;

    result = cJSON_CreateStringArray(strings, 3);
    TEST_ASSERT_NOT_NULL(result);

    item = cJSON_GetArrayItem(result, 0);
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_EQUAL_STRING("first", item->valuestring);

    item = cJSON_GetArrayItem(result, 1);
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_EQUAL_STRING("second", item->valuestring);

    item = cJSON_GetArrayItem(result, 2);
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_EQUAL_STRING("third", item->valuestring);
}

/* Test: string with special characters */
void test_CreateStringArray_SpecialCharactersInStrings(void)
{
    const char *strings[] = {"hello\nworld", "tab\there", "quote\"here"};
    cJSON *item0 = NULL;
    cJSON *item1 = NULL;
    cJSON *item2 = NULL;

    result = cJSON_CreateStringArray(strings, 3);
    TEST_ASSERT_NOT_NULL(result);

    item0 = get_array_item_at(result, 0);
    item1 = get_array_item_at(result, 1);
    item2 = get_array_item_at(result, 2);

    TEST_ASSERT_EQUAL_STRING("hello\nworld", item0->valuestring);
    TEST_ASSERT_EQUAL_STRING("tab\there", item1->valuestring);
    TEST_ASSERT_EQUAL_STRING("quote\"here", item2->valuestring);
}

/* Test: two-element array prev/next linkage */
void test_CreateStringArray_TwoElements_LinkageCorrect(void)
{
    const char *strings[] = {"first", "second"};
    cJSON *first = NULL;
    cJSON *second = NULL;

    result = cJSON_CreateStringArray(strings, 2);
    TEST_ASSERT_NOT_NULL(result);

    first = result->child;
    TEST_ASSERT_NOT_NULL(first);
    TEST_ASSERT_EQUAL_STRING("first", first->valuestring);

    second = first->next;
    TEST_ASSERT_NOT_NULL(second);
    TEST_ASSERT_EQUAL_STRING("second", second->valuestring);

    /* second->next should be NULL */
    TEST_ASSERT_NULL(second->next);

    /* second->prev should point to first */
    TEST_ASSERT_EQUAL_PTR(first, second->prev);

    /* first->prev should point to second (last element) */
    TEST_ASSERT_EQUAL_PTR(second, first->prev);
}

/* Test: large array */
void test_CreateStringArray_LargeArray(void)
{
    const char *strings[100];
    int i = 0;
    char buffers[100][16];

    for (i = 0; i < 100; i++)
    {
        /* Use snprintf to fill buffers */
        snprintf(buffers[i], sizeof(buffers[i]), "item%d", i);
        strings[i] = buffers[i];
    }

    result = cJSON_CreateStringArray(strings, 100);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(100, cJSON_GetArraySize(result));

    /* Check first and last */
    TEST_ASSERT_EQUAL_STRING("item0", cJSON_GetArrayItem(result, 0)->valuestring);
    TEST_ASSERT_EQUAL_STRING("item99", cJSON_GetArrayItem(result, 99)->valuestring);
}

/* Test: count of 1 with NULL strings pointer returns NULL */
void test_CreateStringArray_CountOneNullStrings_ReturnsNull(void)
{
    result = cJSON_CreateStringArray(NULL, 1);
    TEST_ASSERT_NULL(result);
}

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_CreateStringArray_NullStrings_ReturnsNull);
    RUN_TEST(test_CreateStringArray_NegativeCount_ReturnsNull);
    RUN_TEST(test_CreateStringArray_ZeroCount_ReturnsEmptyArray);
    RUN_TEST(test_CreateStringArray_SingleString_ReturnsArrayWithOneElement);
    RUN_TEST(test_CreateStringArray_MultipleStrings_ReturnsCorrectCount);
    RUN_TEST(test_CreateStringArray_MultipleStrings_CorrectValues);
    RUN_TEST(test_CreateStringArray_AllItemsAreStrings);
    RUN_TEST(test_CreateStringArray_NextPointersCorrect);
    RUN_TEST(test_CreateStringArray_PrevPointersCorrect);
    RUN_TEST(test_CreateStringArray_FirstChildPrevPointsToLast);
    RUN_TEST(test_CreateStringArray_ResultTypeIsArray);
    RUN_TEST(test_CreateStringArray_EmptyStringInArray);
    RUN_TEST(test_CreateStringArray_MixedEmptyAndNonEmptyStrings);
    RUN_TEST(test_CreateStringArray_NullStringsZeroCount_ReturnsNull);
    RUN_TEST(test_CreateStringArray_GetArraySizeMatchesCount);
    RUN_TEST(test_CreateStringArray_GetArrayItemReturnsCorrectItems);
    RUN_TEST(test_CreateStringArray_SpecialCharactersInStrings);
    RUN_TEST(test_CreateStringArray_TwoElements_LinkageCorrect);
    RUN_TEST(test_CreateStringArray_LargeArray);
    RUN_TEST(test_CreateStringArray_CountOneNullStrings_ReturnsNull);
    return UNITY_END();
}