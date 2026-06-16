#include "csv.c"
#include "unity.h"

static CSV_BUFFER *dest_buffer = NULL;
static CSV_BUFFER *source_buffer = NULL;

void setUp(void) {
    dest_buffer = csv_create_buffer();
    source_buffer = csv_create_buffer();
}

void tearDown(void) {
    csv_destroy_buffer(dest_buffer);
    csv_destroy_buffer(source_buffer);
    dest_buffer = NULL;
    source_buffer = NULL;
}

static void setup_source_with_rows(const char * const rows[], int num_rows) {
    int i, j;
    char *field;
    char *token;
    char line[1024];

    for (i = 0; i < num_rows; i++) {
        if (rows[i] == NULL) {
            append_row(source_buffer);
            continue;
        }
        append_row(source_buffer);
        strncpy(line, rows[i], sizeof(line) - 1);
        line[sizeof(line) - 1] = '\0';
        field = strtok(line, ",");
        j = 0;
        while (field != NULL) {
            if (j > 0) {
                append_field(source_buffer, i);
            }
            set_field(source_buffer->field[i][j], field);
            field = strtok(NULL, ",");
            j++;
        }
    }
}

static void setup_dest_with_rows(const char * const rows[], int num_rows) {
    int i, j;
    char *field;
    char line[1024];

    for (i = 0; i < num_rows; i++) {
        if (rows[i] == NULL) {
            append_row(dest_buffer);
            continue;
        }
        append_row(dest_buffer);
        strncpy(line, rows[i], sizeof(line) - 1);
        line[sizeof(line) - 1] = '\0';
        field = strtok(line, ",");
        j = 0;
        while (field != NULL) {
            if (j > 0) {
                append_field(dest_buffer, i);
            }
            set_field(dest_buffer->field[i][j], field);
            field = strtok(NULL, ",");
            j++;
        }
    }
}

static int get_field_count(CSV_BUFFER *buf, int row) {
    if (row < 0 || row >= (int)buf->rows) return -1;
    return (int)buf->width[row];
}

static char *get_field_text(CSV_BUFFER *buf, int row, int col) {
    if (row < 0 || row >= (int)buf->rows) return NULL;
    if (col < 0 || col >= (int)buf->width[row]) return NULL;
    if (buf->field[row][col] == NULL || buf->field[row][col]->text == NULL) return NULL;
    return buf->field[row][col]->text;
}

static void assert_field_equal(CSV_BUFFER *buf, int row, int col, const char *expected) {
    char *actual = get_field_text(buf, row, col);
    if (expected == NULL) {
        TEST_ASSERT_NULL_MESSAGE(actual, "Field should be NULL");
    } else {
        TEST_ASSERT_EQUAL_STRING_MESSAGE(expected, actual, "Field content mismatch");
    }
}

static void assert_row_width(CSV_BUFFER *buf, int row, int expected_width) {
    int actual_width = get_field_count(buf, row);
    TEST_ASSERT_EQUAL_INT_MESSAGE(expected_width, actual_width, "Row width mismatch");
}

static void assert_row_count(CSV_BUFFER *buf, int expected_rows) {
    TEST_ASSERT_EQUAL_INT_MESSAGE(expected_rows, (int)buf->rows, "Row count mismatch");
}

void test_csv_copy_row_success_basic(void) {
    const char *src_rows[] = {"a,b,c", "d,e,f"};
    setup_source_with_rows(src_rows, 2);

    int result = csv_copy_row(dest_buffer, 0, source_buffer, 0);
    TEST_ASSERT_EQUAL_INT(0, result);
    assert_row_count(dest_buffer, 1);
    assert_row_width(dest_buffer, 0, 3);
    assert_field_equal(dest_buffer, 0, 0, "a");
    assert_field_equal(dest_buffer, 0, 1, "b");
    assert_field_equal(dest_buffer, 0, 2, "c");
}

void test_csv_copy_row_dest_row_expansion(void) {
    const char *src_rows[] = {"x,y"};
    setup_source_with_rows(src_rows, 1);

    int result = csv_copy_row(dest_buffer, 3, source_buffer, 0);
    TEST_ASSERT_EQUAL_INT(0, result);
    assert_row_count(dest_buffer, 4);
    assert_row_width(dest_buffer, 3, 2);
    assert_field_equal(dest_buffer, 3, 0, "x");
    assert_field_equal(dest_buffer, 3, 1, "y");
}

void test_csv_copy_row_dest_row_shrink(void) {
    const char *dest_rows[] = {"a,b,c,d"};
    setup_dest_with_rows(dest_rows, 1);

    const char *src_rows[] = {"x,y"};
    setup_source_with_rows(src_rows, 1);

    int result = csv_copy_row(dest_buffer, 0, source_buffer, 0);
    TEST_ASSERT_EQUAL_INT(0, result);
    assert_row_width(dest_buffer, 0, 2);
    assert_field_equal(dest_buffer, 0, 0, "x");
    assert_field_equal(dest_buffer, 0, 1, "y");
}

