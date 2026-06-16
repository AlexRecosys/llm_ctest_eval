#include "cJSON.c"
#include "unity.h"

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <float.h>
#include <locale.h>

/* File-scope static variables / fixtures */
static cJSON_Hooks default_hooks = { malloc, free };
static parse_buffer *test_buffer = NULL;
static cJSON test_item;

/* Helper functions and macros */
static void setup_parse_buffer(const char *input) {
    size_t len = strlen(input);
    unsigned char *content = (unsigned char *)default_hooks.malloc(len + 1);
    TEST_ASSERT_NOT_NULL(content);
    memcpy(content, input, len);
    content[len] = '\0';

    test_buffer = (parse_buffer *)default_hooks.malloc(sizeof(parse_buffer));
    TEST_ASSERT_NOT_NULL(test_buffer);

    test_buffer->content = content;
    test_buffer->offset = 0;
    test_buffer->length = len;
    test_buffer->hooks = default_hooks;
}

static void teardown_parse_buffer(void) {
    if (test_buffer != NULL) {
        if (test_buffer->content != NULL) {
            default_hooks.free(test_buffer->content);
        }
        default_hooks.free(test_buffer);
        test_buffer = NULL;
    }
}

static void init_test_item(void) {
    memset(&test_item, 0, sizeof(test_item));
    test_item.type = cJSON_Invalid;
    test_item.valueint = 0;
    test_item.valuedouble = 0.0;
}

/* Test cases */

void test_parse_number_integer_positive(void) {
    setup_parse_buffer("123");
    init_test_item();

    TEST_ASSERT_TRUE(parse_number(&test_item, test_buffer));
    TEST_ASSERT_EQUAL_INT(123, test_item.valueint);
    TEST_ASSERT_EQUAL_DOUBLE(123.0, test_item.valuedouble);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item.type);
    TEST_ASSERT_EQUAL_SIZE(3, test_buffer->offset);

    teardown_parse_buffer();
}

void test_parse_number_integer_negative(void) {
    setup_parse_buffer("-456");
    init_test_item();

    TEST_ASSERT_TRUE(parse_number(&test_item, test_buffer));
    TEST_ASSERT_EQUAL_INT(-456, test_item.valueint);
    TEST_ASSERT_EQUAL_DOUBLE(-456.0, test_item.valuedouble);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item.type);
    TEST_ASSERT_EQUAL_SIZE(4, test_buffer->offset);

    teardown_parse_buffer();
}

void test_parse_number_with_exponent(void) {
    setup_parse_buffer("1.23e4");
    init_test_item();

    TEST_ASSERT_TRUE(parse_number(&test_item, test_buffer));
    TEST_ASSERT_EQUAL_DOUBLE(12300.0, test_item.valuedouble);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item.type);
    TEST_ASSERT_EQUAL_SIZE(6, test_buffer->offset);

    teardown_parse_buffer();
}

void test_parse_number_with_uppercase_exponent(void) {
    setup_parse_buffer("5.67E-2");
    init_test_item();

    TEST_ASSERT_TRUE(parse_number(&test_item, test_buffer));
    TEST_ASSERT_EQUAL_DOUBLE(0.0567, test_item.valuedouble);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item.type);
    TEST_ASSERT_EQUAL_SIZE(7, test_buffer->offset);

    teardown_parse_buffer();
}

void test_parse_number_with_leading_plus(void) {
    setup_parse_buffer("+789");
    init_test_item();

    TEST_ASSERT_TRUE(parse_number(&test_item, test_buffer));
    TEST_ASSERT_EQUAL_INT(789, test_item.valueint);
    TEST_ASSERT_EQUAL_DOUBLE(789.0, test_item.valuedouble);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item.type);
    TEST_ASSERT_EQUAL_SIZE(4, test_buffer->offset);

    teardown_parse_buffer();
}

void test_parse_number_with_decimal_point(void) {
    setup_parse_buffer("3.14159");
    init_test_item();

    TEST_ASSERT_TRUE(parse_number(&test_item, test_buffer));
    TEST_ASSERT_EQUAL_DOUBLE(3.14159, test_item.valuedouble);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item.type);
    TEST_ASSERT_EQUAL_SIZE(7, test_buffer->offset);

    teardown_parse_buffer();
}

