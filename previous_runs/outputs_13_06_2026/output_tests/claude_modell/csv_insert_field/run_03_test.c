#include "csv.c"
#include "unity.h"
#include "csv.h"
#include <stdlib.h>
#include <string.h>
#include <signal.h>

/* File-scope buffer used across tests */
static CSV_BUFFER *buf = NULL;

/* Signal handler for SIGSEGV */
static void sigsegv_handler(int sig)
{
    (void)sig;
    TEST_FAIL_MESSAGE("Segmentation fault (SIGSEGV) caught during test");
}

void setUp(void)
{
    signal(SIGSEGV, sigsegv_handler);
    buf = csv_create_buffer();
    TEST_ASSERT_NOT_NULL_MESSAGE(buf, "csv_create_buffer returned NULL in setUp");
}

void tearDown(void)
{
    if (buf != NULL) {
        csv_destroy_buffer(buf);
        buf = NULL;
    }
    signal(SIGSEGV, SIG_DFL);
}

/* Helper: retrieve field text into a stack buffer and return via strdup-like copy */
static void get_field_str(CSV_BUFFER *b, size_t row, size_t entry,
                           char *out, size_t out_len)
{
    csv_get_field(out, out_len, b, row, entry);
}

/*
 * Test 1: Insert into a non-existent row/entry (out-of-bounds path).
 * When row > buffer->rows-1, csv_set_field is called directly.
 * The buffer starts empty (rows == 0), so any row index triggers the
 * "does not exist" branch.
 */
void test_insert_field_into_empty_buffer_uses_set_path(void)
{
    int ret = csv_insert_field(buf, 0, 0, "hello");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "csv_insert_field should return 0");

    char out[64] = {0};
    get_field_str(buf, 0, 0, out, sizeof(out));
    TEST_ASSERT_EQUAL_STRING_MESSAGE("hello", out,
        "Field at (0,0) should be 'hello' after insert into empty buffer");
}

/*
 * Test 2: Insert at an entry index beyond the current row width.
 * The row exists but the entry index is out of range, so the
 * "does not exist" branch (csv_set_field) is taken.
 */
void test_insert_field_beyond_row_width_uses_set_path(void)
{
    /* Populate row 0 with two fields */
    csv_set_field(buf, 0, 0, "alpha");
    csv_set_field(buf, 0, 1, "beta");

    /* Insert at entry 5 — beyond current width of 2 */
    int ret = csv_insert_field(buf, 0, 5, "gamma");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "csv_insert_field should return 0");

    char out[64] = {0};
    get_field_str(buf, 0, 5, out, sizeof(out));
    TEST_ASSERT_EQUAL_STRING_MESSAGE("gamma", out,
        "Field at (0,5) should be 'gamma' after out-of-bounds insert");
}

/*
 * Test 3: Insert at the beginning of an existing row (entry == 0).
 * All existing fields should shift right by one position.
 */
void test_insert_field_at_beginning_shifts_all_fields_right(void)
{
    /* Set up row 0: ["first", "second", "third"] */
    csv_set_field(buf, 0, 0, "first");
    csv_set_field(buf, 0, 1, "second");
    csv_set_field(buf, 0, 2, "third");

    int ret = csv_insert_field(buf, 0, 0, "new");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "csv_insert_field should return 0");

    /* Width should now be 4 */
    int w = csv_get_width(buf, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(4, w, "Row width should be 4 after insert at beginning");

    char out[64] = {0};

    get_field_str(buf, 0, 0, out, sizeof(out));
    TEST_ASSERT_EQUAL_STRING_MESSAGE("new", out, "Entry 0 should be 'new'");

    get_field_str(buf, 0, 1, out, sizeof(out));
    TEST_ASSERT_EQUAL_STRING_MESSAGE("first", out, "Entry 1 should be 'first'");

    get_field_str(buf, 0, 2, out, sizeof(out));
    TEST_ASSERT_EQUAL_STRING_MESSAGE("second", out, "Entry 2 should be 'second'");

    get_field_str(buf, 0, 3, out, sizeof(out));
    TEST_ASSERT_EQUAL_STRING_MESSAGE("third", out, "Entry 3 should be 'third'");
}

/*
 * Test 4: Insert in the middle of an existing row.
 * Fields at and after the insertion point shift right; earlier fields unchanged.
 */
