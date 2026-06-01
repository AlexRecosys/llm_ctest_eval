#include "cJSON.c"
#include "unity.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <signal.h>
#include <setjmp.h>
#include <locale.h>

/* Global variables for test fixtures */
static cJSON *test_item = NULL;
static parse_buffer test_buffer;
static jmp_buf segv_env;
static volatile sig_atomic_t segv_caught = 0;

/* Signal handler for segmentation faults */
static void segv_handler(int sig) {
    (void)sig;
    segv_caught = 1;
    longjmp(segv_env, 1);
}

/* Helper to initialize parse_buffer with a string */
static void init_parse_buffer(parse_buffer *buf, const char *input) {
    memset(buf, 0, sizeof(parse_buffer));
    buf->content = (const unsigned char *)input;
    buf->length = input ? strlen(input) : 0;
    buf->offset = 0;
    buf->hooks.allocate = malloc;
    buf->hooks.deallocate = free;
    buf->hooks.reallocate = realloc;
}

/* Helper to initialize cJSON item */
static void init_test_item(void) {
    test_item = (cJSON *)malloc(sizeof(cJSON));
    TEST_ASSERT_NOT_NULL(test_item);
    memset(test_item, 0, sizeof(cJSON));
}

/* Setup function */
void setUp(void) {
    segv_caught = 0;
    signal(SIGSEGV, segv_handler);
    init_test_item();
}

/* Teardown function */
void tearDown(void) {
    signal(SIGSEGV, SIG_DFL);
    if (test_item != NULL) {
        free(test_item);
        test_item = NULL;
    }
}

/* Test case 1: Parse a simple integer */
static void test_parse_number_simple_integer(void) {
    init_parse_buffer(&test_buffer, "42");
    TEST_ASSERT_TRUE(parse_number(test_item, &test_buffer));
    TEST_ASSERT_EQUAL_INT(42, test_item->valueint);
    TEST_ASSERT_EQUAL_DOUBLE(42.0, test_item->valuedouble);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item->type);
    TEST_ASSERT_EQUAL_SIZE(2, test_buffer.offset);
}

/* Test case 2: Parse a negative number with decimal point */
static void test_parse_number_negative_decimal(void) {
    init_parse_buffer(&test_buffer, "-3.14159");
    TEST_ASSERT_TRUE(parse_number(test_item, &test_buffer));
    TEST_ASSERT_DOUBLE_WITHIN(0.00001, -3.14159, test_item->valuedouble);
    TEST_ASSERT_EQUAL_INT(-3, test_item->valueint);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item->type);
    TEST_ASSERT_EQUAL_SIZE(8, test_buffer.offset);
}

/* Test case 3: Parse scientific notation */
static void test_parse_number_scientific(void) {
    init_parse_buffer(&test_buffer, "1.23e10");
    TEST_ASSERT_TRUE(parse_number(test_item, &test_buffer));
    TEST_ASSERT_DOUBLE_WITHIN(0.0000001, 1.23e10, test_item->valuedouble);
    TEST_ASSERT_EQUAL_INT(12300000000LL, test_item->valueint);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item->type);
    TEST_ASSERT_EQUAL_SIZE(7, test_buffer.offset);
}

/* Test case 4: Parse overflow (INT_MAX and beyond) */
static void test_parse_number_overflow(void) {
    init_parse_buffer(&test_buffer, "999999999999");
    TEST_ASSERT_TRUE(parse_number(test_item, &test_buffer));
    TEST_ASSERT_EQUAL_INT(INT_MAX, test_item->valueint);
    TEST_ASSERT_GREATER_THAN((double)INT_MAX, test_item->valuedouble);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item->type);
}

/* Test case 5: Parse invalid input (no number) */
static void test_parse_number_invalid(void) {
    init_parse_buffer(&test_buffer, "abc");
    TEST_ASSERT_FALSE(parse_number(test_item, &test_buffer));
    TEST_ASSERT_EQUAL_INT(0, test_buffer.offset);
}

/* Test case 6: Parse with locale-specific decimal point */
static void test_parse_number_locale_decimal(void) {
    char *old_locale = setlocale(LC_NUMERIC, NULL);
    setlocale(LC_NUMERIC, "C");
    init_parse_buffer(&test_buffer, "123.45");
    TEST_ASSERT_TRUE(parse_number(test_item, &test_buffer));
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 123.45, test_item->valuedouble);
    TEST_ASSERT_EQUAL_INT(123, test_item->valueint);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item->type);
    setlocale(LC_NUMERIC, old_locale);
}

