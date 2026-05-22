#include "unity.h"
#include "csv.h"
#include <stdlib.h>
#include <string.h>

// Helper to initialize a buffer with a simple 2x2 grid for predictable testing
static void setup_2x2_buffer(CSV_BUFFER **buffer, const char *initial_value)
{
    *buffer = csv_create_buffer();
    TEST_ASSERT_NOT_NULL(*buffer);

    // Set up a 2x2 buffer with all fields set to initial_value
    csv_set_field(*buffer, 0, 0, initial_value);
    csv_set_field(*buffer, 0, 1, initial_value);
    csv_set_field(*buffer, 1, 0, initial_value);
    csv_set_field(*buffer, 1, 1, initial_value);
}

// Helper to verify field content at (row, col)
static void assert_field_equals(CSV_BUFFER *buffer, size_t row, size_t col, const char *expected)
{
    TEST_ASSERT_TRUE(row < buffer->rows);
    TEST_ASSERT_TRUE(col < buffer->width[row]);
    TEST_ASSERT_EQUAL_STRING(expected, buffer->field[row][col]->text);
}

// Test: Insert in the middle of existing row (shifts right)
TEST(csv_insert_field, insert_in_middle_shifts_right)
{
    CSV_BUFFER *buffer = NULL;
    setup_2x2_buffer(&buffer, "old");

    // Insert "new" at (0, 0) → should shift existing [0][0] → [0][1], [0][1] → [0][2]
    int result = csv_insert_field(buffer, 0, 0, "new");
    TEST_ASSERT_EQUAL_INT(0, result);

    // Verify structure: row 0 now has width 3
    TEST_ASSERT_EQUAL_SIZE(3, buffer->width[0]);
    TEST_ASSERT_EQUAL_SIZE(2, buffer->width[1]); // row 1 unchanged

    // Verify values: "new" at [0][0], "old" shifted to [0][1] and [0][2]
    assert_field_equals(buffer, 0, 0, "new");
    assert_field_equals(buffer, 0, 1, "old");
    assert_field_equals(buffer, 0, 2, "old");

    csv_destroy_buffer(buffer);
}

// Test: Insert beyond current width (calls csv_set_field, no shift)
TEST(csv_insert_field, insert_beyond_width_extends_row)
{
    CSV_BUFFER *buffer = NULL;
    setup_2x2_buffer(&buffer, "old");

    // Insert at (0, 5) → should extend row 0 to width 6
    int result = csv_insert_field(buffer, 0, 5, "new");
    TEST_ASSERT_EQUAL_INT(0, result);

    TEST_ASSERT_EQUAL_SIZE(6, buffer->width[0]);
    TEST_ASSERT_EQUAL_SIZE(2, buffer->width[1]);

    // Fields [0][0]..[0][4] should be "old" (original [0][0],[0][1]) + new empty slots filled by csv_set_field
    // But note: csv_set_field pads with empty fields (likely empty strings)
    // We only know [0][5] must be "new"
    assert_field_equals(buffer, 0, 5, "new");

    // Verify earlier fields are still "old" (since csv_set_field only sets up to requested entry)
    // Actually, csv_set_field uses append_field to grow, and set_field to assign.
    // So [0][0] and [0][1] remain "old", others (2..4) are newly created and likely empty.
    // But we only test what we can rely on:
    assert_field_equals(buffer, 0, 0, "old");
    assert_field_equals(buffer, 0, 1, "old");

    csv_destroy_buffer(buffer);
}

// Test: Insert at last valid position (no shift needed)
TEST(csv_insert_field, insert_at_last_position_no_shift)
{
    CSV_BUFFER *buffer = NULL;
    setup_2x2_buffer(&buffer, "old");

    // Insert at (0, 1) — last valid index before extension
    int result = csv_insert_field(buffer, 0, 1, "new");
    TEST_ASSERT_EQUAL_INT(0, result);

    // Row 0 now has width 3
    TEST_ASSERT_EQUAL_SIZE(3, buffer->width[0]);

    // [0][0] unchanged, [0][1] = "new", [0][2] = old [0][1]
    assert_field_equals(buffer, 0, 0, "old");
    assert_field_equals(buffer, 0, 1, "new");
    assert_field_equals(buffer, 0, 2, "old");

    csv_destroy_buffer(buffer);
}

// Test: Insert into new row (calls csv_set_field to grow rows)
TEST(csv_insert_field, insert_into_new_row_extends_rows)
{
    CSV_BUFFER *buffer = NULL;
    setup_2x2_buffer(&buffer, "old");

    // Insert at (2, 0) — beyond current rows (rows=2, so row 2 is new)
    int result = csv_insert_field(buffer, 2, 0, "new");
    TEST_ASSERT_EQUAL_INT(0, result);

    TEST_ASSERT_EQUAL_SIZE(3, buffer->rows);
    TEST_ASSERT_EQUAL_SIZE(1, buffer->width[2]); // new row has width 1

    assert_field_equals(buffer, 2, 0, "new");

    // Original rows unchanged
    assert_field_equals(buffer, 0, 0, "old");
    assert_field_equals(buffer, 1, 0, "old");

    csv_destroy_buffer(buffer);
}

// Test: Insert at (0,0) of empty buffer (edge case)
TEST(csv_insert_field, insert_into_empty_buffer)
{
    CSV_BUFFER *buffer = csv_create_buffer();
    TEST_ASSERT_NOT_NULL(buffer);

    int result = csv_insert_field(buffer, 0, 0, "first");
    TEST_ASSERT_EQUAL_INT(0, result);

    TEST_ASSERT_EQUAL_SIZE(1, buffer->rows);
    TEST_ASSERT_EQUAL_SIZE(1, buffer->width[0]);
    assert_field_equals(buffer, 0, 0, "first");

    csv_destroy_buffer(buffer);
}