#include "cJSON.c"
#include "unity.h"
#include <string.h>
#include <stdlib.h>

/* Global hooks for memory management */
extern cJSON_Hooks global_hooks;

/* Helper to create a cJSON_String object with initial valuestring */
static cJSON* create_string_object(const char* initial_value)
{
    cJSON* obj = cJSON_CreateString(initial_value);
    TEST_ASSERT_NOT_NULL_MESSAGE(obj, "Failed to create cJSON string object");
    return obj;
}

/* Helper to verify that valuestring matches expected string */
static void assert_valuestring_matches(cJSON* obj, const char* expected)
{
    if (expected == NULL)
    {
        TEST_ASSERT_NULL_MESSAGE(obj->valuestring, "valuestring should be NULL");
    }
    else
    {
        TEST_ASSERT_EQUAL_STRING_MESSAGE(expected, obj->valuestring, "valuestring mismatch");
    }
}

/* Test: NULL object input */
void test_cJSON_SetValuestring_NULL_object(void)
{
    TEST_ASSERT_NULL(cJSON_SetValuestring(NULL, "test"));
}

/* Test: NULL valuestring input */
void test_cJSON_SetValuestring_NULL_valuestring(void)
{
    cJSON* obj = create_string_object("original");
    TEST_ASSERT_NULL(cJSON_SetValuestring(obj, NULL));
    cJSON_Delete(obj);
}

/* Test: Object not a string type (cJSON_Number) */
void test_cJSON_SetValuestring_non_string_type(void)
{
    cJSON* obj = cJSON_CreateNumber(42.0);
    TEST_ASSERT_NULL(cJSON_SetValuestring(obj, "test"));
    cJSON_Delete(obj);
}

/* Test: Object is a reference (cJSON_IsReference flag set) */
void test_cJSON_SetValuestring_is_reference(void)
{
    cJSON* obj = cJSON_CreateStringReference("original");
    TEST_ASSERT_EQUAL_INT(cJSON_String | cJSON_IsReference, obj->type);
    TEST_ASSERT_NULL(cJSON_SetValuestring(obj, "new"));
    cJSON_Delete(obj);
}

/* Test: Object has NULL valuestring (corrupted state) */
void test_cJSON_SetValuestring_null_valuestring(void)
{
    cJSON* obj = cJSON_CreateString("original");
    obj->valuestring = NULL;
    TEST_ASSERT_NULL(cJSON_SetValuestring(obj, "new"));
    cJSON_Delete(obj);
}

/* Test: Shorter or equal length string fits in existing buffer (no reallocation) */
void test_cJSON_SetValuestring_shorter_or_equal_fit(void)
{
    cJSON* obj = create_string_object("original_long_string");
    const char* new_value = "short";
    char* result = cJSON_SetValuestring(obj, new_value);

    TEST_ASSERT_EQUAL_PTR(obj->valuestring, result);
    assert_valuestring_matches(obj, new_value);
    cJSON_Delete(obj);
}

/* Test: Overlapping strings (same buffer) */
void test_cJSON_SetValuestring_overlapping_strings(void)
{
    cJSON* obj = create_string_object("abcdef");
    /* valuestring points to internal buffer; passing same buffer as valuestring */
    char* overlapping = obj->valuestring + 1; /* e.g., "bcdef" */
    TEST_ASSERT_NULL(cJSON_SetValuestring(obj, overlapping));
    cJSON_Delete(obj);
}

/* Test: Longer string requires reallocation */
void test_cJSON_SetValuestring_longer_string(void)
{
    cJSON* obj = create_string_object("short");
    const char* new_value = "much_longer_string_value";
    char* result = cJSON_SetValuestring(obj, new_value);

    TEST_ASSERT_EQUAL_PTR(obj->valuestring, result);
    TEST_ASSERT_NOT_EQUAL_PTR(obj->valuestring, obj->valuestring); /* buffer changed */
    assert_valuestring_matches(obj, new_value);
    cJSON_Delete(obj);
}

/* Test: Same-length string (exact fit) */
void test_cJSON_SetValuestring_same_length(void)
{
    cJSON* obj = create_string_object("abcdef");
    const char* new_value = "xyz123";
    char* result = cJSON_SetValuestring(obj, new_value);

    TEST_ASSERT_EQUAL_PTR(obj->valuestring, result);
    assert_valuestring_matches(obj, new_value);
    cJSON_Delete(obj);
}

