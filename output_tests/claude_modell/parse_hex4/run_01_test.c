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
static void test_parse_hex4_all_zeros(void)
{
    TEST_ASSERT_EQUAL_HEX(0x0000, call_parse_hex4("0000"));
}

/* Test: all uppercase F */
static void test_parse_hex4_all_uppercase_F(void)
{
    TEST_ASSERT_EQUAL_HEX(0xFFFF, call_parse_hex4("FFFF"));
}

/* Test: all lowercase f */
static void test_parse_hex4_all_lowercase_f(void)
{
    TEST_ASSERT_EQUAL_HEX(0xFFFF, call_parse_hex4("ffff"));
}

/* Test: mixed case */
static void test_parse_hex4_mixed_case(void)
{
    TEST_ASSERT_EQUAL_HEX(0xaBcD, call_parse_hex4("aBcD"));
}

/* Test: digits only */
static void test_parse_hex4_digits_only(void)
{
    TEST_ASSERT_EQUAL_HEX(0x1234, call_parse_hex4("1234"));
}

/* Test: uppercase hex letters only */
static void test_parse_hex4_uppercase_letters_only(void)
{
    TEST_ASSERT_EQUAL_HEX(0xABCD, call_parse_hex4("ABCD"));
}

/* Test: lowercase hex letters only */
static void test_parse_hex4_lowercase_letters_only(void)
{
    TEST_ASSERT_EQUAL_HEX(0xabcd, call_parse_hex4("abcd"));
}

/* Test: value 0x0001 */
static void test_parse_hex4_value_0001(void)
{
    TEST_ASSERT_EQUAL_HEX(0x0001, call_parse_hex4("0001"));
}

/* Test: value 0x000F uppercase */
static void test_parse_hex4_value_000F_upper(void)
{
    TEST_ASSERT_EQUAL_HEX(0x000F, call_parse_hex4("000F"));
}

/* Test: value 0x000f lowercase */
static void test_parse_hex4_value_000f_lower(void)
{
    TEST_ASSERT_EQUAL_HEX(0x000f, call_parse_hex4("000f"));
}

/* Test: value 0xDEAD */
static void test_parse_hex4_dead(void)
{
    TEST_ASSERT_EQUAL_HEX(0xDEAD, call_parse_hex4("DEAD"));
}

/* Test: value 0xdead lowercase */
static void test_parse_hex4_dead_lower(void)
{
    TEST_ASSERT_EQUAL_HEX(0xdead, call_parse_hex4("dead"));
}

/* Test: value 0xBEEF */
static void test_parse_hex4_beef(void)
{
    TEST_ASSERT_EQUAL_HEX(0xBEEF, call_parse_hex4("BEEF"));
}

/* Test: value 0x1A2B */
static void test_parse_hex4_1A2B(void)
{
    TEST_ASSERT_EQUAL_HEX(0x1A2B, call_parse_hex4("1A2B"));
}

/* Test: value 0x1a2b lowercase */
static void test_parse_hex4_1a2b_lower(void)
{
    TEST_ASSERT_EQUAL_HEX(0x1a2b, call_parse_hex4("1a2b"));
}

/* Test: value 0x9999 */
static void test_parse_hex4_9999(void)
{
    TEST_ASSERT_EQUAL_HEX(0x9999, call_parse_hex4("9999"));
}

/* Test: value 0xA0B0 */
static void test_parse_hex4_A0B0(void)
{
    TEST_ASSERT_EQUAL_HEX(0xA0B0, call_parse_hex4("A0B0"));
}

/* Test: value 0x0F0F */
static void test_parse_hex4_0F0F(void)
{
    TEST_ASSERT_EQUAL_HEX(0x0F0F, call_parse_hex4("0F0F"));
}

/* Test: value 0x0f0f lowercase */
static void test_parse_hex4_0f0f_lower(void)
{
    TEST_ASSERT_EQUAL_HEX(0x0f0f, call_parse_hex4("0f0f"));
}

/* Test: invalid character 'G' in first position returns 0 */
static void test_parse_hex4_invalid_G_first(void)
{
    TEST_ASSERT_EQUAL_HEX(0x0000, call_parse_hex4("G000"));
}

