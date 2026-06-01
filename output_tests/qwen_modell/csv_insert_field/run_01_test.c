#include "csv.c"
#include "unity.h"
#include "csv.h"
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>

/* Global variables for signal handling */
static sig_atomic_t segv_caught = 0;
static sigjmp_buf jump_buffer;

/* Signal handler for SIGSEGV */
static void segv_handler(int sig) {
    (void)sig;
    segv_caught = 1;
    siglongjmp(jump_buffer, 1);
}

/* Global buffer for test fixtures */
static CSV_BUFFER *buffer = NULL;

/* setUp and tearDown must be non-static per requirements */
void setUp(void) {
    /* Install SIGSEGV handler */
    struct sigaction sa;
    sa.sa_handler = segv_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGSEGV, &sa, NULL);

    /* Create buffer */
    buffer = csv_create_buffer();
    TEST_ASSERT_NOT_NULL(buffer);
}

void tearDown(void) {
    /* Restore default SIGSEGV handler */
    struct sigaction sa;
    sa.sa_handler = SIG_DFL;
    sigaction(SIGSEGV, &sa, NULL);

    /* Clean up buffer */
    if (buffer != NULL) {
        csv_destroy_buffer(buffer);
        buffer = NULL;
    }
    segv_caught = 0;
}

/* Helper to safely run function under test with SIGSEGV protection */
static int run_with_segv_protection(void (*func)(void)) {
    segv_caught = 0;
    int result = sigsetjmp(jump_buffer, 1);
    if (result == 0) {
        func();
        return segv_caught ? -1 : 0;
    } else {
        return -1; /* SIGSEGV caught */
    }
}

/* Helper to set up a row with specific width */
static void setup_row_with_fields(size_t row, size_t width, const char *prefix) {
    /* Ensure row exists */
    while (csv_get_height(buffer) <= (int)row) {
        TEST_ASSERT_EQUAL_INT(0, append_row(buffer));
    }
    
    /* Set fields */
    for (size_t i = 0; i < width; i++) {
        char field_text[32];
        snprintf(field_text, sizeof(field_text), "%s_%zu", prefix, i);
        TEST_ASSERT_EQUAL_INT(0, csv_set_field(buffer, row, i, field_text));
    }
}

/* Test case 1: Insert at existing position (middle of row) */
static void test_csv_insert_field_insert_middle(void) {
    /* Setup: row 0 with 3 fields: ["A", "B", "C"] */
    setup_row_with_fields(0, 3, "field");
    
    /* Insert "X" at position 1 */
    int result = run_with_segv_protection([]() {
        csv_insert_field(buffer, 0, 1, "X");
    });
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_FALSE(segv_caught);
    
    /* Verify: row should now have 4 fields: ["A", "X", "B", "C"] */
    TEST_ASSERT_EQUAL_INT(4, csv_get_width(buffer, 0));
    
    char dest[64];
    TEST_ASSERT_EQUAL_INT(0, csv_get_field(dest, sizeof(dest), buffer, 0, 0));
    TEST_ASSERT_EQUAL_STRING("field_0", dest);
    
    TEST_ASSERT_EQUAL_INT(0, csv_get_field(dest, sizeof(dest), buffer, 0, 1));
    TEST_ASSERT_EQUAL_STRING("X", dest);
    
    TEST_ASSERT_EQUAL_INT(0, csv_get_field(dest, sizeof(dest), buffer, 0, 2));
    TEST_ASSERT_EQUAL_STRING("field_1", dest);
    
    TEST_ASSERT_EQUAL_INT(0, csv_get_field(dest, sizeof(dest), buffer, 0, 3));
    TEST_ASSERT_EQUAL_STRING("field_2", dest);
}

/* Test case 2: Insert at beginning of row */
static void test_csv_insert_field_insert_beginning(void) {
    /* Setup: row 0 with 2 fields: ["A", "B"] */
    setup_row_with_fields(0, 2, "field");
    
    /* Insert "FIRST" at position 0 */
    int result = run_with_segv_protection([]() {
        csv_insert_field(buffer, 0, 0, "FIRST");
    });
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_FALSE(segv_caught);
    
    /* Verify: row should now have 3 fields: ["FIRST", "A", "B"] */
    TEST_ASSERT_EQUAL_INT(3, csv_get_width(buffer, 0));
    
    char dest[64];
    TEST_ASSERT_EQUAL_INT(0, csv_get_field(dest, sizeof(dest), buffer, 0, 0));
    TEST_ASSERT_EQUAL_STRING("FIRST", dest);
    
    TEST_ASSERT_EQUAL_INT(0, csv_get_field(dest, sizeof(dest), buffer, 0, 1));
    TEST_ASSERT_EQUAL_STRING("field_0", dest);
    
    TEST_ASSERT_EQUAL_INT(0, csv_get_field(dest, sizeof(dest), buffer, 0, 2));
    TEST_ASSERT_EQUAL_STRING("field_1", dest);
}

