#include "csv.c"
#include "unity.h"
#include <stdlib.h>
#include <string.h>

/* File-scope fixtures */
static CSV_BUFFER *buf;

/* Helper: populate a buffer with a single row of N fields */
static void fill_row(CSV_BUFFER *b, size_t row, const char **texts, size_t n)
{
    size_t i;
    for (i = 0; i < n; i++) {
        csv_set_field(b, row, i, (char *)texts[i]);
    }
}

/* Helper: retrieve field text into a stack buffer and return it */
static void get_text(CSV_BUFFER *b, size_t row, size_t entry,
                     char *out, size_t out_len)
{
    csv_get_field(out, out_len, b, row, entry);
}

/* ------------------------------------------------------------------ */
/* setUp / tearDown                                                     */
/* ------------------------------------------------------------------ */

void setUp(void)
{
    buf = csv_create_buffer();
    /* Add two rows so tests have something to work with */
    csv_set_field(buf, 0, 0, "A");
    csv_set_field(buf, 0, 1, "B");
    csv_set_field(buf, 0, 2, "C");
    csv_set_field(buf, 0, 3, "D");

    csv_set_field(buf, 1, 0, "X");
    csv_set_field(buf, 1, 1, "Y");
    csv_set_field(buf, 1, 2, "Z");
}

void tearDown(void)
{
    csv_destroy_buffer(buf);
    buf = NULL;
}

/* ------------------------------------------------------------------ */
/* Test cases                                                           */
/* ------------------------------------------------------------------ */

/* Removing the first field shifts remaining fields left */
void test_remove_first_field_shifts_remaining(void)
{
    char text[64];

    int ret = csv_remove_field(buf, 0, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);

    /* Width should decrease by one */
    TEST_ASSERT_EQUAL_INT(3, csv_get_width(buf, 0));

    /* Former field[1] is now field[0] */
    get_text(buf, 0, 0, text, sizeof(text));
    TEST_ASSERT_EQUAL_STRING("B", text);

    get_text(buf, 0, 1, text, sizeof(text));
    TEST_ASSERT_EQUAL_STRING("C", text);

    get_text(buf, 0, 2, text, sizeof(text));
    TEST_ASSERT_EQUAL_STRING("D", text);
}

/* Removing a middle field shifts subsequent fields left */
void test_remove_middle_field_shifts_remaining(void)
{
    char text[64];

    int ret = csv_remove_field(buf, 0, 1);
    TEST_ASSERT_EQUAL_INT(0, ret);

    TEST_ASSERT_EQUAL_INT(3, csv_get_width(buf, 0));

    get_text(buf, 0, 0, text, sizeof(text));
    TEST_ASSERT_EQUAL_STRING("A", text);

    get_text(buf, 0, 1, text, sizeof(text));
    TEST_ASSERT_EQUAL_STRING("C", text);

    get_text(buf, 0, 2, text, sizeof(text));
    TEST_ASSERT_EQUAL_STRING("D", text);
}

/* Removing the last field decrements width */
void test_remove_last_field_decrements_width(void)
{
    char text[64];

    int ret = csv_remove_field(buf, 0, 3);
    TEST_ASSERT_EQUAL_INT(0, ret);

    TEST_ASSERT_EQUAL_INT(3, csv_get_width(buf, 0));

    get_text(buf, 0, 0, text, sizeof(text));
    TEST_ASSERT_EQUAL_STRING("A", text);

    get_text(buf, 0, 1, text, sizeof(text));
    TEST_ASSERT_EQUAL_STRING("B", text);

    get_text(buf, 0, 2, text, sizeof(text));
    TEST_ASSERT_EQUAL_STRING("C", text);
}

/* Removing a field from a second row does not affect the first row */
void test_remove_field_from_second_row_does_not_affect_first_row(void)
{
    char text[64];

    int ret = csv_remove_field(buf, 1, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);

    /* Row 0 untouched */
    TEST_ASSERT_EQUAL_INT(4, csv_get_width(buf, 0));
    get_text(buf, 0, 0, text, sizeof(text));
    TEST_ASSERT_EQUAL_STRING("A", text);

    /* Row 1 shifted */
    TEST_ASSERT_EQUAL_INT(2, csv_get_width(buf, 1));
    get_text(buf, 1, 0, text, sizeof(text));
    TEST_ASSERT_EQUAL_STRING("Y", text);

    get_text(buf, 1, 1, text, sizeof(text));
    TEST_ASSERT_EQUAL_STRING("Z", text);
}

/* Out-of-bounds row returns 0 and leaves buffer unchanged */
void test_remove_field_out_of_bounds_row_returns_zero(void)
{
    int original_height = csv_get_height(buf);
    int original_width  = csv_get_width(buf, 0);

    int ret = csv_remove_field(buf, 99, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);

    /* Buffer unchanged */
    TEST_ASSERT_EQUAL_INT(original_height, csv_get_height(buf));
    TEST_ASSERT_EQUAL_INT(original_width,  csv_get_width(buf, 0));
}

