#include "unity.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* Project headers */
#include "csv_buffer.h"  // Assumed to contain CSV_FIELD, CSV_BUFFER, and function declarations

/* Function declarations from project */
extern CSV_BUFFER *create_buffer(char field_delim, char text_delim);
extern void destroy_buffer(CSV_BUFFER *buffer);
extern int append_row(CSV_BUFFER *buffer);
extern int append_field(CSV_BUFFER *buffer, size_t row);
extern int set_field(CSV_FIELD *field, const char *text);
extern int remove_last_row(CSV_BUFFER *buffer);
extern void destroy_field(CSV_FIELD *field);

/* Static fixtures */
static CSV_BUFFER *test_buffer = NULL;

/* Helper functions */
static bool init_test_buffer_with_rows(size_t num_rows, size_t cols_per_row) {
    test_buffer = create_buffer(',', '"');
    if (!test_buffer) return false;

    for (size_t r = 0; r < num_rows; r++) {
        if (append_row(test_buffer) != 0) {
            destroy_buffer(test_buffer);
            test_buffer = NULL;
            return false;
        }
        for (size_t c = 0; c < cols_per_row; c++) {
            if (append_field(test_buffer, r) != 0) {
                destroy_buffer(test_buffer);
                test_buffer = NULL;
                return false;
            }
            char text[32];
            snprintf(text, sizeof(text), "R%zuC%zu", r, c);
            if (set_field(test_buffer->field[r][c], text) != 0) {
                destroy_buffer(test_buffer);
                test_buffer = NULL;
                return false;
            }
        }
    }
    return true;
}

/* setUp / tearDown */
void setUp(void) {
    test_buffer = NULL;
}

void tearDown(void) {
    if (test_buffer != NULL) {
        destroy_buffer(test_buffer);
        test_buffer = NULL;
    }
}

/* Test Cases */

void test_csv_clear_row_last_row_uses_remove_last_row_success(void) {
    /* Setup: 3 rows, 2 cols each */
    TEST_ASSERT_TRUE_MESSAGE(init_test_buffer_with_rows(3, 2), "Failed to create test buffer");

    /* Verify initial state */
    TEST_ASSERT_EQUAL_SIZE(3, test_buffer->rows);
    TEST_ASSERT_EQUAL_SIZE(2, test_buffer->width[2]);  /* last row has 2 fields */

    /* Act: clear last row (row 2) */
    int result = csv_clear_row(test_buffer, 2);

    /* Assert */
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_SIZE(2, test_buffer->rows);  /* one row removed */
    TEST_ASSERT_EQUAL_SIZE(2, test_buffer->width[1]);  /* previous row 1 is now row 1 */
}

void test_csv_clear_row_last_row_uses_remove_last_row_failure(void) {
    /* Setup: 1 row, 2 cols */
    TEST_ASSERT_TRUE_MESSAGE(init_test_buffer_with_rows(1, 2), "Failed to create test buffer");

    /* Force remove_last_row to fail by corrupting buffer (simulate internal error) */
    /* Since we cannot mock, we rely on real behavior: if remove_last_row fails, csv_clear_row returns 1 */
    /* But we cannot *guarantee* remove_last_row fails without mocks. So instead, test the *normal* path. */
    /* Instead: test non-last-row case to avoid relying on remove_last_row failure path. */
    /* This test is skipped per requirement: no mocks/fakes. So we skip this scenario. */
    TEST_IGNORE_MESSAGE("Cannot reliably test remove_last_row failure without mocks");
}

void test_csv_clear_row_non_last_row_reduces_width_to_one(void) {
    /* Setup: 3 rows, 4 cols each */
    TEST_ASSERT_TRUE_MESSAGE(init_test_buffer_with_rows(3, 4), "Failed to create test buffer");

    /* Verify initial state */
    TEST_ASSERT_EQUAL_SIZE(3, test_buffer->rows);
    TEST_ASSERT_EQUAL_SIZE(4, test_buffer->width[1]);  /* middle row has 4 fields */

    /* Act: clear middle row (row 1) */
    int result = csv_clear_row(test_buffer, 1);

    /* Assert */
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_SIZE(3, test_buffer->rows);  /* row count unchanged */
    TEST_ASSERT_EQUAL_SIZE(1, test_buffer->width[1]);  /* row 1 now has 1 field */
    TEST_ASSERT_EQUAL_STRING("", test_buffer->field[1][0]->text);  /* field is empty string */
}

void test_csv_clear_row_non_last_row_frees_extra_fields(void) {
    /* Setup: 2 rows, 3 cols each */
    TEST_ASSERT_TRUE_MESSAGE(init_test_buffer_with_rows(2, 3), "Failed to create test buffer");

    /* Verify initial state */
    TEST_ASSERT_EQUAL_SIZE(2, test_buffer->rows);
    TEST_ASSERT_EQUAL_SIZE(3, test_buffer->width[0]);

    /* Act: clear first row (row 0) */
    int result = csv_clear_row(test_buffer, 0);

    /* Assert */
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_SIZE(2, test_buffer->rows);
    TEST_ASSERT_EQUAL_SIZE(1, test_buffer->width[0]);  /* reduced to 1 */
    TEST_ASSERT_NOT_NULL(test_buffer->field[0]);       /* pointer still valid */
    TEST_ASSERT_NOT_NULL(test_buffer->field[0][0]);    /* first field still valid */
    TEST_ASSERT_NULL(test_buffer->field[0][1]);        /* no second field */
    TEST_ASSERT_EQUAL_STRING("", test_buffer->field[0][0]->text);
}

void test_csv_clear_row_realloc_failure_fills_back_fields(void) {
    /* Setup: 2 rows, 2 cols each */
    TEST_ASSERT_TRUE_MESSAGE(init_test_buffer_with_rows(2, 2), "Failed to create test buffer");

    /* Verify initial state */
    TEST_ASSERT_EQUAL_SIZE(2, test_buffer->rows);
    TEST_ASSERT_EQUAL_SIZE(2, test_buffer->width[0]);

    /* Note: We cannot *force* realloc to fail portably without mocks/stubs.
       This test is included for completeness but will behave as normal realloc.
       In real testing, one might use a custom allocator or mtrace, but per constraints,
       we must use real functions only. So we test the success path only. */
    TEST_IGNORE_MESSAGE("Cannot reliably test realloc failure without custom allocator or mocks");
}

/* main */
int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_csv_clear_row_last_row_uses_remove_last_row_success);
    RUN_TEST(test_csv_clear_row_non_last_row_reduces_width_to_one);
    RUN_TEST(test_csv_clear_row_non_last_row_frees_extra_fields);
    /* Skipped tests due to constraints */
    // RUN_TEST(test_csv_clear_row_last_row_uses_remove_last_row_failure);
    // RUN_TEST(test_csv_clear_row_realloc_failure_fills_back_fields);

    return UNITY_END();
}