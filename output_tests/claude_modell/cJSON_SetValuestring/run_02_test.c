#include "cJSON.c"
#include "unity.h"
#include "cJSON.h"
#include <string.h>
#include <stdlib.h>

/* -------------------------------------------------------------------------
 * Helper functions
 * ------------------------------------------------------------------------- */

/* Create a proper cJSON String item with a heap-allocated valuestring */
static cJSON *make_string_item(const char *value)
{
    cJSON *item = cJSON_CreateString(value);
    return item;
}

/* -------------------------------------------------------------------------
 * setUp / tearDown
 * ------------------------------------------------------------------------- */

void setUp(void)
{
    /* Reset hooks to defaults */
    cJSON_InitHooks(NULL);
}

void tearDown(void)
{
    /* Nothing to do */
}

/* -------------------------------------------------------------------------
 * Test cases
 * ------------------------------------------------------------------------- */

/* 1. NULL object → returns NULL */
void test_SetValuestring_null_object_returns_null(void)
{
    char *result = cJSON_SetValuestring(NULL, "hello");
    TEST_ASSERT_NULL(result);
}

/* 2. Object type is not cJSON_String → returns NULL */
void test_SetValuestring_non_string_type_returns_null(void)
{
    cJSON *item = cJSON_CreateNumber(42.0);
    TEST_ASSERT_NOT_NULL(item);

    char *result = cJSON_SetValuestring(item, "hello");
    TEST_ASSERT_NULL(result);

    cJSON_Delete(item);
}

/* 3. Object has cJSON_IsReference flag set → returns NULL */
void test_SetValuestring_is_reference_returns_null(void)
{
    cJSON *item = make_string_item("original");
    TEST_ASSERT_NOT_NULL(item);

    item->type |= cJSON_IsReference;

    char *result = cJSON_SetValuestring(item, "new_value");
    TEST_ASSERT_NULL(result);

    /* Manually clear the reference flag so cJSON_Delete works correctly */
    item->type &= ~cJSON_IsReference;
    cJSON_Delete(item);
}

/* 4. Object's valuestring is NULL → returns NULL */
void test_SetValuestring_null_valuestring_in_object_returns_null(void)
{
    cJSON *item = make_string_item("original");
    TEST_ASSERT_NOT_NULL(item);

    /* Corrupt the object by freeing its valuestring and setting to NULL */
    cJSON_free(item->valuestring);
    item->valuestring = NULL;

    char *result = cJSON_SetValuestring(item, "new_value");
    TEST_ASSERT_NULL(result);

    /* Prevent double-free in cJSON_Delete */
    cJSON_Delete(item);
}

/* 5. New valuestring argument is NULL → returns NULL */
void test_SetValuestring_null_new_value_returns_null(void)
{
    cJSON *item = make_string_item("original");
    TEST_ASSERT_NOT_NULL(item);

    char *result = cJSON_SetValuestring(item, NULL);
    TEST_ASSERT_NULL(result);

    cJSON_Delete(item);
}

/* 6. New value shorter than existing → in-place copy, returns same pointer */
void test_SetValuestring_shorter_value_inplace(void)
{
    cJSON *item = make_string_item("hello_world");
    TEST_ASSERT_NOT_NULL(item);

    char *old_ptr = item->valuestring;
    char *result = cJSON_SetValuestring(item, "hi");

    TEST_ASSERT_NOT_NULL(result);
    /* Should be the same buffer (in-place) */
    TEST_ASSERT_EQUAL_PTR(old_ptr, result);
    TEST_ASSERT_EQUAL_STRING("hi", item->valuestring);

    cJSON_Delete(item);
}

/* 7. New value same length as existing → in-place copy, returns same pointer */
void test_SetValuestring_same_length_value_inplace(void)
{
    cJSON *item = make_string_item("hello");
    TEST_ASSERT_NOT_NULL(item);

    char *old_ptr = item->valuestring;
    char *result = cJSON_SetValuestring(item, "world");

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_PTR(old_ptr, result);
    TEST_ASSERT_EQUAL_STRING("world", item->valuestring);

    cJSON_Delete(item);
}

