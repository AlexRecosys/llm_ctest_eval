#include "unity.h"
#include "cJSON.h"

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>

/* -------------------------------------------------------------------------
 * Internal types mirrored from cJSON.c so we can construct parse_buffer
 * instances directly without touching private internals through the public API.
 * These must match the definitions inside cJSON.c exactly.
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
 * We pull it in by including the .c source directly so the linker can see it.
 * If your build system compiles cJSON.c separately, replace the include below
 * with an extern declaration and link accordingly.
 *
 * Because parse_number is declared static inside cJSON.c we need the source.
 * ------------------------------------------------------------------------- */
/* Provide the minimal stubs that cJSON.c needs so it compiles standalone */
#include "cJSON.c"   /* brings in parse_number and all helpers */

/* -------------------------------------------------------------------------
 * File-scope fixtures
 * ------------------------------------------------------------------------- */
static cJSON        *g_item   = NULL;
static parse_buffer  g_buf;

/* -------------------------------------------------------------------------
 * Helper: initialise a parse_buffer pointing at a C string
 * ------------------------------------------------------------------------- */
static void init_parse_buffer(parse_buffer *pb,
                               const char   *content,
                               size_t        offset)
{
    memset(pb, 0, sizeof(*pb));
    pb->content        = (const unsigned char *)content;
    pb->length         = strlen(content);
    pb->offset         = offset;
    pb->depth          = 0;
    pb->hooks.allocate   = malloc;
    pb->hooks.deallocate = free;
    pb->hooks.reallocate = realloc;
}

/* -------------------------------------------------------------------------
 * setUp / tearDown
 * ------------------------------------------------------------------------- */
void setUp(void)
{
    g_item = (cJSON *)calloc(1, sizeof(cJSON));
    memset(&g_buf, 0, sizeof(g_buf));
}

void tearDown(void)
{
    free(g_item);
    g_item = NULL;
}

/* =========================================================================
 * Test cases
 * ========================================================================= */

/* 1. Parse a simple positive integer */
void test_parse_number_positive_integer(void)
{
    const char *input = "42";
    init_parse_buffer(&g_buf, input, 0);

    cJSON_bool result = parse_number(g_item, &g_buf);

    TEST_ASSERT_TRUE_MESSAGE(result, "parse_number should return true for '42'");
    TEST_ASSERT_EQUAL_INT_MESSAGE(cJSON_Number, g_item->type,
                                  "item type should be cJSON_Number");
    TEST_ASSERT_EQUAL_DOUBLE_MESSAGE(42.0, g_item->valuedouble,
                                     "valuedouble should be 42.0");
    TEST_ASSERT_EQUAL_INT_MESSAGE(42, g_item->valueint,
                                  "valueint should be 42");
    /* offset should have advanced past the two digits */
    TEST_ASSERT_EQUAL_UINT_MESSAGE(2, g_buf.offset,
                                   "offset should advance by 2 characters");
}

/* 2. Parse a negative floating-point number */
void test_parse_number_negative_float(void)
{
    const char *input = "-3.14";
    init_parse_buffer(&g_buf, input, 0);

    cJSON_bool result = parse_number(g_item, &g_buf);

    TEST_ASSERT_TRUE_MESSAGE(result, "parse_number should return true for '-3.14'");
    TEST_ASSERT_EQUAL_INT_MESSAGE(cJSON_Number, g_item->type,
                                  "item type should be cJSON_Number");
    TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(1e-9, -3.14, g_item->valuedouble,
                                      "valuedouble should be -3.14");
    TEST_ASSERT_EQUAL_INT_MESSAGE(-3, g_item->valueint,
                                  "valueint should be -3 (truncated)");
    TEST_ASSERT_EQUAL_UINT_MESSAGE(5, g_buf.offset,
                                   "offset should advance by 5 characters");
}

/* 3. Parse a number in scientific notation */
void test_parse_number_scientific_notation(void)
{
    const char *input = "1.5e2";
    init_parse_buffer(&g_buf, input, 0);

    cJSON_bool result = parse_number(g_item, &g_buf);

    TEST_ASSERT_TRUE_MESSAGE(result, "parse_number should return true for '1.5e2'");
    TEST_ASSERT_EQUAL_INT_MESSAGE(cJSON_Number, g_item->type,
                                  "item type should be cJSON_Number");
    TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(1e-9, 150.0, g_item->valuedouble,
                                      "valuedouble should be 150.0");
    TEST_ASSERT_EQUAL_INT_MESSAGE(150, g_item->valueint,
                                  "valueint should be 150");
    TEST_ASSERT_EQUAL_UINT_MESSAGE(5, g_buf.offset,
                                   "offset should advance by 5 characters");
}

/* 4. NULL input_buffer should return false */
void test_parse_number_null_input_buffer(void)
{
    cJSON_bool result = parse_number(g_item, NULL);

    TEST_ASSERT_FALSE_MESSAGE(result,
                              "parse_number should return false for NULL input_buffer");
    /* item should remain untouched */
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, g_item->type,
                                  "item type should remain 0 when input is NULL");
}

/* 5. Overflow: a number larger than INT_MAX should saturate valueint to INT_MAX */
void test_parse_number_overflow_saturates_to_int_max(void)
{
    /* 1e18 is well above INT_MAX on all common platforms */
    const char *input = "1000000000000000000";
    init_parse_buffer(&g_buf, input, 0);

    cJSON_bool result = parse_number(g_item, &g_buf);

    TEST_ASSERT_TRUE_MESSAGE(result,
                             "parse_number should return true for a large number");
    TEST_ASSERT_EQUAL_INT_MESSAGE(cJSON_Number, g_item->type,
                                  "item type should be cJSON_Number");
    TEST_ASSERT_EQUAL_INT_MESSAGE(INT_MAX, g_item->valueint,
                                  "valueint should be saturated to INT_MAX");
    TEST_ASSERT_GREATER_THAN_MESSAGE(0, (int)(g_item->valuedouble),
                                     "valuedouble should be a large positive number");
}

/* =========================================================================
 * main
 * ========================================================================= */
int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_parse_number_positive_integer);
    RUN_TEST(test_parse_number_negative_float);
    RUN_TEST(test_parse_number_scientific_notation);
    RUN_TEST(test_parse_number_null_input_buffer);
    RUN_TEST(test_parse_number_overflow_saturates_to_int_max);

    return UNITY_END();
}