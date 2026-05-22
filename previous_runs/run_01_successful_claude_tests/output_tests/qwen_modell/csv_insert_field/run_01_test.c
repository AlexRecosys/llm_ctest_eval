#include "unity.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* Project headers */
// #include "csv_buffer.h"  /* Assumed header containing declarations */

/* Static fixture variables */
static CSV_BUFFER *buffer = NULL;

/* Helper function declarations (for internal test helpers) */
static void init_buffer_with_data(size_t rows, size_t cols, const char * const * const data);
static void destroy_buffer_and_reset(void);

/* Helper macro for string comparison */
#define TEST_ASSERT_EQUAL_FIELD_STRING(expected, actual, msg) \
    TEST_ASSERT_EQUAL_STRING_MESSAGE(expected, (actual)->text, msg)

/* setUp: Initialize buffer before each test */
void setUp(void)
{
    buffer = csv_create_buffer();
    TEST_ASSERT_NOT_NULL_MESSAGE(buffer, "Failed to create buffer in setUp");
}

/* tearDown: Clean up buffer after each test */
void tearDown(void)
{
    if (buffer != NULL) {
        csv_destroy_buffer(buffer);
        buffer = NULL;
    }
}

/* Helper: Initialize buffer with a 2D array of strings */
static void init_buffer_with_data(size_t rows, size_t cols, const char * const * const data)
{
    for (size_t r = 0; r < rows; r++) {
        for (size_t c = 0; c < cols; c++) {
            csv_set_field(buffer, r, c, data[r * cols + c]);
        }
    }
}

/* Helper: Destroy buffer and reset to NULL */
static void destroy_buffer_and_reset(void)
{
    if (buffer != NULL) {
        csv_destroy_buffer(buffer);
        buffer = NULL;
    }
}

/* Test 1: Insert at existing position (middle of row) */
void test_csv_insert_field_insert_in_middle_of_row(void)
{
    const char *initial_data[] = {
        "a", "b", "c"
    };
    init_buffer_with_data(1, 3, initial_data);

    /* Insert "X" at position 1 (between "a" and "b") */
    int result = csv_insert_field(buffer, 0, 1, "X");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "csv_insert_field should return 0 on success");

    /* Verify field at position 1 is "X" */
    TEST_ASSERT_EQUAL_FIELD_STRING("X", buffer->field[0][1], "Field at [0][1] should be 'X'");

    /* Verify original field at position 1 ("b") moved to position 2 */
    TEST_ASSERT_EQUAL_FIELD_STRING("b", buffer->field[0][2], "Field at [0][2] should be 'b'");

    /* Verify field at position 0 unchanged */
    TEST_ASSERT_EQUAL_FIELD_STRING("a", buffer->field[0][0], "Field at [0][0] should remain 'a'");

    /* Verify width increased by 1 */
    TEST_ASSERT_EQUAL_UINT_MESSAGE(4, buffer->width[0], "Row width should increase by 1");
}

/* Test 2: Insert at end of row (should behave like set_field) */
void test_csv_insert_field_insert_at_end_of_row(void)
{
    const char *initial_data[] = {
        "a", "b"
    };
    init_buffer_with_data(1, 2, initial_data);

    /* Insert "Z" at position 2 (end of row) */
    int result = csv_insert_field(buffer, 0, 2, "Z");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "csv_insert_field should return 0 on success");

    /* Verify new field at position 2 is "Z" */
    TEST_ASSERT_EQUAL_FIELD_STRING("Z", buffer->field[0][2], "Field at [0][2] should be 'Z'");

    /* Verify original fields unchanged */
    TEST_ASSERT_EQUAL_FIELD_STRING("a", buffer->field[0][0], "Field at [0][0] should remain 'a'");
    TEST_ASSERT_EQUAL_FIELD_STRING("b", buffer->field[0][1], "Field at [0][1] should remain 'b'");

    /* Verify width increased by 1 */
    TEST_ASSERT_EQUAL_UINT_MESSAGE(3, buffer->width[0], "Row width should increase by 1");
}

