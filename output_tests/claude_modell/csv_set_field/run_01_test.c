#include "csv.c"
#include "unity.h"
#include "csv.h"
#include <stdlib.h>
#include <stdio.h>

static CSV_BUFFER *buffer;

void setUp(void)
{
    buffer = csv_create_buffer();
}

void tearDown(void)
{
    if (buffer != NULL) {
        csv_destroy_buffer(buffer);
        buffer = NULL;
    }
}

void test_csv_set_field_delim_default_is_comma(void)
{
    TEST_ASSERT_EQUAL_INT(',', buffer->field_delim);
}

void test_csv_set_field_delim_sets_semicolon(void)
{
    csv_set_field_delim(buffer, ';');
    TEST_ASSERT_EQUAL_INT(';', buffer->field_delim);
}

void test_csv_set_field_delim_sets_tab(void)
{
    csv_set_field_delim(buffer, '\t');
    TEST_ASSERT_EQUAL_INT('\t', buffer->field_delim);
}

void test_csv_set_field_delim_sets_pipe(void)
{
    csv_set_field_delim(buffer, '|');
    TEST_ASSERT_EQUAL_INT('|', buffer->field_delim);
}

void test_csv_set_field_delim_sets_space(void)
{
    csv_set_field_delim(buffer, ' ');
    TEST_ASSERT_EQUAL_INT(' ', buffer->field_delim);
}

void test_csv_set_field_delim_sets_colon(void)
{
    csv_set_field_delim(buffer, ':');
    TEST_ASSERT_EQUAL_INT(':', buffer->field_delim);
}

void test_csv_set_field_delim_sets_newline(void)
{
    csv_set_field_delim(buffer, '\n');
    TEST_ASSERT_EQUAL_INT('\n', buffer->field_delim);
}

void test_csv_set_field_delim_sets_null_char(void)
{
    csv_set_field_delim(buffer, '\0');
    TEST_ASSERT_EQUAL_INT('\0', buffer->field_delim);
}

void test_csv_set_field_delim_overwrite_previous_value(void)
{
    csv_set_field_delim(buffer, ';');
    TEST_ASSERT_EQUAL_INT(';', buffer->field_delim);
    csv_set_field_delim(buffer, '|');
    TEST_ASSERT_EQUAL_INT('|', buffer->field_delim);
}

void test_csv_set_field_delim_does_not_affect_text_delim(void)
{
    char original_text_delim = buffer->text_delim;
    csv_set_field_delim(buffer, ';');
    TEST_ASSERT_EQUAL_INT(original_text_delim, buffer->text_delim);
}

void test_csv_set_field_delim_does_not_affect_rows(void)
{
    size_t original_rows = buffer->rows;
    csv_set_field_delim(buffer, ';');
    TEST_ASSERT_EQUAL_INT((int)original_rows, (int)buffer->rows);
}

void test_csv_set_field_delim_sets_back_to_comma(void)
{
    csv_set_field_delim(buffer, ';');
    TEST_ASSERT_EQUAL_INT(';', buffer->field_delim);
    csv_set_field_delim(buffer, ',');
    TEST_ASSERT_EQUAL_INT(',', buffer->field_delim);
}

void test_csv_set_field_delim_sets_printable_ascii(void)
{
    csv_set_field_delim(buffer, '!');
    TEST_ASSERT_EQUAL_INT('!', buffer->field_delim);
}

void test_csv_set_field_delim_sets_digit_char(void)
{
    csv_set_field_delim(buffer, '1');
    TEST_ASSERT_EQUAL_INT('1', buffer->field_delim);
}

void test_csv_set_field_delim_sets_letter_char(void)
{
    csv_set_field_delim(buffer, 'x');
    TEST_ASSERT_EQUAL_INT('x', buffer->field_delim);
}

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_csv_set_field_delim_default_is_comma);
    RUN_TEST(test_csv_set_field_delim_sets_semicolon);
    RUN_TEST(test_csv_set_field_delim_sets_tab);
    RUN_TEST(test_csv_set_field_delim_sets_pipe);
    RUN_TEST(test_csv_set_field_delim_sets_space);
    RUN_TEST(test_csv_set_field_delim_sets_colon);
    RUN_TEST(test_csv_set_field_delim_sets_newline);
    RUN_TEST(test_csv_set_field_delim_sets_null_char);
    RUN_TEST(test_csv_set_field_delim_overwrite_previous_value);
    RUN_TEST(test_csv_set_field_delim_does_not_affect_text_delim);
    RUN_TEST(test_csv_set_field_delim_does_not_affect_rows);
    RUN_TEST(test_csv_set_field_delim_sets_back_to_comma);
    RUN_TEST(test_csv_set_field_delim_sets_printable_ascii);
    RUN_TEST(test_csv_set_field_delim_sets_digit_char);
    RUN_TEST(test_csv_set_field_delim_sets_letter_char);
    return UNITY_END();
}