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
    const unsigned char *content = (const unsigned char *)"0";
    cJSON item = make_empty_item();
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
    const unsigned char *content = (const unsigned char *)"42";
    cJSON item = make_empty_item();
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
    const unsigned char *content = (const unsigned char *)"-7";
    cJSON item = make_empty_item();
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
    const unsigned char *content = (const unsigned char *)"3.14";
    cJSON item = make_empty_item();
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
    const unsigned char *content = (const unsigned char *)"-1.5";
    cJSON item = make_empty_item();
    parse_buffer pb = make_parse_buffer(content, 4, 0);

    cJSON_bool result = parse_number(&item, &pb);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_DOUBLE_WITHIN(1e-10, -1.5, item.valuedouble);
    TEST_ASSERT_EQUAL_INT(-1, item.valueint);
}

/* Scientific notation with lowercase e */
void test_parse_number_scientific_lowercase_e(void)
{
    const unsigned char *content = (const unsigned char *)"1e3";
    cJSON item = make_empty_item();
    parse_buffer pb = make_parse_buffer(content, 3, 0);

    cJSON_bool result = parse_number(&item, &pb);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_DOUBLE_WITHIN(1e-6, 1000.0, item.valuedouble);
    TEST_ASSERT_EQUAL_INT(1000, item.valueint);
}

/* Scientific notation with uppercase E */
void test_parse_number_scientific_uppercase_e(void)
{
    const unsigned char *content = (const unsigned char *)"2E4";
    cJSON item = make_empty_item();
    parse_buffer pb = make_parse_buffer(content, 3, 0);

    cJSON_bool result = parse_number(&item, &pb);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_DOUBLE_WITHIN(1.0, 20000.0, item.valuedouble);
    TEST_ASSERT_EQUAL_INT(20000, item.valueint);
}

/* Scientific notation with negative exponent */
void test_parse_number_scientific_negative_exponent(void)
{
    const unsigned char *content = (const unsigned char *)"1.5e-2";
    cJSON item = make_empty_item();
    parse_buffer pb = make_parse_buffer(content, 6, 0);

    cJSON_bool result = parse_number(&item, &pb);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_DOUBLE_WITHIN(1e-8, 0.015, item.valuedouble);
    TEST_ASSERT_EQUAL_INT(0, item.valueint);
}

/* Scientific notation with positive exponent sign */
void test_parse_number_scientific_positive_exponent(void)
{
    const unsigned char *content = (const unsigned char *)"2.5e+2";
    cJSON item = make_empty_item();
    parse_buffer pb = make_parse_buffer(content, 6, 0);

    cJSON_bool result = parse_number(&item, &pb);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_DOUBLE_WITHIN(1e-6, 250.0, item.valuedouble);
    TEST_ASSERT_EQUAL_INT(250, item.valueint);
}

/* Number followed by non-numeric characters — offset should stop at end of number */
void test_parse_number_stops_at_non_numeric(void)
{
    const unsigned char *content = (const unsigned char *)"123abc";
    cJSON item = make_empty_item();
    parse_buffer pb = make_parse_buffer(content, 6, 0);

    cJSON_bool result = parse_number(&item, &pb);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_DOUBLE_WITHIN(1e-10, 123.0, item.valuedouble);
    TEST_ASSERT_EQUAL_INT(3, (int)pb.offset);
}

/* Number with offset into buffer */
void test_parse_number_with_offset(void)
{
    const unsigned char *content = (const unsigned char *)"xx99";
    cJSON item = make_empty_item();
    parse_buffer pb = make_parse_buffer(content, 4, 2);

    cJSON_bool result = parse_number(&item, &pb);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_DOUBLE_WITHIN(1e-10, 99.0, item.valuedouble);
    TEST_ASSERT_EQUAL_INT(99, item.valueint);
    TEST_ASSERT_EQUAL_INT(4, (int)pb.offset);
}

/* Empty buffer (length == offset) → false because no valid number chars */
void test_parse_number_empty_buffer(void)
{
    const unsigned char *content = (const unsigned char *)"";
    cJSON item = make_empty_item();
    parse_buffer pb = make_parse_buffer(content, 0, 0);

    cJSON_bool result = parse_number(&item, &pb);

    TEST_ASSERT_FALSE(result);
}

/* Non-numeric content → false */
void test_parse_number_non_numeric_content(void)
{
    const unsigned char *content = (const unsigned char *)"abc";
    cJSON item = make_empty_item();
    parse_buffer pb = make_parse_buffer(content, 3, 0);

    cJSON_bool result = parse_number(&item, &pb);

    TEST_ASSERT_FALSE(result);
}

/* Overflow: very large number → valueint saturates to INT_MAX */
void test_parse_number_overflow_positive(void)
{
    const unsigned char *content = (const unsigned char *)"1e309";
    cJSON item = make_empty_item();
    parse_buffer pb = make_parse_buffer(content, 5, 0);

    cJSON_bool result = parse_number(&item, &pb);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(INT_MAX, item.valueint);
}