void test_insert_field_in_middle_shifts_tail_right(void)
{
    /* Set up row 0: ["A", "B", "C", "D"] */
    csv_set_field(buf, 0, 0, "A");
    csv_set_field(buf, 0, 1, "B");
    csv_set_field(buf, 0, 2, "C");
    csv_set_field(buf, 0, 3, "D");

    /* Insert "X" at entry 2 → expected: ["A", "B", "X", "C", "D"] */
    int ret = csv_insert_field(buf, 0, 2, "X");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "csv_insert_field should return 0");

    int w = csv_get_width(buf, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(5, w, "Row width should be 5 after middle insert");

    char out[64] = {0};

    get_field_str(buf, 0, 0, out, sizeof(out));
    TEST_ASSERT_EQUAL_STRING_MESSAGE("A", out, "Entry 0 should remain 'A'");

    get_field_str(buf, 0, 1, out, sizeof(out));
    TEST_ASSERT_EQUAL_STRING_MESSAGE("B", out, "Entry 1 should remain 'B'");

    get_field_str(buf, 0, 2, out, sizeof(out));
    TEST_ASSERT_EQUAL_STRING_MESSAGE("X", out, "Entry 2 should be inserted 'X'");

    get_field_str(buf, 0, 3, out, sizeof(out));
    TEST_ASSERT_EQUAL_STRING_MESSAGE("C", out, "Entry 3 should be shifted 'C'");

    get_field_str(buf, 0, 4, out, sizeof(out));
    TEST_ASSERT_EQUAL_STRING_MESSAGE("D", out, "Entry 4 should be shifted 'D'");
}

/*
 * Test 5: Insert into a specific row when multiple rows exist.
 * Only the target row is modified; other rows remain intact.
 */
void test_insert_field_only_modifies_target_row(void)
{
    /* Row 0: ["row0_a", "row0_b"] */
    csv_set_field(buf, 0, 0, "row0_a");
    csv_set_field(buf, 0, 1, "row0_b");

    /* Row 1: ["row1_a", "row1_b", "row1_c"] */
    csv_set_field(buf, 1, 0, "row1_a");
    csv_set_field(buf, 1, 1, "row1_b");
    csv_set_field(buf, 1, 2, "row1_c");

    /* Insert into row 1 at entry 1 */
    int ret = csv_insert_field(buf, 1, 1, "inserted");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, ret, "csv_insert_field should return 0");

    /* Row 0 should be unchanged */
    int w0 = csv_get_width(buf, 0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, w0, "Row 0 width should still be 2");

    char out[64] = {0};
    get_field_str(buf, 0, 0, out, sizeof(out));
    TEST_ASSERT_EQUAL_STRING_MESSAGE("row0_a", out, "Row 0 entry 0 should be unchanged");

    get_field_str(buf, 0, 1, out, sizeof(out));
    TEST_ASSERT_EQUAL_STRING_MESSAGE("row0_b", out, "Row 0 entry 1 should be unchanged");

    /* Row 1 should now be ["row1_a", "inserted", "row1_b", "row1_c"] */
    int w1 = csv_get_width(buf, 1);
    TEST_ASSERT_EQUAL_INT_MESSAGE(4, w1, "Row 1 width should be 4 after insert");

    get_field_str(buf, 1, 0, out, sizeof(out));
    TEST_ASSERT_EQUAL_STRING_MESSAGE("row1_a", out, "Row 1 entry 0 should be 'row1_a'");

    get_field_str(buf, 1, 1, out, sizeof(out));
    TEST_ASSERT_EQUAL_STRING_MESSAGE("inserted", out, "Row 1 entry 1 should be 'inserted'");

    get_field_str(buf, 1, 2, out, sizeof(out));
    TEST_ASSERT_EQUAL_STRING_MESSAGE("row1_b", out, "Row 1 entry 2 should be 'row1_b'");

    get_field_str(buf, 1, 3, out, sizeof(out));
    TEST_ASSERT_EQUAL_STRING_MESSAGE("row1_c", out, "Row 1 entry 3 should be 'row1_c'");
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_insert_field_into_empty_buffer_uses_set_path);
    RUN_TEST(test_insert_field_beyond_row_width_uses_set_path);
    RUN_TEST(test_insert_field_at_beginning_shifts_all_fields_right);
    RUN_TEST(test_insert_field_in_middle_shifts_tail_right);
    RUN_TEST(test_insert_field_only_modifies_target_row);
    return UNITY_END();
}