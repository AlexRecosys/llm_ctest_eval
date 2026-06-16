#include "csv.c"
#include "unity.h"

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

void test_csv_set_field_delim_should_set_field_delim_to_new_value(void)
{
    char new_delim = ';';
    csv_set_field_delim(buffer, new_delim);
    TEST_ASSERT_EQUAL_INT(new_delim, buffer->field_delim);
}

void test_csv_set_field_delim_should_overwrite_existing_value(void)
{
    char initial_delim = ',';
    char new_delim = '|';
    
    // Verify initial default value
    TEST_ASSERT_EQUAL_INT(initial_delim, buffer->field_delim);
    
    csv_set_field_delim(buffer, new_delim);
    TEST_ASSERT_EQUAL_INT(new_delim, buffer->field_delim);
    
    // Set again to another value
    new_delim = '\t';
    csv_set_field_delim(buffer, new_delim);
    TEST_ASSERT_EQUAL_INT(new_delim, buffer->field_delim);
}

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_csv_set_field_delim_should_set_field_delim_to_new_value);
    RUN_TEST(test_csv_set_field_delim_should_overwrite_existing_value);
    return UNITY_END();
}