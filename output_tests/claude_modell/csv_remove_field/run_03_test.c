#include "csv.c"
#include "unity.h"
#include <stdlib.h>
#include <string.h>

/* File-scope fixtures */
static CSV_BUFFER *buf;

/* Helper: populate a buffer with a single row of N fields */
static void fill_row(CSV_BUFFER *b, size_t row, const char **values, size_t count)
{
    size_t i;
    for (i = 0; i < count; i++) {
        csv_set_field(b, row, i, (char *)values[i]);
    }
}

/* Helper: build a fresh buffer with one row and given fields */
static CSV_BUFFER *make_buffer_one_row(const char **values, size_t count)
{
    CSV_BUFFER *b = csv_create_buffer();
    size_t i;
    for (i = 0; i < count; i++) {
        csv_set_field(b, 0, i, (char *)values[i]);
    }
    return b;
}

/* Helper: build a fresh buffer with multiple rows */
static CSV_BUFFER *make_buffer_multi_row(void)
{
    CSV_BUFFER *b = csv_create_buffer();
    csv_set_field(b, 0, 0, "r0c0");
    csv_set_field(b, 0, 1, "r0c1");
    csv_set_field(b, 0, 2, "r0c2");
    csv_set_field(b, 1, 0, "r1c0");
    csv_set_field(b, 1, 1, "r1c1");
    csv_set_field(b, 1, 2, "r1c2");
    return b;
}

void setUp(void)
{
    buf = csv_create_buffer();
}

void tearDown(void)
{
    csv_destroy_buffer(buf);
    buf = NULL;
}

/* ------------------------------------------------------------------ */
/* Test: remove middle field shifts remaining fields left              */
/* ------------------------------------------------------------------ */
void test_remove_middle_field_shifts_remaining(void)
{
    const char *vals[] = {"alpha", "beta", "gamma", "delta"};
    size_t count = 4;
    size_t i;
    for (i = 0; i < count; i++) {
        csv_set_field(buf, 0, i, (char *)vals[i]);
    }

    /* Remove index 1 ("beta") */
    int ret = csv_remove_field(buf, 0, 1);
    TEST_ASSERT_EQUAL_INT(0, ret);

    /* Width should now be 3 */
    TEST_ASSERT_EQUAL_INT(3, csv_get_width(buf, 0));

    char tmp[64];
    csv_get_field(tmp, sizeof(tmp), buf, 0, 0);
    TEST_ASSERT_EQUAL_STRING("alpha", tmp);

    csv_get_field(tmp, sizeof(tmp), buf, 0, 1);
    TEST_ASSERT_EQUAL_STRING("gamma", tmp);

    csv_get_field(tmp, sizeof(tmp), buf, 0, 2);
    TEST_ASSERT_EQUAL_STRING("delta", tmp);
}

/* ------------------------------------------------------------------ */
/* Test: remove first field                                            */
/* ------------------------------------------------------------------ */
void test_remove_first_field(void)
{
    const char *vals[] = {"one", "two", "three"};
    size_t i;
    for (i = 0; i < 3; i++) {
        csv_set_field(buf, 0, i, (char *)vals[i]);
    }

    int ret = csv_remove_field(buf, 0, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);

    TEST_ASSERT_EQUAL_INT(2, csv_get_width(buf, 0));

    char tmp[64];
    csv_get_field(tmp, sizeof(tmp), buf, 0, 0);
    TEST_ASSERT_EQUAL_STRING("two", tmp);

    csv_get_field(tmp, sizeof(tmp), buf, 0, 1);
    TEST_ASSERT_EQUAL_STRING("three", tmp);
}

/* ------------------------------------------------------------------ */
/* Test: remove last field in a row                                    */
/* ------------------------------------------------------------------ */
void test_remove_last_field_in_row(void)
{
    const char *vals[] = {"x", "y", "z"};
    size_t i;
    for (i = 0; i < 3; i++) {
        csv_set_field(buf, 0, i, (char *)vals[i]);
    }

    int ret = csv_remove_field(buf, 0, 2);
    TEST_ASSERT_EQUAL_INT(0, ret);

    TEST_ASSERT_EQUAL_INT(2, csv_get_width(buf, 0));

    char tmp[64];
    csv_get_field(tmp, sizeof(tmp), buf, 0, 0);
    TEST_ASSERT_EQUAL_STRING("x", tmp);

    csv_get_field(tmp, sizeof(tmp), buf, 0, 1);
    TEST_ASSERT_EQUAL_STRING("y", tmp);
}

/* ------------------------------------------------------------------ */
/* Test: remove only field in a row reduces width to 0                */
/* ------------------------------------------------------------------ */
void test_remove_only_field_reduces_width(void)
{
    csv_set_field(buf, 0, 0, "solo");

    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buf, 0));

    int ret = csv_remove_field(buf, 0, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);

    TEST_ASSERT_EQUAL_INT(0, csv_get_width(buf, 0));
}

