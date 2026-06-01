#include "csv.c"
#include "unity.h"
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>

static CSV_BUFFER *test_buffer = NULL;
static jmp_buf segv_jmp;

static void segv_handler(int sig) {
    (void)sig;
    longjmp(segv_jmp, 1);
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

static int safe_csv_get_field(char *dest, size_t dest_len, CSV_BUFFER *src, size_t row, size_t entry) {
    int result;
    if (setjmp(segv_jmp) == 0) {
        result = csv_get_field(dest, dest_len, src, row, entry);
    } else {
        TEST_FAIL_MESSAGE("Segmentation fault in csv_get_field");
        return -1;
    }
    return result;
}

void test_csv_get_field_success_full_copy(void) {
    char dest[100];
    int ret;

    // Add one row and one field
    csv_set_field(test_buffer, 0, 0, "Hello");

    ret = safe_csv_get_field(dest, sizeof(dest), test_buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING("Hello", dest);
}

void test_csv_get_field_truncation(void) {
    char dest[6];
    int ret;

    csv_set_field(test_buffer, 0, 0, "HelloWorld");

    ret = safe_csv_get_field(dest, sizeof(dest), test_buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT(1, ret);
    TEST_ASSERT_EQUAL_STRING_LEN("Hello", dest, 5);
    TEST_ASSERT_EQUAL_INT('\0', dest[5]);
}

void test_csv_get_field_empty_or_missing_field(void) {
    char dest[100];
    int ret;

    // Empty field
    csv_set_field(test_buffer, 0, 0, "");

    ret = safe_csv_get_field(dest, sizeof(dest), test_buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT(2, ret);
    TEST_ASSERT_EQUAL_STRING("", dest);

    // Missing row
    ret = safe_csv_get_field(dest, sizeof(dest), test_buffer, 99, 0);
    TEST_ASSERT_EQUAL_INT(2, ret);
    TEST_ASSERT_EQUAL_STRING("", dest);

    // Missing entry in existing row
    csv_set_field(test_buffer, 1, 0, "test");
    ret = safe_csv_get_field(dest, sizeof(dest), test_buffer, 1, 99);
    TEST_ASSERT_EQUAL_INT(2, ret);
    TEST_ASSERT_EQUAL_STRING("", dest);
}

void test_csv_get_field_zero_dest_len(void) {
    char dest[100];
    int ret;

    csv_set_field(test_buffer, 0, 0, "Hello");

    ret = safe_csv_get_field(dest, 0, test_buffer, 0, 0);
    TEST_ASSERT_EQUAL_INT(3, ret);
    // dest should remain untouched, but spec says nothing about its content in this case
}

void test_csv_get_field_clears_dest_on_error(void) {
    char dest[100] = "XXXXXXXXXXXXXXXXXXXX";
    int ret;

    // Request non-existent row
    ret = safe_csv_get_field(dest, sizeof(dest), test_buffer, 99, 0);
    TEST_ASSERT_EQUAL_INT(2, ret);
    TEST_ASSERT_EQUAL_STRING("", dest);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_csv_get_field_success_full_copy);
    RUN_TEST(test_csv_get_field_truncation);
    RUN_TEST(test_csv_get_field_empty_or_missing_field);
    RUN_TEST(test_csv_get_field_zero_dest_len);
    RUN_TEST(test_csv_get_field_clears_dest_on_error);

    return UNITY_END();
}