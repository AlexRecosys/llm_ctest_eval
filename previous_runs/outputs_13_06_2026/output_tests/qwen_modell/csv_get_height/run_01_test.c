#include "csv.c"
#include "unity.h"
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>

static CSV_BUFFER *buffer = NULL;
static jmp_buf segv_env;

static void segv_handler(int sig) {
    (void)sig;
    longjmp(segv_env, 1);
}

void setUp(void) {
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

static int safe_csv_get_height(CSV_BUFFER *buf) {
    if (setjmp(segv_env) != 0) {
        return -1; // segfault detected
    }
    return csv_get_height(buf);
}

void test_csv_get_height_returns_zero_for_new_buffer(void) {
    int height = safe_csv_get_height(buffer);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, height, "New buffer should have zero rows");
}

void test_csv_get_height_increments_after_append_row(void) {
    int height = safe_csv_get_height(buffer);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, height, "Initial height should be 0");

    // Use internal function via csv.c linkage — but we cannot call static functions directly.
    // Instead, use public API: csv_load or csv_set_field with row/entry creation.
    // Since append_row is static, we must use csv_set_field to implicitly create rows.
    // csv_set_field creates missing rows/fields as needed.

    int result = csv_set_field(buffer, 0, 0, "test");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "csv_set_field should succeed");

    height = safe_csv_get_height(buffer);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, height, "Height should be 1 after adding first row");
}

void test_csv_get_height_increments_after_multiple_rows(void) {
    // Create row 0
    int result = csv_set_field(buffer, 0, 0, "a");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "csv_set_field row 0 should succeed");

    // Create row 1
    result = csv_set_field(buffer, 1, 0, "b");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "csv_set_field row 1 should succeed");

    // Create row 2
    result = csv_set_field(buffer, 2, 0, "c");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "csv_set_field row 2 should succeed");

    int height = safe_csv_get_height(buffer);
    TEST_ASSERT_EQUAL_INT_MESSAGE(3, height, "Height should be 3 after adding 3 rows");
}

void test_csv_get_height_unchanged_after_clear_row(void) {
    // Add 2 rows
    csv_set_field(buffer, 0, 0, "x");
    csv_set_field(buffer, 1, 0, "y");

    int height = safe_csv_get_height(buffer);
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, height, "Height should be 2 before clearing");

    // Clear row 0 — does NOT remove row, only clears content
    int result = csv_clear_row(buffer, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "csv_clear_row should succeed");

    height = safe_csv_get_height(buffer);
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, height, "Height should remain 2 after clearing row");
}

void test_csv_get_height_returns_zero_after_destroy_and_recreate(void) {
    csv_destroy_buffer(buffer);
    buffer = csv_create_buffer();
    TEST_ASSERT_NOT_NULL(buffer);

    int height = safe_csv_get_height(buffer);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, height, "Recreated buffer should have zero rows");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_csv_get_height_returns_zero_for_new_buffer);
    RUN_TEST(test_csv_get_height_increments_after_append_row);
    RUN_TEST(test_csv_get_height_increments_after_multiple_rows);
    RUN_TEST(test_csv_get_height_unchanged_after_clear_row);
    RUN_TEST(test_csv_get_height_returns_zero_after_destroy_and_recreate);
    return UNITY_END();
}