/* Test case 7: Parse with leading plus sign */
static void test_parse_number_leading_plus(void) {
    init_parse_buffer(&test_buffer, "+42");
    TEST_ASSERT_TRUE(parse_number(test_item, &test_buffer));
    TEST_ASSERT_EQUAL_INT(42, test_item->valueint);
    TEST_ASSERT_EQUAL_DOUBLE(42.0, test_item->valuedouble);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item->type);
    TEST_ASSERT_EQUAL_SIZE(3, test_buffer.offset);
}

/* Test case 8: Parse zero */
static void test_parse_number_zero(void) {
    init_parse_buffer(&test_buffer, "0");
    TEST_ASSERT_TRUE(parse_number(test_item, &test_buffer));
    TEST_ASSERT_EQUAL_INT(0, test_item->valueint);
    TEST_ASSERT_EQUAL_DOUBLE(0.0, test_item->valuedouble);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item->type);
    TEST_ASSERT_EQUAL_SIZE(1, test_buffer.offset);
}

/* Test case 9: Parse negative zero */
static void test_parse_number_negative_zero(void) {
    init_parse_buffer(&test_buffer, "-0");
    TEST_ASSERT_TRUE(parse_number(test_item, &test_buffer));
    TEST_ASSERT_EQUAL_INT(0, test_item->valueint);
    TEST_ASSERT_EQUAL_DOUBLE(-0.0, test_item->valuedouble);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item->type);
    TEST_ASSERT_EQUAL_SIZE(2, test_buffer.offset);
}

/* Test case 10: Parse with exponent lowercase 'e' */
static void test_parse_number_lowercase_exponent(void) {
    init_parse_buffer(&test_buffer, "1.5e-3");
    TEST_ASSERT_TRUE(parse_number(test_item, &test_buffer));
    TEST_ASSERT_DOUBLE_WITHIN(0.000001, 0.0015, test_item->valuedouble);
    TEST_ASSERT_EQUAL_INT(0, test_item->valueint);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item->type);
    TEST_ASSERT_EQUAL_SIZE(6, test_buffer.offset);
}

/* Test case 11: Parse with exponent uppercase 'E' */
static void test_parse_number_uppercase_exponent(void) {
    init_parse_buffer(&test_buffer, "2.5E+2");
    TEST_ASSERT_TRUE(parse_number(test_item, &test_buffer));
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 250.0, test_item->valuedouble);
    TEST_ASSERT_EQUAL_INT(250, test_item->valueint);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item->type);
    TEST_ASSERT_EQUAL_SIZE(6, test_buffer.offset);
}

/* Test case 12: Parse with trailing non-numeric character */
static void test_parse_number_trailing_char(void) {
    init_parse_buffer(&test_buffer, "42abc");
    TEST_ASSERT_TRUE(parse_number(test_item, &test_buffer));
    TEST_ASSERT_EQUAL_INT(42, test_item->valueint);
    TEST_ASSERT_EQUAL_DOUBLE(42.0, test_item->valuedouble);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item->type);
    TEST_ASSERT_EQUAL_SIZE(2, test_buffer.offset);
}

/* Test case 13: Parse with null buffer */
static void test_parse_number_null_buffer(void) {
    TEST_ASSERT_FALSE(parse_number(test_item, NULL));
}

/* Test case 14: Parse with null content buffer */
static void test_parse_number_null_content(void) {
    parse_buffer buf;
    memset(&buf, 0, sizeof(buf));
    buf.content = NULL;
    buf.length = 0;
    buf.offset = 0;
    buf.hooks.allocate = malloc;
    buf.hooks.deallocate = free;
    buf.hooks.reallocate = realloc;
    TEST_ASSERT_FALSE(parse_number(test_item, &buf));
}

/* Test case 15: Parse very small number (underflow to zero) */
static void test_parse_number_underflow(void) {
    init_parse_buffer(&test_buffer, "1e-308");
    TEST_ASSERT_TRUE(parse_number(test_item, &test_buffer));
    TEST_ASSERT_EQUAL_DOUBLE(1e-308, test_item->valuedouble);
    TEST_ASSERT_EQUAL_INT(0, test_item->valueint);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item->type);
    TEST_ASSERT_EQUAL_SIZE(8, test_buffer.offset);
}

