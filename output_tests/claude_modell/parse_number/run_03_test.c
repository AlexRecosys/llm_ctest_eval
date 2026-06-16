#include "cJSON.c"
#include "unity.h"
#include "cJSON.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <float.h>

/* -------------------------------------------------------------------------
 * Helpers
 * ---------------------------------------------------------------------- */

static parse_buffer make_parse_buffer(const unsigned char *content, size_t length, size_t offset)
{
    parse_buffer pb;
    memset(&pb, 0, sizeof(pb));
    pb.content = content;
    pb.length  = length;
    pb.offset  = offset;
    pb.hooks.allocate   = malloc;
    pb.hooks.deallocate = free;
    pb.hooks.reallocate = realloc;
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
 * ---------------------------------------------------------------------- */

void setUp(void)
{
}

void tearDown(void)
{
}

/* -------------------------------------------------------------------------
 * Test cases
 * ---------------------------------------------------------------------- */

/* NULL input_buffer → false */
void test_parse_number_null_input_buffer(void)
{
    cJSON item = make_empty_item();
    cJSON_bool result = parse_number(&item, NULL);
    TEST_ASSERT_FALSE(result);
}

/* input_buffer with NULL content → false */
void test_parse_number_null_content(void)
{
    cJSON item = make_empty_item();
    parse_buffer pb;
    memset(&pb, 0, sizeof(pb));
    pb.content = NULL;
    pb.length  = 0;
    pb.offset  = 0;
    pb.hooks.allocate   = malloc;
    pb.hooks.deallocate = free;
    pb.hooks.reallocate = realloc;
    cJSON_bool result = parse_number(&item, &pb);
    TEST_ASSERT_FALSE(result);
}

/* Simple integer zero */
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

/* Positive integer */
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

/* Negative integer */
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

/* Floating-point number */
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

/* Negative float */
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

/* Scientific notation with lowercase e */
void test_parse_number_scientific_lowercase_e(void)
{
    cJSON item = make_empty_item();
    const unsigned char *content = (const unsigned char *)"1e3";
    parse_buffer pb = make_parse_buffer(content, 3, 0);

    cJSON_bool result = parse_number(&item, &pb);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_DOUBLE_WITHIN(1e-6, 1000.0, item.valuedouble);
    TEST_ASSERT_EQUAL_INT(1000, item.valueint);
}

/* Scientific notation with uppercase E */
void test_parse_number_scientific_uppercase_e(void)
{
    cJSON item = make_empty_item();
    const unsigned char *content = (const unsigned char *)"2E4";
    parse_buffer pb = make_parse_buffer(content, 3, 0);

    cJSON_bool result = parse_number(&item, &pb);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_DOUBLE_WITHIN(1e-6, 20000.0, item.valuedouble);
    TEST_ASSERT_EQUAL_INT(20000, item.valueint);
}

/* Scientific notation with negative exponent */
void test_parse_number_scientific_negative_exponent(void)
{
    cJSON item = make_empty_item();
    const unsigned char *content = (const unsigned char *)"1.5e-2";
    parse_buffer pb = make_parse_buffer(content, 6, 0);

    cJSON_bool result = parse_number(&item, &pb);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_DOUBLE_WITHIN(1e-8, 0.015, item.valuedouble);
    TEST_ASSERT_EQUAL_INT(0, item.valueint);
}

/* Scientific notation with positive exponent sign */
void test_parse_number_scientific_positive_exponent(void)
{
    cJSON item = make_empty_item();
    const unsigned char *content = (const unsigned char *)"2.5e+2";
    parse_buffer pb = make_parse_buffer(content, 6, 0);

    cJSON_bool result = parse_number(&item, &pb);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_DOUBLE_WITHIN(1e-6, 250.0, item.valuedouble);
    TEST_ASSERT_EQUAL_INT(250, item.valueint);
}

/* Number followed by non-numeric characters — offset should stop at end of number */
void test_parse_number_stops_at_non_numeric(void)
{
    cJSON item = make_empty_item();
    const unsigned char *content = (const unsigned char *)"123abc";
    parse_buffer pb = make_parse_buffer(content, 6, 0);

    cJSON_bool result = parse_number(&item, &pb);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_DOUBLE_WITHIN(1e-10, 123.0, item.valuedouble);
    TEST_ASSERT_EQUAL_INT(3, (int)pb.offset);
}

/* Number with non-zero starting offset */
void test_parse_number_with_offset(void)
{
    cJSON item = make_empty_item();
    const unsigned char *content = (const unsigned char *)"xx99";
    parse_buffer pb = make_parse_buffer(content, 4, 2);

    cJSON_bool result = parse_number(&item, &pb);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_DOUBLE_WITHIN(1e-10, 99.0, item.valuedouble);
    TEST_ASSERT_EQUAL_INT(99, item.valueint);
    TEST_ASSERT_EQUAL_INT(4, (int)pb.offset);
}

/* Non-numeric content → false */
void test_parse_number_non_numeric_content(void)
{
    cJSON item = make_empty_item();
    const unsigned char *content = (const unsigned char *)"abc";
    parse_buffer pb = make_parse_buffer(content, 3, 0);

    cJSON_bool result = parse_number(&item, &pb);

    TEST_ASSERT_FALSE(result);
}

/* Empty buffer (length == offset) → false */
void test_parse_number_empty_buffer(void)
{
    cJSON item = make_empty_item();
    const unsigned char *content = (const unsigned char *)"";
    parse_buffer pb = make_parse_buffer(content, 0, 0);

    cJSON_bool result = parse_number(&item, &pb);

    TEST_ASSERT_FALSE(result);
}

/* Overflow: very large positive number → valueint saturates to INT_MAX */
void test_parse_number_overflow_positive(void)
{
    cJSON item = make_empty_item();
    /* 1e308 is within double range but >> INT_MAX */
    const unsigned char *content = (const unsigned char *)"1e308";
    parse_buffer pb = make_parse_buffer(content, 5, 0);

    cJSON_bool result = parse_number(&item, &pb);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(INT_MAX, item.valueint);
}

/* Overflow: very large negative number → valueint saturates to INT_MIN */
void test_parse_number_overflow_negative(void)
{
    cJSON item = make_empty_item();
    const unsigned char *content = (const unsigned char *)"-1e308";
    parse_buffer pb = make_parse_buffer(content, 6, 0);

    cJSON_bool result = parse_number(&item, &pb);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(INT_MIN, item.valueint);
}

/* Exact INT_MAX value → valueint == INT_MAX */
void test_parse_number_exact_int_max(void)
{
    cJSON item = make_empty_item();
    /* INT_MAX = 2147483647 */
    const unsigned char *content = (const unsigned char *)"2147483647";
    parse_buffer pb = make_parse_buffer(content, 10, 0);

    cJSON_bool result = parse_number(&item, &pb);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(INT_MAX, item.valueint);
}

/* Exact INT_MIN value → valueint == INT_MIN */
void test_parse_number_exact_int_min(void)
{
    cJSON item = make_empty_item();
    /* INT_MIN = -2147483648 */
    const unsigned char *content = (const unsigned char *)"-2147483648";
    parse_buffer pb = make_parse_buffer(content, 11, 0);

    cJSON_bool result = parse_number(&item, &pb);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(INT_MIN, item.valueint);
}

/* item type is set to cJSON_Number on success */
void test_parse_number_sets_type(void)
{
    cJSON item = make_empty_item();
    const unsigned char *content = (const unsigned char *)"5";
    parse_buffer pb = make_parse_buffer(content, 1, 0);

    parse_number(&item, &pb);

    TEST_ASSERT_EQUAL_INT(cJSON_Number, item.type);
}

/* Negative zero */
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

/* Number with decimal point only (e.g. "1.") — valid for strtod */
void test_parse_number_trailing_decimal_point(void)
{
    cJSON item = make_empty_item();
    const unsigned char *content = (const unsigned char *)"1.";
    parse_buffer pb = make_parse_buffer(content, 2, 0);

    cJSON_bool result = parse_number(&item, &pb);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_DOUBLE_WITHIN(1e-10, 1.0, item.valuedouble);
}

/* Number starting with decimal point (e.g. ".5") — strtod may or may not parse */
void test_parse_number_leading_decimal_point(void)
{
    cJSON item = make_empty_item();
    /* '.' is accepted by the loop, so number_string_length == 2 */
    const unsigned char *content = (const unsigned char *)".5";
    parse_buffer pb = make_parse_buffer(content, 2, 0);

    /* strtod(".5") == 0.5 on most platforms */
    cJSON_bool result = parse_number(&item, &pb);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_DOUBLE_WITHIN(1e-10, 0.5, item.valuedouble);
}

/* Large multi-digit integer */
void test_parse_number_large_integer(void)
{
    cJSON item = make_empty_item();
    const unsigned char *content = (const unsigned char *)"9999999";
    parse_buffer pb = make_parse_buffer(content, 7, 0);

    cJSON_bool result = parse_number(&item, &pb);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_DOUBLE_WITHIN(1e-6, 9999999.0, item.valuedouble);
    TEST_ASSERT_EQUAL_INT(9999999, item.valueint);
}

/* Offset advances correctly for float */
void test_parse_number_offset_advances_for_float(void)
{
    cJSON item = make_empty_item();
    const unsigned char *content = (const unsigned char *)"1.5,next";
    parse_buffer pb = make_parse_buffer(content, 8, 0);

    cJSON_bool result = parse_number(&item, &pb);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(3, (int)pb.offset);
}

/* -------------------------------------------------------------------------
 * main
 * ---------------------------------------------------------------------- */

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_parse_number_null_input_buffer);
    RUN_TEST(test_parse_number_null_content);
    RUN_TEST(test_parse_number_zero);
    RUN_TEST(test_parse_number_positive_integer);
    RUN_TEST(test_parse_number_negative_integer);
    RUN_TEST(test_parse_number_float);
    RUN_TEST(test_parse_number_negative_float);
    RUN_TEST(test_parse_number_scientific_lowercase_e);
    RUN_TEST(test_parse_number_scientific_uppercase_e);
    RUN_TEST(test_parse_number_scientific_negative_exponent);
    RUN_TEST(test_parse_number_scientific_positive_exponent);
    RUN_TEST(test_parse_number_stops_at_non_numeric);
    RUN_TEST(test_parse_number_with_offset);
    RUN_TEST(test_parse_number_non_numeric_content);
    RUN_TEST(test_parse_number_empty_buffer);
    RUN_TEST(test_parse_number_overflow_positive);
    RUN_TEST(test_parse_number_overflow_negative);
    RUN_TEST(test_parse_number_exact_int_max);
    RUN_TEST(test_parse_number_exact_int_min);
    RUN_TEST(test_parse_number_sets_type);
    RUN_TEST(test_parse_number_negative_zero);
    RUN_TEST(test_parse_number_trailing_decimal_point);
    RUN_TEST(test_parse_number_leading_decimal_point);
    RUN_TEST(test_parse_number_large_integer);
    RUN_TEST(test_parse_number_offset_advances_for_float);
    return UNITY_END();
}