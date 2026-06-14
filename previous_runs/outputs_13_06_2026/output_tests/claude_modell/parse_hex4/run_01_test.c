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
    TEST_FAIL_MESSAGE("Segmentation fault occurred during test");
}

void setUp(void)
{
    signal(SIGSEGV, sigsegv_handler);
}

void tearDown(void)
{
    signal(SIGSEGV, SIG_DFL);
}

/* Test 1: All decimal digits "0123" -> 0x0123 = 291 */
void test_parse_hex4_all_decimal_digits(void)
{
    const unsigned char input[] = {'0', '1', '2', '3'};
    unsigned int result = parse_hex4(input);
    TEST_ASSERT_EQUAL_HEX(0x0123u, result);
}

/* Test 2: Uppercase hex digits "ABCD" -> 0xABCD = 43981 */
void test_parse_hex4_uppercase_hex_digits(void)
{
    const unsigned char input[] = {'A', 'B', 'C', 'D'};
    unsigned int result = parse_hex4(input);
    TEST_ASSERT_EQUAL_HEX(0xABCDu, result);
}

/* Test 3: Lowercase hex digits "abcd" -> 0xabcd = 43981 */
void test_parse_hex4_lowercase_hex_digits(void)
{
    const unsigned char input[] = {'a', 'b', 'c', 'd'};
    unsigned int result = parse_hex4(input);
    TEST_ASSERT_EQUAL_HEX(0xABCDu, result);
}

/* Test 4: Invalid character returns 0 */
void test_parse_hex4_invalid_character_returns_zero(void)
{
    const unsigned char input[] = {'G', '0', '0', '0'};
    unsigned int result = parse_hex4(input);
    TEST_ASSERT_EQUAL_HEX(0x0000u, result);
}

/* Test 5: Mixed case and digits "1aF9" -> 0x1aF9 = 6905 */
void test_parse_hex4_mixed_case_and_digits(void)
{
    const unsigned char input[] = {'1', 'a', 'F', '9'};
    unsigned int result = parse_hex4(input);
    TEST_ASSERT_EQUAL_HEX(0x1AF9u, result);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_parse_hex4_all_decimal_digits);
    RUN_TEST(test_parse_hex4_uppercase_hex_digits);
    RUN_TEST(test_parse_hex4_lowercase_hex_digits);
    RUN_TEST(test_parse_hex4_invalid_character_returns_zero);
    RUN_TEST(test_parse_hex4_mixed_case_and_digits);
    return UNITY_END();
}