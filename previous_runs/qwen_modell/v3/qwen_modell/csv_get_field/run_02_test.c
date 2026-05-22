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

static void run_test_with_segv_protection(void (*test_func)(void)) {
    void (*old_handler)(int) = signal(SIGSEGV, segv_handler);
    if (setjmp(segv_jmp) == 0) {
        test_func();
    } else {
        TEST_ASSERT_FALSE_MESSAGE(1, "Segmentation fault detected");
    }
    signal(SIGSEGV, old_handler);
}

static CSV_BUFFER *csv_buf = NULL;

void setUp(void) {
    csv_buf = NULL;
}

void tearDown(void) {
    if (csv_buf != NULL) {
        for (size_t i = 0; i < csv_buf->rows; i++) {
            for (size_t j = 0; j < csv_buf->width[i]; j++) {
                if (csv_buf->field[i][j] != NULL) {
                    free(csv_buf->field[i][j]->text);
                    free(csv_buf->field[i][j]);
                }
            }
            free(csv_buf->field[i]);
        }
        free(csv_buf->field);
        free(csv_buf->width);
        free(csv_buf);
    }
    csv_buf = NULL;
}

static void create_csv_buffer(size_t rows, size_t *widths, char **texts) {
    csv_buf = (CSV_BUFFER *)malloc(sizeof(CSV_BUFFER));
    csv_buf->rows = rows;
    csv_buf->width = (size_t *)malloc(rows * sizeof(size_t));
    csv_buf->field = (CSV_FIELD ***)malloc(rows * sizeof(CSV_FIELD **));

    for (size_t i = 0; i < rows; i++) {
        csv_buf->width[i] = widths[i];
        csv_buf->field[i] = (CSV_FIELD **)malloc(widths[i] * sizeof(CSV_FIELD *));
        for (size_t j = 0; j < widths[i]; j++) {
            csv_buf->field[i][j] = (CSV_FIELD *)malloc(sizeof(CSV_FIELD));
            csv_buf->field[i][j]->text = strdup(texts[i * 10 + j]);
            csv_buf->field[i][j]->length = strlen(texts[i * 10 + j]);
        }
    }
}

static void test_csv_get_field_dest_len_zero_returns_3(void) {
    char dest[10];
    int ret = csv_get_field(dest, 0, csv_buf, 0, 0);
    TEST_ASSERT_EQUAL_INT(3, ret);
}

static void test_csv_get_field_negative_row_returns_2(void) {
    char dest[10];
    int ret = csv_get_field(dest, sizeof(dest), csv_buf, (size_t)-1, 0);
    TEST_ASSERT_EQUAL_INT(2, ret);
    TEST_ASSERT_EQUAL_INT('\0', dest[0]);
}

static void test_csv_get_field_row_out_of_bounds_returns_2(void) {
    char dest[10];
    int ret = csv_get_field(dest, sizeof(dest), csv_buf, 100, 0);
    TEST_ASSERT_EQUAL_INT(2, ret);
    TEST_ASSERT_EQUAL_INT('\0', dest[0]);
}

static void test_csv_get_field_negative_entry_returns_2(void) {
    char dest[10];
    int ret = csv_get_field(dest, sizeof(dest), csv_buf, 0, (size_t)-1);
    TEST_ASSERT_EQUAL_INT(2, ret);
    TEST_ASSERT_EQUAL_INT('\0', dest[0]);
}

static void test_csv_get_field_entry_out_of_bounds_returns_2(void) {
    char dest[10];
    int ret = csv_get_field(dest, sizeof(dest), csv_buf, 0, 100);
    TEST_ASSERT_EQUAL_INT(2, ret);
    TEST_ASSERT_EQUAL_INT('\0', dest[0]);
}

static void test_csv_get_field_valid_entry_exact_fit_returns_0(void) {
    char texts[1][1] = {"a"};
    size_t widths[1] = {1};
    create_csv_buffer(1, widths, texts);

    char dest[2];
    int ret = csv_get_field(dest, sizeof(dest), csv_buf, 0, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_STRING_LEN("a", dest, 1);
}

static void test_csv_get_field_valid_entry_truncated_returns_1(void) {
    char texts[1][10] = {"hello"};
    size_t widths[1] = {1};
    create_csv_buffer(1, widths, texts);

    char dest[3];
    int ret = csv_get_field(dest, sizeof(dest), csv_buf, 0, 0);
    TEST_ASSERT_EQUAL_INT(1, ret);
    TEST_ASSERT_EQUAL_STRING_LEN("he", dest, 2);
}

static void test_csv_get_field_valid_empty_entry_returns_2(void) {
    char texts[1][1] = {""};
    size_t widths[1] = {1};
    create_csv_buffer(1, widths, texts);

    char dest[10];
    int ret = csv_get_field(dest, sizeof(dest), csv_buf, 0, 0);
    TEST_ASSERT_EQUAL_INT(2, ret);
    TEST_ASSERT_EQUAL_STRING("", dest);
}

static void test_csv_get_field_dest_buffer_cleared_on_error(void) {
    char dest[10] = "XXXXXXXXX";
    int ret = csv_get_field(dest, sizeof(dest), csv_buf, 100, 0);
    TEST_ASSERT_EQUAL_INT(2, ret);
    TEST_ASSERT_EQUAL_INT('\0', dest[0]);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST_CASE_SEGV("csv_get_field: dest_len zero returns 3", test_csv_get_field_dest_len_zero_returns_3);
    RUN_TEST_CASE_SEGV("csv_get_field: negative row returns 2", test_csv_get_field_negative_row_returns_2);
    RUN_TEST_CASE_SEGV("csv_get_field: row out of bounds returns 2", test_csv_get_field_row_out_of_bounds_returns_2);
    RUN_TEST_CASE_SEGV("csv_get_field: negative entry returns 2", test_csv_get_field_negative_entry_returns_2);
    RUN_TEST_CASE_SEGV("csv_get_field: entry out of bounds returns 2", test_csv_get_field_entry_out_of_bounds_returns_2);
    RUN_TEST_CASE_SEGV("csv_get_field: valid entry exact fit returns 0", test_csv_get_field_valid_entry_exact_fit_returns_0);
    RUN_TEST_CASE_SEGV("csv_get_field: valid entry truncated returns 1", test_csv_get_field_valid_entry_truncated_returns_1);
    RUN_TEST_CASE_SEGV("csv_get_field: valid empty entry returns 2", test_csv_get_field_valid_empty_entry_returns_2);
    RUN_TEST_CASE_SEGV("csv_get_field: dest buffer cleared on error", test_csv_get_field_dest_buffer_cleared_on_error);

    return UNITY_END();
}