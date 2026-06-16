#include "cJSON.c"
#include "unity.h"
#include "cJSON.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <float.h>

/* -------------------------------------------------------------------------
 * Helpers
 * ------------------------------------------------------------------------- */

static internal_hooks global_hooks_for_tests;

static void init_hooks_for_tests(void)
{
    global_hooks_for_tests.allocate   = malloc;
    global_hooks_for_tests.deallocate = free;
    global_hooks_for_tests.reallocate = realloc;
}

/* Build a minimal parse_buffer that points at the supplied string.
 * length must equal strlen(content_str) (or the desired window). */
static parse_buffer make_parse_buffer(const unsigned char *content, size_t length, size_t offset)
{
    parse_buffer pb;
    memset(&pb, 0, sizeof(pb));
    pb.content = content;
    pb.length  = length;
    pb.offset  = offset;
    pb.hooks   = global_hooks_for_tests;
    return pb;
}

static cJSON make_empty_item(void)
{
    cJSON item;
    memset(&item, 0, sizeof(item));
    return item;
}

/* -------------------------------------------------------------------------
 * setUp / tearDown
 * ------------------------------------------------------------------------- */

void setUp(void)
{
    init_hooks_for_tests();
}

void tearDown(void)
{
}

/* =========================================================================
 * Test cases
 * ========================================================================= */

/* --- NULL / invalid input ------------------------------------------------ */

void test_parse_number_null_input_buffer_returns_false(void)
{
    cJSON item = make_empty_item();
    cJSON_bool result = parse_number(&item, NULL);
    TEST_ASSERT_FALSE(result);
}

void test_parse_number_null_content_returns_false(void)
{
    cJSON item = make_empty_item();
    parse_buffer pb;
    memset(&pb, 0, sizeof(pb));
    pb.content = NULL;
    pb.length  = 0;
    pb.offset  = 0;
    pb.hooks   = global_hooks_for_tests;
    cJSON_bool result = parse_number(&item, &pb);
    TEST_ASSERT_FALSE(result);
}

/* --- Simple integers ----------------------------------------------------- */

void test_parse_number_zero(void)
{
    cJSON item = make_empty_item();
    const unsigned char *content = (const unsigned char *)"0";
    parse_buffer pb = make_parse_buffer(content, 1, 0);

    cJSON_bool result = parse_number(&item, &pb);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, item.type);
    TEST_ASSERT_DOUBLE_WITHIN(1e-10, 0.0, item.valuedouble);
    TEST_ASSERT_EQUAL_INT(0, item.valueint);
    TEST_ASSERT_EQUAL_INT(1, (int)pb.offset);
}

void test_parse_number_positive_integer(void)
{
    cJSON item = make_empty_item();
    const unsigned char *content = (const unsigned char *)"42";
    parse_buffer pb = make_parse_buffer(content, 2, 0);

    cJSON_bool result = parse_number(&item, &pb);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, item.type);
    TEST_ASSERT_DOUBLE_WITHIN(1e-10, 42.0, item.valuedouble);
    TEST_ASSERT_EQUAL_INT(42, item.valueint);
    TEST_ASSERT_EQUAL_INT(2, (int)pb.offset);
}

void test_parse_number_negative_integer(void)
{
    cJSON item = make_empty_item();
    const unsigned char *content = (const unsigned char *)"-7";
    parse_buffer pb = make_parse_buffer(content, 2, 0);

    cJSON_bool result = parse_number(&item, &pb);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, item.type);
    TEST_ASSERT_DOUBLE_WITHIN(1e-10, -7.0, item.valuedouble);
    TEST_ASSERT_EQUAL_INT(-7, item.valueint);
}

void test_parse_number_large_positive_integer(void)
{
    cJSON item = make_empty_item();
    const unsigned char *content = (const unsigned char *)"1000000";
    parse_buffer pb = make_parse_buffer(content, 7, 0);

    cJSON_bool result = parse_number(&item, &pb);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_DOUBLE_WITHIN(1e-5, 1000000.0, item.valuedouble);
    TEST_ASSERT_EQUAL_INT(1000000, item.valueint);
}

/* --- Floating point ------------------------------------------------------ */