/* Test case 16: Parse INT_MIN boundary */
static void test_parse_number_int_min(void) {
    char int_min_str[32];
    snprintf(int_min_str, sizeof(int_min_str), "%d", INT_MIN);
    init_parse_buffer(&test_buffer, int_min_str);
    TEST_ASSERT_TRUE(parse_number(test_item, &test_buffer));
    TEST_ASSERT_EQUAL_INT(INT_MIN, test_item->valueint);
    TEST_ASSERT_EQUAL_DOUBLE((double)INT_MIN, test_item->valuedouble);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item->type);
}

/* Test case 17: Parse INT_MIN - 1 (should saturate to INT_MIN) */
static void test_parse_number_below_int_min(void) {
    char below_min_str[32];
    snprintf(below_min_str, sizeof(below_min_str), "%d", INT_MIN - 1);
    init_parse_buffer(&test_buffer, below_min_str);
    TEST_ASSERT_TRUE(parse_number(test_item, &test_buffer));
    TEST_ASSERT_EQUAL_INT(INT_MIN, test_item->valueint);
    TEST_ASSERT_LESS_THAN((double)INT_MIN, test_item->valuedouble);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item->type);
}

/* Test case 18: Parse INT_MAX + 1 (should saturate to INT_MAX) */
static void test_parse_number_above_int_max(void) {
    char above_max_str[32];
    snprintf(above_max_str, sizeof(above_max_str), "%d", INT_MAX + 1LL);
    init_parse_buffer(&test_buffer, above_max_str);
    TEST_ASSERT_TRUE(parse_number(test_item, &test_buffer));
    TEST_ASSERT_EQUAL_INT(INT_MAX, test_item->valueint);
    TEST_ASSERT_GREATER_THAN((double)INT_MAX, test_item->valuedouble);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item->type);
}

/* Test case 19: Parse with spaces after number */
static void test_parse_number_trailing_space(void) {
    init_parse_buffer(&test_buffer, "42 ");
    TEST_ASSERT_TRUE(parse_number(test_item, &test_buffer));
    TEST_ASSERT_EQUAL_INT(42, test_item->valueint);
    TEST_ASSERT_EQUAL_DOUBLE(42.0, test_item->valuedouble);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item->type);
    TEST_ASSERT_EQUAL_SIZE(2, test_buffer.offset);
}

/* Test case 20: Parse with leading spaces */
static void test_parse_number_leading_space(void) {
    init_parse_buffer(&test_buffer, "  42");
    TEST_ASSERT_TRUE(parse_number(test_item, &test_buffer));
    TEST_ASSERT_EQUAL_INT(42, test_item->valueint);
    TEST_ASSERT_EQUAL_DOUBLE(42.0, test_item->valuedouble);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item->type);
    TEST_ASSERT_EQUAL_SIZE(4, test_buffer.offset);
}

/* Test case 21: Parse with multiple decimal points (invalid) */
static void test_parse_number_multiple_decimal_points(void) {
    init_parse_buffer(&test_buffer, "1.2.3");
    TEST_ASSERT_TRUE(parse_number(test_item, &test_buffer));
    TEST_ASSERT_EQUAL_DOUBLE(1.2, test_item->valuedouble);
    TEST_ASSERT_EQUAL_INT(1, test_item->valueint);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item->type);
    TEST_ASSERT_EQUAL_SIZE(3, test_buffer.offset);
}

/* Test case 22: Parse with sign in exponent */
static void test_parse_number_sign_in_exponent(void) {
    init_parse_buffer(&test_buffer, "1e+5");
    TEST_ASSERT_TRUE(parse_number(test_item, &test_buffer));
    TEST_ASSERT_EQUAL_DOUBLE(1e5, test_item->valuedouble);
    TEST_ASSERT_EQUAL_INT(100000, test_item->valueint);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item->type);
    TEST_ASSERT_EQUAL_SIZE(4, test_buffer.offset);
}

/* Test case 23: Parse with no decimal point but fractional part */
static void test_parse_number_fractional_no_decimal(void) {
    init_parse_buffer(&test_buffer, "123e2");
    TEST_ASSERT_TRUE(parse_number(test_item, &test_buffer));
    TEST_ASSERT_EQUAL_DOUBLE(12300.0, test_item->valuedouble);
    TEST_ASSERT_EQUAL_INT(12300, test_item->valueint);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item->type);
    TEST_ASSERT_EQUAL_SIZE(5, test_buffer.offset);
}

