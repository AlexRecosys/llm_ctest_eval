#include "csv.c"
#include "unity.h"
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>

/* Global buffer for signal handling */
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

/* Non-static setUp and tearDown required by Unity */
void setUp(void) {
    /* Install SIGSEGV handler */
    struct sigaction sa;
    sa.sa_handler = segv_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGSEGV, &sa, NULL);

    buffer = csv_create_buffer();
    TEST_ASSERT_NOT_NULL(buffer);
}

void tearDown(void) {
    /* Restore default SIGSEGV handler */
    struct sigaction sa;
    sa.sa_handler = SIG_DFL;
    sigaction(SIGSEGV, &sa, NULL);

    if (buffer != NULL) {
        csv_destroy_buffer(buffer);
        buffer = NULL;
    }
}

/* Helper to safely run code that might segfault */
static int run_with_segfault_protection(void (*func)(void)) {
    segv_caught = 0;
    int ret = sigsetjmp(jump_buffer, 1);
    if (ret == 0) {
        func();
        return segv_caught;
    } else {
        /* Jumped here due to SIGSEGV */
        return 1;
    }
}

/* Helper to set up a row with specific width */
static void setup_row_with_fields(size_t row, size_t width, const char *prefix) {
    /* Ensure row exists */
    while (csv_get_height(buffer) <= (int)row) {
        append_row(buffer);
    }
    /* Clear row first */
    csv_clear_row(buffer, row);
    /* Set fields */
    for (size_t i = 0; i < width; i++) {
        char field[32];
        snprintf(field, sizeof(field), "%s_%zu", prefix, i);
        csv_set_field(buffer, row, i, field);
    }
}

/* Test 1: Insert at existing position (middle of row) */
static void test_csv_insert_field_insert_middle(void) {
    /* Setup: row 0 with 3 fields: ["A", "B", "C"] */
    setup_row_with_fields(0, 3, "field");
    
    /* Insert "X" at position 1 */
    int result = csv_insert_field(buffer, 0, 1, "X");
    TEST_ASSERT_EQUAL_INT(0, result);
    
    /* Verify: should have 4 fields: ["A", "X", "B", "C"] */
    TEST_ASSERT_EQUAL_INT(4, csv_get_width(buffer, 0));
    
    char dest[64];
    csv_get_field(dest, sizeof(dest), buffer, 0, 0);
    TEST_ASSERT_EQUAL_STRING("field_0", dest);
    
    csv_get_field(dest, sizeof(dest), buffer, 0, 1);
    TEST_ASSERT_EQUAL_STRING("X", dest);
    
    csv_get_field(dest, sizeof(dest), buffer, 0, 2);
    TEST_ASSERT_EQUAL_STRING("field_1", dest);
    
    csv_get_field(dest, sizeof(dest), buffer, 0, 3);
    TEST_ASSERT_EQUAL_STRING("field_2", dest);
}

/* Test 2: Insert at beginning of row */
static void test_csv_insert_field_insert_beginning(void) {
    /* Setup: row 0 with 2 fields: ["A", "B"] */
    setup_row_with_fields(0, 2, "field");
    
    /* Insert "START" at position 0 */
    int result = csv_insert_field(buffer, 0, 0, "START");
    TEST_ASSERT_EQUAL_INT(0, result);
    
    /* Verify: should have 3 fields: ["START", "A", "B"] */
    TEST_ASSERT_EQUAL_INT(3, csv_get_width(buffer, 0));
    
    char dest[64];
    csv_get_field(dest, sizeof(dest), buffer, 0, 0);
    TEST_ASSERT_EQUAL_STRING("START", dest);
    
    csv_get_field(dest, sizeof(dest), buffer, 0, 1);
    TEST_ASSERT_EQUAL_STRING("field_0", dest);
    
    csv_get_field(dest, sizeof(dest), buffer, 0, 2);
    TEST_ASSERT_EQUAL_STRING("field_1", dest);
}

/* Test 3: Insert at end of row (should behave like append) */
static void test_csv_insert_field_insert_end(void) {
    /* Setup: row 0 with 2 fields: ["A", "B"] */
    setup_row_with_fields(0, 2, "field");
    
    /* Insert "END" at position 2 (end of row) */
    int result = csv_insert_field(buffer, 0, 2, "END");
    TEST_ASSERT_EQUAL_INT(0, result);
    
    /* Verify: should have 3 fields: ["A", "B", "END"] */
    TEST_ASSERT_EQUAL_INT(3, csv_get_width(buffer, 0));
    
    char dest[64];
    csv_get_field(dest, sizeof(dest), buffer, 0, 0);
    TEST_ASSERT_EQUAL_STRING("field_0", dest);
    
    csv_get_field(dest, sizeof(dest), buffer, 0, 1);
    TEST_ASSERT_EQUAL_STRING("field_1", dest);
    
    csv_get_field(dest, sizeof(dest), buffer, 0, 2);
    TEST_ASSERT_EQUAL_STRING("END", dest);
}