/* Test case 3: Insert at end of row (should behave like append) */
static void test_csv_insert_field_insert_end(void) {
    /* Setup: row 0 with 2 fields: ["A", "B"] */
    setup_row_with_fields(0, 2, "field");
    
    /* Insert "LAST" at position 2 (end) */
    int result = run_with_segv_protection([]() {
        csv_insert_field(buffer, 0, 2, "LAST");
    });
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_FALSE(segv_caught);
    
    /* Verify: row should now have 3 fields: ["A", "B", "LAST"] */
    TEST_ASSERT_EQUAL_INT(3, csv_get_width(buffer, 0));
    
    char dest[64];
    TEST_ASSERT_EQUAL_INT(0, csv_get_field(dest, sizeof(dest), buffer, 0, 0));
    TEST_ASSERT_EQUAL_STRING("field_0", dest);
    
    TEST_ASSERT_EQUAL_INT(0, csv_get_field(dest, sizeof(dest), buffer, 0, 1));
    TEST_ASSERT_EQUAL_STRING("field_1", dest);
    
    TEST_ASSERT_EQUAL_INT(0, csv_get_field(dest, sizeof(dest), buffer, 0, 2));
    TEST_ASSERT_EQUAL_STRING("LAST", dest);
}

/* Test case 4: Insert beyond existing row (should create row and set field) */
static void test_csv_insert_field_new_row(void) {
    /* Setup: row 0 with 1 field */
    setup_row_with_fields(0, 1, "existing");
    
    /* Insert "NEW" at row 1, entry 0 (new row) */
    int result = run_with_segv_protection([]() {
        csv_insert_field(buffer, 1, 0, "NEW");
    });
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_FALSE(segv_caught);
    
    /* Verify: buffer should have 2 rows */
    TEST_ASSERT_EQUAL_INT(2, csv_get_height(buffer));
    
    /* Verify new row has 1 field */
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buffer, 1));
    
    char dest[64];
    TEST_ASSERT_EQUAL_INT(0, csv_get_field(dest, sizeof(dest), buffer, 1, 0));
    TEST_ASSERT_EQUAL_STRING("NEW", dest);
}

/* Test case 5: Insert beyond existing entry in existing row (should set field) */
static void test_csv_insert_field_new_entry(void) {
    /* Setup: row 0 with 2 fields: ["A", "B"] */
    setup_row_with_fields(0, 2, "field");
    
    /* Insert "NEW" at position 5 (beyond current width=2) */
    int result = run_with_segv_protection([]() {
        csv_insert_field(buffer, 0, 5, "NEW");
    });
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_FALSE(segv_caught);
    
    /* Verify: row should now have 6 fields (positions 0-5), with empty fields in between */
    TEST_ASSERT_EQUAL_INT(6, csv_get_width(buffer, 0));
    
    char dest[64];
    TEST_ASSERT_EQUAL_INT(0, csv_get_field(dest, sizeof(dest), buffer, 0, 0));
    TEST_ASSERT_EQUAL_STRING("field_0", dest);
    
    TEST_ASSERT_EQUAL_INT(0, csv_get_field(dest, sizeof(dest), buffer, 0, 1));
    TEST_ASSERT_EQUAL_STRING("field_1", dest);
    
    /* Positions 2-4 should be empty */
    for (size_t i = 2; i < 5; i++) {
        TEST_ASSERT_EQUAL_INT(2, csv_get_field(dest, sizeof(dest), buffer, 0, i));
    }
    
    /* Position 5 should be "NEW" */
    TEST_ASSERT_EQUAL_INT(0, csv_get_field(dest, sizeof(dest), buffer, 0, 5));
    TEST_ASSERT_EQUAL_STRING("NEW", dest);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_csv_insert_field_insert_middle);
    RUN_TEST(test_csv_insert_field_insert_beginning);
    RUN_TEST(test_csv_insert_field_insert_end);
    RUN_TEST(test_csv_insert_field_new_row);
    RUN_TEST(test_csv_insert_field_new_entry);
    return UNITY_END();
}