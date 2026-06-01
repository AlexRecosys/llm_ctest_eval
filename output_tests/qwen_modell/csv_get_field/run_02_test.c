#include "csv.c"
#include "unity.h"
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>

/* Global buffer for test fixtures */
static CSV_BUFFER *test_buffer = NULL;
static char *test_dest = NULL;
static size_t test_dest_len = 0;

/* Signal handling for segfaults */
static sig_atomic_t segfault_occurred = 0;
static sigjmp_buf jump_buffer;

static void segv_handler(int sig) {
    (void)sig;
    segfault_occurred = 1;
    siglongjmp(jump_buffer, 1);
}

void setUp(void) {
    /* Install segfault handler */
    struct sigaction sa;
    sa.sa_handler = segv_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGSEGV, &sa, NULL);

    /* Allocate test buffer */
    test_buffer = csv_create_buffer();
    TEST_ASSERT_NOT_NULL(test_buffer);

    /* Allocate destination buffer */
    test_dest_len = 256;
    test_dest = malloc(test_dest_len);
    TEST_ASSERT_NOT_NULL(test_dest);
}

void tearDown(void) {
    /* Free resources */
    if (test_dest != NULL) {
        free(test_dest);
        test_dest = NULL;
    }
    if (test_buffer != NULL) {
        csv_destroy_buffer(test_buffer);
        test_buffer = NULL;
    }

    /* Reset segfault flag */
    segfault_occurred = 0;
}

/* Helper to populate buffer with test data */
static void populate_test_data(void) {
    /* Add 2 rows */
    csv_set_field(test_buffer, 0, 0, "field1");
    csv_set_field(test_buffer, 0, 1, "field2");
    csv_set_field(test_buffer, 0, 2, "field3");
    csv_set_field(test_buffer, 1, 0, "row2field1");
    csv_set_field(test_buffer, 1, 1, "row2field2");
}

/* Test: dest_len == 0 returns 3 */
void test_csv_get_field_dest_len_zero_returns_3(void) {
    populate_test_data();

    int result = csv_get_field(test_dest, 0, test_buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT(3, result);
}

/* Test: valid field copied completely returns 0 */
void test_csv_get_field_valid_field_full_copy_returns_0(void) {
    populate_test_data();

    size_t len = 256;
    char dest[256];
    int result = csv_get_field(dest, len, test_buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_STRING("field1", dest);
}

/* Test: field truncated returns 1 */
void test_csv_get_field_truncated_returns_1(void) {
    populate_test_data();

    char dest[6];  /* too small for "field1" (6 chars + null = 7) */
    int result = csv_get_field(dest, 5, test_buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT(1, result);
    TEST_ASSERT_EQUAL_STRING_LEN("fiel", dest, 5);
    TEST_ASSERT_EQUAL_INT('\0', dest[5]);
}

/* Test: non-existent row/entry returns 2 and clears dest */
void test_csv_get_field_nonexistent_entry_returns_2(void) {
    populate_test_data();

    char dest[256];
    memset(dest, 'X', sizeof(dest));  /* pre-fill with non-zero to detect clearing */

    int result = csv_get_field(dest, sizeof(dest), test_buffer, 99, 0);
    TEST_ASSERT_EQUAL_INT(2, result);

    /* Verify dest is cleared (first char is '\0') */
    TEST_ASSERT_EQUAL_INT('\0', dest[0]);
}

/* Test: empty field returns 2 */
void test_csv_get_field_empty_field_returns_2(void) {
    /* Create buffer with empty field */
    csv_set_field(test_buffer, 0, 0, "");
    csv_set_field(test_buffer, 0, 1, "nonempty");

    char dest[256];
    int result = csv_get_field(dest, sizeof(dest), test_buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT(2, result);
    TEST_ASSERT_EQUAL_STRING("", dest);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_csv_get_field_dest_len_zero_returns_3);
    RUN_TEST(test_csv_get_field_valid_field_full_copy_returns_0);
    RUN_TEST(test_csv_get_field_truncated_returns_1);
    RUN_TEST(test_csv_get_field_nonexistent_entry_returns_2);
    RUN_TEST(test_csv_get_field_empty_field_returns_2);

    return UNITY_END();
}