void test_parse_number_float(void)
{
    cJSON item = make_empty_item();
    const unsigned char *content = (const unsigned char *)"3.14";
    parse_buffer pb = make_parse_buffer(content, 4, 0);

    cJSON_bool result = parse_number(&item, &pb);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, item.type);
    TEST_ASSERT_DOUBLE_WITHIN(1e-6, 3.14, item.valuedouble);
    TEST_ASSERT_EQUAL_INT(3, item.valueint);
}

void test_parse_number_negative_float(void)
{
    cJSON item = make_empty_item();
    const unsigned char *content = (const unsigned char *)"-2.718";
    parse_buffer pb = make_parse_buffer(content, 6, 0);

    cJSON_bool result = parse_number(&item, &pb);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_DOUBLE_WITHIN(1e-6, -2.718, item.valuedouble);
    TEST_ASSERT_EQUAL_INT(-2, item.valueint);
}

void test_parse_number_zero_point_five(void)
{
    cJSON item = make_empty_item();
    const unsigned char *content = (const unsigned char *)"0.5";
    parse_buffer pb = make_parse_buffer(content, 3, 0);

    cJSON_bool result = parse_number(&item, &pb);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_DOUBLE_WITHIN(1e-10, 0.5, item.valuedouble);
    TEST_ASSERT_EQUAL_INT(0, item.valueint);
}

/* --- Scientific notation ------------------------------------------------- */

void test_parse_number_scientific_notation_lowercase_e(void)
{
    cJSON item = make_empty_item();
    const unsigned char *content = (const unsigned char *)"1e3";
    parse_buffer pb = make_parse_buffer(content, 3, 0);

    cJSON_bool result = parse_number(&item, &pb);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_DOUBLE_WITHIN(1e-5, 1000.0, item.valuedouble);
    TEST_ASSERT_EQUAL_INT(1000, item.valueint);
}

void test_parse_number_scientific_notation_uppercase_E(void)
{
    cJSON item = make_empty_item();
    const unsigned char *content = (const unsigned char *)"2E4";
    parse_buffer pb = make_parse_buffer(content, 3, 0);

    cJSON_bool result = parse_number(&item, &pb);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_DOUBLE_WITHIN(1e-3, 20000.0, item.valuedouble);
}

void test_parse_number_scientific_notation_negative_exponent(void)
{
    cJSON item = make_empty_item();
    const unsigned char *content = (const unsigned char *)"1e-3";
    parse_buffer pb = make_parse_buffer(content, 4, 0);

    cJSON_bool result = parse_number(&item, &pb);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_DOUBLE_WITHIN(1e-10, 0.001, item.valuedouble);
    TEST_ASSERT_EQUAL_INT(0, item.valueint);
}

void test_parse_number_scientific_notation_positive_exponent(void)
{
    cJSON item = make_empty_item();
    const unsigned char *content = (const unsigned char *)"5e+2";
    parse_buffer pb = make_parse_buffer(content, 4, 0);

    cJSON_bool result = parse_number(&item, &pb);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_DOUBLE_WITHIN(1e-5, 500.0, item.valuedouble);
    TEST_ASSERT_EQUAL_INT(500, item.valueint);
}

void test_parse_number_float_with_exponent(void)
{
    cJSON item = make_empty_item();
    const unsigned char *content = (const unsigned char *)"1.5e2";
    parse_buffer pb = make_parse_buffer(content, 5, 0);

    cJSON_bool result = parse_number(&item, &pb);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_DOUBLE_WITHIN(1e-5, 150.0, item.valuedouble);
    TEST_ASSERT_EQUAL_INT(150, item.valueint);
}

/* --- Offset handling ----------------------------------------------------- */

void test_parse_number_offset_advances_correctly(void)
{
    cJSON item = make_empty_item();
    const unsigned char *content = (const unsigned char *)"123abc";
    parse_buffer pb = make_parse_buffer(content, 6, 0);

    cJSON_bool result = parse_number(&item, &pb);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(3, (int)pb.offset);
}

void test_parse_number_with_nonzero_initial_offset(void)
{
    cJSON item = make_empty_item();
    /* content: "xx99" — start parsing at offset 2 */
    const unsigned char *content = (const unsigned char *)"xx99";
    parse_buffer pb = make_parse_buffer(content, 4, 2);

    cJSON_bool result = parse_number(&item, &pb);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_DOUBLE_WITHIN(1e-10, 99.0, item.valuedouble);
    TEST_ASSERT_EQUAL_INT(4, (int)pb.offset);
}

