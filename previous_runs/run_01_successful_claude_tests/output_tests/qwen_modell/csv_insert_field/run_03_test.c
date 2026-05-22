#include "unity.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* Project headers */
#include "csv_buffer.h"  /* Assumed header containing declarations */

/* Static fixture variables */
static CSV_BUFFER *buffer = NULL;

/* Helper function declarations (for internal test helpers) */
static void init_buffer_with_data(size_t rows, size_t cols, const char * const * const data);
static void destroy_buffer_and_reset(void);

/* Helper function definitions */

static void init_buffer_with_data(size_t rows, size_t cols, const char * const * const data)
{
    buffer = csv_create_buffer();
    TEST_ASSERT_NOT_NULL_MESSAGE(buffer, "Failed to create buffer");

    /* Initialize first row */
    for (size_t i = 0; i < rows; i++) {
        while (buffer->rows <= i) {
            /* append_row is internal; assume it exists */
            /* We'll use csv_set_field to grow the buffer */
        }
        for (size_t j = 0; j < cols; j++) {
            size_t idx = i * cols + j;
            if (data && data[idx]) {
                csv_set_field(buffer, i, j, (char *)data[idx]);
            } else {
                csv_set_field(buffer, i, j, "");
            }
        }
    }
}

static void destroy_buffer_and_reset(void)
{
    if (buffer != NULL) {
        csv_destroy_buffer(buffer);
        buffer = NULL;
    }
}

/* setUp / tearDown */

void setUp(void)
{
    buffer = NULL;
}

void tearDown(void)
{
    destroy_buffer_and_reset();
}

/* Test Cases */

void test_csv_insert_field_inserts_at_end_when_row_or_entry_out_of_bounds(void)
{
    /* Setup */
    buffer = csv_create_buffer();
    TEST_ASSERT_NOT_NULL(buffer);

    /* Insert into non-existent row */
    int result = csv_insert_field(buffer, 0, 0, "first");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "csv_insert_field should return 0");

    /* Verify field was set */
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, buffer->rows, "Should have 1 row");
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, buffer->width[0], "Should have 1 column");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("first", buffer->field[0][0]->text,
                                     "Field value mismatch");

    /* Insert into non-existent column in existing row */
    result = csv_insert_field(buffer, 0, 5, "last");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "csv_insert_field should return 0");

    /* Verify buffer grew to accommodate column 5 */
    TEST_ASSERT_EQUAL_INT_MESSAGE(6, buffer->width[0], "Should have 6 columns");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("last", buffer->field[0][5]->text,
                                     "Last field value mismatch");
}

void test_csv_insert_field_shifts_fields_right_when_inserting_in_middle(void)
{
    /* Setup: 1 row, 3 fields: ["a", "b", "c"] */
    const char *data[] = {"a", "b", "c"};
    init_buffer_with_data(1, 3, data);

    /* Insert "x" at position 1 */
    int result = csv_insert_field(buffer, 0, 1, "x");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "csv_insert_field should return 0");

    /* Verify shift: ["a", "x", "b", "c"] */
    TEST_ASSERT_EQUAL_INT_MESSAGE(4, buffer->width[0], "Width should be 4");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("a", buffer->field[0][0]->text, "Index 0 mismatch");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("x", buffer->field[0][1]->text, "Index 1 mismatch");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("b", buffer->field[0][2]->text, "Index 2 mismatch");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("c", buffer->field[0][3]->text, "Index 3 mismatch");
}

void test_csv_insert_field_handles_insert_at_beginning(void)
{
    /* Setup: 1 row, 2 fields: ["first", "second"] */
    const char *data[] = {"first", "second"};
    init_buffer_with_data(1, 2, data);

    /* Insert "zeroth" at position 0 */
    int result = csv_insert_field(buffer, 0, 0, "zeroth");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "csv_insert_field should return 0");

    /* Verify shift: ["zeroth", "first", "second"] */
    TEST_ASSERT_EQUAL_INT_MESSAGE(3, buffer->width[0], "Width should be 3");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("zeroth", buffer->field[0][0]->text, "Index 0 mismatch");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("first", buffer->field[0][1]->text, "Index 1 mismatch");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("second", buffer->field[0][2]->text, "Index 2 mismatch");
}

void test_csv_insert_field_handles_insert_at_end_of_existing_row(void)
{
    /* Setup: 1 row, 2 fields: ["a", "b"] */
    const char *data[] = {"a", "b"};
    init_buffer_with_data(1, 2, data);

    /* Insert "c" at position 2 (end of row) */
    int result = csv_insert_field(buffer, 0, 2, "c");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "csv_insert_field should return 0");

    /* Verify: ["a", "b", "c"] */
    TEST_ASSERT_EQUAL_INT_MESSAGE(3, buffer->width[0], "Width should be 3");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("a", buffer->field[0][0]->text, "Index 0 mismatch");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("b", buffer->field[0][1]->text, "Index 1 mismatch");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("c", buffer->field[0][2]->text, "Index 2 mismatch");
}

void test_csv_insert_field_handles_multi_row_buffer(void)
{
    /* Setup: 2 rows, 2 cols each */
    const char *data[] = {
        "r0c0", "r0c1",
        "r1c0", "r1c1"
    };
    init_buffer_with_data(2, 2, data);

    /* Insert "r0new" at row 0, entry 1 */
    int result = csv_insert_field(buffer, 0, 1, "r0new");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "csv_insert_field should return 0");

    /* Row 0 should now have 3 fields: ["r0c0", "r0new", "r0c1"] */
    TEST_ASSERT_EQUAL_INT_MESSAGE(3, buffer->width[0], "Row 0 width should be 3");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("r0c0", buffer->field[0][0]->text, "r0c0 mismatch");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("r0new", buffer->field[0][1]->text, "r0new mismatch");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("r0c1", buffer->field[0][2]->text, "r0c1 mismatch");

    /* Row 1 should be unchanged */
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, buffer->width[1], "Row 1 width should remain 2");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("r1c0", buffer->field[1][0]->text, "r1c0 mismatch");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("r1c1", buffer->field[1][1]->text, "r1c1 mismatch");
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_csv_insert_field_inserts_at_end_when_row_or_entry_out_of_bounds);
    RUN_TEST(test_csv_insert_field_shifts_fields_right_when_inserting_in_middle);
    RUN_TEST(test_csv_insert_field_handles_insert_at_beginning);
    RUN_TEST(test_csv_insert_field_handles_insert_at_end_of_existing_row);
    RUN_TEST(test_csv_insert_field_handles_multi_row_buffer);

    return UNITY_END();
}