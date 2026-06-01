#include "cJSON.c"
#include "unity.h"
#include "cJSON.h"
#include <stdlib.h>
#include <string.h>
#include <signal.h>

/* Signal handling for SIGSEGV */
static volatile int segfault_occurred = 0;

static void sigsegv_handler(int sig)
{
    (void)sig;
    segfault_occurred = 1;
    /* We cannot safely longjmp here in all cases, so just mark it */
}

void setUp(void)
{
    segfault_occurred = 0;
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

/* Test 4: Mixed digits and letters "1a2F" -> 0x1a2F = 6703 */
void test_parse_hex4_mixed_digits_and_letters(void)
{
    const unsigned char input[] = {'1', 'a', '2', 'F'};
    unsigned int result = parse_hex4(input);
    TEST_ASSERT_EQUAL_HEX(0x1A2Fu, result);
}

/* Test 5: Invalid character in input returns 0 */
void test_parse_hex4_invalid_character_returns_zero(void)
{
    /* 'G' is not a valid hex digit */
    const unsigned char input_invalid_first[] = {'G', '1', '2', '3'};
    unsigned int result = parse_hex4(input_invalid_first);
    TEST_ASSERT_EQUAL_HEX(0x0000u, result);

    /* Invalid character in the middle */
    const unsigned char input_invalid_middle[] = {'1', '2', 'Z', '4'};
    result = parse_hex4(input_invalid_middle);
    TEST_ASSERT_EQUAL_HEX(0x0000u, result);

    /* Invalid character at the end */
    const unsigned char input_invalid_last[] = {'1', '2', '3', '!'};
    result = parse_hex4(input_invalid_last);
    TEST_ASSERT_EQUAL_HEX(0x0000u, result);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_parse_hex4_all_decimal_digits);
    RUN_TEST(test_parse_hex4_uppercase_hex_digits);
    RUN_TEST(test_parse_hex4_lowercase_hex_digits);
    RUN_TEST(test_parse_hex4_mixed_digits_and_letters);
    RUN_TEST(test_parse_hex4_invalid_character_returns_zero);
    return UNITY_END();
}