#include "unity.h"
#include "cJSON.h"

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>

/* -------------------------------------------------------------------------
 * Internal types mirrored from cJSON.c so we can construct parse_buffer
 * instances directly without touching private source.
 * These must match the definitions in cJSON.c exactly.
 * ------------------------------------------------------------------------- */

typedef struct internal_hooks
{
    void *(CJSON_CDECL *allocate)(size_t size);
    void  (CJSON_CDECL *deallocate)(void *pointer);
    void *(CJSON_CDECL *reallocate)(void *pointer, size_t size);
} internal_hooks;

typedef struct parse_buffer
{
    const unsigned char *content;
    size_t               length;
    size_t               offset;
    size_t               depth;
    internal_hooks       hooks;
} parse_buffer;

/* -------------------------------------------------------------------------
 * Forward-declare the static function under test.
 * We pull it in via a direct #include of the .c file so the compiler sees
 * the static definition.  Adjust the path if your build places cJSON.c
 * elsewhere.
 * ------------------------------------------------------------------------- */
/* We cannot call a static function from outside its translation unit, so we
 * test parse_number indirectly through cJSON_Parse / cJSON_ParseWithLength,
 * which exercise exactly the same code path.  All five test cases use the
 * public API and verify the observable effects that parse_number produces.
 */

/* -------------------------------------------------------------------------
 * File-scope fixtures
 * ------------------------------------------------------------------------- */
static cJSON *g_item = NULL;

/* -------------------------------------------------------------------------
 * Helper: parse a JSON number string and return the resulting cJSON item.
 * Caller must cJSON_Delete() the returned pointer.
 * ------------------------------------------------------------------------- */
static cJSON *parse_number_string(const char *json_number)
{
    return cJSON_Parse(json_number);
}

/* -------------------------------------------------------------------------
 * setUp / tearDown
 * ------------------------------------------------------------------------- */
void setUp(void)
{
    g_item = NULL;
}

void tearDown(void)
{
    if (g_item != NULL)
    {
        cJSON_Delete(g_item);
        g_item = NULL;
    }
}

/* =========================================================================
 * Test cases
 * ========================================================================= */

/* 1. Parse a simple positive integer */
void test_parse_number_positive_integer(void)
{
    g_item = parse_number_string("42");

    TEST_ASSERT_NOT_NULL_MESSAGE(g_item, "cJSON_Parse should not return NULL for '42'");
    TEST_ASSERT_EQUAL_INT_MESSAGE(cJSON_Number, g_item->type,
                                  "type should be cJSON_Number");
    TEST_ASSERT_EQUAL_DOUBLE_MESSAGE(42.0, g_item->valuedouble,
                                     "valuedouble should be 42.0");
    TEST_ASSERT_EQUAL_INT_MESSAGE(42, g_item->valueint,
                                  "valueint should be 42");
}

/* 2. Parse a negative integer */
void test_parse_number_negative_integer(void)
{
    g_item = parse_number_string("-7");

    TEST_ASSERT_NOT_NULL_MESSAGE(g_item, "cJSON_Parse should not return NULL for '-7'");
    TEST_ASSERT_EQUAL_INT_MESSAGE(cJSON_Number, g_item->type,
                                  "type should be cJSON_Number");
    TEST_ASSERT_EQUAL_DOUBLE_MESSAGE(-7.0, g_item->valuedouble,
                                     "valuedouble should be -7.0");
    TEST_ASSERT_EQUAL_INT_MESSAGE(-7, g_item->valueint,
                                  "valueint should be -7");
}

/* 3. Parse a floating-point number with a decimal point */
void test_parse_number_floating_point(void)
{
    g_item = parse_number_string("3.14159");

    TEST_ASSERT_NOT_NULL_MESSAGE(g_item, "cJSON_Parse should not return NULL for '3.14159'");
    TEST_ASSERT_EQUAL_INT_MESSAGE(cJSON_Number, g_item->type,
                                  "type should be cJSON_Number");
    /* Allow a small tolerance for floating-point representation */
    TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(1e-5, 3.14159, g_item->valuedouble,
                                      "valuedouble should be approximately 3.14159");
    TEST_ASSERT_EQUAL_INT_MESSAGE(3, g_item->valueint,
                                  "valueint should be 3 (truncated)");
}

/* 4. Parse a number in scientific notation */
void test_parse_number_scientific_notation(void)
{
    g_item = parse_number_string("1.5e2");

    TEST_ASSERT_NOT_NULL_MESSAGE(g_item, "cJSON_Parse should not return NULL for '1.5e2'");
    TEST_ASSERT_EQUAL_INT_MESSAGE(cJSON_Number, g_item->type,
                                  "type should be cJSON_Number");
    TEST_ASSERT_EQUAL_DOUBLE_MESSAGE(150.0, g_item->valuedouble,
                                     "valuedouble should be 150.0");
    TEST_ASSERT_EQUAL_INT_MESSAGE(150, g_item->valueint,
                                  "valueint should be 150");
}

/* 5. Parse a very large number that overflows int — valueint should be INT_MAX */
void test_parse_number_overflow_saturates_to_int_max(void)
{
    /* 1e40 is far beyond INT_MAX on any platform */
    g_item = parse_number_string("1e40");

    TEST_ASSERT_NOT_NULL_MESSAGE(g_item, "cJSON_Parse should not return NULL for '1e40'");
    TEST_ASSERT_EQUAL_INT_MESSAGE(cJSON_Number, g_item->type,
                                  "type should be cJSON_Number");
    TEST_ASSERT_GREATER_THAN_MESSAGE(0, g_item->valuedouble,
                                     "valuedouble should be a large positive number");
    TEST_ASSERT_EQUAL_INT_MESSAGE(INT_MAX, g_item->valueint,
                                  "valueint should be saturated to INT_MAX");
}

/* =========================================================================
 * main
 * ========================================================================= */
int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_parse_number_positive_integer);
    RUN_TEST(test_parse_number_negative_integer);
    RUN_TEST(test_parse_number_floating_point);
    RUN_TEST(test_parse_number_scientific_notation);
    RUN_TEST(test_parse_number_overflow_saturates_to_int_max);

    return UNITY_END();
}