/* Overflow: very large negative number → valueint saturates to INT_MIN */
void test_parse_number_overflow_negative(void)
{
    const unsigned char *content = (const unsigned char *)"-1e309";
    cJSON item = make_empty_item();
    parse_buffer pb = make_parse_buffer(content, 6, 0);

    cJSON_bool result = parse_number(&item, &pb);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(INT_MIN, item.valueint);
}

/* Exact INT_MAX value */
void test_parse_number_int_max(void)
{
    const unsigned char *content = (const unsigned char *)"2147483647";
    cJSON item = make_empty_item();
    parse_buffer pb = make_parse_buffer(content, 10, 0);

    cJSON_bool result = parse_number(&item, &pb);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_DOUBLE_WITHIN(1.0, 2147483647.0, item.valuedouble);
}

/* Exact INT_MIN value */
void test_parse_number_int_min(void)
{
    const unsigned char *content = (const unsigned char *)"-2147483648";
    cJSON item = make_empty_item();
    parse_buffer pb = make_parse_buffer(content, 11, 0);

    cJSON_bool result = parse_number(&item, &pb);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_DOUBLE_WITHIN(1.0, -2147483648.0, item.valuedouble);
}

/* item type is set to cJSON_Number on success */
void test_parse_number_sets_type(void)
{
    const unsigned char *content = (const unsigned char *)"5";
    cJSON item = make_empty_item();
    parse_buffer pb = make_parse_buffer(content, 1, 0);

    parse_number(&item, &pb);

    TEST_ASSERT_EQUAL_INT(cJSON_Number, item.type);
}

/* Offset advances correctly for multi-digit number */
void test_parse_number_offset_advances(void)
{
    const unsigned char *content = (const unsigned char *)"1234,rest";
    cJSON item = make_empty_item();
    parse_buffer pb = make_parse_buffer(content, 9, 0);

    cJSON_bool result = parse_number(&item, &pb);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(4, (int)pb.offset);
}

/* Number with only a decimal point (invalid) → false */
void test_parse_number_only_decimal_point(void)
{
    const unsigned char *content = (const unsigned char *)".";
    cJSON item = make_empty_item();
    parse_buffer pb = make_parse_buffer(content, 1, 0);

    cJSON_bool result = parse_number(&item, &pb);

    TEST_ASSERT_FALSE(result);
}

/* Lone minus sign (invalid) → false */
void test_parse_number_lone_minus(void)
{
    const unsigned char *content = (const unsigned char *)"-";
    cJSON item = make_empty_item();
    parse_buffer pb = make_parse_buffer(content, 1, 0);

    cJSON_bool result = parse_number(&item, &pb);

    TEST_ASSERT_FALSE(result);
}

/* Lone plus sign (invalid) → false */
void test_parse_number_lone_plus(void)
{
    const unsigned char *content = (const unsigned char *)"+";
    cJSON item = make_empty_item();
    parse_buffer pb = make_parse_buffer(content, 1, 0);

    cJSON_bool result = parse_number(&item, &pb);

    TEST_ASSERT_FALSE(result);
}

/* Number 1.0 — valueint should be 1 */
void test_parse_number_one_point_zero(void)
{
    const unsigned char *content = (const unsigned char *)"1.0";
    cJSON item = make_empty_item();
    parse_buffer pb = make_parse_buffer(content, 3, 0);

    cJSON_bool result = parse_number(&item, &pb);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_DOUBLE_WITHIN(1e-10, 1.0, item.valuedouble);
    TEST_ASSERT_EQUAL_INT(1, item.valueint);
}

/* Negative float with scientific notation */
void test_parse_number_negative_float_scientific(void)
{
    const unsigned char *content = (const unsigned char *)"-3.5e2";
    cJSON item = make_empty_item();
    parse_buffer pb = make_parse_buffer(content, 6, 0);

    cJSON_bool result = parse_number(&item, &pb);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_DOUBLE_WITHIN(1e-6, -350.0, item.valuedouble);
    TEST_ASSERT_EQUAL_INT(-350, item.valueint);
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
    RUN_TEST(test_parse_number_empty_buffer);
    RUN_TEST(test_parse_number_non_numeric_content);
    RUN_TEST(test_parse_number_overflow_positive);
    RUN_TEST(test_parse_number_overflow_negative);
    RUN_TEST(test_parse_number_int_max);
    RUN_TEST(test_parse_number_int_min);
    RUN_TEST(test_parse_number_sets_type);
    RUN_TEST(test_parse_number_offset_advances);
    RUN_TEST(test_parse_number_only_decimal_point);
    RUN_TEST(test_parse_number_lone_minus);
    RUN_TEST(test_parse_number_lone_plus);
    RUN_TEST(test_parse_number_one_point_zero);
    RUN_TEST(test_parse_number_negative_float_scientific);
    return UNITY_END();
}