/* ------------------------------------------------------------------ */
/* Test: out-of-bounds entry index returns 0 and does not modify      */
/* ------------------------------------------------------------------ */
void test_out_of_bounds_entry_returns_zero_no_change(void)
{
    const char *vals[] = {"a", "b", "c"};
    size_t i;
    for (i = 0; i < 3; i++) {
        csv_set_field(buf, 0, i, (char *)vals[i]);
    }

    /* entry 3 is out of bounds for a row of width 3 (valid: 0-2) */
    int ret = csv_remove_field(buf, 0, 3);
    TEST_ASSERT_EQUAL_INT(0, ret);

    /* Width must remain 3 */
    TEST_ASSERT_EQUAL_INT(3, csv_get_width(buf, 0));

    char tmp[64];
    csv_get_field(tmp, sizeof(tmp), buf, 0, 0);
    TEST_ASSERT_EQUAL_STRING("a", tmp);
    csv_get_field(tmp, sizeof(tmp), buf, 0, 1);
    TEST_ASSERT_EQUAL_STRING("b", tmp);
    csv_get_field(tmp, sizeof(tmp), buf, 0, 2);
    TEST_ASSERT_EQUAL_STRING("c", tmp);
}

/* ------------------------------------------------------------------ */
/* Test: out-of-bounds row index returns 0 and does not modify        */
/* ------------------------------------------------------------------ */
void test_out_of_bounds_row_returns_zero_no_change(void)
{
    csv_set_field(buf, 0, 0, "hello");

    /* row 1 does not exist */
    int ret = csv_remove_field(buf, 1, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);

    /* Row 0 must be untouched */
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buf, 0));

    char tmp[64];
    csv_get_field(tmp, sizeof(tmp), buf, 0, 0);
    TEST_ASSERT_EQUAL_STRING("hello", tmp);
}

/* ------------------------------------------------------------------ */
/* Test: remove field from second row does not affect first row       */
/* ------------------------------------------------------------------ */
void test_remove_field_from_second_row_does_not_affect_first(void)
{
    csv_set_field(buf, 0, 0, "row0col0");
    csv_set_field(buf, 0, 1, "row0col1");
    csv_set_field(buf, 1, 0, "row1col0");
    csv_set_field(buf, 1, 1, "row1col1");
    csv_set_field(buf, 1, 2, "row1col2");

    int ret = csv_remove_field(buf, 1, 1);
    TEST_ASSERT_EQUAL_INT(0, ret);

    /* Row 0 unchanged */
    TEST_ASSERT_EQUAL_INT(2, csv_get_width(buf, 0));
    char tmp[64];
    csv_get_field(tmp, sizeof(tmp), buf, 0, 0);
    TEST_ASSERT_EQUAL_STRING("row0col0", tmp);
    csv_get_field(tmp, sizeof(tmp), buf, 0, 1);
    TEST_ASSERT_EQUAL_STRING("row0col1", tmp);

    /* Row 1 has one fewer field */
    TEST_ASSERT_EQUAL_INT(2, csv_get_width(buf, 1));
    csv_get_field(tmp, sizeof(tmp), buf, 1, 0);
    TEST_ASSERT_EQUAL_STRING("row1col0", tmp);
    csv_get_field(tmp, sizeof(tmp), buf, 1, 1);
    TEST_ASSERT_EQUAL_STRING("row1col2", tmp);
}

/* ------------------------------------------------------------------ */
/* Test: remove field from first row does not affect second row       */
/* ------------------------------------------------------------------ */
void test_remove_field_from_first_row_does_not_affect_second(void)
{
    csv_set_field(buf, 0, 0, "A");
    csv_set_field(buf, 0, 1, "B");
    csv_set_field(buf, 0, 2, "C");
    csv_set_field(buf, 1, 0, "D");
    csv_set_field(buf, 1, 1, "E");

    int ret = csv_remove_field(buf, 0, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);

    TEST_ASSERT_EQUAL_INT(2, csv_get_width(buf, 0));

    char tmp[64];
    csv_get_field(tmp, sizeof(tmp), buf, 0, 0);
    TEST_ASSERT_EQUAL_STRING("B", tmp);
    csv_get_field(tmp, sizeof(tmp), buf, 0, 1);
    TEST_ASSERT_EQUAL_STRING("C", tmp);

    /* Row 1 unchanged */
    TEST_ASSERT_EQUAL_INT(2, csv_get_width(buf, 1));
    csv_get_field(tmp, sizeof(tmp), buf, 1, 0);
    TEST_ASSERT_EQUAL_STRING("D", tmp);
    csv_get_field(tmp, sizeof(tmp), buf, 1, 1);
    TEST_ASSERT_EQUAL_STRING("E", tmp);
}