/* 8. New value longer than existing → new allocation, old buffer freed */
void test_SetValuestring_longer_value_reallocates(void)
{
    cJSON *item = make_string_item("hi");
    TEST_ASSERT_NOT_NULL(item);

    char *old_ptr = item->valuestring;
    char *result = cJSON_SetValuestring(item, "hello_world_long_string");

    TEST_ASSERT_NOT_NULL(result);
    /* New allocation: pointer must differ from old */
    TEST_ASSERT_NOT_EQUAL(old_ptr, result);
    TEST_ASSERT_EQUAL_STRING("hello_world_long_string", item->valuestring);

    cJSON_Delete(item);
}

/* 9. New value longer → valuestring pointer on item is updated */
void test_SetValuestring_longer_value_updates_item_valuestring(void)
{
    cJSON *item = make_string_item("ab");
    TEST_ASSERT_NOT_NULL(item);

    char *result = cJSON_SetValuestring(item, "abcdefghijklmnop");

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_PTR(result, item->valuestring);
    TEST_ASSERT_EQUAL_STRING("abcdefghijklmnop", item->valuestring);

    cJSON_Delete(item);
}

/* 10. Empty string as new value (shorter than any non-empty existing) → in-place */
void test_SetValuestring_empty_new_value_inplace(void)
{
    cJSON *item = make_string_item("non_empty");
    TEST_ASSERT_NOT_NULL(item);

    char *old_ptr = item->valuestring;
    char *result = cJSON_SetValuestring(item, "");

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_PTR(old_ptr, result);
    TEST_ASSERT_EQUAL_STRING("", item->valuestring);

    cJSON_Delete(item);
}

/* 11. Both existing and new value are empty strings → in-place */
void test_SetValuestring_both_empty_strings(void)
{
    cJSON *item = make_string_item("");
    TEST_ASSERT_NOT_NULL(item);

    char *old_ptr = item->valuestring;
    char *result = cJSON_SetValuestring(item, "");

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_PTR(old_ptr, result);
    TEST_ASSERT_EQUAL_STRING("", item->valuestring);

    cJSON_Delete(item);
}

/* 12. New value is a single character (shorter than existing) → in-place */
void test_SetValuestring_single_char_new_value(void)
{
    cJSON *item = make_string_item("abcdef");
    TEST_ASSERT_NOT_NULL(item);

    char *old_ptr = item->valuestring;
    char *result = cJSON_SetValuestring(item, "x");

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_PTR(old_ptr, result);
    TEST_ASSERT_EQUAL_STRING("x", item->valuestring);

    cJSON_Delete(item);
}

/* 13. cJSON_False type → returns NULL (not a string type) */
void test_SetValuestring_false_type_returns_null(void)
{
    cJSON *item = cJSON_CreateFalse();
    TEST_ASSERT_NOT_NULL(item);

    char *result = cJSON_SetValuestring(item, "value");
    TEST_ASSERT_NULL(result);

    cJSON_Delete(item);
}

/* 14. cJSON_NULL type → returns NULL */
void test_SetValuestring_null_type_returns_null(void)
{
    cJSON *item = cJSON_CreateNull();
    TEST_ASSERT_NOT_NULL(item);

    char *result = cJSON_SetValuestring(item, "value");
    TEST_ASSERT_NULL(result);

    cJSON_Delete(item);
}

/* 15. cJSON_Array type → returns NULL */
void test_SetValuestring_array_type_returns_null(void)
{
    cJSON *item = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(item);

    char *result = cJSON_SetValuestring(item, "value");
    TEST_ASSERT_NULL(result);

    cJSON_Delete(item);
}

/* 16. cJSON_Object type → returns NULL */
void test_SetValuestring_object_type_returns_null(void)
{
    cJSON *item = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(item);

    char *result = cJSON_SetValuestring(item, "value");
    TEST_ASSERT_NULL(result);

    cJSON_Delete(item);
}