void test_parse_number_zero(void) {
    setup_parse_buffer("0");
    init_test_item();

    TEST_ASSERT_TRUE(parse_number(&test_item, test_buffer));
    TEST_ASSERT_EQUAL_INT(0, test_item.valueint);
    TEST_ASSERT_EQUAL_DOUBLE(0.0, test_item.valuedouble);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item.type);
    TEST_ASSERT_EQUAL_SIZE(1, test_buffer->offset);

    teardown_parse_buffer();
}

void test_parse_number_negative_zero(void) {
    setup_parse_buffer("-0");
    init_test_item();

    TEST_ASSERT_TRUE(parse_number(&test_item, test_buffer));
    TEST_ASSERT_EQUAL_INT(0, test_item.valueint);
    TEST_ASSERT_EQUAL_DOUBLE(-0.0, test_item.valuedouble);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item.type);
    TEST_ASSERT_EQUAL_SIZE(2, test_buffer->offset);

    teardown_parse_buffer();
}

void test_parse_number_saturates_to_INT_MAX(void) {
    setup_parse_buffer("999999999999");
    init_test_item();

    TEST_ASSERT_TRUE(parse_number(&test_item, test_buffer));
    TEST_ASSERT_EQUAL_INT(INT_MAX, test_item.valueint);
    TEST_ASSERT_EQUAL_DOUBLE(999999999999.0, test_item.valuedouble);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item.type);

    teardown_parse_buffer();
}

void test_parse_number_saturates_to_INT_MIN(void) {
    setup_parse_buffer("-999999999999");
    init_test_item();

    TEST_ASSERT_TRUE(parse_number(&test_item, test_buffer));
    TEST_ASSERT_EQUAL_INT(INT_MIN, test_item.valueint);
    TEST_ASSERT_EQUAL_DOUBLE(-999999999999.0, test_item.valuedouble);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item.type);

    teardown_parse_buffer();
}

void test_parse_number_fails_on_invalid_input(void) {
    setup_parse_buffer("abc");
    init_test_item();

    TEST_ASSERT_FALSE(parse_number(&test_item, test_buffer));
    TEST_ASSERT_EQUAL_INT(cJSON_Invalid, test_item.type);
    TEST_ASSERT_EQUAL_SIZE(0, test_buffer->offset);

    teardown_parse_buffer();
}

void test_parse_number_fails_on_empty_input(void) {
    setup_parse_buffer("");
    init_test_item();

    TEST_ASSERT_FALSE(parse_number(&test_item, test_buffer));
    TEST_ASSERT_EQUAL_INT(cJSON_Invalid, test_item.type);

    teardown_parse_buffer();
}

void test_parse_number_fails_on_null_buffer(void) {
    init_test_item();

    TEST_ASSERT_FALSE(parse_number(&test_item, NULL));
    TEST_ASSERT_EQUAL_INT(cJSON_Invalid, test_item.type);
}

void test_parse_number_fails_on_null_content(void) {
    test_buffer = (parse_buffer *)default_hooks.malloc(sizeof(parse_buffer));
    TEST_ASSERT_NOT_NULL(test_buffer);
    test_buffer->content = NULL;
    test_buffer->offset = 0;
    test_buffer->length = 0;
    test_buffer->hooks = default_hooks;

    init_test_item();

    TEST_ASSERT_FALSE(parse_number(&test_item, test_buffer));
    TEST_ASSERT_EQUAL_INT(cJSON_Invalid, test_item.type);

    default_hooks.free(test_buffer);
}

void test_parse_number_with_trailing_non_number(void) {
    setup_parse_buffer("42xyz");
    init_test_item();

    TEST_ASSERT_TRUE(parse_number(&test_item, test_buffer));
    TEST_ASSERT_EQUAL_INT(42, test_item.valueint);
    TEST_ASSERT_EQUAL_DOUBLE(42.0, test_item.valuedouble);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item.type);
    TEST_ASSERT_EQUAL_SIZE(2, test_buffer->offset);  // only consumed "42"

    teardown_parse_buffer();
}

