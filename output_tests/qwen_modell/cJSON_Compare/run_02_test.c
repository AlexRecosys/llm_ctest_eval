#include "cJSON.c"
#include "unity.h"
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>
#include <math.h>

/* Global variables for test fixtures */
static cJSON *test_item_a = NULL;
static cJSON *test_item_b = NULL;

/* Signal handling for segmentation fault detection */
static sig_atomic_t segv_caught = 0;
static jmp_buf segv_env;

static void segv_handler(int sig) {
    (void)sig;
    segv_caught = 1;
    longjmp(segv_env, 1);
}

/* Helper function to safely run code that might segfault */
static int run_with_segvguard(void (*func)(void)) {
    segv_caught = 0;
    void (*old_handler)(int) = signal(SIGSEGV, segv_handler);
    
    int result = setjmp(segv_env);
    if (result == 0) {
        func();
    }
    
    signal(SIGSEGV, old_handler);
    return segv_caught;
}

/* Helper function to create cJSON objects for testing */
static cJSON* create_test_number(double value) {
    cJSON *item = cJSON_CreateNumber(value);
    TEST_ASSERT_NOT_NULL_MESSAGE(item, "Failed to create number item");
    return item;
}

static cJSON* create_test_string(const char *str) {
    cJSON *item = cJSON_CreateString(str);
    TEST_ASSERT_NOT_NULL_MESSAGE(item, "Failed to create string item");
    return item;
}

static cJSON* create_test_array(void) {
    cJSON *item = cJSON_CreateArray();
    TEST_ASSERT_NOT_NULL_MESSAGE(item, "Failed to create array item");
    return item;
}

static cJSON* create_test_object(void) {
    cJSON *item = cJSON_CreateObject();
    TEST_ASSERT_NOT_NULL_MESSAGE(item, "Failed to create object item");
    return item;
}

static cJSON* create_test_true(void) {
    cJSON *item = cJSON_CreateTrue();
    TEST_ASSERT_NOT_NULL_MESSAGE(item, "Failed to create true item");
    return item;
}

static cJSON* create_test_false(void) {
    cJSON *item = cJSON_CreateFalse();
    TEST_ASSERT_NOT_NULL_MESSAGE(item, "Failed to create false item");
    return item;
}

static cJSON* create_test_null(void) {
    cJSON *item = cJSON_CreateNull();
    TEST_ASSERT_NOT_NULL_MESSAGE(item, "Failed to create null item");
    return item;
}

/* Setup function */
void setUp(void) {
    test_item_a = NULL;
    test_item_b = NULL;
}

/* Teardown function */
void tearDown(void) {
    if (test_item_a != NULL) {
        cJSON_Delete(test_item_a);
        test_item_a = NULL;
    }
    if (test_item_b != NULL) {
        cJSON_Delete(test_item_b);
        test_item_b = NULL;
    }
}

/* Test case 1: Test identical simple objects */
static void test_cJSON_Compare_identical_simple_objects(void) {
    test_item_a = create_test_number(42.5);
    test_item_b = create_test_number(42.5);
    
    TEST_ASSERT_TRUE(cJSON_Compare(test_item_a, test_item_b, cJSON_True));
}

/* Test case 2: Test different numbers */
static void test_cJSON_Compare_different_numbers(void) {
    test_item_a = create_test_number(42.5);
    test_item_b = create_test_number(43.5);
    
    TEST_ASSERT_FALSE(cJSON_Compare(test_item_a, test_item_b, cJSON_True));
}

/* Test case 3: Test identical arrays */
static void test_cJSON_Compare_identical_arrays(void) {
    test_item_a = create_test_array();
    test_item_b = create_test_array();
    
    cJSON_AddItemToArray(test_item_a, create_test_string("hello"));
    cJSON_AddItemToArray(test_item_a, create_test_number(123));
    cJSON_AddItemToArray(test_item_a, create_test_true());
    
    cJSON_AddItemToArray(test_item_b, create_test_string("hello"));
    cJSON_AddItemToArray(test_item_b, create_test_number(123));
    cJSON_AddItemToArray(test_item_b, create_test_true());
    
    TEST_ASSERT_TRUE(cJSON_Compare(test_item_a, test_item_b, cJSON_True));
}

/* Test case 4: Test identical objects with case-sensitive keys */
static void test_cJSON_Compare_identical_objects_case_sensitive(void) {
    test_item_a = create_test_object();
    test_item_b = create_test_object();
    
    cJSON_AddStringToObject(test_item_a, "key", "value");
    cJSON_AddNumberToObject(test_item_a, "count", 42);
    
    cJSON_AddStringToObject(test_item_b, "key", "value");
    cJSON_AddNumberToObject(test_item_b, "count", 42);
    
    TEST_ASSERT_TRUE(cJSON_Compare(test_item_a, test_item_b, cJSON_True));
}

/* Test case 5: Test NULL and invalid inputs */
static void test_cJSON_Compare_null_and_invalid_inputs(void) {
    test_item_a = create_test_number(1.0);
    
    /* NULL vs non-NULL */
    TEST_ASSERT_FALSE(cJSON_Compare(test_item_a, NULL, cJSON_True));
    TEST_ASSERT_FALSE(cJSON_Compare(NULL, test_item_a, cJSON_True));
    TEST_ASSERT_FALSE(cJSON_Compare(NULL, NULL, cJSON_True));
    
    /* Different types */
    cJSON *string_item = create_test_string("test");
    TEST_ASSERT_FALSE(cJSON_Compare(test_item_a, string_item, cJSON_True));
    
    /* Segfault protection */
    void (*segv_func)(void) = NULL;
    
    /* Test with invalid type (simulated by corrupting type field) */
    test_item_a = create_test_number(1.0);
    test_item_b = create_test_number(1.0);
    
    /* Save original type */
    int orig_type_a = test_item_a->type;
    int orig_type_b = test_item_b->type;
    
    /* Set invalid type */
    test_item_a->type = 0xFF;
    
    /* Run with segfault guard */
    segv_func = NULL; /* No segfault expected here */
    TEST_ASSERT_FALSE(cJSON_Compare(test_item_a, test_item_b, cJSON_True));
    
    /* Restore types */
    test_item_a->type = orig_type_a;
    test_item_b->type = orig_type_b;
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_cJSON_Compare_identical_simple_objects);
    RUN_TEST(test_cJSON_Compare_different_numbers);
    RUN_TEST(test_cJSON_Compare_identical_arrays);
    RUN_TEST(test_cJSON_Compare_identical_objects_case_sensitive);
    RUN_TEST(test_cJSON_Compare_null_and_invalid_inputs);
    
    return UNITY_END();
}