void test_csv_copy_row_dest_row_expand(void) {
    const char *dest_rows[] = {"a"};
    setup_dest_with_rows(dest_rows, 1);

    const char *src_rows[] = {"x,y,z"};
    setup_source_with_rows(src_rows, 1);

    int result = csv_copy_row(dest_buffer, 0, source_buffer, 0);
    TEST_ASSERT_EQUAL_INT(0, result);
    assert_row_width(dest_buffer, 0, 3);
    assert_field_equal(dest_buffer, 0, 0, "x");
    assert_field_equal(dest_buffer, 0, 1, "y");
    assert_field_equal(dest_buffer, 0, 2, "z");
}

void test_csv_copy_row_same_buffer_overwrite(void) {
    const char *rows[] = {"a,b,c", "d,e,f"};
    setup_source_with_rows(rows, 2);
    dest_buffer = source_buffer;

    int result = csv_copy_row(dest_buffer, 0, source_buffer, 1);
    TEST_ASSERT_EQUAL_INT(0, result);
    assert_row_width(dest_buffer, 0, 3);
    assert_field_equal(dest_buffer, 0, 0, "d");
    assert_field_equal(dest_buffer, 0, 1, "e");
    assert_field_equal(dest_buffer, 0, 2, "f");
}

void test_csv_copy_row_source_row_out_of_bounds(void) {
    const char *src_rows[] = {"a,b"};
    setup_source_with_rows(src_rows, 1);

    int result = csv_copy_row(dest_buffer, 0, source_buffer, 5);
    TEST_ASSERT_EQUAL_INT(0, result);
    assert_row_count(dest_buffer, 1);
    assert_row_width(dest_buffer, 0, 0);
}

void test_csv_copy_row_empty_source_row(void) {
    const char *src_rows[] = {""};
    setup_source_with_rows(src_rows, 1);

    int result = csv_copy_row(dest_buffer, 0, source_buffer, 0);
    TEST_ASSERT_EQUAL_INT(0, result);
    assert_row_width(dest_buffer, 0, 1);
    assert_field_equal(dest_buffer, 0, 0, "");
}

void test_csv_copy_row_dest_row_clear_on_invalid_source(void) {
    const char *dest_rows[] = {"x,y,z"};
    setup_dest_with_rows(dest_rows, 1);

    int result = csv_copy_row(dest_buffer, 0, source_buffer, 100);
    TEST_ASSERT_EQUAL_INT(0, result);
    assert_row_width(dest_buffer, 0, 0);
}

void test_csv_copy_row_multiple_rows(void) {
    const char *src_rows[] = {"a,b", "c,d", "e,f"};
    setup_source_with_rows(src_rows, 3);

    int result = csv_copy_row(dest_buffer, 1, source_buffer, 2);
    TEST_ASSERT_EQUAL_INT(0, result);
    assert_row_count(dest_buffer, 2);
    assert_row_width(dest_buffer, 1, 2);
    assert_field_equal(dest_buffer, 1, 0, "e");
    assert_field_equal(dest_buffer, 1, 1, "f");
}

void test_csv_copy_row_dest_row_zero_expansion(void) {
    const char *src_rows[] = {"one,two,three"};
    setup_source_with_rows(src_rows, 1);

    int result = csv_copy_row(dest_buffer, 0, source_buffer, 0);
    TEST_ASSERT_EQUAL_INT(0, result);
    assert_row_count(dest_buffer, 1);
    assert_row_width(dest_buffer, 0, 3);
    assert_field_equal(dest_buffer, 0, 0, "one");
    assert_field_equal(dest_buffer, 0, 1, "two");
    assert_field_equal(dest_buffer, 0, 2, "three");
}

void test_csv_copy_row_dest_row_negative_handling(void) {
    const char *src_rows[] = {"a,b"};
    setup_source_with_rows(src_rows, 1);

    int result = csv_copy_row(dest_buffer, -1, source_buffer, 0);
    TEST_ASSERT_EQUAL_INT(0, result);
    assert_row_count(dest_buffer, 1);
    assert_row_width(dest_buffer, 0, 2);
    assert_field_equal(dest_buffer, 0, 0, "a");
    assert_field_equal(dest_buffer, 0, 1, "b");
}

int main(void) {
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_csv_copy_row_success_basic);
    RUN_TEST(test_csv_copy_row_dest_row_expansion);
    RUN_TEST(test_csv_copy_row_dest_row_shrink);
    RUN_TEST(test_csv_copy_row_dest_row_expand);
    RUN_TEST(test_csv_copy_row_same_buffer_overwrite);
    RUN_TEST(test_csv_copy_row_source_row_out_of_bounds);
    RUN_TEST(test_csv_copy_row_empty_source_row);
    RUN_TEST(test_csv_copy_row_dest_row_clear_on_invalid_source);
    RUN_TEST(test_csv_copy_row_multiple_rows);
    RUN_TEST(test_csv_copy_row_dest_row_zero_expansion);
    RUN_TEST(test_csv_copy_row_dest_row_negative_handling);
    return UNITY_END();
}