void test_parse_number_with_leading_whitespace(void) {
    setup_parse_buffer("  123");
    init_test_item();

    /* Note: parse_number does NOT skip leading whitespace; it starts parsing immediately */
    TEST_ASSERT_FALSE(parse_number(&test_item, test_buffer));
    TEST_ASSERT_EQUAL_INT(cJSON_Invalid, test_item.type);

    teardown_parse_buffer();
}

void test_parse_number_with_trailing_whitespace(void) {
    setup_parse_buffer("123  ");
    init_test_item();

    TEST_ASSERT_TRUE(parse_number(&test_item, test_buffer));
    TEST_ASSERT_EQUAL_INT(123, test_item.valueint);
    TEST_ASSERT_EQUAL_DOUBLE(123.0, test_item.valuedouble);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item.type);
    TEST_ASSERT_EQUAL_SIZE(3, test_buffer->offset);  // stops at space

    teardown_parse_buffer();
}

void test_parse_number_scientific_notation_edge_case(void) {
    setup_parse_buffer("1e10");
    init_test_item();

    TEST_ASSERT_TRUE(parse_number(&test_item, test_buffer));
    TEST_ASSERT_EQUAL_DOUBLE(1e10, test_item.valuedouble);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item.type);
    TEST_ASSERT_EQUAL_SIZE(4, test_buffer->offset);

    teardown_parse_buffer();
}

void test_parse_number_negative_exponent(void) {
    setup_parse_buffer("1e-10");
    init_test_item();

    TEST_ASSERT_TRUE(parse_number(&test_item, test_buffer));
    TEST_ASSERT_EQUAL_DOUBLE(1e-10, test_item.valuedouble);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item.type);
    TEST_ASSERT_EQUAL_SIZE(5, test_buffer->offset);

    teardown_parse_buffer();
}

void test_parse_number_large_negative_exponent(void) {
    setup_parse_buffer("1e-308");
    init_test_item();

    TEST_ASSERT_TRUE(parse_number(&test_item, test_buffer));
    TEST_ASSERT_EQUAL_DOUBLE(1e-308, test_item.valuedouble);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item.type);
    TEST_ASSERT_EQUAL_SIZE(8, test_buffer->offset);

    teardown_parse_buffer();
}

void test_parse_number_with_explicit_sign_and_decimal(void) {
    setup_parse_buffer("+12.34");
    init_test_item();

    TEST_ASSERT_TRUE(parse_number(&test_item, test_buffer));
    TEST_ASSERT_EQUAL_DOUBLE(12.34, test_item.valuedouble);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item.type);
    TEST_ASSERT_EQUAL_SIZE(6, test_buffer->offset);

    teardown_parse_buffer();
}

void test_parse_number_with_multiple_decimal_points_fails(void) {
    setup_parse_buffer("12.34.56");
    init_test_item();

    /* parse_number stops at the second '.' because it's not in the allowed set */
    TEST_ASSERT_TRUE(parse_number(&test_item, test_buffer));
    TEST_ASSERT_EQUAL_DOUBLE(12.34, test_item.valuedouble);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item.type);
    TEST_ASSERT_EQUAL_SIZE(5, test_buffer->offset);  // stops at second '.'

    teardown_parse_buffer();
}

void test_parse_number_with_plus_sign_in_exponent(void) {
    setup_parse_buffer("1e+5");
    init_test_item();

    TEST_ASSERT_TRUE(parse_number(&test_item, test_buffer));
    TEST_ASSERT_EQUAL_DOUBLE(1e5, test_item.valuedouble);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item.type);
    TEST_ASSERT_EQUAL_SIZE(4, test_buffer->offset);

    teardown_parse_buffer();
}

void test_parse_number_with_negative_sign_in_exponent(void) {
    setup_parse_buffer("1e-5");
    init_test_item();

    TEST_ASSERT_TRUE(parse_number(&test_item, test_buffer));
    TEST_ASSERT_EQUAL_DOUBLE(1e-5, test_item.valuedouble);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item.type);
    TEST_ASSERT_EQUAL_SIZE(4, test_buffer->offset);

    teardown_parse_buffer();
}

