#include "unity.h"
#include "csv.h"

static CSV_BUFFER *buffer = NULL;

void setUp(void)
{
    buffer = csv_create_buffer();
    TEST_ASSERT_NOT_NULL(buffer);
}

void tearDown(void)
{
    if (buffer != NULL) {
        csv_destroy_buffer(buffer);
        buffer = NULL;
    }
}

static void populate_row_with_fields(CSV_BUFFER *buf, size_t row, size_t field_count)
{
    int ret;
    ret = append_row(buf);
    TEST_ASSERT_EQUAL_INT(0, ret);

    for (size_t i = 0; i < field_count; i++) {
        ret = append_field(buf, row);
        TEST_ASSERT_EQUAL_INT(0, ret);
        ret = set_field(buf->field[row][i], "test");
        TEST_ASSERT_EQUAL_INT(0, ret);
    }
}

static void verify_row_cleared(CSV_BUFFER *buf, size_t row)
{
    TEST_ASSERT_EQUAL_UINT(1, buf->width[row]);
    TEST_ASSERT_NOT_NULL(buf->field[row]);
    TEST_ASSERT_EQUAL_UINT(1, buf->rows);
    TEST_ASSERT_NOT_NULL(buf->field[row][0]);
    TEST_ASSERT_EQUAL_STRING("", buf->field[row][0]->text);
}

static void setup_row_with_multiple_fields(size_t field_count)
{
    populate_row_with_fields(buffer, 0, field_count);
    TEST_ASSERT_EQUAL_UINT(field_count, buffer->width[0]);
}

static void setup_row_as_last_row(size_t field_count)
{
    // Add two rows, then clear the second (last) row
    populate_row_with_fields(buffer, 0, 2);
    populate_row_with_fields(buffer, 1, field_count);
    TEST_ASSERT_EQUAL_UINT(2, buffer->rows);
    TEST_ASSERT_EQUAL_UINT(field_count, buffer->width[1]);
}

static void setup_row_as_non_last_row(size_t field_count)
{
    // Add two rows, then clear the first (non-last) row
    populate_row_with_fields(buffer, 0, field_count);
    populate_row_with_fields(buffer, 1, 2);
    TEST_ASSERT_EQUAL_UINT(2, buffer->rows);
    TEST_ASSERT_EQUAL_UINT(field_count, buffer->width[0]);
}

TEST(CSVBuffer, ClearRow_LastRow_ShrinksSuccessfully)
{
    setup_row_as_last_row(5);

    int ret = csv_clear_row(buffer, 1);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_UINT(1, buffer->rows);
    TEST_ASSERT_EQUAL_UINT(1, buffer->width[0]);
    TEST_ASSERT_NOT_NULL(buffer->field[0]);
    TEST_ASSERT_NOT_NULL(buffer->field[0][0]);
    TEST_ASSERT_EQUAL_STRING("", buffer->field[0][0]->text);
}

TEST(CSVBuffer, ClearRow_NonLastRow_DestroyFieldsAndShrink)
{
    setup_row_as_non_last_row(4);

    int ret = csv_clear_row(buffer, 0);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_UINT(2, buffer->rows);
    TEST_ASSERT_EQUAL_UINT(1, buffer->width[0]);
    TEST_ASSERT_EQUAL_UINT(2, buffer->width[1]);
    TEST_ASSERT_NOT_NULL(buffer->field[0]);
    TEST_ASSERT_NOT_NULL(buffer->field[0][0]);
    TEST_ASSERT_EQUAL_STRING("", buffer->field[0][0]->text);
    // Second row should be untouched
    TEST_ASSERT_EQUAL_UINT(2, buffer->width[1]);
}

TEST(CSVBuffer, ClearRow_RowWithSingleField_ClearsField)
{
    populate_row_with_fields(buffer, 0, 1);
    TEST_ASSERT_EQUAL_UINT(1, buffer->width[0]);

    int ret = csv_clear_row(buffer, 0);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_UINT(1, buffer->width[0]);
    TEST_ASSERT_EQUAL_STRING("", buffer->field[0][0]->text);
}

TEST(CSVBuffer, ClearRow_RowWithEmptyFields_ClearsSuccessfully)
{
    int ret = append_row(buffer);
    TEST_ASSERT_EQUAL_INT(0, ret);

    // Add 3 empty fields
    for (int i = 0; i < 3; i++) {
        ret = append_field(buffer, 0);
        TEST_ASSERT_EQUAL_INT(0, ret);
    }
    TEST_ASSERT_EQUAL_UINT(3, buffer->width[0]);

    ret = csv_clear_row(buffer, 0);

    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_UINT(1, buffer->width[0]);
    TEST_ASSERT_NOT_NULL(buffer->field[0]);
    TEST_ASSERT_NOT_NULL(buffer->field[0][0]);
    TEST_ASSERT_EQUAL_STRING("", buffer->field[0][0]->text);
}

TEST(CSVBuffer, ClearRow_LastRow_CallRemoveLastRow)
{
    // This test verifies that when clearing the last row,
    // csv_clear_row delegates to remove_last_row
    setup_row_as_last_row(3);

    // Capture original state before call
    size_t original_rows = buffer->rows;
    size_t original_width = buffer->width[1];

    int ret = csv_clear_row(buffer, 1);

    // Should have reduced rows to 1
    TEST_ASSERT_EQUAL_INT(0, ret);
    TEST_ASSERT_EQUAL_UINT(1, buffer->rows);
    // The remaining row (index 0) should be untouched
    TEST_ASSERT_EQUAL_UINT(2, buffer->width[0]);
}