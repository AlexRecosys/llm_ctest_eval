#include "csv.c"
#include "unity.h"
#include "csv.h"
#include <stdlib.h>
#include <string.h>
#include <signal.h>

/* File-scope fixture */
static CSV_BUFFER *g_buffer;

/* SIGSEGV handler */
static void sigsegv_handler(int sig)
{
    (void)sig;
    TEST_FAIL_MESSAGE("Segmentation fault (SIGSEGV) caught during test");
}

void setUp(void)
{
    signal(SIGSEGV, sigsegv_handler);
    g_buffer = csv_create_buffer();
}

void tearDown(void)
{
    if (g_buffer != NULL) {
        csv_destroy_buffer(g_buffer);
        g_buffer = NULL;
    }
    signal(SIGSEGV, SIG_DFL);
}

/* Test 1: Set field delimiter to a comma (default value) */
void test_csv_set_field_delim_comma(void)
{
    csv_set_field_delim(g_buffer, ',');
    TEST_ASSERT_EQUAL_INT(',', g_buffer->field_delim);
}

/* Test 2: Set field delimiter to a semicolon */
void test_csv_set_field_delim_semicolon(void)
{
    csv_set_field_delim(g_buffer, ';');
    TEST_ASSERT_EQUAL_INT(';', g_buffer->field_delim);
}

/* Test 3: Set field delimiter to a tab character */
void test_csv_set_field_delim_tab(void)
{
    csv_set_field_delim(g_buffer, '\t');
    TEST_ASSERT_EQUAL_INT('\t', g_buffer->field_delim);
}

/* Test 4: Set field delimiter to a pipe character */
void test_csv_set_field_delim_pipe(void)
{
    csv_set_field_delim(g_buffer, '|');
    TEST_ASSERT_EQUAL_INT('|', g_buffer->field_delim);
}

/* Test 5: Overwrite an existing delimiter with a new one */
void test_csv_set_field_delim_overwrite(void)
{
    /* First set to semicolon */
    csv_set_field_delim(g_buffer, ';');
    TEST_ASSERT_EQUAL_INT_MESSAGE(';', g_buffer->field_delim,
        "Expected field_delim to be ';' after first set");

    /* Then overwrite with colon */
    csv_set_field_delim(g_buffer, ':');
    TEST_ASSERT_EQUAL_INT_MESSAGE(':', g_buffer->field_delim,
        "Expected field_delim to be ':' after overwrite");
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_csv_set_field_delim_comma);
    RUN_TEST(test_csv_set_field_delim_semicolon);
    RUN_TEST(test_csv_set_field_delim_tab);
    RUN_TEST(test_csv_set_field_delim_pipe);
    RUN_TEST(test_csv_set_field_delim_overwrite);
    return UNITY_END();
}