/* Test 4: Insert beyond existing row (should create new row and field) */
static void test_csv_insert_field_new_row(void) {
    /* Setup: row 0 with 1 field */
    setup_row_with_fields(0, 1, "A");
    
    /* Insert "NEW" at row 1, entry 0 (new row) */
    int result = csv_insert_field(buffer, 1, 0, "NEW");
    TEST_ASSERT_EQUAL_INT(0, result);
    
    /* Verify: should have 2 rows, row 1 has 1 field */
    TEST_ASSERT_EQUAL_INT(2, csv_get_height(buffer));
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buffer, 1));
    
    char dest[64];
    csv_get_field(dest, sizeof(dest), buffer, 1, 0);
    TEST_ASSERT_EQUAL_STRING("NEW", dest);
}

/* Test 5: Insert beyond existing entry in existing row (should extend row) */
static void test_csv_insert_field_extend_row(void) {
    /* Setup: row 0 with 2 fields: ["A", "B"] */
    setup_row_with_fields(0, 2, "field");
    
    /* Insert "X" at position 5 (beyond current width=2) */
    int result = csv_insert_field(buffer, 0, 5, "X");
    TEST_ASSERT_EQUAL_INT(0, result);
    
    /* Verify: row should be extended to width 6, with empty fields in between */
    TEST_ASSERT_EQUAL_INT(6, csv_get_width(buffer, 0));
    
    char dest[64];
    csv_get_field(dest, sizeof(dest), buffer, 0, 0);
    TEST_ASSERT_EQUAL_STRING("field_0", dest);
    
    csv_get_field(dest, sizeof(dest), buffer, 0, 1);
    TEST_ASSERT_EQUAL_STRING("field_1", dest);
    
    /* Positions 2-4 should be empty */
    csv_get_field(dest, sizeof(dest), buffer, 0, 2);
    TEST_ASSERT_EQUAL_STRING("", dest);
    
    csv_get_field(dest, sizeof(dest), buffer, 0, 3);
    TEST_ASSERT_EQUAL_STRING("", dest);
    
    csv_get_field(dest, sizeof(dest), buffer, 0, 4);
    TEST_ASSERT_EQUAL_STRING("", dest);
    
    csv_get_field(dest, sizeof(dest), buffer, 0, 5);
    TEST_ASSERT_EQUAL_STRING("X", dest);
}

/* Test 6: Insert into empty row */
static void test_csv_insert_field_empty_row(void) {
    /* Setup: row 0 with 0 fields */
    while (csv_get_height(buffer) <= 0) {
        append_row(buffer);
    }
    csv_clear_row(buffer, 0);
    TEST_ASSERT_EQUAL_INT(0, csv_get_width(buffer, 0));
    
    /* Insert "FIRST" at position 0 */
    int result = csv_insert_field(buffer, 0, 0, "FIRST");
    TEST_ASSERT_EQUAL_INT(0, result);
    
    /* Verify: should have 1 field */
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buffer, 0));
    
    char dest[64];
    csv_get_field(dest, sizeof(dest), buffer, 0, 0);
    TEST_ASSERT_EQUAL_STRING("FIRST", dest);
}

/* Test 7: Insert at invalid row (should segfault or handle gracefully) */
static void test_csv_insert_field_invalid_row(void) {
    /* Setup: row 0 exists */
    setup_row_with_fields(0, 1, "A");
    
    /* Try to insert into row 100 (non-existent) */
    int result = run_with_segfault_protection(
        (void (*)(void))csv_insert_field,
        buffer, 100, 0, "X"
    );
    /* If segfault occurs, it's handled and we return early */
    if (result) {
        TEST_FAIL_MESSAGE("Segmentation fault on invalid row");
    }
    
    /* If no segfault, verify behavior */
    /* Note: Implementation may or may not handle this gracefully */
    /* Based on code, it calls csv_set_field which may create new row */
    /* So we expect it to succeed and create row 100 */
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_INT(101, csv_get_height(buffer));
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buffer, 100));
    
    char dest[64];
    csv_get_field(dest, sizeof(dest), buffer, 100, 0);
    TEST_ASSERT_EQUAL_STRING("X", dest);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_csv_insert_field_insert_middle);
    RUN_TEST(test_csv_insert_field_insert_beginning);
    RUN_TEST(test_csv_insert_field_insert_end);
    RUN_TEST(test_csv_insert_field_new_row);
    RUN_TEST(test_csv_insert_field_extend_row);
    RUN_TEST(test_csv_insert_field_empty_row);
    RUN_TEST(test_csv_insert_field_invalid_row);
    
    return UNITY_END();
}