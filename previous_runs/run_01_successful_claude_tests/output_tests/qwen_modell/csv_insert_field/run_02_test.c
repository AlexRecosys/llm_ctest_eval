#include "unity.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* Project headers */
#include "csv_buffer.h"  /* Assumed header containing declarations */

/* Static fixture variables */
static CSV_BUFFER *buffer = NULL;

/* Helper function declarations */
static void init_buffer_with_rows_and_width(CSV_BUFFER *buf, size_t rows, size_t *widths);
static void destroy_buffer_contents(CSV_BUFFER *buf);

/* Helper function definitions */

static void init_buffer_with_rows_and_width(CSV_BUFFER *buf, size_t rows, size_t *widths)
{
    size_t i, j;
    buf->rows = rows;
    buf->width = malloc(rows * sizeof(size_t));
    TEST_ASSERT_NOT_NULL_MESSAGE(buf->width, "Failed to allocate width array");

    buf->field = malloc(rows * sizeof(CSV_FIELD **));
    TEST_ASSERT_NOT_NULL_MESSAGE(buf->field, "Failed to allocate field array");

    for (i = 0; i < rows; i++) {
        buf->width[i] = widths[i];
        buf->field[i] = malloc(widths[i] * sizeof(CSV_FIELD *));
        TEST_ASSERT_NOT_NULL_MESSAGE(buf->field[i], "Failed to allocate row fields");

        for (j = 0; j < widths[i]; j++) {
            buf->field[i][j] = malloc(sizeof(CSV_FIELD));
            TEST_ASSERT_NOT_NULL_MESSAGE(buf->field[i][j], "Failed to allocate field");
            buf->field[i][j]->text = NULL;
            buf->field[i][j]->length = 0;
        }
    }
}

static void destroy_buffer_contents(CSV_BUFFER *buf)
{
    size_t i, j;
    if (buf == NULL || buf->field == NULL) return;

    for (i = 0; i < buf->rows; i++) {
        if (buf->field[i] == NULL) continue;
        for (j = 0; j < buf->width[i]; j++) {
            if (buf->field[i][j] != NULL) {
                if (buf->field[i][j]->text != NULL) {
                    free(buf->field[i][j]->text);
                    buf->field[i][j]->text = NULL;
                }
                free(buf->field[i][j]);
                buf->field[i][j] = NULL;
            }
        }
        free(buf->field[i]);
        buf->field[i] = NULL;
    }
    free(buf->field);
    buf->field = NULL;
    free(buf->width);
    buf->width = NULL;
}

/* setUp / tearDown */

void setUp(void)
{
    buffer = csv_create_buffer();
    TEST_ASSERT_NOT_NULL_MESSAGE(buffer, "Failed to create buffer in setUp");
}

void tearDown(void)
{
    if (buffer != NULL) {
        csv_destroy_buffer(buffer);
        buffer = NULL;
    }
}

/* Test cases */

void test_csv_insert_field_inserts_at_end_when_row_or_entry_out_of_bounds(void)
{
    size_t widths[] = {2};
    init_buffer_with_rows_and_width(buffer, 1, widths);

    /* Insert at row 1 (beyond existing row 0) */
    int result = csv_insert_field(buffer, 1, 0, "new_field");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "csv_insert_field should return 0");

    /* Verify new row was created */
    TEST_ASSERT_EQUAL_SIZE_MESSAGE(2, buffer->rows, "Row count should increase to 2");
    TEST_ASSERT_EQUAL_SIZE_MESSAGE(1, buffer->width[1], "New row should have width 1");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("new_field", buffer->field[1][0]->text,
                                     "Field should be set correctly");
}

void test_csv_insert_field_inserts_at_end_of_existing_row(void)
{
    size_t widths[] = {2};
    init_buffer_with_rows_and_width(buffer, 1, widths);

    /* Set initial fields */
    csv_set_field(buffer, 0, 0, "first");
    csv_set_field(buffer, 0, 1, "second");

    /* Insert at entry 2 (beyond existing width 2) */
    int result = csv_insert_field(buffer, 0, 2, "third");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "csv_insert_field should return 0");

    /* Verify field was appended */
    TEST_ASSERT_EQUAL_SIZE_MESSAGE(3, buffer->width[0], "Row width should increase to 3");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("third", buffer->field[0][2]->text,
                                     "New field should be appended");
}

void test_csv_insert_field_shifts_existing_fields_when_inserting_in_middle(void)
{
    size_t widths[] = {3};
    init_buffer_with_rows_and_width(buffer, 1, widths);

    /* Set initial fields */
    csv_set_field(buffer, 0, 0, "A");
    csv_set_field(buffer, 0, 1, "B");
    csv_set_field(buffer, 0, 2, "C");

    /* Insert at entry 1 (middle) */
    int result = csv_insert_field(buffer, 0, 1, "X");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "csv_insert_field should return 0");

    /* Verify shift occurred */
    TEST_ASSERT_EQUAL_SIZE_MESSAGE(4, buffer->width[0], "Row width should increase to 4");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("A", buffer->field[0][0]->text,
                                     "First field unchanged");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("X", buffer->field[0][1]->text,
                                     "Inserted field at position 1");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("B", buffer->field[0][2]->text,
                                     "Original field B shifted to position 2");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("C", buffer->field[0][3]->text,
                                     "Original field C shifted to position 3");
}

void test_csv_insert_field_handles_empty_buffer(void)
{
    /* Buffer is empty (rows=0) */
    int result = csv_insert_field(buffer, 0, 0, "first");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "csv_insert_field should return 0");

    TEST_ASSERT_EQUAL_SIZE_MESSAGE(1, buffer->rows, "Row count should be 1");
    TEST_ASSERT_EQUAL_SIZE_MESSAGE(1, buffer->width[0], "Row width should be 1");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("first", buffer->field[0][0]->text,
                                     "Field should be set");
}

void test_csv_insert_field_preserves_existing_fields_when_inserting_at_end_of_row(void)
{
    size_t widths[] = {2};
    init_buffer_with_rows_and_width(buffer, 1, widths);

    /* Set initial fields */
    csv_set_field(buffer, 0, 0, "alpha");
    csv_set_field(buffer, 0, 1, "beta");

    /* Insert at entry 2 (end of row) */
    int result = csv_insert_field(buffer, 0, 2, "gamma");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, result, "csv_insert_field should return 0");

    /* Verify all fields preserved */
    TEST_ASSERT_EQUAL_SIZE_MESSAGE(3, buffer->width[0], "Row width should be 3");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("alpha", buffer->field[0][0]->text,
                                     "First field preserved");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("beta", buffer->field[0][1]->text,
                                     "Second field preserved");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("gamma", buffer->field[0][2]->text,
                                     "New field appended");
}

/* main function */
int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_csv_insert_field_inserts_at_end_when_row_or_entry_out_of_bounds);
    RUN_TEST(test_csv_insert_field_inserts_at_end_of_existing_row);
    RUN_TEST(test_csv_insert_field_shifts_existing_fields_when_inserting_in_middle);
    RUN_TEST(test_csv_insert_field_handles_empty_buffer);
    RUN_TEST(test_csv_insert_field_preserves_existing_fields_when_inserting_at_end_of_row);

    return UNITY_END();
}