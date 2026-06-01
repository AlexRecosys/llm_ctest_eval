#include "csv.c"
#include "unity.h"
#include "csv.h"
#include <stdlib.h>
#include <string.h>
#include <signal.h>

/* File-scope fixture */
static CSV_BUFFER *test_buffer = NULL;

/* SIGSEGV handler */
static void sigsegv_handler(int sig)
{
    (void)sig;
    TEST_FAIL_MESSAGE("Segmentation fault (SIGSEGV) caught during test execution");
}

void setUp(void)
{
    signal(SIGSEGV, sigsegv_handler);
    test_buffer = csv_create_buffer();
}

void tearDown(void)
{
    if (test_buffer != NULL) {
        csv_destroy_buffer(test_buffer);
        test_buffer = NULL;
    }
    signal(SIGSEGV, SIG_DFL);
}

/* Test 1: Default field delimiter is ',' after csv_create_buffer */
void test_default_field_delim_is_comma(void)
{
    TEST_ASSERT_NOT_NULL_MESSAGE(test_buffer, "csv_create_buffer returned NULL");
    TEST_ASSERT_EQUAL_INT_MESSAGE(',', (int)test_buffer->field_delim,
        "Default field delimiter should be ','");
}

/* Test 2: csv_set_field_delim sets a new delimiter (semicolon) */
void test_set_field_delim_semicolon(void)
{
    TEST_ASSERT_NOT_NULL_MESSAGE(test_buffer, "csv_create_buffer returned NULL");
    csv_set_field_delim(test_buffer, ';');
    TEST_ASSERT_EQUAL_INT_MESSAGE(';', (int)test_buffer->field_delim,
        "Field delimiter should be ';' after setting");
}

/* Test 3: csv_set_field_delim sets a tab character as delimiter */
void test_set_field_delim_tab(void)
{
    TEST_ASSERT_NOT_NULL_MESSAGE(test_buffer, "csv_create_buffer returned NULL");
    csv_set_field_delim(test_buffer, '\t');
    TEST_ASSERT_EQUAL_INT_MESSAGE('\t', (int)test_buffer->field_delim,
        "Field delimiter should be '\\t' after setting");
}

/* Test 4: csv_set_field_delim can be called multiple times; last call wins */
void test_set_field_delim_overwrite(void)
{
    TEST_ASSERT_NOT_NULL_MESSAGE(test_buffer, "csv_create_buffer returned NULL");
    csv_set_field_delim(test_buffer, ';');
    TEST_ASSERT_EQUAL_INT_MESSAGE(';', (int)test_buffer->field_delim,
        "Field delimiter should be ';' after first set");

    csv_set_field_delim(test_buffer, '|');
    TEST_ASSERT_EQUAL_INT_MESSAGE('|', (int)test_buffer->field_delim,
        "Field delimiter should be '|' after second set (overwrite)");
}

/* Test 5: csv_set_field_delim does not affect text_delim */
void test_set_field_delim_does_not_change_text_delim(void)
{
    TEST_ASSERT_NOT_NULL_MESSAGE(test_buffer, "csv_create_buffer returned NULL");

    /* Record the text_delim before changing field_delim */
    char original_text_delim = test_buffer->text_delim;

    csv_set_field_delim(test_buffer, '|');

    TEST_ASSERT_EQUAL_INT_MESSAGE('|', (int)test_buffer->field_delim,
        "Field delimiter should be '|' after setting");
    TEST_ASSERT_EQUAL_INT_MESSAGE((int)original_text_delim, (int)test_buffer->text_delim,
        "text_delim should remain unchanged after csv_set_field_delim");
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_default_field_delim_is_comma);
    RUN_TEST(test_set_field_delim_semicolon);
    RUN_TEST(test_set_field_delim_tab);
    RUN_TEST(test_set_field_delim_overwrite);
    RUN_TEST(test_set_field_delim_does_not_change_text_delim);
    return UNITY_END();
}