/* Test 3: Insert beyond existing row (should create row and call set_field) */
void test_csv_insert_field_insert_beyond_existing_row(void)
{
    const char *initial_data[] = {
        "a", "b"
    };
    init_buffer_with_data(1, 2, initial_data);

    /* Insert "X" at row 1, entry 0 (new row) */
    int result = csv_insert_field(buffer, 1, 0, "X");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "csv_insert_field should return 0 on success");

    /* Verify new row was created */
    TEST_ASSERT_EQUAL_UINT_MESSAGE(2, buffer->rows, "Number of rows should increase to 2");

    /* Verify field at [1][0] is "X" */
    TEST_ASSERT_EQUAL_FIELD_STRING("X", buffer->field[1][0], "Field at [1][0] should be 'X'");

    /* Verify original row unchanged */
    TEST_ASSERT_EQUAL_FIELD_STRING("a", buffer->field[0][0], "Field at [0][0] should remain 'a'");
    TEST_ASSERT_EQUAL_FIELD_STRING("b", buffer->field[0][1], "Field at [0][1] should remain 'b'");
}

/* Test 4: Insert beyond existing column in existing row (should call set_field) */
void test_csv_insert_field_insert_beyond_existing_column(void)
{
    const char *initial_data[] = {
        "a", "b"
    };
    init_buffer_with_data(1, 2, initial_data);

    /* Insert "Y" at row 0, entry 5 (beyond current width) */
    int result = csv_insert_field(buffer, 0, 5, "Y");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "csv_insert_field should return 0 on success");

    /* Verify field at [0][5] is "Y" */
    TEST_ASSERT_EQUAL_FIELD_STRING("Y", buffer->field[0][5], "Field at [0][5] should be 'Y'");

    /* Verify original fields unchanged */
    TEST_ASSERT_EQUAL_FIELD_STRING("a", buffer->field[0][0], "Field at [0][0] should remain 'a'");
    TEST_ASSERT_EQUAL_FIELD_STRING("b", buffer->field[0][1], "Field at [0][1] should remain 'b'");

    /* Verify intermediate fields are empty (or default) */
    for (size_t i = 2; i < 5; i++) {
        TEST_ASSERT_EQUAL_FIELD_STRING("", buffer->field[0][i], "Intermediate field at [0][%zu] should be empty", i);
    }

    /* Verify width is 6 */
    TEST_ASSERT_EQUAL_UINT_MESSAGE(6, buffer->width[0], "Row width should be 6");
}

/* Test 5: Insert at beginning of row (shift all fields right) */
void test_csv_insert_field_insert_at_beginning_of_row(void)
{
    const char *initial_data[] = {
        "a", "b", "c"
    };
    init_buffer_with_data(1, 3, initial_data);

    /* Insert "First" at position 0 */
    int result = csv_insert_field(buffer, 0, 0, "First");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "csv_insert_field should return 0 on success");

    /* Verify new field at position 0 */
    TEST_ASSERT_EQUAL_FIELD_STRING("First", buffer->field[0][0], "Field at [0][0] should be 'First'");

    /* Verify original fields shifted right */
    TEST_ASSERT_EQUAL_FIELD_STRING("a", buffer->field[0][1], "Field at [0][1] should be 'a'");
    TEST_ASSERT_EQUAL_FIELD_STRING("b", buffer->field[0][2], "Field at [0][2] should be 'b'");
    TEST_ASSERT_EQUAL_FIELD_STRING("c", buffer->field[0][3], "Field at [0][3] should be 'c'");

    /* Verify width increased by 1 */
    TEST_ASSERT_EQUAL_UINT_MESSAGE(4, buffer->width[0], "Row width should increase by 1");
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_csv_insert_field_insert_in_middle_of_row);
    RUN_TEST(test_csv_insert_field_insert_at_end_of_row);
    RUN_TEST(test_csv_insert_field_insert_beyond_existing_row);
    RUN_TEST(test_csv_insert_field_insert_beyond_existing_column);
    RUN_TEST(test_csv_insert_field_insert_at_beginning_of_row);

    return UNITY_END();
}