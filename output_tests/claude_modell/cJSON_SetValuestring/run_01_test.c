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
    /* nothing */
}

/* =========================================================================
 * Test cases
 * ========================================================================= */

/* 1. NULL object → NULL */
void test_SetValuestring_null_object_returns_null(void)
{
    char *result = cJSON_SetValuestring(NULL, "hello");
    TEST_ASSERT_NULL(result);
}

/* 2. Object type is not cJSON_String → NULL */
void test_SetValuestring_non_string_type_returns_null(void)
{
    cJSON *item = cJSON_CreateNumber(42.0);
    TEST_ASSERT_NOT_NULL(item);

    char *result = cJSON_SetValuestring(item, "hello");
    TEST_ASSERT_NULL(result);

    cJSON_Delete(item);
}

/* 3. Object has cJSON_IsReference flag set → NULL */
void test_SetValuestring_reference_type_returns_null(void)
{
    cJSON *item = cJSON_CreateString("original");
    TEST_ASSERT_NOT_NULL(item);
    item->type |= cJSON_IsReference;

    char *result = cJSON_SetValuestring(item, "new");
    TEST_ASSERT_NULL(result);

    /* Manually clear the reference flag before deleting so cJSON_Delete
       does not try to free a stack/literal pointer */
    item->type &= ~cJSON_IsReference;
    cJSON_Delete(item);
}

/* 4. NULL valuestring argument → NULL */
void test_SetValuestring_null_valuestring_arg_returns_null(void)
{
    cJSON *item = cJSON_CreateString("original");
    TEST_ASSERT_NOT_NULL(item);

    char *result = cJSON_SetValuestring(item, NULL);
    TEST_ASSERT_NULL(result);

    cJSON_Delete(item);
}

/* 5. Object's valuestring is NULL (corrupted) → NULL */
void test_SetValuestring_corrupted_object_valuestring_null_returns_null(void)
{
    cJSON *item = cJSON_CreateString("original");
    TEST_ASSERT_NOT_NULL(item);

    /* Simulate corruption: free the existing valuestring and set to NULL */
    cJSON_free(item->valuestring);
    item->valuestring = NULL;

    char *result = cJSON_SetValuestring(item, "new");
    TEST_ASSERT_NULL(result);

    /* Avoid double-free in cJSON_Delete by setting valuestring back to NULL */
    item->valuestring = NULL;
    cJSON_Delete(item);
}

/* 6. New string shorter than existing → in-place copy, same pointer */
void test_SetValuestring_shorter_new_string_inplace(void)
{
    cJSON *item = cJSON_CreateString("hello world");
    TEST_ASSERT_NOT_NULL(item);

    char *old_ptr = item->valuestring;
    char *result  = cJSON_SetValuestring(item, "hi");

    TEST_ASSERT_NOT_NULL(result);
    /* Should reuse the same buffer */
    TEST_ASSERT_EQUAL_PTR(old_ptr, result);
    TEST_ASSERT_EQUAL_STRING("hi", item->valuestring);

    cJSON_Delete(item);
}

/* 7. New string same length as existing → in-place copy, same pointer */
void test_SetValuestring_same_length_string_inplace(void)
{
    cJSON *item = cJSON_CreateString("abc");
    TEST_ASSERT_NOT_NULL(item);

    char *old_ptr = item->valuestring;
    char *result  = cJSON_SetValuestring(item, "xyz");

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_PTR(old_ptr, result);
    TEST_ASSERT_EQUAL_STRING("xyz", item->valuestring);

    cJSON_Delete(item);
}

/* 8. New string longer than existing → new allocation, different pointer */
void test_SetValuestring_longer_new_string_reallocates(void)
{
    cJSON *item = cJSON_CreateString("hi");
    TEST_ASSERT_NOT_NULL(item);

    char *old_ptr = item->valuestring;
    char *result  = cJSON_SetValuestring(item, "hello world");

    TEST_ASSERT_NOT_NULL(result);
    /* Must be a new allocation */
    TEST_ASSERT_NOT_EQUAL(old_ptr, result);
    TEST_ASSERT_EQUAL_STRING("hello world", item->valuestring);

    cJSON_Delete(item);
}

/* 9. New string longer → returned pointer equals item->valuestring */
void test_SetValuestring_longer_returns_new_valuestring(void)
{
    cJSON *item = cJSON_CreateString("a");
    TEST_ASSERT_NOT_NULL(item);

    char *result = cJSON_SetValuestring(item, "abcdefghij");

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_PTR(item->valuestring, result);
    TEST_ASSERT_EQUAL_STRING("abcdefghij", result);

    cJSON_Delete(item);
}

/* 10. Empty new string (shorter than any non-empty existing) → in-place */
void test_SetValuestring_empty_new_string_inplace(void)
{
    cJSON *item = cJSON_CreateString("hello");
    TEST_ASSERT_NOT_NULL(item);

    char *old_ptr = item->valuestring;
    char *result  = cJSON_SetValuestring(item, "");

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_PTR(old_ptr, result);
    TEST_ASSERT_EQUAL_STRING("", item->valuestring);

    cJSON_Delete(item);
}

/* 11. Both existing and new are empty strings → in-place */
void test_SetValuestring_both_empty_strings(void)
{
    cJSON *item = cJSON_CreateString("");
    TEST_ASSERT_NOT_NULL(item);

    char *old_ptr = item->valuestring;
    char *result  = cJSON_SetValuestring(item, "");

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_PTR(old_ptr, result);
    TEST_ASSERT_EQUAL_STRING("", item->valuestring);

    cJSON_Delete(item);
}

