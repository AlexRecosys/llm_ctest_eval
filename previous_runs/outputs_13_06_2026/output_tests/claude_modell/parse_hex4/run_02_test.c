#include "cJSON.c"
#include "unity.h"
#include "cJSON.h"
#include <stdlib.h>
#include <string.h>
#include <signal.h>

/* Signal handler for segmentation faults */
static void sigsegv_handler(int sig)
{
    (void)sig;
    TEST_FAIL_MESSAGE("Segmentation fault occurred during test execution");
}

void setUp(void)
{
    signal(SIGSEGV, sigsegv_handler);
}

void tearDown(void)
{
    signal(SIGSEGV, SIG_DFL);
}

/* Test: all decimal digits 0-9 */
void test_parse_hex4_all_decimal_digits(void)
{
    /* "0123" = 0x0123 = 291 */
    const unsigned char input[] = {'0', '1', '2', '3'};
    unsigned int result = parse_hex4(input);
    TEST_ASSERT_EQUAL_HEX(0x0123u, result);
}

/* Test: uppercase hex digits A-F */
void test_parse_hex4_uppercase_hex_digits(void)
{
    /* "ABCD" = 0xABCD = 43981 */
    const unsigned char input[] = {'A', 'B', 'C', 'D'};
    unsigned int result = parse_hex4(input);
    TEST_ASSERT_EQUAL_HEX(0xABCDu, result);
}

/* Test: lowercase hex digits a-f */
void test_parse_hex4_lowercase_hex_digits(void)
{
    /* "abcd" = 0xabcd = 43981 */
    const unsigned char input[] = {'a', 'b', 'c', 'd'};
    unsigned int result = parse_hex4(input);
    TEST_ASSERT_EQUAL_HEX(0xABCDu, result);
}

/* Test: mixed case and digits */
void test_parse_hex4_mixed_case_and_digits(void)
{
    /* "1a2F" = 0x1A2F = 6703 */
    const unsigned char input[] = {'1', 'a', '2', 'F'};
    unsigned int result = parse_hex4(input);
    TEST_ASSERT_EQUAL_HEX(0x1A2Fu, result);
}

/* Test: invalid character returns 0 */
void test_parse_hex4_invalid_character_returns_zero(void)
{
    /* "12G4" - 'G' is invalid */
    const unsigned char input[] = {'1', '2', 'G', '4'};
    unsigned int result = parse_hex4(input);
    TEST_ASSERT_EQUAL_UINT(0u, result);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_parse_hex4_all_decimal_digits);
    RUN_TEST(test_parse_hex4_uppercase_hex_digits);
    RUN_TEST(test_parse_hex4_lowercase_hex_digits);
    RUN_TEST(test_parse_hex4_mixed_case_and_digits);
    RUN_TEST(test_parse_hex4_invalid_character_returns_zero);
    return UNITY_END();
}