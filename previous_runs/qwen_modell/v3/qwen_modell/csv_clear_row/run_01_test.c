#include "unity.h"
#include "csv.h"
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>
#include <stdio.h>

static jmp_buf jump_buffer;
static void segv_handler(int sig) {
    (void)sig;
    longjmp(jump_buffer, 1);
}

void setUp(void) {
    signal(SIGSEGV, segv_handler);
}

void tearDown(void) {
    signal(SIGSEGV, SIG_DFL);
}

static int safe_csv_clear_row(CSV_BUFFER *buffer, size_t row) {
    int result;
    if (setjmp(jump_buffer) == 0) {
        result = csv_clear_row(buffer, row);
    } else {
        // Segfault occurred
        return -1;
    }
    return result;
}

void test_csv_clear_row_last_row_delegates_to_remove_last_row_success(void) {
    CSV_BUFFER *buf = malloc(sizeof(CSV_BUFFER));
    TEST_ASSERT_NOT_NULL(buf);

    buf->rows = 3;
    buf->width = malloc(3 * sizeof(size_t));
    TEST_ASSERT_NOT_NULL(buf->width);
    buf->field = malloc(3 * sizeof(CSV_FIELD**));
    TEST_ASSERT_NOT_NULL(buf->field);

    buf->width[2] = 2;
    buf->field[2] = malloc(2 * sizeof(CSV_FIELD*));
    TEST_ASSERT_NOT_NULL(buf->field[2]);
    buf->field[2][0] = malloc(sizeof(CSV_FIELD));
    buf->field[2][1] = malloc(sizeof(CSV_FIELD));
    TEST_ASSERT_NOT_NULL(buf->field[2][0]);
    TEST_ASSERT_NOT_NULL(buf->field[2][1]);

    // Mock remove_last_row to succeed
    extern int (*remove_last_row_ptr)(CSV_BUFFER*);
    static int mock_remove_last_row(CSV_BUFFER *b) { (void)b; return 0; }
    remove_last_row_ptr = mock_remove_last_row;

    int result = safe_csv_clear_row(buf, 2);
    TEST_ASSERT_EQUAL_INT(0, result);

    free(buf->field[2][0]);
    free(buf->field[2][1]);
    free(buf->field[2]);
    free(buf->width);
    free(buf);
}

void test_csv_clear_row_last_row_delegates_to_remove_last_row_failure(void) {
    CSV_BUFFER *buf = malloc(sizeof(CSV_BUFFER));
    TEST_ASSERT_NOT_NULL(buf);

    buf->rows = 3;
    buf->width = malloc(3 * sizeof(size_t));
    TEST_ASSERT_NOT_NULL(buf->width);
    buf->field = malloc(3 * sizeof(CSV_FIELD**));
    TEST_ASSERT_NOT_NULL(buf->field);

    buf->width[2] = 2;
    buf->field[2] = malloc(2 * sizeof(CSV_FIELD*));
    TEST_ASSERT_NOT_NULL(buf->field[2]);
    buf->field[2][0] = malloc(sizeof(CSV_FIELD));
    buf->field[2][1] = malloc(sizeof(CSV_FIELD));
    TEST_ASSERT_NOT_NULL(buf->field[2][0]);
    TEST_ASSERT_NOT_NULL(buf->field[2][1]);

    // Mock remove_last_row to fail
    extern int (*remove_last_row_ptr)(CSV_BUFFER*);
    static int mock_remove_last_row(CSV_BUFFER *b) { (void)b; return 1; }
    remove_last_row_ptr = mock_remove_last_row;

    int result = safe_csv_clear_row(buf, 2);
    TEST_ASSERT_EQUAL_INT(1, result);

    free(buf->field[2][0]);
    free(buf->field[2][1]);
    free(buf->field[2]);
    free(buf->width);
    free(buf);
}