/* 12. cJSON_Array type → NULL (not a string type) */
void test_SetValuestring_array_type_returns_null(void)
{
    cJSON *item = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL(item);

    char *result = cJSON_SetValuestring(item, "value");
    TEST_ASSERT_NULL(result);

    cJSON_Delete(item);
}

/* 13. cJSON_Object type → NULL (not a string type) */
void test_SetValuestring_object_type_returns_null(void)
{
    cJSON *item = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL(item);

    char *result = cJSON_SetValuestring(item, "value");
    TEST_ASSERT_NULL(result);

    cJSON_Delete(item);
}

/* 14. cJSON_NULL type → NULL */
void test_SetValuestring_null_json_type_returns_null(void)
{
    cJSON *item = cJSON_CreateNull();
    TEST_ASSERT_NOT_NULL(item);

    char *result = cJSON_SetValuestring(item, "value");
    TEST_ASSERT_NULL(result);

    cJSON_Delete(item);
}

/* 15. cJSON_True type → NULL */
void test_SetValuestring_true_type_returns_null(void)
{
    cJSON *item = cJSON_CreateTrue();
    TEST_ASSERT_NOT_NULL(item);

    char *result = cJSON_SetValuestring(item, "value");
    TEST_ASSERT_NULL(result);

    cJSON_Delete(item);
}

/* 16. Multiple sequential calls: first shorter, then longer */
void test_SetValuestring_multiple_calls(void)
{
    cJSON *item = cJSON_CreateString("medium string here");
    TEST_ASSERT_NOT_NULL(item);

    /* First call: shorter → in-place */
    char *r1 = cJSON_SetValuestring(item, "short");
    TEST_ASSERT_NOT_NULL(r1);
    TEST_ASSERT_EQUAL_STRING("short", item->valuestring);

    /* Second call: longer than current "short" → reallocate */
    char *r2 = cJSON_SetValuestring(item, "a much longer string than before");
    TEST_ASSERT_NOT_NULL(r2);
    TEST_ASSERT_EQUAL_STRING("a much longer string than before", item->valuestring);

    cJSON_Delete(item);
}

/* 17. Verify content correctness after in-place replacement */
void test_SetValuestring_content_correct_after_inplace(void)
{
    cJSON *item = cJSON_CreateString("AAAAAAAAAA"); /* 10 chars */
    TEST_ASSERT_NOT_NULL(item);

    char *result = cJSON_SetValuestring(item, "BBBBB"); /* 5 chars */
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("BBBBB", result);
    TEST_ASSERT_EQUAL_STRING("BBBBB", item->valuestring);

    cJSON_Delete(item);
}

/* 18. Verify content correctness after reallocation */
void test_SetValuestring_content_correct_after_realloc(void)
{
    cJSON *item = cJSON_CreateString("AB"); /* 2 chars */
    TEST_ASSERT_NOT_NULL(item);

    char *result = cJSON_SetValuestring(item, "ABCDEFGHIJKLMNOP"); /* 16 chars */
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("ABCDEFGHIJKLMNOP", result);
    TEST_ASSERT_EQUAL_STRING("ABCDEFGHIJKLMNOP", item->valuestring);

    cJSON_Delete(item);
}

/* 19. cJSON_False type → NULL */
void test_SetValuestring_false_type_returns_null(void)
{
    cJSON *item = cJSON_CreateFalse();
    TEST_ASSERT_NOT_NULL(item);

    char *result = cJSON_SetValuestring(item, "value");
    TEST_ASSERT_NULL(result);

    cJSON_Delete(item);
}

/* 20. String item with special characters */
void test_SetValuestring_special_characters(void)
{
    cJSON *item = cJSON_CreateString("placeholder text here");
    TEST_ASSERT_NOT_NULL(item);

    const char *special = "tab:\there newline:\nquote:\"";
    char *result = cJSON_SetValuestring(item, special);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING(special, item->valuestring);

    cJSON_Delete(item);
}

/* =========================================================================
 * main
 * ========================================================================= */

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();

    RUN_TEST(test_SetValuestring_null_object_returns_null);
    RUN_TEST(test_SetValuestring_non_string_type_returns_null);
    RUN_TEST(test_SetValuestring_reference_type_returns_null);
    RUN_TEST(test_SetValuestring_null_valuestring_arg_returns_null);
    RUN_TEST(test_SetValuestring_corrupted_object_valuestring_null_returns_null);
    RUN_TEST(test_SetValuestring_shorter_new_string_inplace);
    RUN_TEST(test_SetValuestring_same_length_string_inplace);
    RUN_TEST(test_SetValuestring_longer_new_string_reallocates);
    RUN_TEST(test_SetValuestring_longer_returns_new_valuestring);
    RUN_TEST(test_SetValuestring_empty_new_string_inplace);
    RUN_TEST(test_SetValuestring_both_empty_strings);
    RUN_TEST(test_SetValuestring_array_type_returns_null);
    RUN_TEST(test_SetValuestring_object_type_returns_null);
    RUN_TEST(test_SetValuestring_null_json_type_returns_null);
    RUN_TEST(test_SetValuestring_true_type_returns_null);
    RUN_TEST(test_SetValuestring_multiple_calls);
    RUN_TEST(test_SetValuestring_content_correct_after_inplace);
    RUN_TEST(test_SetValuestring_content_correct_after_realloc);
    RUN_TEST(test_SetValuestring_false_type_returns_null);
    RUN_TEST(test_SetValuestring_special_characters);

    return UNITY_END();
}