#include "cJSON.c"
#include "unity.h"
#include "cJSON.h"
#include <stddef.h>
#include <string.h>

/* File-scope fixtures */
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
static cJSON *get_array_item_by_walk(cJSON *array, int index)
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
    TEST_ASSERT_EQUAL_INT(0, cJSON_GetArraySize(result));
    TEST_ASSERT_NULL(result->child);
}

/* Test: single element array */
void test_CreateStringArray_SingleElement_ReturnsArrayWithOneString(void)
{
    const char *strings[] = {"hello"};
    cJSON *item = NULL;

    result = cJSON_CreateStringArray(strings, 1);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(cJSON_IsArray(result));
    TEST_ASSERT_EQUAL_INT(1, cJSON_GetArraySize(result));

    item = cJSON_GetArrayItem(result, 0);
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_TRUE(cJSON_IsString(item));
    TEST_ASSERT_EQUAL_STRING("hello", item->valuestring);
}

/* Test: multiple elements array */
void test_CreateStringArray_MultipleElements_ReturnsArrayWithAllStrings(void)
{
    const char *strings[] = {"foo", "bar", "baz"};
    cJSON *item = NULL;
    int i = 0;

    result = cJSON_CreateStringArray(strings, 3);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(cJSON_IsArray(result));
    TEST_ASSERT_EQUAL_INT(3, cJSON_GetArraySize(result));

    for (i = 0; i < 3; i++)
    {
        item = cJSON_GetArrayItem(result, i);
        TEST_ASSERT_NOT_NULL_MESSAGE(item, "Array item should not be NULL");
        TEST_ASSERT_TRUE_MESSAGE(cJSON_IsString(item), "Array item should be a string");
        TEST_ASSERT_EQUAL_STRING(strings[i], item->valuestring);
    }
}

/* Test: result type is cJSON_Array */
void test_CreateStringArray_ResultType_IsArray(void)
{
    const char *strings[] = {"a", "b"};
    result = cJSON_CreateStringArray(strings, 2);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(cJSON_Array, result->type);
}

/* Test: linked list next pointers are correct */
void test_CreateStringArray_LinkedList_NextPointersCorrect(void)
{
    const char *strings[] = {"one", "two", "three"};
    cJSON *first = NULL;
    cJSON *second = NULL;
    cJSON *third = NULL;

    result = cJSON_CreateStringArray(strings, 3);
    TEST_ASSERT_NOT_NULL(result);

    first = result->child;
    TEST_ASSERT_NOT_NULL(first);
    TEST_ASSERT_EQUAL_STRING("one", first->valuestring);

    second = first->next;
    TEST_ASSERT_NOT_NULL(second);
    TEST_ASSERT_EQUAL_STRING("two", second->valuestring);

    third = second->next;
    TEST_ASSERT_NOT_NULL(third);
    TEST_ASSERT_EQUAL_STRING("three", third->valuestring);

    TEST_ASSERT_NULL(third->next);
}