/* Test case 24: Parse with negative exponent */
static void test_parse_number_negative_exponent(void) {
    init_parse_buffer(&test_buffer, "1e-2");
    TEST_ASSERT_TRUE(parse_number(test_item, &test_buffer));
    TEST_ASSERT_EQUAL_DOUBLE(0.01, test_item->valuedouble);
    TEST_ASSERT_EQUAL_INT(0, test_item->valueint);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item->type);
    TEST_ASSERT_EQUAL_SIZE(4, test_buffer.offset);
}

/* Test case 25: Parse with very large exponent */
static void test_parse_number_large_exponent(void) {
    init_parse_buffer(&test_buffer, "1e308");
    TEST_ASSERT_TRUE(parse_number(test_item, &test_buffer));
    TEST_ASSERT_EQUAL_DOUBLE(1e308, test_item->valuedouble);
    TEST_ASSERT_EQUAL_INT(INT_MAX, test_item->valueint);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item->type);
    TEST_ASSERT_EQUAL_SIZE(7, test_buffer.offset);
}

/* Test case 26: Parse with negative large exponent */
static void test_parse_number_negative_large_exponent(void) {
    init_parse_buffer(&test_buffer, "1e-308");
    TEST_ASSERT_TRUE(parse_number(test_item, &test_buffer));
    TEST_ASSERT_EQUAL_DOUBLE(1e-308, test_item->valuedouble);
    TEST_ASSERT_EQUAL_INT(0, test_item->valueint);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item->type);
    TEST_ASSERT_EQUAL_SIZE(8, test_buffer.offset);
}

/* Test case 27: Parse with sign only (invalid) */
static void test_parse_number_sign_only(void) {
    init_parse_buffer(&test_buffer, "+");
    TEST_ASSERT_FALSE(parse_number(test_item, &test_buffer));
    TEST_ASSERT_EQUAL_INT(0, test_buffer.offset);
}

/* Test case 28: Parse with just a decimal point (invalid) */
static void test_parse_number_decimal_only(void) {
    init_parse_buffer(&test_buffer, ".");
    TEST_ASSERT_FALSE(parse_number(test_item, &test_buffer));
    TEST_ASSERT_EQUAL_INT(0, test_buffer.offset);
}

/* Test case 29: Parse with just a minus sign (invalid) */
static void test_parse_number_minus_only(void) {
    init_parse_buffer(&test_buffer, "-");
    TEST_ASSERT_FALSE(parse_number(test_item, &test_buffer));
    TEST_ASSERT_EQUAL_INT(0, test_buffer.offset);
}

/* Test case 30: Parse with just a plus sign (invalid) */
static void test_parse_number_plus_only(void) {
    init_parse_buffer(&test_buffer, "+");
    TEST_ASSERT_FALSE(parse_number(test_item, &test_buffer));
    TEST_ASSERT_EQUAL_INT(0, test_buffer.offset);
}

/* Test case 31: Parse with exponent but no digits after e */
static void test_parse_number_exponent_no_digits(void) {
    init_parse_buffer(&test_buffer, "1e");
    TEST_ASSERT_TRUE(parse_number(test_item, &test_buffer));
    TEST_ASSERT_EQUAL_DOUBLE(1.0, test_item->valuedouble);
    TEST_ASSERT_EQUAL_INT(1, test_item->valueint);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item->type);
    TEST_ASSERT_EQUAL_SIZE(1, test_buffer.offset);
}

/* Test case 32: Parse with exponent and sign but no digits after e */
static void test_parse_number_exponent_sign_no_digits(void) {
    init_parse_buffer(&test_buffer, "1e+");
    TEST_ASSERT_TRUE(parse_number(test_item, &test_buffer));
    TEST_ASSERT_EQUAL_DOUBLE(1.0, test_item->valuedouble);
    TEST_ASSERT_EQUAL_INT(1, test_item->valueint);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item->type);
    TEST_ASSERT_EQUAL_SIZE(1, test_buffer.offset);
}