/* ------------------------------------------------------------------ */
/* Test: sequential removal of all fields empties the row             */
/* ------------------------------------------------------------------ */
void test_sequential_removal_empties_row(void)
{
    csv_set_field(buf, 0, 0, "p");
    csv_set_field(buf, 0, 1, "q");
    csv_set_field(buf, 0, 2, "r");

    csv_remove_field(buf, 0, 0);
    TEST_ASSERT_EQUAL_INT(2, csv_get_width(buf, 0));

    csv_remove_field(buf, 0, 0);
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buf, 0));

    csv_remove_field(buf, 0, 0);
    TEST_ASSERT_EQUAL_INT(0, csv_get_width(buf, 0));
}

/* ------------------------------------------------------------------ */
/* Test: remove field with large entry index (SIZE_MAX-like) is safe  */
/* ------------------------------------------------------------------ */
void test_remove_field_large_entry_index_is_safe(void)
{
    csv_set_field(buf, 0, 0, "only");

    /* Use a very large entry index */
    int ret = csv_remove_field(buf, 0, 9999);
    TEST_ASSERT_EQUAL_INT(0, ret);

    /* Row must be untouched */
    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buf, 0));
}

/* ------------------------------------------------------------------ */
/* Test: remove field preserves content of adjacent fields exactly    */
/* ------------------------------------------------------------------ */
void test_remove_field_preserves_adjacent_content_exactly(void)
{
    csv_set_field(buf, 0, 0, "keep_me_0");
    csv_set_field(buf, 0, 1, "remove_me");
    csv_set_field(buf, 0, 2, "keep_me_2");
    csv_set_field(buf, 0, 3, "keep_me_3");

    int ret = csv_remove_field(buf, 0, 1);
    TEST_ASSERT_EQUAL_INT(0, ret);

    TEST_ASSERT_EQUAL_INT(3, csv_get_width(buf, 0));

    char tmp[64];
    csv_get_field(tmp, sizeof(tmp), buf, 0, 0);
    TEST_ASSERT_EQUAL_STRING("keep_me_0", tmp);

    csv_get_field(tmp, sizeof(tmp), buf, 0, 1);
    TEST_ASSERT_EQUAL_STRING("keep_me_2", tmp);

    csv_get_field(tmp, sizeof(tmp), buf, 0, 2);
    TEST_ASSERT_EQUAL_STRING("keep_me_3", tmp);
}

/* ------------------------------------------------------------------ */
/* Test: remove field from row with two fields, remove first          */
/* ------------------------------------------------------------------ */
void test_remove_first_of_two_fields(void)
{
    csv_set_field(buf, 0, 0, "first");
    csv_set_field(buf, 0, 1, "second");

    int ret = csv_remove_field(buf, 0, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);

    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buf, 0));

    char tmp[64];
    csv_get_field(tmp, sizeof(tmp), buf, 0, 0);
    TEST_ASSERT_EQUAL_STRING("second", tmp);
}

/* ------------------------------------------------------------------ */
/* Test: remove field from row with two fields, remove second         */
/* ------------------------------------------------------------------ */
void test_remove_second_of_two_fields(void)
{
    csv_set_field(buf, 0, 0, "first");
    csv_set_field(buf, 0, 1, "second");

    int ret = csv_remove_field(buf, 0, 1);
    TEST_ASSERT_EQUAL_INT(0, ret);

    TEST_ASSERT_EQUAL_INT(1, csv_get_width(buf, 0));

    char tmp[64];
    csv_get_field(tmp, sizeof(tmp), buf, 0, 0);
    TEST_ASSERT_EQUAL_STRING("first", tmp);
}

/* ------------------------------------------------------------------ */
/* main                                                                */
/* ------------------------------------------------------------------ */
int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_remove_middle_field_shifts_remaining);
    RUN_TEST(test_remove_first_field);
    RUN_TEST(test_remove_last_field_in_row);
    RUN_TEST(test_remove_only_field_reduces_width);
    RUN_TEST(test_out_of_bounds_entry_returns_zero_no_change);
    RUN_TEST(test_out_of_bounds_row_returns_zero_no_change);
    RUN_TEST(test_remove_field_from_second_row_does_not_affect_first);
    RUN_TEST(test_remove_field_from_first_row_does_not_affect_second);
    RUN_TEST(test_sequential_removal_empties_row);
    RUN_TEST(test_remove_field_large_entry_index_is_safe);
    RUN_TEST(test_remove_field_preserves_adjacent_content_exactly);
    RUN_TEST(test_remove_first_of_two_fields);
    RUN_TEST(test_remove_second_of_two_fields);
    return UNITY_END();
}