/* Test: Empty string replacement */
void test_cJSON_SetValuestring_empty_string(void)
{
    cJSON* obj = create_string_object("original");
    const char* new_value = "";
    char* result = cJSON_SetValuestring(obj, new_value);

    TEST_ASSERT_EQUAL_PTR(obj->valuestring, result);
    TEST_ASSERT_EQUAL_INT(0, strlen(obj->valuestring));
    cJSON_Delete(obj);
}

/* Test: Replacing with same string (idempotent) */
void test_cJSON_SetValuestring_same_string(void)
{
    cJSON* obj = create_string_object("same");
    char* result = cJSON_SetValuestring(obj, "same");

    TEST_ASSERT_EQUAL_PTR(obj->valuestring, result);
    assert_valuestring_matches(obj, "same");
    cJSON_Delete(obj);
}

/* Test: Memory leak check — old buffer freed when longer string allocated */
void test_cJSON_SetValuestring_old_buffer_freed(void)
{
    cJSON* obj = create_string_object("short");
    char* old_ptr = obj->valuestring;

    /* Replace with longer string */
    cJSON_SetValuestring(obj, "much_longer_string_value");

    /* Old pointer should no longer be valid (freed) */
    /* We cannot directly assert on freed memory, but we verify new pointer is different */
    TEST_ASSERT_NOT_EQUAL_PTR(old_ptr, obj->valuestring);
    cJSON_Delete(obj);
}

/* Test: NULL valuestring after successful set */
void test_cJSON_SetValuestring_after_set_valuestring_null(void)
{
    cJSON* obj = create_string_object("original");
    cJSON_SetValuestring(obj, "new_value");
    TEST_ASSERT_NOT_NULL(obj->valuestring);
    TEST_ASSERT_EQUAL_STRING("new_value", obj->valuestring);

    /* Now try to set to NULL — should fail and leave original unchanged */
    TEST_ASSERT_NULL(cJSON_SetValuestring(obj, NULL));
    TEST_ASSERT_EQUAL_STRING("new_value", obj->valuestring);
    cJSON_Delete(obj);
}

/* Test: Non-reference string with overlapping but non-identical buffers */
void test_cJSON_SetValuestring_non_overlapping_buffers(void)
{
    cJSON* obj = create_string_object("original");
    char external_buffer[100];
    strcpy(external_buffer, "external");

    /* Ensure no overlap: external_buffer is stack-allocated, obj->valuestring is heap */
    char* result = cJSON_SetValuestring(obj, external_buffer);
    TEST_ASSERT_EQUAL_PTR(obj->valuestring, result);
    assert_valuestring_matches(obj, "external");
    cJSON_Delete(obj);
}

/* Test: Very long string replacement */
void test_cJSON_SetValuestring_very_long_string(void)
{
    cJSON* obj = create_string_object("short");
    size_t len = 10000;
    char* long_str = (char*)cJSON_malloc(len + 1);
    memset(long_str, 'x', len);
    long_str[len] = '\0';

    char* result = cJSON_SetValuestring(obj, long_str);
    TEST_ASSERT_EQUAL_PTR(obj->valuestring, result);
    TEST_ASSERT_EQUAL_INT(len, strlen(obj->valuestring));
    TEST_ASSERT_EQUAL_MEMORY(long_str, obj->valuestring, len);

    cJSON_free(long_str);
    cJSON_Delete(obj);
}

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();

    RUN_TEST(test_cJSON_SetValuestring_NULL_object);
    RUN_TEST(test_cJSON_SetValuestring_NULL_valuestring);
    RUN_TEST(test_cJSON_SetValuestring_non_string_type);
    RUN_TEST(test_cJSON_SetValuestring_is_reference);
    RUN_TEST(test_cJSON_SetValuestring_null_valuestring);
    RUN_TEST(test_cJSON_SetValuestring_shorter_or_equal_fit);
    RUN_TEST(test_cJSON_SetValuestring_overlapping_strings);
    RUN_TEST(test_cJSON_SetValuestring_longer_string);
    RUN_TEST(test_cJSON_SetValuestring_same_length);
    RUN_TEST(test_cJSON_SetValuestring_empty_string);
    RUN_TEST(test_cJSON_SetValuestring_same_string);
    RUN_TEST(test_cJSON_SetValuestring_old_buffer_freed);
    RUN_TEST(test_cJSON_SetValuestring_after_set_valuestring_null);
    RUN_TEST(test_cJSON_SetValuestring_non_overlapping_buffers);
    RUN_TEST(test_cJSON_SetValuestring_very_long_string);

    return UNITY_END();
}