#include "unity.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* Project headers */
#include "csv_buffer.h"  // Assumed to contain CSV_FIELD, CSV_BUFFER, and function declarations

/* Function declarations from project (assumed to exist) */
extern CSV_BUFFER *create_buffer(char field_delim, char text_delim);
extern void destroy_buffer(CSV_BUFFER *buffer);
extern int append_row(CSV_BUFFER *buffer);
extern int append_field(CSV_BUFFER *buffer, size_t row);
extern int set_field(CSV_FIELD *field, const char *text);
extern int remove_last_row(CSV_BUFFER *buffer);
extern void destroy_field(CSV_FIELD *field);

/* Static fixture variables */
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
    /* Setup: 2 rows, 3 cols each */
    TEST_ASSERT_TRUE_MESSAGE(init_test_buffer_with_rows(2, 3), "Failed to create test buffer");

    /* Act: clear last row (row 1) */
    int result = csv_clear_row(test_buffer, 1);

    /* Assert */
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "csv_clear_row should return 0 on success");
    TEST_ASSERT_EQUAL_SIZE_MESSAGE(1, test_buffer->rows, "Row count should decrease by 1");
    TEST_ASSERT_EQUAL_SIZE_MESSAGE(1, test_buffer->width[0], "Remaining row should have 1 field");
    TEST_ASSERT_NULL_MESSAGE(test_buffer->field[1], "Row 1 should be freed");
    TEST_ASSERT_NULL_MESSAGE(test_buffer->width[1], "Width for row 1 should be freed");
}

void test_csv_clear_row_last_row_uses_remove_last_row_failure(void) {
    /* Setup: 1 row, 3 cols */
    TEST_ASSERT_TRUE_MESSAGE(init_test_buffer_with_rows(1, 3), "Failed to create test buffer");

    /* Force remove_last_row to fail by corrupting buffer (simulate internal error) */
    /* Note: In real code, we'd need to mock or inject failure; but per constraints, no mocks */
    /* Instead, we test the *normal* path where remove_last_row succeeds */
    /* This test is omitted per requirement: we cannot force failure without mocks */
    /* So we skip this test and rely on test_csv_clear_row_last_row_uses_remove_last_row_success */
    /* But we need 5 tests, so we'll add more below */
    TEST_IGNORE_MESSAGE("Skipped: cannot test remove_last_row failure without mocks");
}

void test_csv_clear_row_middle_row_reduces_to_one_field(void) {
    /* Setup: 3 rows, 4 cols each */
    TEST_ASSERT_TRUE_MESSAGE(init_test_buffer_with_rows(3, 4), "Failed to create test buffer");

    /* Save original field pointers for verification */
    CSV_FIELD **original_fields = test_buffer->field[1];
    size_t original_width = test_buffer->width[1];

    /* Act: clear middle row (row 1) */
    int result = csv_clear_row(test_buffer, 1);

    /* Assert */
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "csv_clear_row should return 0 on success");
    TEST_ASSERT_EQUAL_SIZE_MESSAGE(3, test_buffer->rows, "Row count should remain unchanged");
    TEST_ASSERT_EQUAL_SIZE_MESSAGE(1, test_buffer->width[1], "Row 1 width should be 1");
    TEST_ASSERT_NOT_NULL_MESSAGE(test_buffer->field[1], "Row 1 field array should exist");
    TEST_ASSERT_EQUAL_SIZE_MESSAGE(1, test_buffer->width[0], "Row 0 unchanged");
    TEST_ASSERT_EQUAL_SIZE_MESSAGE(4, test_buffer->width[2], "Row 2 unchanged");

    /* Verify only first field remains and is empty */
    TEST_ASSERT_NOT_NULL_MESSAGE(test_buffer->field[1][0], "Row 1, field 0 should exist");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("", test_buffer->field[1][0]->text, "Remaining field should be empty");
    TEST_ASSERT_NULL_MESSAGE(test_buffer->field[1][1], "Row 1, field 1 should be NULL (freed)");
}

void test_csv_clear_row_middle_row_realloc_fails_restores_fields(void) {
    /* Setup: 2 rows, 3 cols each */
    TEST_ASSERT_TRUE_MESSAGE(init_test_buffer_with_rows(2, 3), "Failed to create test buffer");

    /* Save original state for verification */
    size_t original_width = test_buffer->width[0];
    char *original_text_0_0 = test_buffer->field[0][0]->text ? strdup(test_buffer->field[0][0]->text) : NULL;
    char *original_text_0_1 = test_buffer->field[0][1]->text ? strdup(test_buffer->field[0][1]->text) : NULL;
    char *original_text_0_2 = test_buffer->field[0][2]->text ? strdup(test_buffer->field[0][2]->text) : NULL;

    /* Force realloc to fail by setting buffer->field[0] to a pointer that realloc cannot shrink */
    /* This is a known technique: temporarily replace field[0] with a pointer to a large block */
    /* But since we cannot control realloc behavior directly, we simulate via memory exhaustion */
    /* However, per constraints: no mocks/stubs/fakes, and must use real functions */
    /* So we skip this test — it's not reliably testable without mocks */
    /* Instead, we add a test for a row with only 1 field (edge case) */
    TEST_IGNORE_MESSAGE("Skipped: cannot reliably test realloc failure without mocks");
}

void test_csv_clear_row_single_field_row_no_change(void) {
    /* Setup: 2 rows, 1 col each */
    TEST_ASSERT_TRUE_MESSAGE(init_test_buffer_with_rows(2, 1), "Failed to create test buffer");

    /* Save original state */
    char *original_text_0_0 = test_buffer->field[0][0]->text ? strdup(test_buffer->field[0][0]->text) : NULL;

    /* Act: clear row 0 (which already has 1 field) */
    int result = csv_clear_row(test_buffer, 0);

    /* Assert */
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "csv_clear_row should return 0");
    TEST_ASSERT_EQUAL_SIZE_MESSAGE(2, test_buffer->rows, "Row count unchanged");
    TEST_ASSERT_EQUAL_SIZE_MESSAGE(1, test_buffer->width[0], "Width remains 1");
    TEST_ASSERT_NOT_NULL_MESSAGE(test_buffer->field[0], "Field array still exists");
    TEST_ASSERT_NOT_NULL_MESSAGE(test_buffer->field[0][0], "Field still exists");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("", test_buffer->field[0][0]->text, "Field should be cleared to empty string");

    /* Cleanup */
    if (original_text_0_0) free(original_text_0_0);
}

void test_csv_clear_row_empty_buffer_returns_error(void) {
    /* Setup: empty buffer */
    test_buffer = create_buffer(',', '"');
    TEST_ASSERT_NOT_NULL_MESSAGE(test_buffer, "Buffer creation should succeed");

    /* Act & Assert */
    int result = csv_clear_row(test_buffer, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, result, "csv_clear_row should return 1 for invalid row");
}

/* main */
int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_csv_clear_row_last_row_uses_remove_last_row_success);
    RUN_TEST(test_csv_clear_row_middle_row_reduces_to_one_field);
    RUN_TEST(test_csv_clear_row_single_field_row_no_change);
    RUN_TEST(test_csv_clear_row_empty_buffer_returns_error);
    // Note: Skipped tests due to constraints (no mocks for failure injection)
    // RUN_TEST(test_csv_clear_row_middle_row_realloc_fails_restores_fields);

    return UNITY_END();
}