/* Test case 33: Parse with exponent and negative sign but no digits after e */
static void test_parse_number_exponent_negative_sign_no_digits(void) {
    init_parse_buffer(&test_buffer, "1e-");
    TEST_ASSERT_TRUE(parse_number(test_item, &test_buffer));
    TEST_ASSERT_EQUAL_DOUBLE(1.0, test_item->valuedouble);
    TEST_ASSERT_EQUAL_INT(1, test_item->valueint);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item->type);
    TEST_ASSERT_EQUAL_SIZE(1, test_buffer.offset);
}

/* Test case 34: Parse with leading zeros */
static void test_parse_number_leading_zeros(void) {
    init_parse_buffer(&test_buffer, "00042");
    TEST_ASSERT_TRUE(parse_number(test_item, &test_buffer));
    TEST_ASSERT_EQUAL_INT(42, test_item->valueint);
    TEST_ASSERT_EQUAL_DOUBLE(42.0, test_item->valuedouble);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item->type);
    TEST_ASSERT_EQUAL_SIZE(5, test_buffer.offset);
}

/* Test case 35: Parse with leading zeros and decimal */
static void test_parse_number_leading_zeros_decimal(void) {
    init_parse_buffer(&test_buffer, "000.42");
    TEST_ASSERT_TRUE(parse_number(test_item, &test_buffer));
    TEST_ASSERT_EQUAL_DOUBLE(0.42, test_item->valuedouble);
    TEST_ASSERT_EQUAL_INT(0, test_item->valueint);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item->type);
    TEST_ASSERT_EQUAL_SIZE(6, test_buffer.offset);
}

/* Test case 36: Parse with trailing zeros after decimal */
static void test_parse_number_trailing_zeros_decimal(void) {
    init_parse_buffer(&test_buffer, "42.000");
    TEST_ASSERT_TRUE(parse_number(test_item, &test_buffer));
    TEST_ASSERT_EQUAL_DOUBLE(42.0, test_item->valuedouble);
    TEST_ASSERT_EQUAL_INT(42, test_item->valueint);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item->type);
    TEST_ASSERT_EQUAL_SIZE(6, test_buffer.offset);
}

/* Test case 37: Parse with exponent and leading zeros */
static void test_parse_number_exponent_leading_zeros(void) {
    init_parse_buffer(&test_buffer, "001.5e2");
    TEST_ASSERT_TRUE(parse_number(test_item, &test_buffer));
    TEST_ASSERT_EQUAL_DOUBLE(150.0, test_item->valuedouble);
    TEST_ASSERT_EQUAL_INT(150, test_item->valueint);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item->type);
    TEST_ASSERT_EQUAL_SIZE(7, test_buffer.offset);
}

/* Test case 38: Parse with exponent and trailing zeros */
static void test_parse_number_exponent_trailing_zeros(void) {
    init_parse_buffer(&test_buffer, "1.500e2");
    TEST_ASSERT_TRUE(parse_number(test_item, &test_buffer));
    TEST_ASSERT_EQUAL_DOUBLE(150.0, test_item->valuedouble);
    TEST_ASSERT_EQUAL_INT(150, test_item->valueint);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item->type);
    TEST_ASSERT_EQUAL_SIZE(7, test_buffer.offset);
}

/* Test case 39: Parse with exponent and leading zeros in mantissa */
static void test_parse_number_exponent_leading_zeros_mantissa(void) {
    init_parse_buffer(&test_buffer, "0015e1");
    TEST_ASSERT_TRUE(parse_number(test_item, &test_buffer));
    TEST_ASSERT_EQUAL_DOUBLE(150.0, test_item->valuedouble);
    TEST_ASSERT_EQUAL_INT(150, test_item->valueint);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item->type);
    TEST_ASSERT_EQUAL_SIZE(6, test_buffer.offset);
}

/* Test case 40: Parse with exponent and trailing zeros in mantissa */
static void test_parse_number_exponent_trailing_zeros_mantissa(void) {
    init_parse_buffer(&test_buffer, "15.00e1");
    TEST_ASSERT_TRUE(parse_number(test_item, &test_buffer));
    TEST_ASSERT_EQUAL_DOUBLE(150.0, test_item->valuedouble);
    TEST_ASSERT_EQUAL_INT(150, test_item->valueint);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item->type);
    TEST_ASSERT_EQUAL_SIZE(7, test_buffer.offset);
}

