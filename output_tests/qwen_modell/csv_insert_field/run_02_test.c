#include "csv.c"
#include "unity.h"
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>

static CSV_BUFFER *buffer = NULL;
static jmp_buf jump_buffer;

static void segv_handler(int sig) {
    (void)sig;
    longjmp(jump_buffer, 1);
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

static void setup_row_with_fields(CSV_BUFFER *buf, size_t row, size_t count) {
    csv_clear_row(buf, row);
    for (size_t i = 0; i < count; i++) {
        csv_set_field(buf, row, i, "field");
    }
}

static void test_csv_insert_field_inserts_at_end_when_row_or_entry_exceeds_bounds(void) {
    // Setup: create a buffer with one row, width 2
    csv_clear_row(buffer, 0);
    csv_set_field(buffer, 0, 0, "a");
    csv_set_field(buffer, 0, 1, "b");

    // Act: insert at row=0, entry=2 (beyond current width)
    int result = csv_insert_field(buffer, 0, 2, "c");

    // Assert
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_INT(3, csv_get_width(buffer, 0));
    char dest[32] = {0};
    csv_get_field(dest, sizeof(dest), buffer, 0, 2);
    TEST_ASSERT_EQUAL_STRING("c", dest);
}

static void test_csv_insert_field_shifts_right_when_inserting_in_middle(void) {
    // Setup: row with 3 fields: ["a", "b", "c"]
    csv_clear_row(buffer, 0);
    csv_set_field(buffer, 0, 0, "a");
    csv_set_field(buffer, 0, 1, "b");
    csv_set_field(buffer, 0, 2, "c");

    // Act: insert "x" at entry=1
    int result = csv_insert_field(buffer, 0, 1, "x");

    // Assert
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_INT(4, csv_get_width(buffer, 0));
    char dest[32] = {0};
    csv_get_field(dest, sizeof(dest), buffer, 0, 0);
    TEST_ASSERT_EQUAL_STRING("a", dest);
    csv_get_field(dest, sizeof(dest), buffer, 0, 1);
    TEST_ASSERT_EQUAL_STRING("x", dest);
    csv_get_field(dest, sizeof(dest), buffer, 0, 2);
    TEST_ASSERT_EQUAL_STRING("b", dest);
    csv_get_field(dest, sizeof(dest), buffer, 0, 3);
    TEST_ASSERT_EQUAL_STRING("c", dest);
}

static void test_csv_insert_field_shifts_right_when_inserting_at_beginning(void) {
    // Setup: row with 2 fields: ["a", "b"]
    csv_clear_row(buffer, 0);
    csv_set_field(buffer, 0, 0, "a");
    csv_set_field(buffer, 0, 1, "b");

    // Act: insert "x" at entry=0
    int result = csv_insert_field(buffer, 0, 0, "x");

    // Assert
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_INT(3, csv_get_width(buffer, 0));
    char dest[32] = {0};
    csv_get_field(dest, sizeof(dest), buffer, 0, 0);
    TEST_ASSERT_EQUAL_STRING("x", dest);
    csv_get_field(dest, sizeof(dest), buffer, 0, 1);
    TEST_ASSERT_EQUAL_STRING("a", dest);
    csv_get_field(dest, sizeof(dest), buffer, 0, 2);
    TEST_ASSERT_EQUAL_STRING("b", dest);
}

static void test_csv_insert_field_creates_new_row_if_row_does_not_exist(void) {
    // Setup: buffer with 1 row (row 0)
    csv_clear_row(buffer, 0);
    csv_set_field(buffer, 0, 0, "existing");

    // Act: insert into row=1 (nonexistent), entry=0
    int result = csv_insert_field(buffer, 1, 0, "new");

    // Assert
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_INT(2, csv_get_height(buffer));
    char dest[32] = {0};
    csv_get_field(dest, sizeof(dest), buffer, 1, 0);
    TEST_ASSERT_EQUAL_STRING("new", dest);
}

static void test_csv_insert_field_handles_empty_string_field(void) {
    // Setup: row with 2 fields: ["a", "b"]
    csv_clear_row(buffer, 0);
    csv_set_field(buffer, 0, 0, "a");
    csv_set_field(buffer, 0, 1, "b");

    // Act: insert empty string at entry=1
    int result = csv_insert_field(buffer, 0, 1, "");

    // Assert
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_INT(3, csv_get_width(buffer, 0));
    char dest[32] = {0};
    csv_get_field(dest, sizeof(dest), buffer, 0, 1);
    TEST_ASSERT_EQUAL_STRING("", dest);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_csv_insert_field_inserts_at_end_when_row_or_entry_exceeds_bounds);
    RUN_TEST(test_csv_insert_field_shifts_right_when_inserting_in_middle);
    RUN_TEST(test_csv_insert_field_shifts_right_when_inserting_at_beginning);
    RUN_TEST(test_csv_insert_field_creates_new_row_if_row_does_not_exist);
    RUN_TEST(test_csv_insert_field_handles_empty_string_field);
    return UNITY_END();
}