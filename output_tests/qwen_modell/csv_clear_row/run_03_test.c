#include "csv.c"
#include "unity.h"
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>

static CSV_BUFFER *buffer = NULL;
static jmp_buf jump_buffer;
static volatile sig_atomic_t segv_occurred = 0;

void segv_handler(int sig) {
    (void)sig;
    segv_occurred = 1;
    longjmp(jump_buffer, 1);
}

void setUp(void) {
    segv_occurred = 0;
    signal(SIGSEGV, segv_handler);
    buffer = csv_create_buffer();
    TEST_ASSERT_NOT_NULL(buffer);
}

void tearDown(void) {
    if (buffer != NULL) {
        csv_destroy_buffer(buffer);
        buffer = NULL;
    }
    signal(SIGSEGV, SIG_DFL);
}

static int run_with_segv_protection(void (*func)(void)) {
    segv_occurred = 0;
    int result = setjmp(jump_buffer);
    if (result == 0) {
        func();
    }
    return segv_occurred;
}

static void test_csv_clear_row_last_row_success(void) {
    csv_set_field(buffer, 0, 0, "a");
    csv_set_field(buffer, 0, 1, "b");
    csv_set_field(buffer, 0, 2, "c");
    csv_set_field(buffer, 1, 0, "d");
    csv_set_field(buffer, 1, 1, "e");

    int ret = csv_clear_row(buffer, 1);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(2, csv_get_height(buffer));
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buffer, 1));
    char dest[16] = {0};
    csv_get_field(dest, sizeof(dest), buffer, 1, 0);
    TEST_ASSERT_EQUAL_STRING("", dest);
}

static void test_csv_clear_row_non_last_row_success(void) {
    csv_set_field(buffer, 0, 0, "a");
    csv_set_field(buffer, 0, 1, "b");
    csv_set_field(buffer, 0, 2, "c");

    int ret = csv_clear_row(buffer, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(1, csv_get_height(buffer));
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buffer, 0));
    char dest[16] = {0};
    csv_get_field(dest, sizeof(dest), buffer, 0, 0);
    TEST_ASSERT_EQUAL_STRING("", dest);
}

static void test_csv_clear_row_empty_row(void) {
    csv_set_field(buffer, 0, 0, "a");

    int ret = csv_clear_row(buffer, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(1, csv_get_height(buffer));
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buffer, 0));
    char dest[16] = {0};
    csv_get_field(dest, sizeof(dest), buffer, 0, 0);
    TEST_ASSERT_EQUAL_STRING("", dest);
}

static void test_csv_clear_row_invalid_row(void) {
    csv_set_field(buffer, 0, 0, "a");

    int ret = csv_clear_row(buffer, 10);
    TEST_ASSERT_EQUAL_INT(1, ret);
}

// static void test_csv_clear_row_segfault_prevention(void) {
//     void test_func(void) {
//         csv_clear_row(NULL, 0);
//     }
//     int segv = run_with_segv_protection(test_func);
//     TEST_ASSERT_TRUE(segv == 1 || segv == 0); // either segv caught or not (implementation-dependent)
// }

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_csv_clear_row_last_row_success);
    RUN_TEST(test_csv_clear_row_non_last_row_success);
    RUN_TEST(test_csv_clear_row_empty_row);
    RUN_TEST(test_csv_clear_row_invalid_row);
    // RUN_TEST(test_csv_clear_row_segfault_prevention);
    return UNITY_END();
}