/* Test case 41: Parse with exponent and leading zeros in exponent */
static void test_parse_number_exponent_leading_zeros_exp(void) {
    init_parse_buffer(&test_buffer, "1.5e002");
    TEST_ASSERT_TRUE(parse_number(test_item, &test_buffer));
    TEST_ASSERT_EQUAL_DOUBLE(150.0, test_item->valuedouble);
    TEST_ASSERT_EQUAL_INT(150, test_item->valueint);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item->type);
    TEST_ASSERT_EQUAL_SIZE(7, test_buffer.offset);
}

/* Test case 42: Parse with exponent and trailing zeros in exponent */
static void test_parse_number_exponent_trailing_zeros_exp(void) {
    init_parse_buffer(&test_buffer, "1.5e200");
    TEST_ASSERT_TRUE(parse_number(test_item, &test_buffer));
    TEST_ASSERT_EQUAL_DOUBLE(1.5e200, test_item->valuedouble);
    TEST_ASSERT_EQUAL_INT(INT_MAX, test_item->valueint);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item->type);
    TEST_ASSERT_EQUAL_SIZE(7, test_buffer.offset);
}

/* Test case 43: Parse with exponent and leading zeros in negative exponent */
static void test_parse_number_exponent_leading_zeros_neg_exp(void) {
    init_parse_buffer(&test_buffer, "1.5e-002");
    TEST_ASSERT_TRUE(parse_number(test_item, &test_buffer));
    TEST_ASSERT_EQUAL_DOUBLE(0.015, test_item->valuedouble);
    TEST_ASSERT_EQUAL_INT(0, test_item->valueint);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item->type);
    TEST_ASSERT_EQUAL_SIZE(8, test_buffer.offset);
}

/* Test case 44: Parse with exponent and trailing zeros in negative exponent */
static void test_parse_number_exponent_trailing_zeros_neg_exp(void) {
    init_parse_buffer(&test_buffer, "1.5e-020");
    TEST_ASSERT_TRUE(parse_number(test_item, &test_buffer));
    TEST_ASSERT_EQUAL_DOUBLE(1.5e-20, test_item->valuedouble);
    TEST_ASSERT_EQUAL_INT(0, test_item->valueint);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item->type);
    TEST_ASSERT_EQUAL_SIZE(9, test_buffer.offset);
}

/* Test case 45: Parse with exponent and sign in negative exponent */
static void test_parse_number_exponent_sign_neg_exp(void) {
    init_parse_buffer(&test_buffer, "1.5e-2");
    TEST_ASSERT_TRUE(parse_number(test_item, &test_buffer));
    TEST_ASSERT_EQUAL_DOUBLE(0.015, test_item->valuedouble);
    TEST_ASSERT_EQUAL_INT(0, test_item->valueint);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item->type);
    TEST_ASSERT_EQUAL_SIZE(6, test_buffer.offset);
}

/* Test case 46: Parse with exponent and sign in positive exponent */
static void test_parse_number_exponent_sign_pos_exp(void) {
    init_parse_buffer(&test_buffer, "1.5e+2");
    TEST_ASSERT_TRUE(parse_number(test_item, &test_buffer));
    TEST_ASSERT_EQUAL_DOUBLE(150.0, test_item->valuedouble);
    TEST_ASSERT_EQUAL_INT(150, test_item->valueint);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item->type);
    TEST_ASSERT_EQUAL_SIZE(6, test_buffer.offset);
}

/* Test case 47: Parse with exponent and no sign in positive exponent */
static void test_parse_number_exponent_no_sign_pos_exp(void) {
    init_parse_buffer(&test_buffer, "1.5e2");
    TEST_ASSERT_TRUE(parse_number(test_item, &test_buffer));
    TEST_ASSERT_EQUAL_DOUBLE(150.0, test_item->valuedouble);
    TEST_ASSERT_EQUAL_INT(150, test_item->valueint);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item->type);
    TEST_ASSERT_EQUAL_SIZE(5, test_buffer.offset);
}

/* Test case 48: Parse with exponent and sign in negative exponent */
static void test_parse_number_exponent_sign_neg_exp2(void) {
    init_parse_buffer(&test_buffer, "1.5e-2");
    TEST_ASSERT_TRUE(parse_number(test_item, &test_buffer));
    TEST_ASSERT_EQUAL_DOUBLE(0.015, test_item->valuedouble);
    TEST_ASSERT_EQUAL_INT(0, test_item->valueint);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item->type);
    TEST_ASSERT_EQUAL_SIZE(6, test_buffer.offset);
}