/* Out-of-bounds entry returns 0 and leaves row unchanged */
void test_remove_field_out_of_bounds_entry_returns_zero(void)
{
    int original_width = csv_get_width(buf, 0);

    int ret = csv_remove_field(buf, 0, 99);
    TEST_ASSERT_EQUAL_INT(0, ret);

    TEST_ASSERT_EQUAL_INT(original_width, csv_get_width(buf, 0));
}

/* Removing the only field in a row reduces width to zero */
void test_remove_only_field_reduces_width_to_zero(void)
{
    CSV_BUFFER *b = csv_create_buffer();
    csv_set_field(b, 0, 0, "ONLY");

    TEST_ASSERT_EQUAL_INT(1, csv_get_width(b, 0));

    int ret = csv_remove_field(b, 0, 0);
    TEST_ASSERT_EQUAL_INT(0, ret);

    TEST_ASSERT_EQUAL_INT(0, csv_get_width(b, 0));

    csv_destroy_buffer(b);
}

/* Removing all fields one by one empties the row */
void test_remove_all_fields_one_by_one(void)
{
    CSV_BUFFER *b = csv_create_buffer();
    csv_set_field(b, 0, 0, "P");
    csv_set_field(b, 0, 1, "Q");
    csv_set_field(b, 0, 2, "R");

    csv_remove_field(b, 0, 0); /* P Q R -> Q R */
    csv_remove_field(b, 0, 0); /* Q R   -> R   */
    csv_remove_field(b, 0, 0); /* R     -> (empty) */

    TEST_ASSERT_EQUAL_INT(0, csv_get_width(b, 0));

    csv_destroy_buffer(b);
}

/* Return value is always 0 for valid inputs */
void test_remove_field_return_value_is_zero_for_valid_input(void)
{
    int ret;

    ret = csv_remove_field(buf, 0, 2);
    TEST_ASSERT_EQUAL_INT(0, ret);

    ret = csv_remove_field(buf, 1, 1);
    TEST_ASSERT_EQUAL_INT(0, ret);
}

/* Removing second-to-last field leaves only one field */
void test_remove_second_to_last_field_leaves_one(void)
{
    char text[64];

    CSV_BUFFER *b = csv_create_buffer();
    csv_set_field(b, 0, 0, "FIRST");
    csv_set_field(b, 0, 1, "SECOND");

    csv_remove_field(b, 0, 0);

    TEST_ASSERT_EQUAL_INT(1, csv_get_width(b, 0));
    get_text(b, 0, 0, text, sizeof(text));
    TEST_ASSERT_EQUAL_STRING("SECOND", text);

    csv_destroy_buffer(b);
}

/* Removing a field with empty string content works correctly */
void test_remove_field_with_empty_string_content(void)
{
    char text[64];

    CSV_BUFFER *b = csv_create_buffer();
    csv_set_field(b, 0, 0, "BEFORE");
    csv_set_field(b, 0, 1, "");
    csv_set_field(b, 0, 2, "AFTER");

    csv_remove_field(b, 0, 1);

    TEST_ASSERT_EQUAL_INT(2, csv_get_width(b, 0));

    get_text(b, 0, 0, text, sizeof(text));
    TEST_ASSERT_EQUAL_STRING("BEFORE", text);

    get_text(b, 0, 1, text, sizeof(text));
    TEST_ASSERT_EQUAL_STRING("AFTER", text);

    csv_destroy_buffer(b);
}

/* Height of buffer is unchanged after removing a field */
void test_remove_field_does_not_change_buffer_height(void)
{
    int height_before = csv_get_height(buf);
    csv_remove_field(buf, 0, 0);
    int height_after = csv_get_height(buf);
    TEST_ASSERT_EQUAL_INT(height_before, height_after);
}

/* ------------------------------------------------------------------ */
/* main                                                                 */
/* ------------------------------------------------------------------ */

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_remove_first_field_shifts_remaining);
    RUN_TEST(test_remove_middle_field_shifts_remaining);
    RUN_TEST(test_remove_last_field_decrements_width);
    RUN_TEST(test_remove_field_from_second_row_does_not_affect_first_row);
    RUN_TEST(test_remove_field_out_of_bounds_row_returns_zero);
    RUN_TEST(test_remove_field_out_of_bounds_entry_returns_zero);
    RUN_TEST(test_remove_only_field_reduces_width_to_zero);
    RUN_TEST(test_remove_all_fields_one_by_one);
    RUN_TEST(test_remove_field_return_value_is_zero_for_valid_input);
    RUN_TEST(test_remove_second_to_last_field_leaves_one);
    RUN_TEST(test_remove_field_with_empty_string_content);
    RUN_TEST(test_remove_field_does_not_change_buffer_height);
    return UNITY_END();
}