void test_parse_number_with_zero_exponent(void) {
    setup_parse_buffer("123e0");
    init_test_item();

    TEST_ASSERT_TRUE(parse_number(&test_item, test_buffer));
    TEST_ASSERT_EQUAL_DOUBLE(123.0, test_item.valuedouble);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item.type);
    TEST_ASSERT_EQUAL_SIZE(5, test_buffer->offset);

    teardown_parse_buffer();
}

void test_parse_number_with_negative_number_and_decimal(void) {
    setup_parse_buffer("-123.456");
    init_test_item();

    TEST_ASSERT_TRUE(parse_number(&test_item, test_buffer));
    TEST_ASSERT_EQUAL_DOUBLE(-123.456, test_item.valuedouble);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item.type);
    TEST_ASSERT_EQUAL_SIZE(8, test_buffer->offset);

    teardown_parse_buffer();
}

void test_parse_number_with_huge_number(void) {
    setup_parse_buffer("1.7976931348623157e308");
    init_test_item();

    TEST_ASSERT_TRUE(parse_number(&test_item, test_buffer));
    TEST_ASSERT_EQUAL_DOUBLE(DBL_MAX, test_item.valuedouble);
    TEST_ASSERT_EQUAL_INT(INT_MAX, test_item.valueint);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item.type);

    teardown_parse_buffer();
}

void test_parse_number_with_tiny_number(void) {
    setup_parse_buffer("2.2250738585072014e-308");
    init_test_item();

    TEST_ASSERT_TRUE(parse_number(&test_item, test_buffer));
    TEST_ASSERT_EQUAL_DOUBLE(DBL_MIN, test_item.valuedouble);
    TEST_ASSERT_EQUAL_INT(0, test_item.valueint);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item.type);

    teardown_parse_buffer();
}

void test_parse_number_with_negative_overflow(void) {
    setup_parse_buffer("-1.7976931348623157e308");
    init_test_item();

    TEST_ASSERT_TRUE(parse_number(&test_item, test_buffer));
    TEST_ASSERT_EQUAL_DOUBLE(-DBL_MAX, test_item.valuedouble);
    TEST_ASSERT_EQUAL_INT(INT_MIN, test_item.valueint);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item.type);

    teardown_parse_buffer();
}

void test_parse_number_with_infinity(void) {
    setup_parse_buffer("1e309");
    init_test_item();

    TEST_ASSERT_TRUE(parse_number(&test_item, test_buffer));
    TEST_ASSERT_EQUAL_DOUBLE(INFINITY, test_item.valuedouble);
    TEST_ASSERT_EQUAL_INT(INT_MAX, test_item.valueint);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item.type);

    teardown_parse_buffer();
}

void test_parse_number_with_negative_infinity(void) {
    setup_parse_buffer("-1e309");
    init_test_item();

    TEST_ASSERT_TRUE(parse_number(&test_item, test_buffer));
    TEST_ASSERT_EQUAL_DOUBLE(-INFINITY, test_item.valuedouble);
    TEST_ASSERT_EQUAL_INT(INT_MIN, test_item.valueint);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item.type);

    teardown_parse_buffer();
}

void test_parse_number_with_nan(void) {
    setup_parse_buffer("NaN");
    init_test_item();

    /* strtod may return NaN, but parse_number should fail because strtod returns after_end == number_c_string */
    TEST_ASSERT_FALSE(parse_number(&test_item, test_buffer));
    TEST_ASSERT_EQUAL_INT(cJSON_Invalid, test_item.type);

    teardown_parse_buffer();
}

void test_parse_number_with_inf(void) {
    setup_parse_buffer("Inf");
    init_test_item();

    /* strtod may parse "Inf" as infinity, but parse_number should fail because strtod returns after_end == number_c_string */
    TEST_ASSERT_FALSE(parse_number(&test_item, test_buffer));
    TEST_ASSERT_EQUAL_INT(cJSON_Invalid, test_item.type);

    teardown_parse_buffer();
}