/* Test: invalid character 'g' in first position returns 0 */
static void test_parse_hex4_invalid_g_first(void)
{
    TEST_ASSERT_EQUAL_HEX(0x0000, call_parse_hex4("g000"));
}

/* Test: invalid character in second position returns 0 */
static void test_parse_hex4_invalid_second_position(void)
{
    TEST_ASSERT_EQUAL_HEX(0x0000, call_parse_hex4("0G00"));
}

/* Test: invalid character in third position returns 0 */
static void test_parse_hex4_invalid_third_position(void)
{
    TEST_ASSERT_EQUAL_HEX(0x0000, call_parse_hex4("00G0"));
}

/* Test: invalid character in fourth position returns 0 */
static void test_parse_hex4_invalid_fourth_position(void)
{
    TEST_ASSERT_EQUAL_HEX(0x0000, call_parse_hex4("000G"));
}

/* Test: space character is invalid */
static void test_parse_hex4_space_invalid(void)
{
    TEST_ASSERT_EQUAL_HEX(0x0000, call_parse_hex4(" 000"));
}

/* Test: colon character is invalid (just above '9') */
static void test_parse_hex4_colon_invalid(void)
{
    TEST_ASSERT_EQUAL_HEX(0x0000, call_parse_hex4(":000"));
}

/* Test: '@' character is invalid (just below 'A') */
static void test_parse_hex4_at_sign_invalid(void)
{
    TEST_ASSERT_EQUAL_HEX(0x0000, call_parse_hex4("@000"));
}

/* Test: '`' character is invalid (just below 'a') */
static void test_parse_hex4_backtick_invalid(void)
{
    TEST_ASSERT_EQUAL_HEX(0x0000, call_parse_hex4("`000"));
}

/* Test: boundary digit '0' */
static void test_parse_hex4_boundary_digit_0(void)
{
    TEST_ASSERT_EQUAL_HEX(0x0000, call_parse_hex4("0000"));
}

/* Test: boundary digit '9' */
static void test_parse_hex4_boundary_digit_9(void)
{
    TEST_ASSERT_EQUAL_HEX(0x9999, call_parse_hex4("9999"));
}

/* Test: boundary uppercase 'A' */
static void test_parse_hex4_boundary_upper_A(void)
{
    TEST_ASSERT_EQUAL_HEX(0xAAAA, call_parse_hex4("AAAA"));
}

/* Test: boundary uppercase 'F' */
static void test_parse_hex4_boundary_upper_F(void)
{
    TEST_ASSERT_EQUAL_HEX(0xFFFF, call_parse_hex4("FFFF"));
}

/* Test: boundary lowercase 'a' */
static void test_parse_hex4_boundary_lower_a(void)
{
    TEST_ASSERT_EQUAL_HEX(0xaaaa, call_parse_hex4("aaaa"));
}

/* Test: boundary lowercase 'f' */
static void test_parse_hex4_boundary_lower_f(void)
{
    TEST_ASSERT_EQUAL_HEX(0xffff, call_parse_hex4("ffff"));
}

/* Test: mixed digits and uppercase */
static void test_parse_hex4_mixed_digits_upper(void)
{
    TEST_ASSERT_EQUAL_HEX(0x1F2E, call_parse_hex4("1F2E"));
}

/* Test: mixed digits and lowercase */
static void test_parse_hex4_mixed_digits_lower(void)
{
    TEST_ASSERT_EQUAL_HEX(0x1f2e, call_parse_hex4("1f2e"));
}

/* Test: value 0xC0DE */
static void test_parse_hex4_C0DE(void)
{
    TEST_ASSERT_EQUAL_HEX(0xC0DE, call_parse_hex4("C0DE"));
}

/* Test: value 0xc0de lowercase */
static void test_parse_hex4_c0de_lower(void)
{
    TEST_ASSERT_EQUAL_HEX(0xc0de, call_parse_hex4("c0de"));
}

/* Test: value 0xFACE */
static void test_parse_hex4_FACE(void)
{
    TEST_ASSERT_EQUAL_HEX(0xFACE, call_parse_hex4("FACE"));
}

