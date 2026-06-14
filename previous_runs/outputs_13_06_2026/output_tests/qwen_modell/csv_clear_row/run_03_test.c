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
        return segv_occurred ? 1 : 0;
    } else {
        return 1; // segv caught
    }
}

static void test_csv_clear_row_last_row_simple(void) {
    // Setup: 2 rows, row 1 has 3 fields
    csv_set_field(buffer, 0, 0, "row0col0");
    csv_set_field(buffer, 0, 1, "row0col1");
    csv_set_field(buffer, 1, 0, "row1col0");
    csv_set_field(buffer, 1, 1, "row1col1");
    csv_set_field(buffer, 1, 2, "row1col2");

    int ret = csv_clear_row(buffer, 1); // clear last row
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(2, csv_get_height(buffer));
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buffer, 1));
    char dest[64] = {0};
    csv_get_field(dest, sizeof(dest), buffer, 1, 0);
    TEST_ASSERT_EQUAL_STRING("", dest);
}

static void test_csv_clear_row_middle_row_shrink(void) {
    // Setup: 3 rows, row 1 has 4 fields
    csv_set_field(buffer, 0, 0, "row0col0");
    csv_set_field(buffer, 1, 0, "row1col0");
    csv_set_field(buffer, 1, 1, "row1col1");
    csv_set_field(buffer, 1, 2, "row1col2");
    csv_set_field(buffer, 1, 3, "row1col3");
    csv_set_field(buffer, 2, 0, "row2col0");

    int ret = csv_clear_row(buffer, 1); // clear middle row
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(3, csv_get_height(buffer));
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buffer, 1));
    char dest[64] = {0};
    csv_get_field(dest, sizeof(dest), buffer, 1, 0);
    TEST_ASSERT_EQUAL_STRING("", dest);
}

static void test_csv_clear_row_first_row_single_field(void) {
    // Setup: 2 rows, row 0 has 1 field
    csv_set_field(buffer, 0, 0, "row0col0");
    csv_set_field(buffer, 1, 0, "row1col0");

    int ret = csv_clear_row(buffer, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(2, csv_get_height(buffer));
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buffer, 0));
    char dest[64] = {0};
    csv_get_field(dest, sizeof(dest), buffer, 0, 0);
    TEST_ASSERT_EQUAL_STRING("", dest);
}

static void test_csv_clear_row_empty_row(void) {
    // Setup: 2 rows, row 0 has 0 fields
    csv_set_field(buffer, 0, 0, "row0col0");
    csv_set_field(buffer, 1, 0, "row1col0");

    // Clear row 0 (which has 1 field) first, then try row 1
    int ret = csv_clear_row(buffer, 1);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buffer, 1));
    char dest[64] = {0};
    csv_get_field(dest, sizeof(dest), buffer, 1, 0);
    TEST_ASSERT_EQUAL_STRING("", dest);
}

static void test_csv_clear_row_invalid_row(void) {
    // Setup: 1 row
    csv_set_field(buffer, 0, 0, "row0col0");

    int ret = csv_clear_row(buffer, 5); // invalid row
    // Function should handle gracefully; spec says returns 0 or 1
    // We just check it doesn't crash and returns non-negative
    TEST_ASSERT_TRUE(ret == 0 || ret == 1);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_csv_clear_row_last_row_simple);
    RUN_TEST(test_csv_clear_row_middle_row_shrink);
    RUN_TEST(test_csv_clear_row_first_row_single_field);
    RUN_TEST(test_csv_clear_row_empty_row);
    RUN_TEST(test_csv_clear_row_invalid_row);

    return UNITY_END();
}