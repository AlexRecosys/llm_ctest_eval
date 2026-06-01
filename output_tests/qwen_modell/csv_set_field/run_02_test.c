#include "csv.c"
#include "unity.h"
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>

static CSV_BUFFER *buffer = NULL;
static jmp_buf jump_buffer;

static void segv_handler(int sig) {
    (void)sig;
    longjmp(jump_buffer, 1);
}

void setUp(void) {
    signal(SIGSEGV, segv_handler);
    buffer = csv_create_buffer();
    TEST_ASSERT_NOT_NULL(buffer);
}

void tearDown(void) {
    if (buffer != NULL) {
        csv_destroy_buffer(buffer);
        buffer = NULL;
    }
    signal(SIGSEGV, SIG_DFL);
}

static void test_csv_set_field_delim_sets_delimiter(void) {
    char original = buffer->field_delim;
    char new_delim = ';';
    
    csv_set_field_delim(buffer, new_delim);
    
    TEST_ASSERT_EQUAL_CHAR_MESSAGE(new_delim, buffer->field_delim, "field_delim should be updated to new_delim");
}

static void test_csv_set_field_delim_preserves_text_delim(void) {
    char original_text_delim = buffer->text_delim;
    char new_field_delim = '|';
    
    csv_set_field_delim(buffer, new_field_delim);
    
    TEST_ASSERT_EQUAL_CHAR_MESSAGE(original_text_delim, buffer->text_delim, "text_delim should remain unchanged");
}

static void test_csv_set_field_delim_with_existing_data(void) {
    // Add some data to the buffer first
    csv_set_field(buffer, 0, 0, "test");
    
    char new_delim = ':';
    csv_set_field_delim(buffer, new_delim);
    
    TEST_ASSERT_EQUAL_CHAR_MESSAGE(new_delim, buffer->field_delim, "field_delim should be updated even with existing data");
}

static void test_csv_set_field_delim_multiple_calls(void) {
    char delims[] = {';', '|', ',', '\t', ' '};
    size_t i;
    
    for (i = 0; i < sizeof(delims)/sizeof(delims[0]); i++) {
        csv_set_field_delim(buffer, delims[i]);
        TEST_ASSERT_EQUAL_CHAR_MESSAGE(delims[i], buffer->field_delim, "field_delim should be updated on each call");
    }
}

static void test_csv_set_field_delim_null_buffer_segfault(void) {
    if (setjmp(jump_buffer) == 0) {
        csv_set_field_delim(NULL, ',');
        TEST_FAIL_MESSAGE("Should have caused segmentation fault");
    }
    // If we reach here via longjmp, test passes
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_csv_set_field_delim_sets_delimiter);
    RUN_TEST(test_csv_set_field_delim_preserves_text_delim);
    RUN_TEST(test_csv_set_field_delim_with_existing_data);
    RUN_TEST(test_csv_set_field_delim_multiple_calls);
    RUN_TEST(test_csv_set_field_delim_null_buffer_segfault);
    return UNITY_END();
}