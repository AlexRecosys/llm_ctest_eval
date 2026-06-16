#include "csv.c"
#include "unity.h"
#include "csv.h"
#include <stdlib.h>
#include <stdio.h>

static CSV_BUFFER *test_buffer;

void setUp(void)
{
    test_buffer = csv_create_buffer();
}

void tearDown(void)
{
    if (test_buffer != NULL) {
        csv_destroy_buffer(test_buffer);
        test_buffer = NULL;
    }
}

void test_csv_set_field_delim_default_is_comma(void)
{
    TEST_ASSERT_EQUAL_INT(',', test_buffer->field_delim);
}

void test_csv_set_field_delim_set_to_semicolon(void)
{
    csv_set_field_delim(test_buffer, ';');
    TEST_ASSERT_EQUAL_INT(';', test_buffer->field_delim);
}

void test_csv_set_field_delim_set_to_tab(void)
{
    csv_set_field_delim(test_buffer, '\t');
    TEST_ASSERT_EQUAL_INT('\t', test_buffer->field_delim);
}

void test_csv_set_field_delim_set_to_pipe(void)
{
    csv_set_field_delim(test_buffer, '|');
    TEST_ASSERT_EQUAL_INT('|', test_buffer->field_delim);
}

void test_csv_set_field_delim_set_to_space(void)
{
    csv_set_field_delim(test_buffer, ' ');
    TEST_ASSERT_EQUAL_INT(' ', test_buffer->field_delim);
}

void test_csv_set_field_delim_set_to_colon(void)
{
    csv_set_field_delim(test_buffer, ':');
    TEST_ASSERT_EQUAL_INT(':', test_buffer->field_delim);
}

void test_csv_set_field_delim_overwrite_previous_value(void)
{
    csv_set_field_delim(test_buffer, ';');
    TEST_ASSERT_EQUAL_INT(';', test_buffer->field_delim);
    csv_set_field_delim(test_buffer, '|');
    TEST_ASSERT_EQUAL_INT('|', test_buffer->field_delim);
}

void test_csv_set_field_delim_set_back_to_comma(void)
{
    csv_set_field_delim(test_buffer, ';');
    TEST_ASSERT_EQUAL_INT(';', test_buffer->field_delim);
    csv_set_field_delim(test_buffer, ',');
    TEST_ASSERT_EQUAL_INT(',', test_buffer->field_delim);
}

void test_csv_set_field_delim_does_not_affect_text_delim(void)
{
    char original_text_delim = test_buffer->text_delim;
    csv_set_field_delim(test_buffer, ';');
    TEST_ASSERT_EQUAL_INT(original_text_delim, test_buffer->text_delim);
}

void test_csv_set_field_delim_does_not_affect_rows(void)
{
    size_t original_rows = test_buffer->rows;
    csv_set_field_delim(test_buffer, ';');
    TEST_ASSERT_EQUAL_INT((int)original_rows, (int)test_buffer->rows);
}

void test_csv_set_field_delim_set_to_null_char(void)
{
    csv_set_field_delim(test_buffer, '\0');
    TEST_ASSERT_EQUAL_INT('\0', test_buffer->field_delim);
}

void test_csv_set_field_delim_set_to_newline(void)
{
    csv_set_field_delim(test_buffer, '\n');
    TEST_ASSERT_EQUAL_INT('\n', test_buffer->field_delim);
}

void test_csv_set_field_delim_multiple_buffers_independent(void)
{
    CSV_BUFFER *buffer2 = csv_create_buffer();
    TEST_ASSERT_NOT_NULL(buffer2);

    csv_set_field_delim(test_buffer, ';');
    csv_set_field_delim(buffer2, '|');

    TEST_ASSERT_EQUAL_INT(';', test_buffer->field_delim);
    TEST_ASSERT_EQUAL_INT('|', buffer2->field_delim);

    csv_destroy_buffer(buffer2);
}

void test_csv_set_field_delim_set_to_hash(void)
{
    csv_set_field_delim(test_buffer, '#');
    TEST_ASSERT_EQUAL_INT('#', test_buffer->field_delim);
}

void test_csv_set_field_delim_set_to_tilde(void)
{
    csv_set_field_delim(test_buffer, '~');
    TEST_ASSERT_EQUAL_INT('~', test_buffer->field_delim);
}

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_csv_set_field_delim_default_is_comma);
    RUN_TEST(test_csv_set_field_delim_set_to_semicolon);
    RUN_TEST(test_csv_set_field_delim_set_to_tab);
    RUN_TEST(test_csv_set_field_delim_set_to_pipe);
    RUN_TEST(test_csv_set_field_delim_set_to_space);
    RUN_TEST(test_csv_set_field_delim_set_to_colon);
    RUN_TEST(test_csv_set_field_delim_overwrite_previous_value);
    RUN_TEST(test_csv_set_field_delim_set_back_to_comma);
    RUN_TEST(test_csv_set_field_delim_does_not_affect_text_delim);
    RUN_TEST(test_csv_set_field_delim_does_not_affect_rows);
    RUN_TEST(test_csv_set_field_delim_set_to_null_char);
    RUN_TEST(test_csv_set_field_delim_set_to_newline);
    RUN_TEST(test_csv_set_field_delim_multiple_buffers_independent);
    RUN_TEST(test_csv_set_field_delim_set_to_hash);
    RUN_TEST(test_csv_set_field_delim_set_to_tilde);
    return UNITY_END();
}