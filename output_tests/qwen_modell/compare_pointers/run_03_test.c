#include "cJSON.c"
#include "unity.h"

#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>
#include <ctype.h>

/* Global variables for signal handling */
static sig_atomic_t segv_caught = 0;
static sigjmp_buf jump_buffer;

/* Signal handler for SIGSEGV */
static void segv_handler(int sig) {
    (void)sig;
    segv_caught = 1;
    siglongjmp(jump_buffer, 1);
}

/* Global fixtures */
static char *test_name = NULL;
static char *test_pointer = NULL;

void setUp(void) {
    segv_caught = 0;
    test_name = NULL;
    test_pointer = NULL;
}

void tearDown(void) {
    /* No dynamic allocations in this test suite */
    (void)test_name;
    (void)test_pointer;
}

/* Helper to run function under SIGSEGV protection */
static cJSON_bool safe_compare_pointers(const unsigned char *name, const unsigned char *pointer, cJSON_bool case_sensitive) {
    if (sigsetjmp(jump_buffer, 1) != 0) {
        segv_caught = 1;
        return false;
    }
    /* Install handler */
    void (*old_handler)(int) = signal(SIGSEGV, segv_handler);
    cJSON_bool result = compare_pointers(name, pointer, case_sensitive);
    /* Restore handler */
    signal(SIGSEGV, old_handler);
    return result;
}

/* Test 1: NULL inputs */
static void test_compare_pointers_null_inputs(void) {
    TEST_ASSERT_FALSE_MESSAGE(safe_compare_pointers(NULL, (const unsigned char*)"test", false), "NULL name should return false");
    TEST_ASSERT_FALSE_MESSAGE(safe_compare_pointers((const unsigned char*)"test", NULL, false), "NULL pointer should return false");
    TEST_ASSERT_FALSE_MESSAGE(safe_compare_pointers(NULL, NULL, false), "Both NULL should return false");
}

/* Test 2: Exact match (case-sensitive) */
static void test_compare_pointers_exact_match_case_sensitive(void) {
    TEST_ASSERT_TRUE_MESSAGE(safe_compare_pointers((const unsigned char*)"test", (const unsigned char*)"test", true), "Exact match (case-sensitive) should return true");
}

/* Test 3: Case-insensitive match */
static void test_compare_pointers_case_insensitive(void) {
    TEST_ASSERT_TRUE_MESSAGE(safe_compare_pointers((const unsigned char*)"Test", (const unsigned char*)"test", false), "Case-insensitive match should return true");
    TEST_ASSERT_TRUE_MESSAGE(safe_compare_pointers((const unsigned char*)"TEST", (const unsigned char*)"test", false), "Case-insensitive uppercase should return true");
}

/* Test 4: Escape sequences (~0 for ~, ~1 for /) */
static void test_compare_pointers_escape_sequences(void) {
    TEST_ASSERT_TRUE_MESSAGE(safe_compare_pointers((const unsigned char*)"~test", (const unsigned char*)"~0test", false), "~0 escape for ~ should match");
    TEST_ASSERT_TRUE_MESSAGE(safe_compare_pointers((const unsigned char*)"/test", (const unsigned char*)"~1test", false), "~1 escape for / should match");
    TEST_ASSERT_TRUE_MESSAGE(safe_compare_pointers((const unsigned char*)"a~b", (const unsigned char*)"a~0b", false), "Mixed ~ escape should match");
    TEST_ASSERT_TRUE_MESSAGE(safe_compare_pointers((const unsigned char*)"a/b", (const unsigned char*)"a~1b", false), "Mixed / escape should match");
}

/* Test 5: Mismatched lengths and invalid escape sequences */
static void test_compare_pointers_mismatch_and_invalid_escape(void) {
    TEST_ASSERT_FALSE_MESSAGE(safe_compare_pointers((const unsigned char*)"test", (const unsigned char*)"tes", false), "Shorter pointer should return false");
    TEST_ASSERT_FALSE_MESSAGE(safe_compare_pointers((const unsigned char*)"tes", (const unsigned char*)"test", false), "Longer pointer should return false");
    TEST_ASSERT_FALSE_MESSAGE(safe_compare_pointers((const unsigned char*)"~", (const unsigned char*)"~0", false), "Trailing ~ should return false");
    TEST_ASSERT_FALSE_MESSAGE(safe_compare_pointers((const unsigned char*)"x", (const unsigned char*)"~2", false), "Invalid escape ~2 should return false");
    TEST_ASSERT_FALSE_MESSAGE(safe_compare_pointers((const unsigned char*)"~", (const unsigned char*)"~x", false), "Invalid escape ~x should return false");
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_compare_pointers_null_inputs);
    RUN_TEST(test_compare_pointers_exact_match_case_sensitive);
    RUN_TEST(test_compare_pointers_case_insensitive);
    RUN_TEST(test_compare_pointers_escape_sequences);
    RUN_TEST(test_compare_pointers_mismatch_and_invalid_escape);

    return UNITY_END();
}