void test_parse_number_offset_stops_at_non_numeric_char(void)
{
    cJSON item = make_empty_item();
    const unsigned char *content = (const unsigned char *)"42,";
    parse_buffer pb = make_parse_buffer(content, 3, 0);

    cJSON_bool result = parse_number(&item, &pb);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(2, (int)pb.offset);
}

/* --- Saturation / overflow ----------------------------------------------- */

void test_parse_number_overflow_positive_saturates_to_INT_MAX(void)
{
    cJSON item = make_empty_item();
    /* A number larger than INT_MAX */
    const unsigned char *content = (const unsigned char *)"99999999999999999999";
    parse_buffer pb = make_parse_buffer(content, strlen((const char *)content), 0);

    cJSON_bool result = parse_number(&item, &pb);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(INT_MAX, item.valueint);
}

void test_parse_number_overflow_negative_saturates_to_INT_MIN(void)
{
    cJSON item = make_empty_item();
    const unsigned char *content = (const unsigned char *)"-99999999999999999999";
    parse_buffer pb = make_parse_buffer(content, strlen((const char *)content), 0);

    cJSON_bool result = parse_number(&item, &pb);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(INT_MIN, item.valueint);
}

void test_parse_number_exactly_INT_MAX_value(void)
{
    cJSON item = make_empty_item();
    /* INT_MAX = 2147483647 */
    const unsigned char *content = (const unsigned char *)"2147483647";
    parse_buffer pb = make_parse_buffer(content, strlen((const char *)content), 0);

    cJSON_bool result = parse_number(&item, &pb);

    TEST_ASSERT_TRUE(result);
    /* >= INT_MAX triggers saturation */
    TEST_ASSERT_EQUAL_INT(INT_MAX, item.valueint);
}

void test_parse_number_exactly_INT_MIN_value(void)
{
    cJSON item = make_empty_item();
    /* INT_MIN = -2147483648 */
    const unsigned char *content = (const unsigned char *)"-2147483648";
    parse_buffer pb = make_parse_buffer(content, strlen((const char *)content), 0);

    cJSON_bool result = parse_number(&item, &pb);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(INT_MIN, item.valueint);
}

/* --- item->type is set --------------------------------------------------- */

void test_parse_number_sets_type_to_cJSON_Number(void)
{
    cJSON item = make_empty_item();
    const unsigned char *content = (const unsigned char *)"1";
    parse_buffer pb = make_parse_buffer(content, 1, 0);

    parse_number(&item, &pb);

    TEST_ASSERT_EQUAL_INT(cJSON_Number, item.type);
}

/* --- Edge: number followed immediately by end of buffer ----------------- */

void test_parse_number_at_exact_buffer_end(void)
{
    cJSON item = make_empty_item();
    const unsigned char *content = (const unsigned char *)"7";
    parse_buffer pb = make_parse_buffer(content, 1, 0);

    cJSON_bool result = parse_number(&item, &pb);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_DOUBLE_WITHIN(1e-10, 7.0, item.valuedouble);
    TEST_ASSERT_EQUAL_INT(1, (int)pb.offset);
}

/* --- Negative zero ------------------------------------------------------- */

void test_parse_number_negative_zero(void)
{
    cJSON item = make_empty_item();
    const unsigned char *content = (const unsigned char *)"-0";
    parse_buffer pb = make_parse_buffer(content, 2, 0);

    cJSON_bool result = parse_number(&item, &pb);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_DOUBLE_WITHIN(1e-10, 0.0, item.valuedouble);
    TEST_ASSERT_EQUAL_INT(0, item.valueint);
}

/* --- Multiple digits ----------------------------------------------------- */

void test_parse_number_multi_digit_integer(void)
{
    cJSON item = make_empty_item();
    const unsigned char *content = (const unsigned char *)"9876543";
    parse_buffer pb = make_parse_buffer(content, 7, 0);

    cJSON_bool result = parse_number(&item, &pb);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_DOUBLE_WITHIN(1.0, 9876543.0, item.valuedouble);
    TEST_ASSERT_EQUAL_INT(9876543, item.valueint);
}

