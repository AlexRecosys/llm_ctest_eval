#include "csv.c"
#include "unity.h"
#include "csv.h"
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>

static CSV_BUFFER *buffer = NULL;
static jmp_buf jump_buffer;
static volatile sig_atomic_t segv_caught = 0;

static void segv_handler(int sig) {
    (void)sig;
    segv_caught = 1;
    longjmp(jump_buffer, 1);
}

void setUp(void) {
    segv_caught = 0;
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

static void setup_buffer_with_data(void) {
    csv_set_field(buffer, 0, 0, "A1");
    csv_set_field(buffer, 0, 1, "B1");
    csv_set_field(buffer, 0, 2, "C1");
    csv_set_field(buffer, 1, 0, "A2");
    csv_set_field(buffer, 1, 1, "B2");
}

static void setup_buffer_with_one_field(void) {
    csv_set_field(buffer, 0, 0, "OnlyOne");
}

static void setup_buffer_with_empty_row(void) {
    csv_set_field(buffer, 0, 0, "A1");
    csv_set_field(buffer, 1, 0, "B1");
}

static int run_with_segv_protection(void (*func)(void)) {
    segv_caught = 0;
    int result = setjmp(jump_buffer);
    if (result == 0) {
        func();
    }
    return segv_caught;
}

static void test_csv_insert_field_insert_at_end_of_row(void) {
    setup_buffer_with_one_field();
    
    int ret = csv_insert_field(buffer, 0, 1, "NewField");
    TEST_ASSERT_EQUAL_INT(0, ret);
    
    char dest[64];
    csv_get_field(dest, sizeof(dest), buffer, 0, 0);
    TEST_ASSERT_EQUAL_STRING("OnlyOne", dest);
    
    csv_get_field(dest, sizeof(dest), buffer, 0, 1);
    TEST_ASSERT_EQUAL_STRING("NewField", dest);
    
    TEST_ASSERT_EQUAL_INT(2, csv_get_width(buffer, 0));
}

static void test_csv_insert_field_insert_in_middle_of_row(void) {
    setup_buffer_with_data();
    
    int ret = csv_insert_field(buffer, 0, 1, "Inserted");
    TEST_ASSERT_EQUAL_INT(0, ret);
    
    char dest[64];
    csv_get_field(dest, sizeof(dest), buffer, 0, 0);
    TEST_ASSERT_EQUAL_STRING("A1", dest);
    
    csv_get_field(dest, sizeof(dest), buffer, 0, 1);
    TEST_ASSERT_EQUAL_STRING("Inserted", dest);
    
    csv_get_field(dest, sizeof(dest), buffer, 0, 2);
    TEST_ASSERT_EQUAL_STRING("B1", dest);
    
    csv_get_field(dest, sizeof(dest), buffer, 0, 3);
    TEST_ASSERT_EQUAL_STRING("C1", dest);
    
    TEST_ASSERT_EQUAL_INT(4, csv_get_width(buffer, 0));
}

static void test_csv_insert_field_insert_at_beginning_of_row(void) {
    setup_buffer_with_data();
    
    int ret = csv_insert_field(buffer, 0, 0, "First");
    TEST_ASSERT_EQUAL_INT(0, ret);
    
    char dest[64];
    csv_get_field(dest, sizeof(dest), buffer, 0, 0);
    TEST_ASSERT_EQUAL_STRING("First", dest);
    
    csv_get_field(dest, sizeof(dest), buffer, 0, 1);
    TEST_ASSERT_EQUAL_STRING("A1", dest);
    
    csv_get_field(dest, sizeof(dest), buffer, 0, 2);
    TEST_ASSERT_EQUAL_STRING("B1", dest);
    
    csv_get_field(dest, sizeof(dest), buffer, 0, 3);
    TEST_ASSERT_EQUAL_STRING("C1", dest);
    
    TEST_ASSERT_EQUAL_INT(4, csv_get_width(buffer, 0));
}

static void test_csv_insert_field_insert_beyond_end_creates_new_field(void) {
    setup_buffer_with_one_field();
    
    int ret = csv_insert_field(buffer, 0, 5, "NewField");
    TEST_ASSERT_EQUAL_INT(0, ret);
    
    char dest[64];
    csv_get_field(dest, sizeof(dest), buffer, 0, 0);
    TEST_ASSERT_EQUAL_STRING("OnlyOne", dest);
    
    csv_get_field(dest, sizeof(dest), buffer, 0, 5);
    TEST_ASSERT_EQUAL_STRING("NewField", dest);
    
    TEST_ASSERT_EQUAL_INT(6, csv_get_width(buffer, 0));
}

static void test_csv_insert_field_insert_into_new_row(void) {
    setup_buffer_with_one_field();
    
    int ret = csv_insert_field(buffer, 1, 0, "Row1Field0");
    TEST_ASSERT_EQUAL_INT(0, ret);
    
    char dest[64];
    csv_get_field(dest, sizeof(dest), buffer, 0, 0);
    TEST_ASSERT_EQUAL_STRING("OnlyOne", dest);
    
    csv_get_field(dest, sizeof(dest), buffer, 1, 0);
    TEST_ASSERT_EQUAL_STRING("Row1Field0", dest);
    
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buffer, 0));
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buffer, 1));
    TEST_ASSERT_EQUAL_INT(2, csv_get_height(buffer));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_csv_insert_field_insert_at_end_of_row);
    RUN_TEST(test_csv_insert_field_insert_in_middle_of_row);
    RUN_TEST(test_csv_insert_field_insert_at_beginning_of_row);
    RUN_TEST(test_csv_insert_field_insert_beyond_end_creates_new_field);
    RUN_TEST(test_csv_insert_field_insert_into_new_row);
    return UNITY_END();
}