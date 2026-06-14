#include "csv.c"
#include "unity.h"
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>

static CSV_BUFFER *test_buffer = NULL;
static jmp_buf segv_env;

static void segv_handler(int sig) {
    (void)sig;
    longjmp(segv_env, 1);
}

void setUp(void) {
    signal(SIGSEGV, segv_handler);
    test_buffer = csv_create_buffer();
    TEST_ASSERT_NOT_NULL(test_buffer);
}

void tearDown(void) {
    if (test_buffer != NULL) {
        csv_destroy_buffer(test_buffer);
        test_buffer = NULL;
    }
    signal(SIGSEGV, SIG_DFL);
}

static int safe_csv_remove_row(CSV_BUFFER *buffer, size_t row) {
    if (setjmp(segv_env) == 0) {
        return csv_remove_row(buffer, row);
    } else {
        return -1;  // segfault occurred
    }
}

void test_csv_remove_row_basic_removal(void) {
    // Setup: 3 rows, 2 fields each
    csv_set_field(test_buffer, 0, 0, "a1");
    csv_set_field(test_buffer, 0, 1, "a2");
    csv_set_field(test_buffer, 1, 0, "b1");
    csv_set_field(test_buffer, 1, 1, "b2");
    csv_set_field(test_buffer, 2, 0, "c1");
    csv_set_field(test_buffer, 2, 1, "c2");

    TEST_ASSERT_EQUAL_INT(3, csv_get_height(test_buffer));

    // Remove middle row (row 1)
    int result = safe_csv_remove_row(test_buffer, 1);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_INT(2, csv_get_height(test_buffer));

    // Verify remaining rows
    char buf[32];
    csv_get_field(buf, sizeof(buf), test_buffer, 0, 0);
    TEST_ASSERT_EQUAL_STRING("a1", buf);
    csv_get_field(buf, sizeof(buf), test_buffer, 0, 1);
    TEST_ASSERT_EQUAL_STRING("a2", buf);
    csv_get_field(buf, sizeof(buf), test_buffer, 1, 0);
    TEST_ASSERT_EQUAL_STRING("c1", buf);
    csv_get_field(buf, sizeof(buf), test_buffer, 1, 1);
    TEST_ASSERT_EQUAL_STRING("c2", buf);
}

void test_csv_remove_row_first_row(void) {
    // Setup: 3 rows
    csv_set_field(test_buffer, 0, 0, "first");
    csv_set_field(test_buffer, 1, 0, "second");
    csv_set_field(test_buffer, 2, 0, "third");

    int result = safe_csv_remove_row(test_buffer, 0);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_INT(2, csv_get_height(test_buffer));

    char buf[32];
    csv_get_field(buf, sizeof(buf), test_buffer, 0, 0);
    TEST_ASSERT_EQUAL_STRING("second", buf);
    csv_get_field(buf, sizeof(buf), test_buffer, 1, 0);
    TEST_ASSERT_EQUAL_STRING("third", buf);
}

void test_csv_remove_row_last_row(void) {
    // Setup: 3 rows
    csv_set_field(test_buffer, 0, 0, "first");
    csv_set_field(test_buffer, 1, 0, "second");
    csv_set_field(test_buffer, 2, 0, "third");

    int result = safe_csv_remove_row(test_buffer, 2);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_INT(2, csv_get_height(test_buffer));

    char buf[32];
    csv_get_field(buf, sizeof(buf), test_buffer, 0, 0);
    TEST_ASSERT_EQUAL_STRING("first", buf);
    csv_get_field(buf, sizeof(buf), test_buffer, 1, 0);
    TEST_ASSERT_EQUAL_STRING("second", buf);
}

void test_csv_remove_row_out_of_bounds(void) {
    // Setup: 2 rows
    csv_set_field(test_buffer, 0, 0, "a");
    csv_set_field(test_buffer, 1, 0, "b");

    // Try to remove row 2 (out of bounds: rows = 2, max valid index = 1)
    int result = safe_csv_remove_row(test_buffer, 2);
    TEST_ASSERT_EQUAL_INT(0, result);  // Function returns 0 on no-op per spec
    TEST_ASSERT_EQUAL_INT(2, csv_get_height(test_buffer));
}

void test_csv_remove_row_single_row(void) {
    // Setup: 1 row
    csv_set_field(test_buffer, 0, 0, "only");

    int result = safe_csv_remove_row(test_buffer, 0);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_INT(0, csv_get_height(test_buffer));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_csv_remove_row_basic_removal);
    RUN_TEST(test_csv_remove_row_first_row);
    RUN_TEST(test_csv_remove_row_last_row);
    RUN_TEST(test_csv_remove_row_out_of_bounds);
    RUN_TEST(test_csv_remove_row_single_row);
    return UNITY_END();
}