/* --- valuedouble is set correctly for float ----------------------------- */

void test_parse_number_valuedouble_precision(void)
{
    cJSON item = make_empty_item();
    const unsigned char *content = (const unsigned char *)"123.456";
    parse_buffer pb = make_parse_buffer(content, 7, 0);

    cJSON_bool result = parse_number(&item, &pb);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_DOUBLE_WITHIN(1e-6, 123.456, item.valuedouble);
    TEST_ASSERT_EQUAL_INT(123, item.valueint);
}

/* --- Integration: round-trip via public API ----------------------------- */

void test_parse_number_via_public_api_integer(void)
{
    cJSON *item = cJSON_Parse("42");
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_TRUE(cJSON_IsNumber(item));
    TEST_ASSERT_DOUBLE_WITHIN(1e-10, 42.0, item->valuedouble);
    TEST_ASSERT_EQUAL_INT(42, item->valueint);
    cJSON_Delete(item);
}

void test_parse_number_via_public_api_float(void)
{
    cJSON *item = cJSON_Parse("3.14");
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_TRUE(cJSON_IsNumber(item));
    TEST_ASSERT_DOUBLE_WITHIN(1e-6, 3.14, item->valuedouble);
    cJSON_Delete(item);
}

void test_parse_number_via_public_api_negative(void)
{
    cJSON *item = cJSON_Parse("-100");
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_TRUE(cJSON_IsNumber(item));
    TEST_ASSERT_DOUBLE_WITHIN(1e-10, -100.0, item->valuedouble);
    TEST_ASSERT_EQUAL_INT(-100, item->valueint);
    cJSON_Delete(item);
}

void test_parse_number_via_public_api_scientific(void)
{
    cJSON *item = cJSON_Parse("1e10");
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_TRUE(cJSON_IsNumber(item));
    TEST_ASSERT_DOUBLE_WITHIN(1e4, 1e10, item->valuedouble);
    cJSON_Delete(item);
}

/* =========================================================================
 * main
 * ========================================================================= */

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();

    RUN_TEST(test_parse_number_null_input_buffer_returns_false);
    RUN_TEST(test_parse_number_null_content_returns_false);
    RUN_TEST(test_parse_number_zero);
    RUN_TEST(test_parse_number_positive_integer);
    RUN_TEST(test_parse_number_negative_integer);
    RUN_TEST(test_parse_number_large_positive_integer);
    RUN_TEST(test_parse_number_float);
    RUN_TEST(test_parse_number_negative_float);
    RUN_TEST(test_parse_number_zero_point_five);
    RUN_TEST(test_parse_number_scientific_notation_lowercase_e);
    RUN_TEST(test_parse_number_scientific_notation_uppercase_E);
    RUN_TEST(test_parse_number_scientific_notation_negative_exponent);
    RUN_TEST(test_parse_number_scientific_notation_positive_exponent);
    RUN_TEST(test_parse_number_float_with_exponent);
    RUN_TEST(test_parse_number_offset_advances_correctly);
    RUN_TEST(test_parse_number_with_nonzero_initial_offset);
    RUN_TEST(test_parse_number_offset_stops_at_non_numeric_char);
    RUN_TEST(test_parse_number_overflow_positive_saturates_to_INT_MAX);
    RUN_TEST(test_parse_number_overflow_negative_saturates_to_INT_MIN);
    RUN_TEST(test_parse_number_exactly_INT_MAX_value);
    RUN_TEST(test_parse_number_exactly_INT_MIN_value);
    RUN_TEST(test_parse_number_sets_type_to_cJSON_Number);
    RUN_TEST(test_parse_number_at_exact_buffer_end);
    RUN_TEST(test_parse_number_negative_zero);
    RUN_TEST(test_parse_number_multi_digit_integer);
    RUN_TEST(test_parse_number_valuedouble_precision);
    RUN_TEST(test_parse_number_via_public_api_integer);
    RUN_TEST(test_parse_number_via_public_api_float);
    RUN_TEST(test_parse_number_via_public_api_negative);
    RUN_TEST(test_parse_number_via_public_api_scientific);

    return UNITY_END();
}