void test_csv_clear_row_non_last_row_shrink_succeeds(void) {
    CSV_BUFFER *buf = malloc(sizeof(CSV_BUFFER));
    TEST_ASSERT_NOT_NULL(buf);

    buf->rows = 3;
    buf->width = malloc(3 * sizeof(size_t));
    TEST_ASSERT_NOT_NULL(buf->width);
    buf->field = malloc(3 * sizeof(CSV_FIELD**));
    TEST_ASSERT_NOT_NULL(buf->field);

    buf->width[1] = 3;
    buf->field[1] = malloc(3 * sizeof(CSV_FIELD*));
    TEST_ASSERT_NOT_NULL(buf->field[1]);
    buf->field[1][0] = malloc(sizeof(CSV_FIELD));
    buf->field[1][1] = malloc(sizeof(CSV_FIELD));
    buf->field[1][2] = malloc(sizeof(CSV_FIELD));
    TEST_ASSERT_NOT_NULL(buf->field[1][0]);
    TEST_ASSERT_NOT_NULL(buf->field[1][1]);
    TEST_ASSERT_NOT_NULL(buf->field[1][2]);

    // Ensure realloc succeeds
    extern void *(*realloc_ptr)(void *, size_t);
    static void *mock_realloc(void *ptr, size_t size) {
        (void)size;
        return ptr; // shrink to same pointer
    }
    realloc_ptr = mock_realloc;

    int result = safe_csv_clear_row(buf, 1);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_INT(1, buf->width[1]);
    TEST_ASSERT_EQUAL_INT(1, buf->field[1] ? 1 : 0); // only one pointer

    free(buf->field[1][0]);
    free(buf->field[1]);
    free(buf->width);
    free(buf->field);
    free(buf);
}

void test_csv_clear_row_non_last_row_shrink_fails_recreates_fields(void) {
    CSV_BUFFER *buf = malloc(sizeof(CSV_BUFFER));
    TEST_ASSERT_NOT_NULL(buf);

    buf->rows = 3;
    buf->width = malloc(3 * sizeof(size_t));
    TEST_ASSERT_NOT_NULL(buf->width);
    buf->field = malloc(3 * sizeof(CSV_FIELD**));
    TEST_ASSERT_NOT_NULL(buf->field);

    buf->width[1] = 3;
    buf->field[1] = malloc(3 * sizeof(CSV_FIELD*));
    TEST_ASSERT_NOT_NULL(buf->field[1]);
    buf->field[1][0] = malloc(sizeof(CSV_FIELD));
    buf->field[1][1] = malloc(sizeof(CSV_FIELD));
    buf->field[1][2] = malloc(sizeof(CSV_FIELD));
    TEST_ASSERT_NOT_NULL(buf->field[1][0]);
    TEST_ASSERT_NOT_NULL(buf->field[1][1]);
    TEST_ASSERT_NOT_NULL(buf->field[1][2]);

    // Mock realloc to fail
    extern void *(*realloc_ptr)(void *, size_t);
    static void *mock_realloc(void *ptr, size_t size) {
        (void)ptr; (void)size;
        return NULL;
    }
    realloc_ptr = mock_realloc;

    int result = safe_csv_clear_row(buf, 1);
    TEST_ASSERT_EQUAL_INT(1, result);
    TEST_ASSERT_EQUAL_INT(1, buf->width[1]); // should be reset to 1

    free(buf->field[1][0]);
    free(buf->field[1]);
    free(buf->width);
    free(buf->field);
    free(buf);
}

void test_csv_clear_row_segfault_on_null_buffer(void) {
    int result = safe_csv_clear_row(NULL, 0);
    TEST_ASSERT_EQUAL_INT(-1, result);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_csv_clear_row_last_row_delegates_to_remove_last_row_success);
    RUN_TEST(test_csv_clear_row_last_row_delegates_to_remove_last_row_failure);
    RUN_TEST(test_csv_clear_row_non_last_row_shrink_succeeds);
    RUN_TEST(test_csv_clear_row_non_last_row_shrink_fails_recreates_fields);
    RUN_TEST(test_csv_clear_row_segfault_on_null_buffer);
    return UNITY_END();
}