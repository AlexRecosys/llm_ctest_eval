#include "cJSON.c"
#include "unity.h"
#include <stddef.h>
#include <string.h>

/* setUp and tearDown are required by Unity */
void setUp(void)
{
}

void tearDown(void)
{
}

/* Helper macro to call parse_hex4 with a string literal safely */
static unsigned call_parse_hex4(const char *str)
{
    return parse_hex4((const unsigned char *)str);
}

/* Test: all zeros */
void test_parse_hex4_all_zeros(void)
{
    TEST_ASSERT_EQUAL_HEX(0x0000, call_parse_hex4("0000"));
}

/* Test: all uppercase F */
void test_parse_hex4_all_uppercase_F(void)
{
    TEST_ASSERT_EQUAL_HEX(0xFFFF, call_parse_hex4("FFFF"));
}

/* Test: all lowercase f */
void test_parse_hex4_all_lowercase_f(void)
{
    TEST_ASSERT_EQUAL_HEX(0xFFFF, call_parse_hex4("ffff"));
}

/* Test: mixed case */
void test_parse_hex4_mixed_case(void)
{
    TEST_ASSERT_EQUAL_HEX(0xaBcD, call_parse_hex4("aBcD"));
}

/* Test: digits only */
void test_parse_hex4_digits_only(void)
{
    TEST_ASSERT_EQUAL_HEX(0x1234, call_parse_hex4("1234"));
}

/* Test: uppercase hex letters A-F */
void test_parse_hex4_uppercase_ABCDEF(void)
{
    TEST_ASSERT_EQUAL_HEX(0xABCD, call_parse_hex4("ABCD"));
}

/* Test: lowercase hex letters a-f */
void test_parse_hex4_lowercase_abcdef(void)
{
    TEST_ASSERT_EQUAL_HEX(0xabcd, call_parse_hex4("abcd"));
}

/* Test: value 0x0001 */
void test_parse_hex4_value_0001(void)
{
    TEST_ASSERT_EQUAL_HEX(0x0001, call_parse_hex4("0001"));
}

/* Test: value 0x000F uppercase */
void test_parse_hex4_value_000F_upper(void)
{
    TEST_ASSERT_EQUAL_HEX(0x000F, call_parse_hex4("000F"));
}

/* Test: value 0x000f lowercase */
void test_parse_hex4_value_000f_lower(void)
{
    TEST_ASSERT_EQUAL_HEX(0x000F, call_parse_hex4("000f"));
}

/* Test: value 0xF000 */
void test_parse_hex4_value_F000(void)
{
    TEST_ASSERT_EQUAL_HEX(0xF000, call_parse_hex4("F000"));
}

/* Test: value 0x0F00 */
void test_parse_hex4_value_0F00(void)
{
    TEST_ASSERT_EQUAL_HEX(0x0F00, call_parse_hex4("0F00"));
}

/* Test: value 0x00F0 */
void test_parse_hex4_value_00F0(void)
{
    TEST_ASSERT_EQUAL_HEX(0x00F0, call_parse_hex4("00F0"));
}

/* Test: value 0x00A0 */
void test_parse_hex4_value_00A0(void)
{
    TEST_ASSERT_EQUAL_HEX(0x00A0, call_parse_hex4("00A0"));
}

/* Test: value 0x1A2B */
void test_parse_hex4_value_1A2B(void)
{
    TEST_ASSERT_EQUAL_HEX(0x1A2B, call_parse_hex4("1A2B"));
}

/* Test: value 0x1a2b lowercase */
void test_parse_hex4_value_1a2b_lower(void)
{
    TEST_ASSERT_EQUAL_HEX(0x1A2B, call_parse_hex4("1a2b"));
}

/* Test: value 0xDEAD */
void test_parse_hex4_value_DEAD(void)
{
    TEST_ASSERT_EQUAL_HEX(0xDEAD, call_parse_hex4("DEAD"));
}

/* Test: value 0xBEEF */
void test_parse_hex4_value_BEEF(void)
{
    TEST_ASSERT_EQUAL_HEX(0xBEEF, call_parse_hex4("BEEF"));
}

