#include "csv.c"
#include "unity.h"

static CSV_BUFFER *buffer = NULL;

void setUp(void) {
    buffer = csv_create_buffer();
    TEST_ASSERT_NOT_NULL(buffer);
}

void tearDown(void) {
    csv_destroy_buffer(buffer);
    buffer = NULL;
}

static void add_row_with_fields(CSV_BUFFER *buf, size_t num_fields, ...) {
    va_list args;
    size_t i;

    append_row(buf);
    va_start(args, num_fields);
    for (i = 0; i < num_fields; i++) {
        char *field_text = va_arg(args, char *);
        if (field_text != NULL) {
            append_field(buf, buf->rows - 1);
            set_field(&buf->field[buf->rows - 1][i], field_text);
        }
    }
    va_end(args);
}

static void fill_buffer_with_rows(CSV_BUFFER *buf, size_t num_rows, size_t fields_per_row) {
    size_t i, j;
    char field_text[32];

    for (i = 0; i < num_rows; i++) {
        append_row(buf);
        for (j = 0; j < fields_per_row; j++) {
            append_field(buf, i);
            snprintf(field_text, sizeof(field_text), "R%zuF%zu", i, j);
            set_field(&buf->field[i][j], field_text);
        }
    }
}

void test_csv_remove_row_valid_middle_row(void) {
    fill_buffer_with_rows(buffer, 5, 3);

    int result = csv_remove_row(buffer, 2);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_SIZE(4, buffer->rows);
    TEST_ASSERT_EQUAL_SIZE(3, buffer->width[0]);
    TEST_ASSERT_EQUAL_SIZE(3, buffer->width[1]);
    TEST_ASSERT_EQUAL_SIZE(3, buffer->width[2]);
    TEST_ASSERT_EQUAL_SIZE(3, buffer->width[3]);

    char field[64];
    csv_get_field(field, sizeof(field), buffer, 0, 0);
    TEST_ASSERT_EQUAL_STRING("R0F0", field);
    csv_get_field(field, sizeof(field), buffer, 1, 0);
    TEST_ASSERT_EQUAL_STRING("R1F0", field);
    csv_get_field(field, sizeof(field), buffer, 2, 0);
    TEST_ASSERT_EQUAL_STRING("R3F0", field);
    csv_get_field(field, sizeof(field), buffer, 3, 0);
    TEST_ASSERT_EQUAL_STRING("R4F0", field);
}

void test_csv_remove_row_first_row(void) {
    fill_buffer_with_rows(buffer, 4, 2);

    int result = csv_remove_row(buffer, 0);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_SIZE(3, buffer->rows);

    char field[64];
    csv_get_field(field, sizeof(field), buffer, 0, 0);
    TEST_ASSERT_EQUAL_STRING("R1F0", field);
    csv_get_field(field, sizeof(field), buffer, 1, 0);
    TEST_ASSERT_EQUAL_STRING("R2F0", field);
    csv_get_field(field, sizeof(field), buffer, 2, 0);
    TEST_ASSERT_EQUAL_STRING("R3F0", field);
}

void test_csv_remove_row_last_row(void) {
    fill_buffer_with_rows(buffer, 3, 2);

    int result = csv_remove_row(buffer, 2);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_SIZE(2, buffer->rows);

    char field[64];
    csv_get_field(field, sizeof(field), buffer, 1, 1);
    TEST_ASSERT_EQUAL_STRING("R1F1", field);
}

void test_csv_remove_row_out_of_bounds_high(void) {
    fill_buffer_with_rows(buffer, 3, 2);

    int result = csv_remove_row(buffer, 5);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_SIZE(3, buffer->rows);
}

void test_csv_remove_row_on_empty_buffer(void) {
    int result = csv_remove_row(buffer, 0);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_SIZE(0, buffer->rows);
}

void test_csv_remove_row_single_row_buffer(void) {
    fill_buffer_with_rows(buffer, 1, 2);

    int result = csv_remove_row(buffer, 0);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_SIZE(0, buffer->rows);
}

void test_csv_remove_row_preserves_other_rows(void) {
    fill_buffer_with_rows(buffer, 4, 3);

    int result = csv_remove_row(buffer, 1);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_SIZE(3, buffer->rows);

    char field[64];
    csv_get_field(field, sizeof(field), buffer, 0, 2);
    TEST_ASSERT_EQUAL_STRING("R0F2", field);
    csv_get_field(field, sizeof(field), buffer, 1, 2);
    TEST_ASSERT_EQUAL_STRING("R2F2", field);
    csv_get_field(field, sizeof(field), buffer, 2, 2);
    TEST_ASSERT_EQUAL_STRING("R3F2", field);
}

int main(void) {
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_csv_remove_row_valid_middle_row);
    RUN_TEST(test_csv_remove_row_first_row);
    RUN_TEST(test_csv_remove_row_last_row);
    RUN_TEST(test_csv_remove_row_out_of_bounds_high);
    RUN_TEST(test_csv_remove_row_on_empty_buffer);
    RUN_TEST(test_csv_remove_row_single_row_buffer);
    RUN_TEST(test_csv_remove_row_preserves_other_rows);
    return UNITY_END();
}