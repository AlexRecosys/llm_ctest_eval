#include "csv.c"
#include "unity.h"
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>

/* Global variables for signal handling */
static sig_atomic_t segv_occurred = 0;
static sigjmp_buf jump_buffer;

/* Signal handler for SIGSEGV */
static void segv_handler(int sig) {
    (void)sig;
    segv_occurred = 1;
    siglongjmp(jump_buffer, 1);
}

/* Global buffer for test fixtures */
static CSV_BUFFER *test_buffer = NULL;

void setUp(void) {
    /* Install SIGSEGV handler */
    struct sigaction sa;
    sa.sa_handler = segv_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGSEGV, &sa, NULL);

    /* Create a fresh buffer for each test */
    test_buffer = csv_create_buffer();
    TEST_ASSERT_NOT_NULL(test_buffer);
}

void tearDown(void) {
    /* Remove SIGSEGV handler */
    struct sigaction sa;
    sa.sa_handler = SIG_DFL;
    sigaction(SIGSEGV, &sa, NULL);

    /* Clean up buffer */
    if (test_buffer != NULL) {
        csv_destroy_buffer(test_buffer);
        test_buffer = NULL;
    }
}

/* Helper function to safely run code that might segfault */
static int run_with_segv_protection(void (*func)(void)) {
    segv_occurred = 0;
    int result = sigsetjmp(jump_buffer, 1);
    if (result == 0) {
        func();
        return segv_occurred ? -1 : 0;
    } else {
        return -1; /* segv occurred */
    }
}

/* Helper to set up a simple 2x2 CSV buffer */
static void setup_2x2_buffer(void) {
    /* Add two rows */
    int ret = csv_set_field(test_buffer, 0, 0, "field1");
    TEST_ASSERT_EQUAL_INT(0, ret);
    ret = csv_set_field(test_buffer, 0, 1, "field2");
    TEST_ASSERT_EQUAL_INT(0, ret);
    ret = csv_set_field(test_buffer, 1, 0, "field3");
    TEST_ASSERT_EQUAL_INT(0, ret);
    ret = csv_set_field(test_buffer, 1, 1, "field4");
    TEST_ASSERT_EQUAL_INT(0, ret);
}

/* Test case 1: Normal case - whole entry copied successfully */
static void test_csv_get_field_normal_success(void) {
    char dest[32];
    setup_2x2_buffer();

    int result = csv_get_field(dest, sizeof(dest), test_buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_STRING("field1", dest);
}

/* Test case 2: Entry truncated - dest buffer too small */
static void test_csv_get_field_truncated(void) {
    char dest[6]; /* Only 5 chars + null terminator */
    setup_2x2_buffer();

    int result = csv_get_field(dest, sizeof(dest), test_buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT(1, result);
    TEST_ASSERT_EQUAL_STRING_LEN("field", dest, 5);
    TEST_ASSERT_EQUAL_INT('\0', dest[5]);
}

/* Test case 3: Empty field (exists but empty string) */
static void test_csv_get_field_empty_field(void) {
    char dest[32];
    setup_2x2_buffer();

    /* Clear the first field to make it empty */
    int ret = csv_clear_row(test_buffer, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);

    int result = csv_get_field(dest, sizeof(dest), test_buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT(2, result);
    TEST_ASSERT_EQUAL_STRING("", dest);
}

/* Test case 4: Non-existent row or entry */
static void test_csv_get_field_nonexistent(void) {
    char dest[32];
    setup_2x2_buffer();

    /* Test non-existent row */
    int result = csv_get_field(dest, sizeof(dest), test_buffer, 10, 0);
    TEST_ASSERT_EQUAL_INT(2, result);
    TEST_ASSERT_EQUAL_STRING("", dest);

    /* Test non-existent entry in existing row */
    result = csv_get_field(dest, sizeof(dest), test_buffer, 0, 10);
    TEST_ASSERT_EQUAL_INT(2, result);
    TEST_ASSERT_EQUAL_STRING("", dest);
}

/* Test case 5: dest_len == 0 returns 3 and does not crash */
static void test_csv_get_field_zero_dest_len(void) {
    char dest[32];
    setup_2x2_buffer();

    int result = csv_get_field(dest, 0, test_buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT(3, result);
    /* dest should remain unchanged (but we can't rely on contents) */
}

/* Test case 6: Segfault protection for NULL buffer */
static void test_csv_get_field_null_buffer(void) {
    char dest[32];
    int result = run_with_segv_protection(NULL);
    if (result == -1) {
        TEST_FAIL_MESSAGE("Segmentation fault detected");
    }

    /* Now test with NULL buffer */
    result = run_with_segv_protection(NULL);
    if (result == -1) {
        TEST_FAIL_MESSAGE("Segmentation fault detected");
    }

    /* Actually call with NULL buffer */
    segv_occurred = 0;
    result = csv_get_field(dest, sizeof(dest), NULL, 0, 0);
    /* If segv occurred, test fails */
    TEST_ASSERT_FALSE(segv_occurred);
    /* If no segv, we expect undefined behavior but not crash */
}

/* Test case 7: Segfault protection for NULL dest */
static void test_csv_get_field_null_dest(void) {
    setup_2x2_buffer();

    int result = run_with_segv_protection(NULL);
    if (result == -1) {
        TEST_FAIL_MESSAGE("Segmentation fault detected");
    }

    /* Actually call with NULL dest */
    segv_occurred = 0;
    result = csv_get_field(NULL, 32, test_buffer, 0, 0);
    TEST_ASSERT_FALSE(segv_occurred);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_csv_get_field_normal_success);
    RUN_TEST(test_csv_get_field_truncated);
    RUN_TEST(test_csv_get_field_empty_field);
    RUN_TEST(test_csv_get_field_nonexistent);
    RUN_TEST(test_csv_get_field_zero_dest_len);
    RUN_TEST(test_csv_get_field_null_buffer);
    RUN_TEST(test_csv_get_field_null_dest);

    return UNITY_END();
}