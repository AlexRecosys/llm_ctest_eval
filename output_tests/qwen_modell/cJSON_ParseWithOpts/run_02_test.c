#include "cJSON.c"
#include "unity.h"

/* Global variables for test fixtures */
static const char *parse_end = NULL;
static cJSON *result = NULL;

/* Helper functions */

/* Helper to compare two cJSON objects for structural equality */
static cJSON_bool cJSON_Compare_Structural(const cJSON * const a, const cJSON * const b)
{
    if (a == b)
    {
        return cJSON_True;
    }
    if (a == NULL || b == NULL)
    {
        return cJSON_False;
    }
    if (a->type != b->type)
    {
        return cJSON_False;
    }

    /* Skip type-specific checks for non-leaf types */
    switch (a->type & ~cJSON_IsReference)
    {
        case cJSON_String:
            if (0 != strcmp(a->valuestring, b->valuestring))
            {
                return cJSON_False;
            }
            break;
        case cJSON_Number:
            if (a->valuedouble != b->valuedouble)
            {
                return cJSON_False;
            }
            break;
        case cJSON_True:
        case cJSON_False:
        case cJSON_NULL:
            break;
        case cJSON_Array:
        case cJSON_Object:
        {
            const cJSON *child_a = a->child;
            const cJSON *child_b = b->child;
            while (child_a != NULL && child_b != NULL)
            {
                if (!cJSON_Compare_Structural(child_a, child_b))
                {
                    return cJSON_False;
                }
                child_a = child_a->next;
                child_b = child_b->next;
            }
            if (child_a != NULL || child_b != NULL)
            {
                return cJSON_False;
            }
            break;
        }
        default:
            return cJSON_False;
    }

    /* Compare string name (for object items) */
    if ((a->type & (cJSON_Object | cJSON_Array)) == 0 && a->string != NULL && b->string != NULL)
    {
        if (0 != strcmp(a->string, b->string))
        {
            return cJSON_False;
        }
    }

    return cJSON_True;
}

/* Helper to parse and compare expected result */
static void assert_parsed_equal(const char *json, const char *expected_json, cJSON_bool require_null_terminated)
{
    result = cJSON_ParseWithOpts(json, &parse_end, require_null_terminated);
    TEST_ASSERT_NOT_NULL_MESSAGE(result, "Parsing failed unexpectedly");

    cJSON *expected = cJSON_Parse(expected_json);
    TEST_ASSERT_NOT_NULL_MESSAGE(expected, "Expected JSON parsing failed");

    TEST_ASSERT_TRUE_MESSAGE(cJSON_Compare_Structural(result, expected),
                             "Parsed result does not match expected structure");

    cJSON_Delete(expected);
    cJSON_Delete(result);
    result = NULL;
}

/* Test Cases */

void test_cJSON_ParseWithOpts_NULL_input_returns_NULL(void)
{
    result = cJSON_ParseWithOpts(NULL, &parse_end, cJSON_False);
    TEST_ASSERT_NULL(result);
}

void test_cJSON_ParseWithOpts_empty_string_returns_NULL_terminated_required(void)
{
    result = cJSON_ParseWithOpts("", &parse_end, cJSON_True);
    TEST_ASSERT_NULL(result);
}

void test_cJSON_ParseWithOpts_empty_string_returns_NULL_terminated_not_required(void)
{
    result = cJSON_ParseWithOpts("", &parse_end, cJSON_False);
    TEST_ASSERT_NULL(result);
}

void test_cJSON_ParseWithOpts_simple_valid_json_returns_object(void)
{
    result = cJSON_ParseWithOpts("{}", &parse_end, cJSON_True);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(cJSON_Object, result->type);
    TEST_ASSERT_NULL(result->child);
    TEST_ASSERT_EQUAL_PTR(&parse_end[-1], "\0"); /* Ensure null terminator was consumed */
}

void test_cJSON_ParseWithOpts_simple_array_returns_object(void)
{
    result = cJSON_ParseWithOpts("[]", &parse_end, cJSON_True);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(cJSON_Array, result->type);
    TEST_ASSERT_NULL(result->child);
}

void test_cJSON_ParseWithOpts_number_returns_correct_value(void)
{
    result = cJSON_ParseWithOpts("42", &parse_end, cJSON_True);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, result->type);
    TEST_ASSERT_EQUAL_DOUBLE(42.0, result->valuedouble);
}

void test_cJSON_ParseWithOpts_string_returns_correct_value(void)
{
    result = cJSON_ParseWithOpts("\"hello\"", &parse_end, cJSON_True);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(cJSON_String, result->type);
    TEST_ASSERT_EQUAL_STRING("hello", result->valuestring);
}

void test_cJSON_ParseWithOpts_nested_object_returns_correct_structure(void)
{
    const char *json = "{\"a\":1,\"b\":{\"c\":2}}";
    const char *expected = "{\"a\":1,\"b\":{\"c\":2}}";

    assert_parsed_equal(json, expected, cJSON_True);
}

