#include "csv.c"
#include "unity.h"
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>

/* Global variables for test fixtures */
static CSV_BUFFER *test_buffer = NULL;
static jmp_buf segv_env;

/* Signal handler for SIGSEGV */
static void segv_handler(int sig) {
    (void)sig;
    longjmp(segv_env, 1);
}

/* setUp and tearDown must be non-static per requirements */
void setUp(void) {
    /* Install SIGSEGV handler */
    struct sigaction sa;
    sa.sa_handler = segv_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGSEGV, &sa, NULL);

    test_buffer = csv_create_buffer();
    TEST_ASSERT_NOT_NULL(test_buffer);
}

void tearDown(void) {
    /* Restore default SIGSEGV handler */
    struct sigaction sa;
    sa.sa_handler = SIG_DFL;
    sigaction(SIGSEGV, &sa, NULL);

    if (test_buffer != NULL) {
        csv_destroy_buffer(test_buffer);
        test_buffer = NULL;
    }
}

/* Helper to safely run code that might segfault */
static int run_with_segv_protection(jmp_buf env, void (*func)(void)) {
    int result = setjmp(env);
    if (result == 0) {
        func();
        return 0; /* No segfault */
    }
    return 1; /* Segfault occurred */
}

/* Test case 1: Valid field copy with sufficient buffer size */
static void test_csv_get_field_success_full_copy(void) {
    int ret;
    char dest[100];

    /* Add a row and a field */
    ret = csv_set_field(test_buffer, 0, 0, "Hello, World!");
    TEST_ASSERT_EQUAL_INT(0, ret);

    /* Get the field */
    ret = csv_get_field(dest, sizeof(dest), test_buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING("Hello, World!", dest);
}

/* Test case 2: Truncation when destination buffer is too small */
static void test_csv_get_field_truncation(void) {
    int ret;
    char dest[10];

    /* Add a field with content longer than dest */
    ret = csv_set_field(test_buffer, 0, 0, "Hello, World!");
    TEST_ASSERT_EQUAL_INT(0, ret);

    /* Get the field with small buffer */
    ret = csv_get_field(dest, sizeof(dest), test_buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT(1, ret);
    TEST_ASSERT_EQUAL_STRING_LEN("Hello, Wo", dest, sizeof(dest) - 1);
    TEST_ASSERT_EQUAL_INT('\0', dest[sizeof(dest) - 1]);
}

/* Test case 3: Empty field returns 2 and clears buffer */
static void test_csv_get_field_empty_field(void) {
    int ret;
    char dest[50];
    char expected[50] = {0};

    /* Add an empty field */
    ret = csv_set_field(test_buffer, 0, 0, "");
    TEST_ASSERT_EQUAL_INT(0, ret);

    /* Get the empty field */
    ret = csv_get_field(dest, sizeof(dest), test_buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT(2, ret);
    TEST_ASSERT_EQUAL_MEMORY(expected, dest, sizeof(dest));
}

/* Test case 4: Invalid row or entry returns 2 and clears buffer */
static void test_csv_get_field_invalid_entry(void) {
    int ret;
    char dest[50];
    char expected[50] = {0};

    /* Add one row with one field */
    ret = csv_set_field(test_buffer, 0, 0, "test");
    TEST_ASSERT_EQUAL_INT(0, ret);

    /* Try to access non-existent row */
    ret = csv_get_field(dest, sizeof(dest), test_buffer, 1, 0);
    TEST_ASSERT_EQUAL_INT(2, ret);
    TEST_ASSERT_EQUAL_MEMORY(expected, dest, sizeof(dest));

    /* Try to access non-existent entry in existing row */
    ret = csv_get_field(dest, sizeof(dest), test_buffer, 0, 1);
    TEST_ASSERT_EQUAL_INT(2, ret);
    TEST_ASSERT_EQUAL_MEMORY(expected, dest, sizeof(dest));
}

/* Test case 5: Zero destination length returns 3 */
static void test_csv_get_field_zero_dest_len(void) {
    int ret;
    char dest[50];

    /* Add a field */
    ret = csv_set_field(test_buffer, 0, 0, "test");
    TEST_ASSERT_EQUAL_INT(0, ret);

    /* Get with zero length */
    ret = csv_get_field(dest, 0, test_buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT(3, ret);
}

/* Test case 6: Segfault protection for NULL buffer */
static void test_csv_get_field_null_buffer(void) {
    int ret;
    char dest[50];

    /* Install protection and attempt call with NULL buffer */
    if (run_with_segv_protection(segv_env, NULL)) {
        /* Skip if segv protection not available */
        TEST_FAIL_MESSAGE("SIGSEGV protection not working");
    }

    /* This should not crash */
    ret = csv_get_field(dest, sizeof(dest), NULL, 0, 0);
    /* If we get here without crash, we assume it's safe */
    /* Note: behavior is undefined per spec, but we test for crash safety */
    TEST_FAIL_MESSAGE("Should not reach here if NULL buffer causes segfault");
}

/* Override test case 6 with proper NULL buffer handling test */
static void test_csv_get_field_null_dest(void) {
    int ret;

    /* Add a field */
    ret = csv_set_field(test_buffer, 0, 0, "test");
    TEST_ASSERT_EQUAL_INT(0, ret);

    /* Try with NULL destination (should not crash) */
    if (run_with_segv_protection(segv_env, NULL)) {
        TEST_FAIL_MESSAGE("SIGSEGV protection not working");
    }

    ret = csv_get_field(NULL, 50, test_buffer, 0, 0);
    /* If we get here without crash, we assume it's safe */
    TEST_FAIL_MESSAGE("Should not reach here if NULL dest causes segfault");
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_csv_get_field_success_full_copy);
    RUN_TEST(test_csv_get_field_truncation);
    RUN_TEST(test_csv_get_field_empty_field);
    RUN_TEST(test_csv_get_field_invalid_entry);
    RUN_TEST(test_csv_get_field_zero_dest_len);

    return UNITY_END();
}