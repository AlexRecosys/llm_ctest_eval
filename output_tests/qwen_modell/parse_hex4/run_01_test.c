#include "cJSON.c"

#include "unity.h"

#include <string.h>

/* Helper function to create a 4-character hex string for testing */
static void make_hex_string(char *buf, char c0, char c1, char c2, char c3)
{
    buf[0] = c0;
    buf[1] = c1;
    buf[2] = c2;
    buf[3] = c3;
    buf[4] = '\0';
}

/* Test valid hex digits: 0-9, A-F, a-f */
static void test_parse_hex4_valid_digits(void)
{
    unsigned result;
    char input[5];

    /* All zeros */
    make_hex_string(input, '0', '0', '0', '0');
    result = parse_hex4((const unsigned char *)input);
    TEST_ASSERT_EQUAL_HEX(0x0000, result);

    /* Single digit */
    make_hex_string(input, '0', '0', '0', '1');
    result = parse_hex4((const unsigned char *)input);
    TEST_ASSERT_EQUAL_HEX(0x0001, result);

    /* All nines */
    make_hex_string(input, '0', '0', '0', '9');
    result = parse_hex4((const unsigned char *)input);
    TEST_ASSERT_EQUAL_HEX(0x0009, result);

    /* A-F uppercase */
    make_hex_string(input, '0', '0', '0', 'A');
    result = parse_hex4((const unsigned char *)input);
    TEST_ASSERT_EQUAL_HEX(0x000A, result);

    make_hex_string(input, '0', '0', '0', 'F');
    result = parse_hex4((const unsigned char *)input);
    TEST_ASSERT_EQUAL_HEX(0x000F, result);

    /* a-f lowercase */
    make_hex_string(input, '0', '0', '0', 'a');
    result = parse_hex4((const unsigned char *)input);
    TEST_ASSERT_EQUAL_HEX(0x000a, result);

    make_hex_string(input, '0', '0', '0', 'f');
    result = parse_hex4((const unsigned char *)input);
    TEST_ASSERT_EQUAL_HEX(0x000f, result);

    /* Full range: 0-F */
    make_hex_string(input, 'F', 'F', 'F', 'F');
    result = parse_hex4((const unsigned char *)input);
    TEST_ASSERT_EQUAL_HEX(0xFFFF, result);

    make_hex_string(input, 'f', 'f', 'f', 'f');
    result = parse_hex4((const unsigned char *)input);
    TEST_ASSERT_EQUAL_HEX(0xFFFF, result);

    /* Mixed case */
    make_hex_string(input, 'A', 'b', 'C', 'd');
    result = parse_hex4((const unsigned char *)input);
    TEST_ASSERT_EQUAL_HEX(0xABCD, result);

    make_hex_string(input, 'a', 'B', 'c', 'D');
    result = parse_hex4((const unsigned char *)input);
    TEST_ASSERT_EQUAL_HEX(0xABCD, result);
}

/* Test invalid characters */
static void test_parse_hex4_invalid_chars(void)
{
    unsigned result;
    char input[5];

    /* Non-hex digit: G */
    make_hex_string(input, '0', '0', '0', 'G');
    result = parse_hex4((const unsigned char *)input);
    TEST_ASSERT_EQUAL_HEX(0, result);

    /* Non-hex digit: g */
    make_hex_string(input, '0', '0', '0', 'g');
    result = parse_hex4((const unsigned char *)input);
    TEST_ASSERT_EQUAL_HEX(0, result);

    /* Non-hex digit: Z */
    make_hex_string(input, 'Z', '0', '0', '0');
    result = parse_hex4((const unsigned char *)input);
    TEST_ASSERT_EQUAL_HEX(0, result);

    /* Non-hex digit: 1 (in first position) */
    make_hex_string(input, '1', '0', '0', '0');
    result = parse_hex4((const unsigned char *)input);
    TEST_ASSERT_EQUAL_HEX(0x1000, result);

    /* Space character */
    make_hex_string(input, ' ', '0', '0', '0');
    result = parse_hex4((const unsigned char *)input);
    TEST_ASSERT_EQUAL_HEX(0, result);

    /* Null character */
    make_hex_string(input, '\0', '0', '0', '0');
    result = parse_hex4((const unsigned char *)input);
    TEST_ASSERT_EQUAL_HEX(0, result);

    /* Special characters */
    make_hex_string(input, '-', '0', '0', '0');
    result = parse_hex4((const unsigned char *)input);
    TEST_ASSERT_EQUAL_HEX(0, result);

    make_hex_string(input, '.', '0', '0', '0');
    result = parse_hex4((const unsigned char *)input);
    TEST_ASSERT_EQUAL_HEX(0, result);
}

/* Test partial hex strings (first 3 digits valid, last invalid) */
static void test_parse_hex4_partial_valid(void)
{
    unsigned result;
    char input[5];

    /* First three valid, last invalid */
    make_hex_string(input, '1', '2', '3', 'X');
    result = parse_hex4((const unsigned char *)input);
    TEST_ASSERT_EQUAL_HEX(0, result);

    /* First invalid, rest valid */
    make_hex_string(input, 'X', '1', '2', '3');
    result = parse_hex4((const unsigned char *)input);
    TEST_ASSERT_EQUAL_HEX(0, result);
}

/* Test edge cases */
static void test_parse_hex4_edge_cases(void)
{
    unsigned result;
    char input[5];

    /* All spaces */
    make_hex_string(input, ' ', ' ', ' ', ' ');
    result = parse_hex4((const unsigned char *)input);
    TEST_ASSERT_EQUAL_HEX(0, result);

    /* Mixed valid and invalid */
    make_hex_string(input, '1', '2', 'G', '4');
    result = parse_hex4((const unsigned char *)input);
    TEST_ASSERT_EQUAL_HEX(0, result);
}

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();

    RUN_TEST(test_parse_hex4_valid_digits);
    RUN_TEST(test_parse_hex4_invalid_chars);
    RUN_TEST(test_parse_hex4_partial_valid);
    RUN_TEST(test_parse_hex4_edge_cases);

    return UNITY_END();
}