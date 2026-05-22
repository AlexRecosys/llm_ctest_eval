#include "unity.h"
#include "csv.h"

static CSV_BUFFER *buffer = NULL;

void setUp(void)
{
    buffer = csv_create_buffer();
    TEST_ASSERT_NOT_NULL_MESSAGE(buffer, "Failed to create buffer");
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
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "append_row failed");

    for (size_t i = 0; i < field_count; i++) {
        ret = append_field(buf, row);
        TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "append_field failed");
        ret = set_field(buf->field[row][i], "test");
        TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "set_field failed");
    }
}

static void verify_row_clear(CSV_BUFFER *buf, size_t row, size_t expected_width)
{
    TEST_ASSERT_EQUAL_UINT_MESSAGE(expected_width, buf->width[row], "Row width not 1 after clear");
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, (int)buf->width[row], "Row width should be 1");
    TEST_ASSERT_NOT_NULL_MESSAGE(buf->field[row], "Field array should not be NULL");
    TEST_ASSERT_NOT_NULL_MESSAGE(buf->field[row][0], "First field should exist");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("", buf->field[row][0]->text, "Field should be empty string");
}

static void setup_multi_row_buffer(size_t num_rows, size_t fields_per_row)
{
    tearDown();
    setUp();

    for (size_t r = 0; r < num_rows; r++) {
        populate_row_with_fields(buffer, r, fields_per_row);
    }
}

static void setup_last_row_clearable_buffer(size_t num_rows, size_t fields_per_row)
{
    setup_multi_row_buffer(num_rows, fields_per_row);
}

static void setup_non_last_row_clearable_buffer(size_t num_rows, size_t fields_per_row, size_t target_row)
{
    setup_multi_row_buffer(num_rows, fields_per_row);
    TEST_ASSERT_TRUE_MESSAGE(target_row < num_rows, "Target row must exist");
}

static void setup_buffer_with_single_field_row(size_t num_rows)
{
    tearDown();
    setUp();

    for (size_t r = 0; r < num_rows; r++) {
        int ret = append_row(buffer);
        TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "append_row failed");
        ret = append_field(buffer, r);
        TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "append_field failed");
        ret = set_field(buffer->field[r][0], "keep");
        TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "set_field failed");
    }
}

static void setup_buffer_with_empty_row(size_t num_rows)
{
    tearDown();
    setUp();

    for (size_t r = 0; r < num_rows; r++) {
        int ret = append_row(buffer);
        TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "append_row failed");
    }
}

static void setup_buffer_with_varied_rows(size_t *row_widths, size_t num_rows)
{
    tearDown();
    setUp();

    for (size_t r = 0; r < num_rows; r++) {
        int ret = append_row(buffer);
        TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "append_row failed");
        for (size_t f = 0; f < row_widths[r]; f++) {
            ret = append_field(buffer, r);
            TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "append_field failed");
            char field_text[32];
            snprintf(field_text, sizeof(field_text), "r%zu_f%zu", r, f);
            ret = set_field(buffer->field[r][f], field_text);
            TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "set_field failed");
        }
    }
}

void test_csv_clear_row_last_row_simple_case(void)
{
    populate_row_with_fields(buffer, 0, 5);
    TEST_ASSERT_EQUAL_UINT_MESSAGE(5, buffer->width[0], "Initial width should be 5");

    int ret = csv_clear_row(buffer, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "csv_clear_row should succeed");
    verify_row_clear(buffer, 0, 1);
    TEST_ASSERT_EQUAL_UINT_MESSAGE(1, buffer->rows, "Row count should remain 1");
}

void test_csv_clear_row_non_last_row_preserves_other_rows(void)
{
    setup_multi_row_buffer(3, 4);

    size_t original_rows = buffer->rows;
    size_t original_width_0 = buffer->width[0];
    size_t original_width_1 = buffer->width[1];
    size_t original_width_2 = buffer->width[2];

    int ret = csv_clear_row(buffer, 1);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "csv_clear_row should succeed");

    TEST_ASSERT_EQUAL_UINT_MESSAGE(original_rows, buffer->rows, "Row count should not change");
    TEST_ASSERT_EQUAL_UINT_MESSAGE(original_width_0, buffer->width[0], "Row 0 width should be unchanged");
    TEST_ASSERT_EQUAL_UINT_MESSAGE(original_width_2, buffer->width[2], "Row 2 width should be unchanged");

    verify_row_clear(buffer, 1, 1);
}

void test_csv_clear_row_last_row_optimization_path(void)
{
    setup_last_row_clearable_buffer(2, 3);

    size_t original_rows = buffer->rows;
    size_t original_width_1 = buffer->width[1];

    int ret = csv_clear_row(buffer, 1);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "csv_clear_row should succeed");

    TEST_ASSERT_EQUAL_UINT_MESSAGE(original_rows, buffer->rows, "Row count should not change");
    verify_row_clear(buffer, 1, 1);
}

void test_csv_clear_row_single_field_row_no_change(void)
{
    setup_buffer_with_single_field_row(2);

    int ret = csv_clear_row(buffer, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "csv_clear_row should succeed");

    verify_row_clear(buffer, 0, 1);
    TEST_ASSERT_EQUAL_UINT_MESSAGE(2, buffer->rows, "Row count should remain 2");
}

void test_csv_clear_row_empty_row(void)
{
    setup_buffer_with_empty_row(2);

    int ret = csv_clear_row(buffer, 1);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "csv_clear_row should succeed on empty row");

    verify_row_clear(buffer, 1, 1);
}