/* Test: linked list prev pointers are correct */
void test_CreateStringArray_LinkedList_PrevPointersCorrect(void)
{
    const char *strings[] = {"one", "two", "three"};
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

/* Test: child->prev points to last element (circular prev) */
void test_CreateStringArray_ChildPrev_PointsToLastElement(void)
{
    const char *strings[] = {"alpha", "beta", "gamma"};
    cJSON *first = NULL;
    cJSON *last = NULL;

    result = cJSON_CreateStringArray(strings, 3);
    TEST_ASSERT_NOT_NULL(result);

    first = result->child;
    TEST_ASSERT_NOT_NULL(first);

    last = get_array_item_by_walk(result, 2);
    TEST_ASSERT_NOT_NULL(last);
    TEST_ASSERT_EQUAL_STRING("gamma", last->valuestring);

    /* child->prev should point to the last element */
    TEST_ASSERT_EQUAL_PTR(last, first->prev);
}

/* Test: empty string in array */
void test_CreateStringArray_EmptyStringElement_IsHandled(void)
{
    const char *strings[] = {"", "nonempty", ""};
    cJSON *item = NULL;

    result = cJSON_CreateStringArray(strings, 3);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(3, cJSON_GetArraySize(result));

    item = cJSON_GetArrayItem(result, 0);
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_EQUAL_STRING("", item->valuestring);

    item = cJSON_GetArrayItem(result, 1);
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_EQUAL_STRING("nonempty", item->valuestring);

    item = cJSON_GetArrayItem(result, 2);
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_EQUAL_STRING("", item->valuestring);
}

/* Test: single element child->prev points to itself (last element) */
void test_CreateStringArray_SingleElement_ChildPrevPointsToSelf(void)
{
    const char *strings[] = {"only"};
    cJSON *first = NULL;

    result = cJSON_CreateStringArray(strings, 1);
    TEST_ASSERT_NOT_NULL(result);

    first = result->child;
    TEST_ASSERT_NOT_NULL(first);

    /* For single element, child->prev should point to itself (the last element) */
    TEST_ASSERT_EQUAL_PTR(first, first->prev);
}

/* Test: NULL strings pointer with zero count returns NULL */
void test_CreateStringArray_NullStringsZeroCount_ReturnsNull(void)
{
    result = cJSON_CreateStringArray(NULL, 0);
    TEST_ASSERT_NULL(result);
}

/* Test: strings with special characters */
void test_CreateStringArray_SpecialCharacters_ArePreserved(void)
{
    const char *strings[] = {"hello\nworld", "tab\there", "quote\"here"};
    cJSON *item = NULL;

    result = cJSON_CreateStringArray(strings, 3);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(3, cJSON_GetArraySize(result));

    item = cJSON_GetArrayItem(result, 0);
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_EQUAL_STRING("hello\nworld", item->valuestring);

    item = cJSON_GetArrayItem(result, 1);
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_EQUAL_STRING("tab\there", item->valuestring);

    item = cJSON_GetArrayItem(result, 2);
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_EQUAL_STRING("quote\"here", item->valuestring);
}

/* Test: large array */
void test_CreateStringArray_LargeArray_AllElementsCorrect(void)
{
    const char *strings[] = {
        "s0", "s1", "s2", "s3", "s4",
        "s5", "s6", "s7", "s8", "s9"
    };
    cJSON *item = NULL;
    int i = 0;

    result = cJSON_CreateStringArray(strings, 10);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(10, cJSON_GetArraySize(result));

    for (i = 0; i < 10; i++)
    {
        item = cJSON_GetArrayItem(result, i);
        TEST_ASSERT_NOT_NULL_MESSAGE(item, "Item should not be NULL");
        TEST_ASSERT_EQUAL_STRING(strings[i], item->valuestring);
    }
}

/* Test: each element is of type cJSON_String */
void test_CreateStringArray_AllElements_AreStringType(void)
{
    const char *strings[] = {"x", "y", "z"};
    cJSON *item = NULL;
    int i = 0;

    result = cJSON_CreateStringArray(strings, 3);
    TEST_ASSERT_NOT_NULL(result);

    for (i = 0; i < 3; i++)
    {
        item = cJSON_GetArrayItem(result, i);
        TEST_ASSERT_NOT_NULL(item);
        TEST_ASSERT_EQUAL_INT(cJSON_String, item->type);
    }
}

/* Test: last element next pointer is NULL */
void test_CreateStringArray_LastElement_NextIsNull(void)
{
    const char *strings[] = {"a", "b", "c"};
    cJSON *last = NULL;

    result = cJSON_CreateStringArray(strings, 3);
    TEST_ASSERT_NOT_NULL(result);

    last = get_array_item_by_walk(result, 2);
    TEST_ASSERT_NOT_NULL(last);
    TEST_ASSERT_NULL(last->next);
}

/* Test: count of -100 returns NULL */
void test_CreateStringArray_VeryNegativeCount_ReturnsNull(void)
{
    const char *strings[] = {"a", "b"};
    result = cJSON_CreateStringArray(strings, -100);
    TEST_ASSERT_NULL(result);
}

/* Test: two element array linked list integrity */
void test_CreateStringArray_TwoElements_LinkedListIntegrity(void)
{
    const char *strings[] = {"first", "second"};
    cJSON *first = NULL;
    cJSON *second = NULL;

    result = cJSON_CreateStringArray(strings, 2);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(2, cJSON_GetArraySize(result));

    first = result->child;
    TEST_ASSERT_NOT_NULL(first);
    TEST_ASSERT_EQUAL_STRING("first", first->valuestring);

    second = first->next;
    TEST_ASSERT_NOT_NULL(second);
    TEST_ASSERT_EQUAL_STRING("second", second->valuestring);

    /* second->prev should point to first */
    TEST_ASSERT_EQUAL_PTR(first, second->prev);

    /* first->prev should point to second (last element) */
    TEST_ASSERT_EQUAL_PTR(second, first->prev);

    /* second->next should be NULL */
    TEST_ASSERT_NULL(second->next);
}

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_CreateStringArray_NullStrings_ReturnsNull);
    RUN_TEST(test_CreateStringArray_NegativeCount_ReturnsNull);
    RUN_TEST(test_CreateStringArray_ZeroCount_ReturnsEmptyArray);
    RUN_TEST(test_CreateStringArray_SingleElement_ReturnsArrayWithOneString);
    RUN_TEST(test_CreateStringArray_MultipleElements_ReturnsArrayWithAllStrings);
    RUN_TEST(test_CreateStringArray_ResultType_IsArray);
    RUN_TEST(test_CreateStringArray_LinkedList_NextPointersCorrect);
    RUN_TEST(test_CreateStringArray_LinkedList_PrevPointersCorrect);
    RUN_TEST(test_CreateStringArray_ChildPrev_PointsToLastElement);
    RUN_TEST(test_CreateStringArray_EmptyStringElement_IsHandled);
    RUN_TEST(test_CreateStringArray_SingleElement_ChildPrevPointsToSelf);
    RUN_TEST(test_CreateStringArray_NullStringsZeroCount_ReturnsNull);
    RUN_TEST(test_CreateStringArray_SpecialCharacters_ArePreserved);
    RUN_TEST(test_CreateStringArray_LargeArray_AllElementsCorrect);
    RUN_TEST(test_CreateStringArray_AllElements_AreStringType);
    RUN_TEST(test_CreateStringArray_LastElement_NextIsNull);
    RUN_TEST(test_CreateStringArray_VeryNegativeCount_ReturnsNull);
    RUN_TEST(test_CreateStringArray_TwoElements_LinkedListIntegrity);
    return UNITY_END();
}