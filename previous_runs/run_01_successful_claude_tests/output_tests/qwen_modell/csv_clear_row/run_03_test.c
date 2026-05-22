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
static bool init_test_buffer_with_rows(size_t rows, size_t cols_per_row) {
    test_buffer = create_buffer(',', '"');
    if (test_buffer == NULL) return false;

    for (size_t r = 0; r < rows; r++) {
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
            char buf[32];
            snprintf(buf, sizeof(buf), "R%zuC%zu", r, c);
            if (set_field(test_buffer->field[r][c], buf) != 0) {
                destroy_buffer(test_buffer);
                test_buffer = NULL;
                return false;
            }
        }
    }
    return true;
}

static bool row_has_expected_width(size_t row, size_t expected_width) {
    if (test_buffer == NULL) return false;
    return test_buffer->width[row] == expected_width;
}

static bool row_field_is_empty(size_t row, size_t col) {
    if (test_buffer == NULL || test_buffer->field == NULL ||
        test_buffer->field[row] == NULL || test_buffer->field[row][col] == NULL) {
        return false;
    }
    return test_buffer->field[row][col]->text != NULL &&
           test_buffer->field[row][col]->text[0] == '\0' &&
           test_buffer->field[row][col]->length == 0;
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

void test_csv_clear_row_last_row_delegates_to_remove_last_row_success(void) {
    /* Setup: 2 rows, 3 cols each */
    TEST_ASSERT_TRUE_MESSAGE(init_test_buffer_with_rows(2, 3),
                             "Failed to initialize test buffer");

    /* Act: clear last row (row 1) */
    int result = csv_clear_row(test_buffer, 1);

    /* Assert */
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "csv_clear_row should return 0 on success");
    TEST_ASSERT_EQUAL_SIZE_MESSAGE(1, test_buffer->rows, "Row count should decrease by 1");
    TEST_ASSERT_EQUAL_SIZE_MESSAGE(1, test_buffer->width[0], "Remaining row should retain its width");
    TEST_ASSERT_NOT_NULL_MESSAGE(test_buffer->field[0], "First row should still exist");
}

void test_csv_clear_row_last_row_delegates_to_remove_last_row_failure(void) {
    /* Setup: 1 row, 3 cols */
    TEST_ASSERT_TRUE_MESSAGE(init_test_buffer_with_rows(1, 3),
                             "Failed to initialize test buffer");

    /* Force remove_last_row to fail by corrupting buffer (simulate internal error) */
    /* Note: In real code, we'd need to mock or inject failure; but per constraints, no mocks */
    /* Instead, we test the *normal* path where remove_last_row succeeds */
    /* This test is skipped because we cannot force remove_last_row to fail without mocks */
    /* So we skip this test case and use a different scenario */
    TEST_IGNORE_MESSAGE("Cannot test remove_last_row failure without mocks");
}

void test_csv_clear_row_middle_row_reduces_to_one_field(void) {
    /* Setup: 3 rows, 4 cols each */
    TEST_ASSERT_TRUE_MESSAGE(init_test_buffer_with_rows(3, 4),
                             "Failed to initialize test buffer");

    size_t original_rows = test_buffer->rows;
    size_t original_width = test_buffer->width[1];

    /* Act: clear middle row (row 1) */
    int result = csv_clear_row(test_buffer, 1);

    /* Assert */
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "csv_clear_row should return 0 on success");
    TEST_ASSERT_EQUAL_SIZE_MESSAGE(original_rows, test_buffer->rows,
                                   "Row count should remain unchanged");
    TEST_ASSERT_EQUAL_SIZE_MESSAGE(1, test_buffer->width[1],
                                   "Target row width should be reduced to 1");
    TEST_ASSERT_TRUE_MESSAGE(row_field_is_empty(1, 0),
                             "Remaining field in target row should be empty");
    TEST_ASSERT_NULL_MESSAGE(test_buffer->field[1][1],
                             "Second field in target row should be NULL after realloc");
}

void test_csv_clear_row_middle_row_realloc_fails_restores_fields(void) {
    /* Setup: 2 rows, 3 cols each */
    TEST_ASSERT_TRUE_MESSAGE(init_test_buffer_with_rows(2, 3),
                             "Failed to initialize test buffer");

    size_t original_width = test_buffer->width[0];

    /* Temporarily override realloc to force failure */
    /* Since we cannot use mocks, we simulate by corrupting the buffer's field pointer */
    /* This is unsafe but acceptable in test code for robustness testing */
    /* Instead, we rely on the fact that realloc(NULL, size) always succeeds, so we cannot force failure */
    /* Therefore, this test is skipped per constraints */
    TEST_IGNORE_MESSAGE("Cannot force realloc failure without mocks/stubs");
}

void test_csv_clear_row_first_row_reduces_to_one_field(void) {
    /* Setup: 2 rows, 5 cols each */
    TEST_ASSERT_TRUE_MESSAGE(init_test_buffer_with_rows(2, 5),
                             "Failed to initialize test buffer");

    size_t original_rows = test_buffer->rows;
    size_t original_width = test_buffer->width[0];

    /* Act: clear first row (row 0) */
    int result = csv_clear_row(test_buffer, 0);

    /* Assert */
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "csv_clear_row should return 0 on success");
    TEST_ASSERT_EQUAL_SIZE_MESSAGE(original_rows, test_buffer->rows,
                                   "Row count should remain unchanged");
    TEST_ASSERT_EQUAL_SIZE_MESSAGE(1, test_buffer->width[0],
                                   "First row width should be reduced to 1");
    TEST_ASSERT_TRUE_MESSAGE(row_field_is_empty(0, 0),
                             "Remaining field in first row should be empty");
    TEST_ASSERT_NULL_MESSAGE(test_buffer->field[0][1],
                             "Second field in first row should be NULL after realloc");
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_csv_clear_row_last_row_delegates_to_remove_last_row_success);
    RUN_TEST(test_csv_clear_row_middle_row_reduces_to_one_field);
    RUN_TEST(test_csv_clear_row_first_row_reduces_to_one_field);
    /* Skipped tests due to constraints */
    RUN_TEST(test_csv_clear_row_last_row_delegates_to_remove_last_row_failure);
    RUN_TEST(test_csv_clear_row_middle_row_realloc_fails_restores_fields);

    return UNITY_END();
}