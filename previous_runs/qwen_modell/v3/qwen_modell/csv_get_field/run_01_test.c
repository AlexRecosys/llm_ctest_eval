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

static CSV_BUFFER *test_buf = NULL;
static char *test_dest = NULL;
static size_t test_dest_len = 0;

void setUp(void) {
    signal(SIGSEGV, segv_handler);
    test_dest = malloc(256);
    test_dest_len = 256;
    memset(test_dest, 'X', 256);
    test_dest[255] = '\0';
}

void tearDown(void) {
    if (test_buf != NULL) {
        for (size_t r = 0; r < test_buf->rows; r++) {
            for (size_t c = 0; c < test_buf->width[r]; c++) {
                if (test_buf->field[r][c] != NULL) {
                    free(test_buf->field[r][c]->text);
                    free(test_buf->field[r][c]);
                }
            }
            free(test_buf->field[r]);
        }
        free(test_buf->field);
        free(test_buf->width);
        free(test_buf);
        test_buf = NULL;
    }
    free(test_dest);
    test_dest = NULL;
}

static CSV_BUFFER *create_test_buffer(size_t rows, size_t cols, const char *data[]) {
    CSV_BUFFER *buf = calloc(1, sizeof(CSV_BUFFER));
    buf->rows = rows;
    buf->width = calloc(rows, sizeof(size_t));
    buf->field = calloc(rows, sizeof(CSV_FIELD **));
    
    for (size_t r = 0; r < rows; r++) {
        buf->width[r] = cols;
        buf->field[r] = calloc(cols, sizeof(CSV_FIELD *));
        for (size_t c = 0; c < cols; c++) {
            if (data && data[r * cols + c]) {
                buf->field[r][c] = malloc(sizeof(CSV_FIELD));
                buf->field[r][c]->text = strdup(data[r * cols + c]);
                buf->field[r][c]->length = strlen(data[r * cols + c]);
            } else {
                buf->field[r][c] = NULL;
            }
        }
    }
    return buf;
}

static int run_with_segv_protection(void (*func)(void)) {
    int result = setjmp(jump_buffer);
    if (result == 0) {
        func();
        return 0; // no segfault
    } else {
        return 1; // segfault occurred
    }
}

static void test_dest_write(void) {
    csv_get_field(test_dest, test_dest_len, test_buf, 0, 0);
}

void test_csv_get_field_dest_len_zero_returns_3(void) {
    test_buf = create_test_buffer(1, 1, (const char *[]){"hello"});
    int ret = csv_get_field(test_dest, 0, test_buf, 0, 0);
    TEST_ASSERT_EQUAL_INT(3, ret);
    TEST_ASSERT_EQUAL_INT('\0', test_dest[0]);
}

void test_csv_get_field_negative_row_returns_2(void) {
    test_buf = create_test_buffer(1, 1, (const char *[]){"hello"});
    int ret = csv_get_field(test_dest, test_dest_len, test_buf, (size_t)-1, 0);
    TEST_ASSERT_EQUAL_INT(2, ret);
    TEST_ASSERT_EQUAL_INT('\0', test_dest[0]);
}

void test_csv_get_field_row_out_of_bounds_returns_2(void) {
    test_buf = create_test_buffer(1, 1, (const char *[]){"hello"});
    int ret = csv_get_field(test_dest, test_dest_len, test_buf, 1, 0);
    TEST_ASSERT_EQUAL_INT(2, ret);
    TEST_ASSERT_EQUAL_INT('\0', test_dest[0]);
}

void test_csv_get_field_negative_entry_returns_2(void) {
    test_buf = create_test_buffer(1, 1, (const char *[]){"hello"});
    int ret = csv_get_field(test_dest, test_dest_len, test_buf, 0, (size_t)-1);
    TEST_ASSERT_EQUAL_INT(2, ret);
    TEST_ASSERT_EQUAL_INT('\0', test_dest[0]);
}

void test_csv_get_field_entry_out_of_bounds_returns_2(void) {
    test_buf = create_test_buffer(1, 1, (const char *[]){"hello"});
    int ret = csv_get_field(test_dest, test_dest_len, test_buf, 0, 1);
    TEST_ASSERT_EQUAL_INT(2, ret);
    TEST_ASSERT_EQUAL_INT('\0', test_dest[0]);
}

void test_csv_get_field_valid_nonempty_entry_returns_0(void) {
    test_buf = create_test_buffer(1, 1, (const char *[]){"hello"});
    int ret = csv_get_field(test_dest, test_dest_len, test_buf, 0, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING("hello", test_dest);
}

void test_csv_get_field_valid_empty_entry_returns_2(void) {
    test_buf = create_test_buffer(1, 1, (const char *[]){""});
    int ret = csv_get_field(test_dest, test_dest_len, test_buf, 0, 0);
    TEST_ASSERT_EQUAL_INT(2, ret);
    TEST_ASSERT_EQUAL_STRING("", test_dest);
}

void test_csv_get_field_truncation_returns_1(void) {
    test_buf = create_test_buffer(1, 1, (const char *[]){"hello"});
    test_dest_len = 3;
    int ret = csv_get_field(test_dest, test_dest_len, test_buf, 0, 0);
    TEST_ASSERT_EQUAL_INT(1, ret);
    TEST_ASSERT_EQUAL_STRING_LEN("hel", test_dest, 3);
    TEST_ASSERT_EQUAL_INT('\0', test_dest[3]);
}

void test_csv_get_field_dest_buffer_null_does_not_crash(void) {
    test_buf = create_test_buffer(1, 1, (const char *[]){"hello"});
    int ret = csv_get_field(NULL, test_dest_len, test_buf, 0, 0);
    TEST_ASSERT_EQUAL_INT(2, ret);
}

void test_csv_get_field_src_null_does_not_crash(void) {
    int ret = csv_get_field(test_dest, test_dest_len, NULL, 0, 0);
    TEST_ASSERT_EQUAL_INT(2, ret);
    TEST_ASSERT_EQUAL_INT('\0', test_dest[0]);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_csv_get_field_dest_len_zero_returns_3);
    RUN_TEST(test_csv_get_field_negative_row_returns_2);
    RUN_TEST(test_csv_get_field_row_out_of_bounds_returns_2);
    RUN_TEST(test_csv_get_field_negative_entry_returns_2);
    RUN_TEST(test_csv_get_field_entry_out_of_bounds_returns_2);
    RUN_TEST(test_csv_get_field_valid_nonempty_entry_returns_0);
    RUN_TEST(test_csv_get_field_valid_empty_entry_returns_2);
    RUN_TEST(test_csv_get_field_truncation_returns_1);
    RUN_TEST(test_csv_get_field_dest_buffer_null_does_not_crash);
    RUN_TEST(test_csv_get_field_src_null_does_not_crash);
    return UNITY_END();
}