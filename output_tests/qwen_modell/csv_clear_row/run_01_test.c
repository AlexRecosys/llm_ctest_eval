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
    ret = csv_set_field(buf, row, 0, "first");
    TEST_ASSERT_EQUAL_INT(0, ret);
    for (size_t i = 1; i < field_count; i++) {
        ret = csv_set_field(buf, row, i, "extra");
        TEST_ASSERT_EQUAL_INT(0, ret);
    }
}

static void verify_row_cleared(CSV_BUFFER *buf, size_t row)
{
    char dest[64] = {0};
    int ret = csv_get_field(dest, sizeof(dest), buf, row, 0);
    TEST_ASSERT_EQUAL_INT(2, ret);  // empty field
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buf, row));
}

static void verify_row_not_cleared(CSV_BUFFER *buf, size_t row, size_t expected_width)
{
    TEST_ASSERT_EQUAL_INT(expected_width, (size_t)csv_get_width(buf, row));
}

static void create_multi_row_buffer_with_fields(size_t num_rows, size_t fields_per_row)
{
    int ret;
    for (size_t r = 0; r < num_rows; r++) {
        ret = csv_set_field(buffer, r, 0, "row");
        TEST_ASSERT_EQUAL_INT(0, ret);
        for (size_t f = 1; f < fields_per_row; f++) {
            ret = csv_set_field(buffer, r, f, "col");
            TEST_ASSERT_EQUAL_INT(0, ret);
        }
    }
}

TEST(csv_clear_row, should_clear_last_row_efficiently)
{
    // Setup: create 2 rows, each with 3 fields
    create_multi_row_buffer_with_fields(2, 3);

    // Act: clear the last row (row 1)
    int result = csv_clear_row(buffer, 1);

    // Assert
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_INT(1, csv_get_height(buffer));
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buffer, 0));
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buffer, 1));  // row 1 now has 1 field
    verify_row_cleared(buffer, 1);
}

TEST(csv_clear_row, should_clear_middle_row_and_preserve_other_rows)
{
    // Setup: create 3 rows, each with 4 fields
    create_multi_row_buffer_with_fields(3, 4);

    // Act: clear middle row (row 1)
    int result = csv_clear_row(buffer, 1);

    // Assert
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_INT(3, csv_get_height(buffer));
    TEST_ASSERT_EQUAL_INT(4, csv_get_width(buffer, 0));
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buffer, 1));
    TEST_ASSERT_EQUAL_INT(4, csv_get_width(buffer, 2));
    verify_row_cleared(buffer, 1);
}

TEST(csv_clear_row, should_handle_row_with_single_field)
{
    // Setup: create 1 row with 1 field
    csv_set_field(buffer, 0, 0, "single");

    // Act: clear the only row
    int result = csv_clear_row(buffer, 0);

    // Assert
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_INT(1, csv_get_height(buffer));
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buffer, 0));
    verify_row_cleared(buffer, 0);
}

TEST(csv_clear_row, should_handle_row_with_many_fields)
{
    // Setup: create 1 row with 10 fields
    create_multi_row_buffer_with_fields(1, 10);

    // Act: clear the row
    int result = csv_clear_row(buffer, 0);

    // Assert
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_INT(1, csv_get_height(buffer));
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buffer, 0));
    verify_row_cleared(buffer, 0);
}

TEST(csv_clear_row, should_preserve_other_rows_when_clearing_non_last_row)
{
    // Setup: create 3 rows with varying field counts
    csv_set_field(buffer, 0, 0, "row0col0");
    csv_set_field(buffer, 0, 1, "row0col1");
    csv_set_field(buffer, 1, 0, "row1col0");
    csv_set_field(buffer, 1, 1, "row1col1");
    csv_set_field(buffer, 1, 2, "row1col2");
    csv_set_field(buffer, 2, 0, "row2col0");

    // Act: clear middle row (row 1)
    int result = csv_clear_row(buffer, 1);

    // Assert
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_INT(3, csv_get_height(buffer));
    TEST_ASSERT_EQUAL_INT(2, csv_get_width(buffer, 0));
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buffer, 1));
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buffer, 2));

    // Verify content of preserved rows
    char dest[64] = {0};
    csv_get_field(dest, sizeof(dest), buffer, 0, 0);
    TEST_ASSERT_EQUAL_STRING("row0col0", dest);
    csv_get_field(dest, sizeof(dest), buffer, 2, 0);
    TEST_ASSERT_EQUAL_STRING("row2col0", dest);
    verify_row_cleared(buffer, 1);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(csv_clear_row_should_clear_last_row_efficiently);
    RUN_TEST(csv_clear_row_should_clear_middle_row_and_preserve_other_rows);
    RUN_TEST(csv_clear_row_should_handle_row_with_single_field);
    RUN_TEST(csv_clear_row_should_handle_row_with_many_fields);
    RUN_TEST(csv_clear_row_should_preserve_other_rows_when_clearing_non_last_row);
    return UNITY_END();
}