/* Test: value 0xCAFE */
void test_parse_hex4_value_CAFE(void)
{
    TEST_ASSERT_EQUAL_HEX(0xCAFE, call_parse_hex4("CAFE"));
}

/* Test: value 0xcafe lowercase */
void test_parse_hex4_value_cafe_lower(void)
{
    TEST_ASSERT_EQUAL_HEX(0xCAFE, call_parse_hex4("cafe"));
}

/* Test: invalid character 'G' in first position returns 0 */
void test_parse_hex4_invalid_G_first(void)
{
    TEST_ASSERT_EQUAL_HEX(0x0000, call_parse_hex4("G000"));
}

/* Test: invalid character 'g' in first position returns 0 */
void test_parse_hex4_invalid_g_first(void)
{
    TEST_ASSERT_EQUAL_HEX(0x0000, call_parse_hex4("g000"));
}

/* Test: invalid character in second position returns 0 */
void test_parse_hex4_invalid_second_position(void)
{
    TEST_ASSERT_EQUAL_HEX(0x0000, call_parse_hex4("0G00"));
}

/* Test: invalid character in third position returns 0 */
void test_parse_hex4_invalid_third_position(void)
{
    TEST_ASSERT_EQUAL_HEX(0x0000, call_parse_hex4("00G0"));
}

/* Test: invalid character in fourth position returns 0 */
void test_parse_hex4_invalid_fourth_position(void)
{
    TEST_ASSERT_EQUAL_HEX(0x0000, call_parse_hex4("000G"));
}

/* Test: invalid character space returns 0 */
void test_parse_hex4_invalid_space(void)
{
    TEST_ASSERT_EQUAL_HEX(0x0000, call_parse_hex4(" 000"));
}

/* Test: invalid character colon (just above '9') returns 0 */
void test_parse_hex4_invalid_colon(void)
{
    TEST_ASSERT_EQUAL_HEX(0x0000, call_parse_hex4(":000"));
}

/* Test: invalid character '@' (just below 'A') returns 0 */
void test_parse_hex4_invalid_at_sign(void)
{
    TEST_ASSERT_EQUAL_HEX(0x0000, call_parse_hex4("@000"));
}

/* Test: invalid character '`' (just below 'a') returns 0 */
void test_parse_hex4_invalid_backtick(void)
{
    TEST_ASSERT_EQUAL_HEX(0x0000, call_parse_hex4("`000"));
}

/* Test: boundary digit '0' */
void test_parse_hex4_boundary_digit_0(void)
{
    TEST_ASSERT_EQUAL_HEX(0x0000, call_parse_hex4("0000"));
}

/* Test: boundary digit '9' */
void test_parse_hex4_boundary_digit_9(void)
{
    TEST_ASSERT_EQUAL_HEX(0x9999, call_parse_hex4("9999"));
}

/* Test: boundary uppercase 'A' */
void test_parse_hex4_boundary_upper_A(void)
{
    TEST_ASSERT_EQUAL_HEX(0xAAAA, call_parse_hex4("AAAA"));
}

/* Test: boundary uppercase 'F' */
void test_parse_hex4_boundary_upper_F(void)
{
    TEST_ASSERT_EQUAL_HEX(0xFFFF, call_parse_hex4("FFFF"));
}

/* Test: boundary lowercase 'a' */
void test_parse_hex4_boundary_lower_a(void)
{
    TEST_ASSERT_EQUAL_HEX(0xAAAA, call_parse_hex4("aaaa"));
}

/* Test: boundary lowercase 'f' */
void test_parse_hex4_boundary_lower_f(void)
{
    TEST_ASSERT_EQUAL_HEX(0xFFFF, call_parse_hex4("ffff"));
}

/* Test: mixed digits and uppercase */
void test_parse_hex4_mixed_digits_upper(void)
{
    TEST_ASSERT_EQUAL_HEX(0x9F1A, call_parse_hex4("9F1A"));
}