/* Test: value 0xface lowercase */
static void test_parse_hex4_face_lower(void)
{
    TEST_ASSERT_EQUAL_HEX(0xface, call_parse_hex4("face"));
}

/* Test: value 0x0010 */
static void test_parse_hex4_0010(void)
{
    TEST_ASSERT_EQUAL_HEX(0x0010, call_parse_hex4("0010"));
}

/* Test: value 0x0100 */
static void test_parse_hex4_0100(void)
{
    TEST_ASSERT_EQUAL_HEX(0x0100, call_parse_hex4("0100"));
}

/* Test: value 0x1000 */
static void test_parse_hex4_1000(void)
{
    TEST_ASSERT_EQUAL_HEX(0x1000, call_parse_hex4("1000"));
}

/* Test: invalid character '-' */
static void test_parse_hex4_minus_invalid(void)
{
    TEST_ASSERT_EQUAL_HEX(0x0000, call_parse_hex4("-000"));
}

/* Test: invalid character 'z' */
static void test_parse_hex4_z_invalid(void)
{
    TEST_ASSERT_EQUAL_HEX(0x0000, call_parse_hex4("z000"));
}

/* Test: invalid character 'Z' */
static void test_parse_hex4_Z_invalid(void)
{
    TEST_ASSERT_EQUAL_HEX(0x0000, call_parse_hex4("Z000"));
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
    RUN_TEST(test_parse_hex4_uppercase_letters_only);
    RUN_TEST(test_parse_hex4_lowercase_letters_only);
    RUN_TEST(test_parse_hex4_value_0001);
    RUN_TEST(test_parse_hex4_value_000F_upper);
    RUN_TEST(test_parse_hex4_value_000f_lower);
    RUN_TEST(test_parse_hex4_dead);
    RUN_TEST(test_parse_hex4_dead_lower);
    RUN_TEST(test_parse_hex4_beef);
    RUN_TEST(test_parse_hex4_1A2B);
    RUN_TEST(test_parse_hex4_1a2b_lower);
    RUN_TEST(test_parse_hex4_9999);
    RUN_TEST(test_parse_hex4_A0B0);
    RUN_TEST(test_parse_hex4_0F0F);
    RUN_TEST(test_parse_hex4_0f0f_lower);
    RUN_TEST(test_parse_hex4_invalid_G_first);
    RUN_TEST(test_parse_hex4_invalid_g_first);
    RUN_TEST(test_parse_hex4_invalid_second_position);
    RUN_TEST(test_parse_hex4_invalid_third_position);
    RUN_TEST(test_parse_hex4_invalid_fourth_position);
    RUN_TEST(test_parse_hex4_space_invalid);
    RUN_TEST(test_parse_hex4_colon_invalid);
    RUN_TEST(test_parse_hex4_at_sign_invalid);
    RUN_TEST(test_parse_hex4_backtick_invalid);
    RUN_TEST(test_parse_hex4_boundary_digit_0);
    RUN_TEST(test_parse_hex4_boundary_digit_9);
    RUN_TEST(test_parse_hex4_boundary_upper_A);
    RUN_TEST(test_parse_hex4_boundary_upper_F);
    RUN_TEST(test_parse_hex4_boundary_lower_a);
    RUN_TEST(test_parse_hex4_boundary_lower_f);
    RUN_TEST(test_parse_hex4_mixed_digits_upper);
    RUN_TEST(test_parse_hex4_mixed_digits_lower);
    RUN_TEST(test_parse_hex4_C0DE);
    RUN_TEST(test_parse_hex4_c0de_lower);
    RUN_TEST(test_parse_hex4_FACE);
    RUN_TEST(test_parse_hex4_face_lower);
    RUN_TEST(test_parse_hex4_0010);
    RUN_TEST(test_parse_hex4_0100);
    RUN_TEST(test_parse_hex4_1000);
    RUN_TEST(test_parse_hex4_minus_invalid);
    RUN_TEST(test_parse_hex4_z_invalid);
    RUN_TEST(test_parse_hex4_Z_invalid);
    return UNITY_END();
}