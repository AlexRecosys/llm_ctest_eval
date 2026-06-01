#include "cJSON.c"
#include "unity.h"

#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>

/* Global variables for signal handling */
static sig_atomic_t segv_caught = 0;
static jmp_buf segv_env;

/* Signal handler for SIGSEGV */
static void segv_handler(int sig) {
    (void)sig;
    segv_caught = 1;
    longjmp(segv_env, 1);
}

/* setUp and tearDown must be non-static per requirements */
void setUp(void) {
    segv_caught = 0;
    signal(SIGSEGV, segv_handler);
}

void tearDown(void) {
    signal(SIGSEGV, SIG_DFL);
}

/* Test helper to safely call parse_hex4 with segv protection */
static unsigned safe_parse_hex4(const unsigned char *input) {
    if (setjmp(segv_env) == 0) {
        return parse_hex4(input);
    } else {
        return 0xFFFFFFFFu; /* Return sentinel value on segv */
    }
}

/* Test case 1: Valid hex string "0000" */
static void test_parse_hex4_valid_zero(void) {
    const unsigned char input[] = "0000";
    unsigned result = safe_parse_hex4(input);
    TEST_ASSERT_EQUAL_HEX16(0x0000, result);
}

/* Test case 2: Valid hex string "FFFF" */
static void test_parse_hex4_valid_max(void) {
    const unsigned char input[] = "FFFF";
    unsigned result = safe_parse_hex4(input);
    TEST_ASSERT_EQUAL_HEX16(0xFFFF, result);
}

/* Test case 3: Valid hex string with mixed case "aAbB" */
static void test_parse_hex4_mixed_case(void) {
    const unsigned char input[] = "aAbB";
    unsigned result = safe_parse_hex4(input);
    TEST_ASSERT_EQUAL_HEX16(0xAABB, result);
}

/* Test case 4: Invalid character in input */
static void test_parse_hex4_invalid_char(void) {
    const unsigned char input[] = "12X4";
    unsigned result = safe_parse_hex4(input);
    TEST_ASSERT_EQUAL_HEX16(0, result);
}

/* Test case 5: Null pointer input (should not crash) */
static void test_parse_hex4_null_input(void) {
    unsigned result = safe_parse_hex4(NULL);
    TEST_ASSERT_EQUAL_HEX16(0xFFFFFFFFu, result);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_parse_hex4_valid_zero);
    RUN_TEST(test_parse_hex4_valid_max);
    RUN_TEST(test_parse_hex4_mixed_case);
    RUN_TEST(test_parse_hex4_invalid_char);
    RUN_TEST(test_parse_hex4_null_input);
    return UNITY_END();
}