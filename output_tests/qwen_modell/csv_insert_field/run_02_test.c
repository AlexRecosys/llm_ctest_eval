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
    csv_destroy_buffer(buffer);
    buffer = NULL;
}

static void verify_field(CSV_BUFFER *buf, size_t row, size_t entry, const char *expected)
{
    char dest[256] = {0};
    int result = csv_get_field(dest, sizeof(dest), buf, row, entry);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_STRING(expected, dest);
}

static void setup_initial_2x2_buffer(void)
{
    csv_set_field(buffer, 0, 0, "A1");
    csv_set_field(buffer, 0, 1, "B1");
    csv_set_field(buffer, 1, 0, "A2");
    csv_set_field(buffer, 1, 1, "B2");
}

static void setup_initial_1x3_buffer(void)
{
    csv_set_field(buffer, 0, 0, "X1");
    csv_set_field(buffer, 0, 1, "X2");
    csv_set_field(buffer, 0, 2, "X3");
}

static void setup_initial_3x1_buffer(void)
{
    csv_set_field(buffer, 0, 0, "Y1");
    csv_set_field(buffer, 1, 0, "Y2");
    csv_set_field(buffer, 2, 0, "Y3");
}

void test_csv_insert_field_insert_at_end_of_row_should_append(void)
{
    setup_initial_1x3_buffer();

    int result = csv_set_field(buffer, 0, 3, "NEW");
    TEST_ASSERT_EQUAL_INT(0, result);

    TEST_ASSERT_EQUAL_INT(4, csv_get_width(buffer, 0));
    verify_field(buffer, 0, 3, "NEW");
}

void test_csv_insert_field_insert_beyond_row_width_should_append(void)
{
    setup_initial_1x3_buffer();

    int result = csv_set_field(buffer, 0, 5, "FAR");
    TEST_ASSERT_EQUAL_INT(0, result);

    TEST_ASSERT_EQUAL_INT(6, csv_get_width(buffer, 0));
    verify_field(buffer, 0, 5, "FAR");
    // Intermediate fields should be empty
    char dest[256] = {0};
    csv_get_field(dest, sizeof(dest), buffer, 0, 3);
    TEST_ASSERT_EQUAL_STRING("", dest);
    csv_get_field(dest, sizeof(dest), buffer, 0, 4);
    TEST_ASSERT_EQUAL_STRING("", dest);
}

void test_csv_insert_field_insert_in_middle_should_shift_right(void)
{
    setup_initial_1x3_buffer();

    // Insert at index 1: should shift X2 and X3 right
    int result = csv_set_field(buffer, 0, 1, "INSERTED");
    TEST_ASSERT_EQUAL_INT(0, result);

    TEST_ASSERT_EQUAL_INT(4, csv_get_width(buffer, 0));
    verify_field(buffer, 0, 0, "X1");
    verify_field(buffer, 0, 1, "INSERTED");
    verify_field(buffer, 0, 2, "X2");
    verify_field(buffer, 0, 3, "X3");
}

void test_csv_insert_field_insert_at_start_should_shift_all(void)
{
    setup_initial_1x3_buffer();

    int result = csv_set_field(buffer, 0, 0, "FIRST");
    TEST_ASSERT_EQUAL_INT(0, result);

    TEST_ASSERT_EQUAL_INT(4, csv_get_width(buffer, 0));
    verify_field(buffer, 0, 0, "FIRST");
    verify_field(buffer, 0, 1, "X1");
    verify_field(buffer, 0, 2, "X2");
    verify_field(buffer, 0, 3, "X3");
}

void test_csv_insert_field_insert_in_empty_row_should_create_field(void)
{
    csv_set_field(buffer, 0, 0, "Initial");
    csv_clear_row(buffer, 1); // Ensure row 1 exists but is empty (width=1, cleared)
    // Now row 1 has 1 field (cleared), so width=1

    int result = csv_set_field(buffer, 1, 0, "NEW_ROW_FIELD");
    TEST_ASSERT_EQUAL_INT(0, result);

    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buffer, 1));
    verify_field(buffer, 1, 0, "NEW_ROW_FIELD");
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_csv_insert_field_insert_at_end_of_row_should_append);
    RUN_TEST(test_csv_insert_field_insert_beyond_row_width_should_append);
    RUN_TEST(test_csv_insert_field_insert_in_middle_should_shift_right);
    RUN_TEST(test_csv_insert_field_insert_at_start_should_shift_all);
    RUN_TEST(test_csv_insert_field_insert_in_empty_row_should_create_field);

    return UNITY_END();
}