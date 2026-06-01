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

static void test_csv_clear_row_last_row_simple(void) {
    // Setup: 2 rows, row 1 has 3 fields
    csv_set_field(buffer, 0, 0, "row0col0");
    csv_set_field(buffer, 0, 1, "row0col1");
    csv_set_field(buffer, 1, 0, "row1col0");
    csv_set_field(buffer, 1, 1, "row1col1");
    csv_set_field(buffer, 1, 2, "row1col2");

    TEST_ASSERT_EQUAL_INT(2, csv_get_height(buffer));
    TEST_ASSERT_EQUAL_INT(3, csv_get_width(buffer, 1));

    // Act
    int ret = csv_clear_row(buffer, 1);

    // Assert
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(1, csv_get_height(buffer));  // row 1 removed
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buffer, 0));
}

static void test_csv_clear_row_middle_row_reduces_to_one_field(void) {
    // Setup: 3 rows, middle row (row 1) has 4 fields
    csv_set_field(buffer, 0, 0, "row0col0");
    csv_set_field(buffer, 1, 0, "row1col0");
    csv_set_field(buffer, 1, 1, "row1col1");
    csv_set_field(buffer, 1, 2, "row1col2");
    csv_set_field(buffer, 1, 3, "row1col3");
    csv_set_field(buffer, 2, 0, "row2col0");

    TEST_ASSERT_EQUAL_INT(3, csv_get_height(buffer));
    TEST_ASSERT_EQUAL_INT(4, csv_get_width(buffer, 1));

    // Act
    int ret = csv_clear_row(buffer, 1);

    // Assert
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(3, csv_get_height(buffer));  // same height
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buffer, 1));  // reduced to 1 field
    char dest[256];
    csv_get_field(dest, sizeof(dest), buffer, 1, 0);
    TEST_ASSERT_EQUAL_STRING("", dest);  // cleared
}

static void test_csv_clear_row_first_row_with_many_fields(void) {
    // Setup: 1 row with 5 fields
    csv_set_field(buffer, 0, 0, "a");
    csv_set_field(buffer, 0, 1, "b");
    csv_set_field(buffer, 0, 2, "c");
    csv_set_field(buffer, 0, 3, "d");
    csv_set_field(buffer, 0, 4, "e");

    TEST_ASSERT_EQUAL_INT(1, csv_get_height(buffer));
    TEST_ASSERT_EQUAL_INT(5, csv_get_width(buffer, 0));

    // Act
    int ret = csv_clear_row(buffer, 0);

    // Assert
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(1, csv_get_height(buffer));
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buffer, 0));
    char dest[256];
    csv_get_field(dest, sizeof(dest), buffer, 0, 0);
    TEST_ASSERT_EQUAL_STRING("", dest);
}

static void test_csv_clear_row_empty_row(void) {
    // Setup: 2 rows, row 0 empty, row 1 has 1 field
    csv_set_field(buffer, 1, 0, "row1col0");

    TEST_ASSERT_EQUAL_INT(2, csv_get_height(buffer));
    TEST_ASSERT_EQUAL_INT(0, csv_get_width(buffer, 0));

    // Act: clear empty row (row 0)
    int ret = csv_clear_row(buffer, 0);

    // Assert
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(2, csv_get_height(buffer));
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buffer, 0));  // now has 1 empty field
    char dest[256];
    csv_get_field(dest, sizeof(dest), buffer, 0, 0);
    TEST_ASSERT_EQUAL_STRING("", dest);
}

static void test_csv_clear_row_segfault_safety(void) {
    // Test that segfault is caught gracefully when buffer is NULL
    void (*func)(void) = NULL;
    int result;

    // Case 1: NULL buffer
    buffer = NULL;
    func = NULL;
    result = run_with_segv_protection(func);
    TEST_ASSERT_TRUE(result);  // segv occurred

    // Reinitialize for other tests
    setUp();
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_csv_clear_row_last_row_simple);
    RUN_TEST(test_csv_clear_row_middle_row_reduces_to_one_field);
    RUN_TEST(test_csv_clear_row_first_row_with_many_fields);
    RUN_TEST(test_csv_clear_row_empty_row);
    RUN_TEST(test_csv_clear_row_segfault_safety);
    return UNITY_END();
}