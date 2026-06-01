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

static void test_csv_clear_row_last_row_shrink_success(void) {
    // Setup: create buffer with 2 rows, each with multiple fields
    csv_set_field(buffer, 0, 0, "row0col0");
    csv_set_field(buffer, 0, 1, "row0col1");
    csv_set_field(buffer, 1, 0, "row1col0");
    csv_set_field(buffer, 1, 1, "row1col1");
    csv_set_field(buffer, 1, 2, "row1col2");

    TEST_ASSERT_EQUAL_INT(2, csv_get_height(buffer));
    TEST_ASSERT_EQUAL_INT(3, csv_get_width(buffer, 1));

    // Act: clear last row (row 1)
    int ret = csv_clear_row(buffer, 1);

    // Assert
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(2, csv_get_height(buffer)); // row count unchanged
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buffer, 1)); // width reduced to 1
    char dest[64] = {0};
    csv_get_field(dest, sizeof(dest), buffer, 1, 0);
    TEST_ASSERT_EQUAL_STRING("", dest); // cleared field is empty
}

static void test_csv_clear_row_middle_row_shrink_success(void) {
    // Setup: create buffer with 2 rows, each with multiple fields
    csv_set_field(buffer, 0, 0, "row0col0");
    csv_set_field(buffer, 0, 1, "row0col1");
    csv_set_field(buffer, 0, 2, "row0col2");
    csv_set_field(buffer, 1, 0, "row1col0");

    TEST_ASSERT_EQUAL_INT(2, csv_get_height(buffer));
    TEST_ASSERT_EQUAL_INT(3, csv_get_width(buffer, 0));

    // Act: clear middle row (row 0)
    int ret = csv_clear_row(buffer, 0);

    // Assert
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(2, csv_get_height(buffer));
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buffer, 0));
    char dest[64] = {0};
    csv_get_field(dest, sizeof(dest), buffer, 0, 0);
    TEST_ASSERT_EQUAL_STRING("", dest);
}

static void test_csv_clear_row_single_field_row(void) {
    // Setup: row with only one field
    csv_set_field(buffer, 0, 0, "onlyfield");

    TEST_ASSERT_EQUAL_INT(1, csv_get_height(buffer));
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buffer, 0));

    // Act: clear row
    int ret = csv_clear_row(buffer, 0);

    // Assert
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buffer, 0));
    char dest[64] = {0};
    csv_get_field(dest, sizeof(dest), buffer, 0, 0);
    TEST_ASSERT_EQUAL_STRING("", dest);
}

static void test_csv_clear_row_empty_row(void) {
    // Setup: add row (no fields initially)
    csv_set_field(buffer, 0, 0, "row0col0");
    csv_set_field(buffer, 0, 1, "row0col1");
    csv_clear_row(buffer, 0); // clear to get empty row (1 field, empty)
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buffer, 0));

    // Act: clear again (should still succeed)
    int ret = csv_clear_row(buffer, 0);

    // Assert
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buffer, 0));
    char dest[64] = {0};
    csv_get_field(dest, sizeof(dest), buffer, 0, 0);
    TEST_ASSERT_EQUAL_STRING("", dest);
}

static void test_csv_clear_row_invalid_row(void) {
    // Setup: buffer with 1 row
    csv_set_field(buffer, 0, 0, "row0col0");

    // Act & Assert: try to clear non-existent row
    int ret = run_with_segv_protection([]() {
        csv_clear_row(buffer, 99);
    });

    // We expect either a segv or a non-zero return (implementation-dependent)
    // Since the function spec says it returns 0 on success, 1 on memory failure,
    // and doesn't specify error for invalid row, but the implementation likely
    // accesses buffer->field[row] without bounds check, so segv is possible.
    // However, per requirements, we must handle segv gracefully and report failure.
    TEST_ASSERT_TRUE(ret == 0 || ret == 1); // if no segv, must be 0 or 1
    // But since we can't rely on bounds checking, and segv is possible,
    // we just ensure no crash escapes our handler
    TEST_ASSERT_FALSE(segv_occurred);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_csv_clear_row_last_row_shrink_success);
    RUN_TEST(test_csv_clear_row_middle_row_shrink_success);
    RUN_TEST(test_csv_clear_row_single_field_row);
    RUN_TEST(test_csv_clear_row_empty_row);
    RUN_TEST(test_csv_clear_row_invalid_row);

    return UNITY_END();
}