/* Test: mixed digits and lowercase */
void test_parse_hex4_mixed_digits_lower(void)
{
    TEST_ASSERT_EQUAL_HEX(0x9f1a, call_parse_hex4("9f1a"));
}

/* Test: value 0x0010 */
void test_parse_hex4_value_0010(void)
{
    TEST_ASSERT_EQUAL_HEX(0x0010, call_parse_hex4("0010"));
}

/* Test: value 0x0100 */
void test_parse_hex4_value_0100(void)
{
    TEST_ASSERT_EQUAL_HEX(0x0100, call_parse_hex4("0100"));
}

/* Test: value 0x1000 */
void test_parse_hex4_value_1000(void)
{
    TEST_ASSERT_EQUAL_HEX(0x1000, call_parse_hex4("1000"));
}

/* Test: uppercase E */
void test_parse_hex4_uppercase_E(void)
{
    TEST_ASSERT_EQUAL_HEX(0xEEEE, call_parse_hex4("EEEE"));
}

/* Test: lowercase e */
void test_parse_hex4_lowercase_e(void)
{
    TEST_ASSERT_EQUAL_HEX(0xEEEE, call_parse_hex4("eeee"));
}

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_parse_hex4_all_zeros);
    RUN_TEST(test_parse_hex4_all_uppercase_F);
    RUN_TEST(test_parse_hex4_all_lowercase_f);
    RUN_TEST(test_parse_hex4_mixed_case);
    RUN_TEST(test_parse_hex4_digits_only);
    RUN_TEST(test_parse_hex4_uppercase_ABCDEF);
    RUN_TEST(test_parse_hex4_lowercase_abcdef);
    RUN_TEST(test_parse_hex4_value_0001);
    RUN_TEST(test_parse_hex4_value_000F_upper);
    RUN_TEST(test_parse_hex4_value_000f_lower);
    RUN_TEST(test_parse_hex4_value_F000);
    RUN_TEST(test_parse_hex4_value_0F00);
    RUN_TEST(test_parse_hex4_value_00F0);
    RUN_TEST(test_parse_hex4_value_00A0);
    RUN_TEST(test_parse_hex4_value_1A2B);
    RUN_TEST(test_parse_hex4_value_1a2b_lower);
    RUN_TEST(test_parse_hex4_value_DEAD);
    RUN_TEST(test_parse_hex4_value_BEEF);
    RUN_TEST(test_parse_hex4_value_CAFE);
    RUN_TEST(test_parse_hex4_value_cafe_lower);
    RUN_TEST(test_parse_hex4_invalid_G_first);
    RUN_TEST(test_parse_hex4_invalid_g_first);
    RUN_TEST(test_parse_hex4_invalid_second_position);
    RUN_TEST(test_parse_hex4_invalid_third_position);
    RUN_TEST(test_parse_hex4_invalid_fourth_position);
    RUN_TEST(test_parse_hex4_invalid_space);
    RUN_TEST(test_parse_hex4_invalid_colon);
    RUN_TEST(test_parse_hex4_invalid_at_sign);
    RUN_TEST(test_parse_hex4_invalid_backtick);
    RUN_TEST(test_parse_hex4_boundary_digit_0);
    RUN_TEST(test_parse_hex4_boundary_digit_9);
    RUN_TEST(test_parse_hex4_boundary_upper_A);
    RUN_TEST(test_parse_hex4_boundary_upper_F);
    RUN_TEST(test_parse_hex4_boundary_lower_a);
    RUN_TEST(test_parse_hex4_boundary_lower_f);
    RUN_TEST(test_parse_hex4_mixed_digits_upper);
    RUN_TEST(test_parse_hex4_mixed_digits_lower);
    RUN_TEST(test_parse_hex4_value_0010);
    RUN_TEST(test_parse_hex4_value_0100);
    RUN_TEST(test_parse_hex4_value_1000);
    RUN_TEST(test_parse_hex4_uppercase_E);
    RUN_TEST(test_parse_hex4_lowercase_e);
    return UNITY_END();
}