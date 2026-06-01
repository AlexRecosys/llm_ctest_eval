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
static jmp_buf jump_buffer;
static volatile sig_atomic_t segv_occurred = 0;

/* Signal handler for segmentation faults */
static void segv_handler(int sig) {
    (void)sig;
    segv_occurred = 1;
    longjmp(jump_buffer, 1);
}

/* Setup function */
void setUp(void) {
    /* Set up signal handler for segmentation faults */
    struct sigaction sa;
    sa.sa_handler = segv_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGSEGV, &sa, NULL);

    /* Initialize test item */
    test_item = (cJSON *)malloc(sizeof(cJSON));
    TEST_ASSERT_NOT_NULL_MESSAGE(test_item, "Failed to allocate test item");
    memset(test_item, 0, sizeof(cJSON));

    /* Initialize parse buffer with default hooks */
    memset(&test_buffer, 0, sizeof(test_buffer));
    test_buffer.hooks.allocate = malloc;
    test_buffer.hooks.deallocate = free;
    test_buffer.hooks.reallocate = realloc;
}

/* Teardown function */
void tearDown(void) {
    /* Clean up test item */
    if (test_item != NULL) {
        free(test_item);
        test_item = NULL;
    }

    /* Reset signal handler */
    signal(SIGSEGV, SIG_DFL);
}

/* Helper function to safely call parse_number with segv protection */
static cJSON_bool safe_parse_number(cJSON *item, parse_buffer *buf) {
    segv_occurred = 0;
    if (setjmp(jump_buffer) == 0) {
        return parse_number(item, buf);
    } else {
        return false; /* segfault occurred */
    }
}

/* Test case 1: Parse a simple integer */
void test_parse_number_simple_integer(void) {
    const char *input = "42";
    test_buffer.content = (const unsigned char *)input;
    test_buffer.length = strlen(input);
    test_buffer.offset = 0;

    cJSON_bool result = safe_parse_number(test_item, &test_buffer);
    
    TEST_ASSERT_TRUE_MESSAGE(result, "Parsing simple integer should succeed");
    TEST_ASSERT_EQUAL_INT(42, test_item->valueint);
    TEST_ASSERT_EQUAL_DOUBLE(42.0, test_item->valuedouble);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item->type);
    TEST_ASSERT_EQUAL_SIZE(strlen(input), test_buffer.offset);
}

/* Test case 2: Parse a negative number with decimal point */
void test_parse_number_negative_decimal(void) {
    const char *input = "-3.14159";
    test_buffer.content = (const unsigned char *)input;
    test_buffer.length = strlen(input);
    test_buffer.offset = 0;

    cJSON_bool result = safe_parse_number(test_item, &test_buffer);
    
    TEST_ASSERT_TRUE_MESSAGE(result, "Parsing negative decimal should succeed");
    TEST_ASSERT_DOUBLE_WITHIN(0.000001, -3.14159, test_item->valuedouble);
    TEST_ASSERT_EQUAL_INT(-3, test_item->valueint);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item->type);
    TEST_ASSERT_EQUAL_SIZE(strlen(input), test_buffer.offset);
}

/* Test case 3: Parse a number with scientific notation */
void test_parse_number_scientific(void) {
    const char *input = "1.23e10";
    test_buffer.content = (const unsigned char *)input;
    test_buffer.length = strlen(input);
    test_buffer.offset = 0;

    cJSON_bool result = safe_parse_number(test_item, &test_buffer);
    
    TEST_ASSERT_TRUE_MESSAGE(result, "Parsing scientific notation should succeed");
    TEST_ASSERT_DOUBLE_WITHIN(0.000001, 1.23e10, test_item->valuedouble);
    TEST_ASSERT_EQUAL_INT(12300000000, test_item->valueint);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item->type);
    TEST_ASSERT_EQUAL_SIZE(strlen(input), test_buffer.offset);
}

/* Test case 4: Parse a number that overflows to INT_MAX */
void test_parse_number_overflow_to_int_max(void) {
    const char *input = "999999999999";
    test_buffer.content = (const unsigned char *)input;
    test_buffer.length = strlen(input);
    test_buffer.offset = 0;

    cJSON_bool result = safe_parse_number(test_item, &test_buffer);
    
    TEST_ASSERT_TRUE_MESSAGE(result, "Parsing large positive number should succeed");
    TEST_ASSERT_EQUAL_INT(INT_MAX, test_item->valueint);
    TEST_ASSERT_EQUAL_DOUBLE(999999999999.0, test_item->valuedouble);
    TEST_ASSERT_EQUAL_INT(cJSON_Number, test_item->type);
    TEST_ASSERT_EQUAL_SIZE(strlen(input), test_buffer.offset);
}

/* Test case 5: Parse invalid input (no digits) */
void test_parse_number_invalid_input(void) {
    const char *input = "abc";
    test_buffer.content = (const unsigned char *)input;
    test_buffer.length = strlen(input);
    test_buffer.offset = 0;

    cJSON_bool result = safe_parse_number(test_item, &test_buffer);
    
    TEST_ASSERT_FALSE_MESSAGE(result, "Parsing invalid input should fail");
    TEST_ASSERT_EQUAL_INT(0, test_item->valueint);
    TEST_ASSERT_EQUAL_INT(0, test_item->valuedouble);
    TEST_ASSERT_EQUAL_INT(cJSON_Invalid, test_item->type);
    TEST_ASSERT_EQUAL_SIZE(0, test_buffer.offset);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_parse_number_simple_integer);
    RUN_TEST(test_parse_number_negative_decimal);
    RUN_TEST(test_parse_number_scientific);
    RUN_TEST(test_parse_number_overflow_to_int_max);
    RUN_TEST(test_parse_number_invalid_input);
    return UNITY_END();
}