void test_parse_number_with_null_terminated_input(void) {
    setup_parse_buffer("42");
    init_test_item();

    TEST_ASSERT_TRUE(parse_number(&test_item, test_buffer));
    TEST_ASSERT_EQUAL_INT(42, test_item.valueint);
    TEST_ASSERT_EQUAL_DOUBLE(42.0, test_item.valuedouble);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item.type);
    TEST_ASSERT_EQUAL_SIZE(2, test_buffer->offset);

    teardown_parse_buffer();
}

void test_parse_number_with_locale_decimal_point(void) {
    char *original_locale = setlocale(LC_NUMERIC, NULL);
    if (original_locale == NULL) {
        original_locale = strdup("C");
    }
    char *saved_locale = strdup(original_locale);

    /* Set to a locale that uses comma as decimal separator */
    if (setlocale(LC_NUMERIC, "de_DE") == NULL && setlocale(LC_NUMERIC, "de_DE.UTF-8") == NULL) {
        /* If German locale not available, skip this test */
        free(saved_locale);
        return;
    }

    setup_parse_buffer("3.14");
    init_test_item();

    /* parse_number replaces '.' with locale decimal point before calling strtod */
    TEST_ASSERT_TRUE(parse_number(&test_item, test_buffer));
    TEST_ASSERT_EQUAL_DOUBLE(3.14, test_item.valuedouble);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item.type);

    teardown_parse_buffer();

    /* Restore original locale */
    setlocale(LC_NUMERIC, saved_locale);
    free(saved_locale);
}

/* Main function */
int main(void) {
    unity_install_sighandler();
    UNITY_BEGIN();

    RUN_TEST(test_parse_number_integer_positive);
    RUN_TEST(test_parse_number_integer_negative);
    RUN_TEST(test_parse_number_with_exponent);
    RUN_TEST(test_parse_number_with_uppercase_exponent);
    RUN_TEST(test_parse_number_with_leading_plus);
    RUN_TEST(test_parse_number_with_decimal_point);
    RUN_TEST(test_parse_number_zero);
    RUN_TEST(test_parse_number_negative_zero);
    RUN_TEST(test_parse_number_saturates_to_INT_MAX);
    RUN_TEST(test_parse_number_saturates_to_INT_MIN);
    RUN_TEST(test_parse_number_fails_on_invalid_input);
    RUN_TEST(test_parse_number_fails_on_empty_input);
    RUN_TEST(test_parse_number_fails_on_null_buffer);
    RUN_TEST(test_parse_number_fails_on_null_content);
    RUN_TEST(test_parse_number_with_trailing_non_number);
    RUN_TEST(test_parse_number_with_leading_whitespace);
    RUN_TEST(test_parse_number_with_trailing_whitespace);
    RUN_TEST(test_parse_number_scientific_notation_edge_case);
    RUN_TEST(test_parse_number_negative_exponent);
    RUN_TEST(test_parse_number_large_negative_exponent);
    RUN_TEST(test_parse_number_with_explicit_sign_and_decimal);
    RUN_TEST(test_parse_number_with_multiple_decimal_points_fails);
    RUN_TEST(test_parse_number_with_plus_sign_in_exponent);
    RUN_TEST(test_parse_number_with_negative_sign_in_exponent);
    RUN_TEST(test_parse_number_with_zero_exponent);
    RUN_TEST(test_parse_number_with_negative_number_and_decimal);
    RUN_TEST(test_parse_number_with_huge_number);
    RUN_TEST(test_parse_number_with_tiny_number);
    RUN_TEST(test_parse_number_with_negative_overflow);
    RUN_TEST(test_parse_number_with_infinity);
    RUN_TEST(test_parse_number_with_negative_infinity);
    RUN_TEST(test_parse_number_with_nan);
    RUN_TEST(test_parse_number_with_inf);
    RUN_TEST(test_parse_number_with_null_terminated_input);
    RUN_TEST(test_parse_number_with_locale_decimal_point);

    return UNITY_END();
}