/* Test case 49: Parse with exponent and sign in positive exponent */
static void test_parse_number_exponent_sign_pos_exp2(void) {
    init_parse_buffer(&test_buffer, "1.5e+2");
    TEST_ASSERT_TRUE(parse_number(test_item, &test_buffer));
    TEST_ASSERT_EQUAL_DOUBLE(150.0, test_item->valuedouble);
    TEST_ASSERT_EQUAL_INT(150, test_item->valueint);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item->type);
    TEST_ASSERT_EQUAL_SIZE(6, test_buffer.offset);
}

/* Test case 50: Parse with exponent and no sign in positive exponent */
static void test_parse_number_exponent_no_sign_pos_exp2(void) {
    init_parse_buffer(&test_buffer, "1.5e2");
    TEST_ASSERT_TRUE(parse_number(test_item, &test_buffer));
    TEST_ASSERT_EQUAL_DOUBLE(150.0, test_item->valuedouble);
    TEST_ASSERT_EQUAL_INT(150, test_item->valueint);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item->type);
    TEST_ASSERT_EQUAL_SIZE(5, test_buffer.offset);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_parse_number_simple_integer);
    RUN_TEST(test_parse_number_negative_decimal);
    RUN_TEST(test_parse_number_scientific);
    RUN_TEST(test_parse_number_overflow);
    RUN_TEST(test_parse_number_invalid);
    RUN_TEST(test_parse_number_locale_decimal);
    RUN_TEST(test_parse_number_leading_plus);
    RUN_TEST(test_parse_number_zero);
    RUN_TEST(test_parse_number_negative_zero);
    RUN_TEST(test_parse_number_lowercase_exponent);
    RUN_TEST(test_parse_number_uppercase_exponent);
    RUN_TEST(test_parse_number_trailing_char);
    RUN_TEST(test_parse_number_null_buffer);
    RUN_TEST(test_parse_number_null_content);
    RUN_TEST(test_parse_number_underflow);
    RUN_TEST(test_parse_number_int_min);
    RUN_TEST(test_parse_number_below_int_min);
    RUN_TEST(test_parse_number_above_int_max);
    RUN_TEST(test_parse_number_trailing_space);
    RUN_TEST(test_parse_number_leading_space);
    RUN_TEST(test_parse_number_multiple_decimal_points);
    RUN_TEST(test_parse_number_sign_in_exponent);
    RUN_TEST(test_parse_number_fractional_no_decimal);
    RUN_TEST(test_parse_number_negative_exponent);
    RUN_TEST(test_parse_number_large_exponent);
    RUN_TEST(test_parse_number_negative_large_exponent);
    RUN_TEST(test_parse_number_sign_only);
    RUN_TEST(test_parse_number_decimal_only);
    RUN_TEST(test_parse_number_minus_only);
    RUN_TEST(test_parse_number_plus_only);
    RUN_TEST(test_parse_number_exponent_no_digits);
    RUN_TEST(test_parse_number_exponent_sign_no_digits);
    RUN_TEST(test_parse_number_exponent_negative_sign_no_digits);
    RUN_TEST(test_parse_number_leading_zeros);
    RUN_TEST(test_parse_number_leading_zeros_decimal);
    RUN_TEST(test_parse_number_trailing_zeros_decimal);
    RUN_TEST(test_parse_number_exponent_leading_zeros);
    RUN_TEST(test_parse_number_exponent_trailing_zeros);
    RUN_TEST(test_parse_number_exponent_leading_zeros_mantissa);
    RUN_TEST(test_parse_number_exponent_trailing_zeros_mantissa);
    RUN_TEST(test_parse_number_exponent_leading_zeros_exp);
    RUN_TEST(test_parse_number_exponent_trailing_zeros_exp);
    RUN_TEST(test_parse_number_exponent_leading_zeros_neg_exp);
    RUN_TEST(test_parse_number_exponent_trailing_zeros_neg_exp);
    RUN_TEST(test_parse_number_exponent_sign_neg_exp);
    RUN_TEST(test_parse_number_exponent_sign_pos_exp);
    RUN_TEST(test_parse_number_exponent_no_sign_pos_exp);
    RUN_TEST(test_parse_number_exponent_sign_neg_exp2);
    RUN_TEST(test_parse_number_exponent_sign_pos_exp2);
    RUN_TEST(test_parse_number_exponent_no_sign_pos_exp2);
    return UNITY_END();
}