/* 17. Verify content correctness after longer replacement */
void test_SetValuestring_longer_value_content_correct(void)
{
    const char *new_val = "this_is_a_much_longer_replacement_string";
    cJSON *item = make_string_item("short");
    TEST_ASSERT_NOT_NULL(item);

    char *result = cJSON_SetValuestring(item, new_val);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING(new_val, result);
    TEST_ASSERT_EQUAL_STRING(new_val, item->valuestring);

    cJSON_Delete(item);
}

/* 18. Multiple sequential calls: first shorter, then longer */
void test_SetValuestring_multiple_calls(void)
{
    cJSON *item = make_string_item("medium_string");
    TEST_ASSERT_NOT_NULL(item);

    /* First call: shorter → in-place */
    char *result1 = cJSON_SetValuestring(item, "short");
    TEST_ASSERT_NOT_NULL(result1);
    TEST_ASSERT_EQUAL_STRING("short", item->valuestring);

    /* Second call: longer → reallocate */
    char *result2 = cJSON_SetValuestring(item, "a_very_long_string_indeed");
    TEST_ASSERT_NOT_NULL(result2);
    TEST_ASSERT_EQUAL_STRING("a_very_long_string_indeed", item->valuestring);

    /* Third call: shorter again → in-place */
    char *result3 = cJSON_SetValuestring(item, "ok");
    TEST_ASSERT_NOT_NULL(result3);
    TEST_ASSERT_EQUAL_STRING("ok", item->valuestring);

    cJSON_Delete(item);
}

/* 19. String item created via cJSON_Parse → SetValuestring works */
void test_SetValuestring_on_parsed_string_item(void)
{
    cJSON *root = cJSON_Parse("{\"key\":\"original_value\"}");
    TEST_ASSERT_NOT_NULL(root);

    cJSON *item = cJSON_GetObjectItem(root, "key");
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_TRUE(item->type & cJSON_String);

    char *result = cJSON_SetValuestring(item, "new_value");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("new_value", item->valuestring);

    cJSON_Delete(root);
}

/* 20. String item created via cJSON_Parse, replace with longer value */
void test_SetValuestring_on_parsed_string_item_longer(void)
{
    cJSON *root = cJSON_Parse("{\"key\":\"hi\"}");
    TEST_ASSERT_NOT_NULL(root);

    cJSON *item = cJSON_GetObjectItem(root, "key");
    TEST_ASSERT_NOT_NULL(item);

    char *result = cJSON_SetValuestring(item, "a_much_longer_replacement");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("a_much_longer_replacement", item->valuestring);

    cJSON_Delete(root);
}

/* -------------------------------------------------------------------------
 * main
 * ------------------------------------------------------------------------- */

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();

    RUN_TEST(test_SetValuestring_null_object_returns_null);
    RUN_TEST(test_SetValuestring_non_string_type_returns_null);
    RUN_TEST(test_SetValuestring_is_reference_returns_null);
    RUN_TEST(test_SetValuestring_null_valuestring_in_object_returns_null);
    RUN_TEST(test_SetValuestring_null_new_value_returns_null);
    RUN_TEST(test_SetValuestring_shorter_value_inplace);
    RUN_TEST(test_SetValuestring_same_length_value_inplace);
    RUN_TEST(test_SetValuestring_longer_value_reallocates);
    RUN_TEST(test_SetValuestring_longer_value_updates_item_valuestring);
    RUN_TEST(test_SetValuestring_empty_new_value_inplace);
    RUN_TEST(test_SetValuestring_both_empty_strings);
    RUN_TEST(test_SetValuestring_single_char_new_value);
    RUN_TEST(test_SetValuestring_false_type_returns_null);
    RUN_TEST(test_SetValuestring_null_type_returns_null);
    RUN_TEST(test_SetValuestring_array_type_returns_null);
    RUN_TEST(test_SetValuestring_object_type_returns_null);
    RUN_TEST(test_SetValuestring_longer_value_content_correct);
    RUN_TEST(test_SetValuestring_multiple_calls);
    RUN_TEST(test_SetValuestring_on_parsed_string_item);
    RUN_TEST(test_SetValuestring_on_parsed_string_item_longer);

    return UNITY_END();
}