void test_cJSON_ParseWithOpts_array_with_elements_returns_correct_structure(void)
{
    const char *json = "[1,2,3]";
    const char *expected = "[1,2,3]";

    assert_parsed_equal(json, expected, cJSON_True);
}

void test_cJSON_ParseWithOpts_null_terminated_required_and_missing_returns_NULL(void)
{
    /* Manually construct non-null-terminated string */
    const char *json = "{\"a\":1}";
    size_t len = strlen(json);
    char *non_null_terminated = (char*)cJSON_malloc(len);
    memcpy(non_null_terminated, json, len);

    result = cJSON_ParseWithOpts(non_null_terminated, &parse_end, cJSON_True);
    TEST_ASSERT_NULL(result);

    cJSON_free(non_null_terminated);
}

void test_cJSON_ParseWithOpts_null_terminated_not_required_and_missing_returns_success(void)
{
    const char *json = "{\"a\":1}";
    size_t len = strlen(json);
    char *non_null_terminated = (char*)cJSON_malloc(len);
    memcpy(non_null_terminated, json, len);

    result = cJSON_ParseWithOpts(non_null_terminated, &parse_end, cJSON_False);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(cJSON_Object, result->type);

    cJSON_Delete(result);
    cJSON_free(non_null_terminated);
}

void test_cJSON_ParseWithOpts_return_parse_end_points_to_null_terminator_when_required(void)
{
    const char *json = "{\"a\":1}";
    result = cJSON_ParseWithOpts(json, &parse_end, cJSON_True);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_PTR(&json[strlen(json)], parse_end);
}

void test_cJSON_ParseWithOpts_return_parse_end_points_to_last_parsed_char_when_not_required(void)
{
    const char *json = "{\"a\":1}";
    size_t len = strlen(json);
    char *non_null_terminated = (char*)cJSON_malloc(len);
    memcpy(non_null_terminated, json, len);

    result = cJSON_ParseWithOpts(non_null_terminated, &parse_end, cJSON_False);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_PTR(&non_null_terminated[len - 1], parse_end);

    cJSON_Delete(result);
    cJSON_free(non_null_terminated);
}

void test_cJSON_ParseWithOpts_invalid_json_returns_NULL_and_error_ptr_set(void)
{
    const char *json = "{invalid}";
    result = cJSON_ParseWithOpts(json, &parse_end, cJSON_True);
    TEST_ASSERT_NULL(result);
    TEST_ASSERT_NOT_NULL(cJSON_GetErrorPtr());
}

void test_cJSON_ParseWithOpts_whitespace_only_returns_NULL(void)
{
    result = cJSON_ParseWithOpts("   \t\n\r  ", &parse_end, cJSON_True);
    TEST_ASSERT_NULL(result);
}

void test_cJSON_ParseWithOpts_valid_json_with_whitespace_returns_success(void)
{
    const char *json = "  { \"key\" : \"value\" }  ";
    const char *expected = "{\"key\":\"value\"}";

    assert_parsed_equal(json, expected, cJSON_True);
}

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();

    RUN_TEST(test_cJSON_ParseWithOpts_NULL_input_returns_NULL);
    RUN_TEST(test_cJSON_ParseWithOpts_empty_string_returns_NULL_terminated_required);
    RUN_TEST(test_cJSON_ParseWithOpts_empty_string_returns_NULL_terminated_not_required);
    RUN_TEST(test_cJSON_ParseWithOpts_simple_valid_json_returns_object);
    RUN_TEST(test_cJSON_ParseWithOpts_simple_array_returns_object);
    RUN_TEST(test_cJSON_ParseWithOpts_number_returns_correct_value);
    RUN_TEST(test_cJSON_ParseWithOpts_string_returns_correct_value);
    RUN_TEST(test_cJSON_ParseWithOpts_nested_object_returns_correct_structure);
    RUN_TEST(test_cJSON_ParseWithOpts_array_with_elements_returns_correct_structure);
    RUN_TEST(test_cJSON_ParseWithOpts_null_terminated_required_and_missing_returns_NULL);
    RUN_TEST(test_cJSON_ParseWithOpts_null_terminated_not_required_and_missing_returns_success);
    RUN_TEST(test_cJSON_ParseWithOpts_return_parse_end_points_to_null_terminator_when_required);
    RUN_TEST(test_cJSON_ParseWithOpts_return_parse_end_points_to_last_parsed_char_when_not_required);
    RUN_TEST(test_cJSON_ParseWithOpts_invalid_json_returns_NULL_and_error_ptr_set);
    RUN_TEST(test_cJSON_ParseWithOpts_whitespace_only_returns_NULL);
    RUN_TEST(test_cJSON_ParseWithOpts_valid_json_with_whitespace_returns_success);

    return UNITY_END();
}