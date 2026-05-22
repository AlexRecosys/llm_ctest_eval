#include "unity.h"
#include "csv.h"
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>
#include <stdio.h>

static sig_atomic_t segv_occurred = 0;
static sigjmp_buf jump_buffer;

static void segv_handler(int signum) {
    (void)signum;
    segv_occurred = 1;
    siglongjmp(jump_buffer, 1);
}

static void setup_signal_handler(void) {
    struct sigaction sa;
    sa.sa_handler = segv_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGSEGV, &sa, NULL);
}

static void reset_segvs(void) {
    segv_occurred = 0;
}

static int test_with_segvs(void (*test_func)(void)) {
    reset_segvs();
    if (sigsetjmp(jump_buffer, 1) == 0) {
        test_func();
    }
    return segv_occurred;
}

static void test_segv_fail(void) {
    TEST_ASSERT_FALSE_MESSAGE(segv_occurred, "Segmentation fault detected");
}

/* Global test fixtures */
static CSV_BUFFER *buffer = NULL;

void setUp(void) {
    buffer = create_buffer();
    TEST_ASSERT_NOT_NULL(buffer);
}

void tearDown(void) {
    if (buffer != NULL) {
        free_buffer(buffer);
        buffer = NULL;
    }
}

/* Test: csv_clear_row succeeds when clearing last row (delegates to remove_last_row) */
void test_csv_clear_row_clear_last_row_success(void) {
    /* Setup: add 2 rows */
    add_row(buffer);
    add_row(buffer);
    add_field(buffer, 0, "a");
    add_field(buffer, 1, "b");

    /* Act */
    int result = csv_clear_row(buffer, 1);  /* row 1 is last */

    /* Assert */
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_SIZE(1, buffer->rows);
}

/* Test: csv_clear_row fails when remove_last_row fails */
void test_csv_clear_row_clear_last_row_failure(void) {
    /* Setup: add 1 row */
    add_row(buffer);
    add_field(buffer, 0, "x");

    /* Force remove_last_row to fail by corrupting buffer state (simulate internal error) */
    /* Note: In real code, remove_last_row might fail on invalid state; here we simulate by temporarily breaking buffer */
    size_t orig_rows = buffer->rows;
    buffer->rows = 0;  /* simulate invalid state for remove_last_row */

    /* Act */
    int result = csv_clear_row(buffer, 0);

    /* Restore */
    buffer->rows = orig_rows;

    /* Assert */
    TEST_ASSERT_EQUAL_INT(1, result);
}

/* Test: csv_clear_row shrinks row to one empty field when row is not last and realloc succeeds */
void test_csv_clear_row_non_last_row_realloc_succeeds(void) {
    /* Setup: 2 rows, row 0 has 3 fields */
    add_row(buffer);
    add_row(buffer);
    add_field(buffer, 0, "a");
    add_field(buffer, 0, "b");
    add_field(buffer, 0, "c");

    /* Act */
    int result = csv_clear_row(buffer, 0);

    /* Assert */
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_INT(1, buffer->width[0]);
    TEST_ASSERT_EQUAL_STRING("", buffer->field[0][0]->value);
    TEST_ASSERT_EQUAL_SIZE(2, buffer->rows);
}

/* Test: csv_clear_row fails to shrink row and reconstructs fields when realloc fails */
void test_csv_clear_row_realloc_fails_reconstructs_fields(void) {
    /* Setup: 2 rows, row 0 has 3 fields */
    add_row(buffer);
    add_row(buffer);
    add_field(buffer, 0, "a");
    add_field(buffer, 0, "b");
    add_field(buffer, 0, "c");

    /* Mock realloc to fail: temporarily override realloc behavior */
    /* Since we cannot easily mock realloc, simulate failure by forcing NULL return via environment */
    /* Instead, we'll use a trick: temporarily replace realloc with a dummy that returns NULL */
    void *(*orig_realloc)(void *, size_t) = realloc;
    /* In real test, you'd use a mock; here we simulate by corrupting memory to cause NULL realloc */
    /* But Unity doesn't allow mocking easily; instead, we rely on test with segv guard */

    /* To avoid segv, we'll skip this path in pure C without mocking framework */
    /* Instead, we test that if realloc fails, function returns 1 and reconstructs */
    /* But without mocking, we cannot force realloc to fail deterministically */
    /* So we skip this test in this strict requirement set */

    /* However, per requirements, we must test *all* paths */
    /* Alternative: use setrlimit to restrict memory and force realloc to fail */
    struct rlimit rl;
    rl.rlim_cur = rl.rlim_max = 1024 * 1024;  /* 1MB limit */
    setrlimit(RLIMIT_AS, &rl);

    /* Act */
    int result = csv_clear_row(buffer, 0);

    /* Restore limits */
    rl.rlim_cur = rl.rlim_max = RLIM_INFINITY;
    setrlimit(RLIMIT_AS, &rl);

    /* Assert */
    TEST_ASSERT_EQUAL_INT(1, result);
    TEST_ASSERT_EQUAL_INT(1, buffer->width[0]);
    TEST_ASSERT_EQUAL_STRING("", buffer->field[0][0]->value);
}

/* Test: csv_clear_row handles segfault gracefully (e.g., NULL buffer) */
void test_csv_clear_row_null_buffer(void) {
    setup_signal_handler();

    int segv = test_with_segvs(() {
        (void)csv_clear_row(NULL, 0);
    });

    TEST_ASSERT_FALSE_MESSAGE(!segv, "Expected segfault for NULL buffer");
    test_segv_fail();
}

/* Test: csv_clear_row handles invalid row index (out of bounds) */
void test_csv_clear_row_out_of_bounds_row(void) {
    add_row(buffer);
    add_field(buffer, 0, "x");

    setup_signal_handler();

    int segv = test_with_segvs(() {
        (void)csv_clear_row(buffer, 100);
    });

    /* Out-of-bounds may cause segv or not, depending on implementation */
    /* If no segv, function may return error; but we test segv handling */
    test_segv_fail();
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_csv_clear_row_clear_last_row_success);
    RUN_TEST(test_csv_clear_row_clear_last_row_failure);
    RUN_TEST(test_csv_clear_row_non_last_row_realloc_succeeds);
    RUN_TEST(test_csv_clear_row_realloc_fails_reconstructs_fields);
    RUN_TEST(test_csv_clear_row_null_buffer);
    RUN_TEST(test_csv_clear_row_out_of_bounds_row);

    return UNITY_END();
}