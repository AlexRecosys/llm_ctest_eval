#include "unity.h"
#include "csv.h"
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>
#include <stdio.h>

static jmp_buf segv_jmp;
static void segv_handler(int sig) {
    (void)sig;
    longjmp(segv_jmp, 1);
}

static void setup_signal_handler(void) {
    struct sigaction sa;
    sa.sa_handler = segv_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGSEGV, &sa, NULL);
}

static void teardown_signal_handler(void) {
    struct sigaction sa;
    sa.sa_handler = SIG_DFL;
    sigaction(SIGSEGV, &sa, NULL);
}

static CSV_BUFFER *test_buf = NULL;

void setUp(void) {
    setup_signal_handler();
    test_buf = NULL;
}

void tearDown(void) {
    teardown_signal_handler();
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
}

static CSV_BUFFER *create_test_buffer(size_t rows, size_t cols, const char *const *const *data) {
    CSV_BUFFER *buf = calloc(1, sizeof(CSV_BUFFER));
    TEST_ASSERT_NOT_NULL(buf);

    buf->rows = rows;
    buf->width = calloc(rows, sizeof(size_t));
    TEST_ASSERT_NOT_NULL(buf->width);

    buf->field = calloc(rows, sizeof(CSV_FIELD **));
    TEST_ASSERT_NOT_NULL(buf->field);

    for (size_t r = 0; r < rows; r++) {
        buf->width[r] = cols;
        buf->field[r] = calloc(cols, sizeof(CSV_FIELD *));
        TEST_ASSERT_NOT_NULL(buf->field[r]);

        for (size_t c = 0; c < cols; c++) {
            if (data && data[r] && data[r][c]) {
                buf->field[r][c] = calloc(1, sizeof(CSV_FIELD));
                TEST_ASSERT_NOT_NULL(buf->field[r][c]);
                buf->field[r][c]->text = strdup(data[r][c]);
                TEST_ASSERT_NOT_NULL(buf->field[r][c]->text);
                buf->field[r][c]->length = strlen(data[r][c]);
            } else {
                buf->field[r][c] = NULL;
            }
        }
    }

    return buf;
}

static int run_with_segv_protection(void (*func)(void)) {
    int ret = setjmp(segv_jmp);
    if (ret == 0) {
        func();
        return 0;
    } else {
        return 1;
    }
}

static void test_dest_null(void) {
    char dest[10];
    CSV_BUFFER buf = {0};
    int result = csv_get_field(NULL, 10, &buf, 0, 0);
    TEST_ASSERT_EQUAL_INT(3, result);
}

static void test_dest_len_zero(void) {
    char dest[10];
    CSV_BUFFER buf = {0};
    int result = csv_get_field(dest, 0, &buf, 0, 0);
    TEST_ASSERT_EQUAL_INT(3, result);
}

static void test_row_negative(void) {
    char dest[10];
    CSV_BUFFER buf = {0};
    int result = csv_get_field(dest, 10, &buf, -1, 0);
    TEST_ASSERT_EQUAL_INT(2, result);
}

static void test_entry_negative(void) {
    char dest[10];
    CSV_BUFFER buf = {0};
    buf.rows = 1;
    buf.width = calloc(1, sizeof(size_t));
    buf.width[0] = 1;
    buf.field = calloc(1, sizeof(CSV_FIELD **));
    buf.field[0] = calloc(1, sizeof(CSV_FIELD *));
    int result = csv_get_field(dest, 10, &buf, 0, -1);
    TEST_ASSERT_EQUAL_INT(2, result);
    free(buf.field[0]);
    free(buf.field);
    free(buf.width);
}

static void test_row_out_of_bounds(void) {
    char dest[10];
    CSV_BUFFER buf = {0};
    buf.rows = 1;
    buf.width = calloc(1, sizeof(size_t));
    buf.width[0] = 1;
    buf.field = calloc(1, sizeof(CSV_FIELD **));
    buf.field[0] = calloc(1, sizeof(CSV_FIELD *));
    int result = csv_get_field(dest, 10, &buf, 1, 0);
    TEST_ASSERT_EQUAL_INT(2, result);
    free(buf.field[0]);
    free(buf.field);
    free(buf.width);
}

static void test_entry_out_of_bounds(void) {
    char dest[10];
    CSV_BUFFER buf = {0};
    buf.rows = 1;
    buf.width = calloc(1, sizeof(size_t));
    buf.width[0] = 1;
    buf.field = calloc(1, sizeof(CSV_FIELD **));
    buf.field[0] = calloc(1, sizeof(CSV_FIELD *));
    int result = csv_get_field(dest, 10, &buf, 0, 1);
    TEST_ASSERT_EQUAL_INT(2, result);
    free(buf.field[0]);
    free(buf.field);
    free(buf.width);
}

static void test_empty_dest_buffer_cleared_on_error(void) {
    char dest[10] = "XXXXXXXXX";
    CSV_BUFFER buf = {0};
    int result = csv_get_field(dest, 10, &buf, -1, 0);
    TEST_ASSERT_EQUAL_INT(2, result);
    TEST_ASSERT_EQUAL_INT('\0', dest[0]);
}

static void test_valid_entry_returns_zero(void) {
    const char *data[1][1] = {{"hello"}};
    test_buf = create_test_buffer(1, 1, (const char *const *const *)data);
    char dest[10];
    int result = csv_get_field(dest, 10, test_buf, 0, 0);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_STRING_LEN("hello", dest, 5);
}

static void test_valid_entry_truncates_if_dest_too_small(void) {
    const char *data[1][1] = {{"hello"}};
    test_buf = create_test_buffer(1, 1, (const char *const *const *)data);
    char dest[4];
    int result = csv_get_field(dest, 4, test_buf, 0, 0);
    TEST_ASSERT_EQUAL_INT(1, result);
    TEST_ASSERT_EQUAL_STRING_LEN("hel", dest, 3);
    TEST_ASSERT_EQUAL_INT('\0', dest[3]);
}

static void test_empty_field_returns_two(void) {
    const char *data[1][1] = {{""}};
    test_buf = create_test_buffer(1, 1, (const char *const *const *)data);
    char dest[10];
    int result = csv_get_field(dest, 10, test_buf, 0, 0);
    TEST_ASSERT_EQUAL_INT(2, result);
    TEST_ASSERT_EQUAL_STRING("", dest);
}

static void test_null_field_pointer(void) {
    test_buf = create_test_buffer(1, 1, NULL);
    char dest[10];
    int result = csv_get_field(dest, 10, test_buf, 0, 0);
    TEST_ASSERT_EQUAL_INT(2, result);
    TEST_ASSERT_EQUAL_INT('\0', dest[0]);
}

static void test_dest_buffer_null_with_nonzero_len(void) {
    CSV_BUFFER buf = {0};
    buf.rows = 1;
    buf.width = calloc(1, sizeof(size_t));
    buf.width[0] = 1;
    buf.field = calloc(1, sizeof(CSV_FIELD **));
    buf.field[0] = calloc(1, sizeof(CSV_FIELD *));
    buf.field[0][0] = calloc(1, sizeof(CSV_FIELD));
    buf.field[0][0]->text = strdup("test");
    buf.field[0][0]->length = 4;

    int result = csv_get_field(NULL, 10, &buf, 0, 0);
    TEST_ASSERT_EQUAL_INT(3, result);

    free(buf.field[0][0]->text);
    free(buf.field[0][0]);
    free(buf.field[0]);
    free(buf.field);
    free(buf.width);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_dest_null);
    RUN_TEST(test_dest_len_zero);
    RUN_TEST(test_row_negative);
    RUN_TEST(test_entry_negative);
    RUN_TEST(test_row_out_of_bounds);
    RUN_TEST(test_entry_out_of_bounds);
    RUN_TEST(test_empty_dest_buffer_cleared_on_error);
    RUN_TEST(test_valid_entry_returns_zero);
    RUN_TEST(test_valid_entry_truncates_if_dest_too_small);
    RUN_TEST(test_empty_field_returns_two);
    RUN_TEST(test_null_field_pointer);
    RUN_TEST(test_dest_buffer_null_with_nonzero_len);

    return UNITY_END();
}