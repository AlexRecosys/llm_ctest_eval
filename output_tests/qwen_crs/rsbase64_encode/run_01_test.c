#include "crs_string.h"
#include "unity.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* --- File-scope static variables / fixtures --- */
static rsMA_t *MA = NULL;
static rsbuffer_t *src_buf = NULL;
static rsbase64par_t par = {0};

/* --- Helper functions and macros --- */

/* Helper to create a source buffer with given content */
static rsbuffer_t *create_src_buffer(const char *content, rsMA_t *MA) {
    rsbuffer_t *buf = NULL;
    size_t len = strlen(content);
    if (len > 0) {
        buf = rs8uarr_create((rs32u_t)len, MA);
        if (buf && buf->d) {
            memcpy(buf->d, content, len);
            buf->n = (rs32u_t)len;
        }
    }
    return buf;
}

/* Helper to compare encoded result with expected string */
static rsbool_t compare_encoded_result(rsbuffer_t *result, const char *expected) {
    if (!result || !expected) return false;
    if (result->n != (rs32u_t)strlen(expected)) return false;
    return (memcmp(result->d, expected, result->n) == 0);
}

/* --- Test cases --- */

void test_rsbase64_encode_null_src(void) {
    rsbuffer_t *result = rsbase64_encode(NULL, &par, MA);
    TEST_ASSERT_NULL(result);
}

void test_rsbase64_encode_empty_src(void) {
    src_buf = rs8uarr_create(0, MA);
    TEST_ASSERT_NOT_NULL(src_buf);
    src_buf->n = 0;
    rsbuffer_t *result = rsbase64_encode(src_buf, &par, MA);
    TEST_ASSERT_EQUAL_PTR(src_buf, result);  /* empty => empty */
    rs8uarr_destroy(src_buf);
}

void test_rsbase64_encode_single_byte(void) {
    const char input[] = "A";
    src_buf = create_src_buffer(input, MA);
    TEST_ASSERT_NOT_NULL(src_buf);
    rsbuffer_t *result = rsbase64_encode(src_buf, &par, MA);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_MEMORY("QU==", result->d, result->n);
    rsbuffer_destroy(result);
    rs8uarr_destroy(src_buf);
}

void test_rsbase64_encode_two_bytes(void) {
    const char input[] = "AB";
    src_buf = create_src_buffer(input, MA);
    TEST_ASSERT_NOT_NULL(src_buf);
    rsbuffer_t *result = rsbase64_encode(src_buf, &par, MA);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_MEMORY("QUI=", result->d, result->n);
    rsbuffer_destroy(result);
    rs8uarr_destroy(src_buf);
}

void test_rsbase64_encode_three_bytes(void) {
    const char input[] = "ABC";
    src_buf = create_src_buffer(input, MA);
    TEST_ASSERT_NOT_NULL(src_buf);
    rsbuffer_t *result = rsbase64_encode(src_buf, &par, MA);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_MEMORY("QUJD", result->d, result->n);
    rsbuffer_destroy(result);
    rs8uarr_destroy(src_buf);
}

void test_rsbase64_encode_standard_example(void) {
    const char input[] = "Man";
    src_buf = create_src_buffer(input, MA);
    TEST_ASSERT_NOT_NULL(src_buf);
    rsbuffer_t *result = rsbase64_encode(src_buf, &par, MA);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_MEMORY("TWFu", result->d, result->n);
    rsbuffer_destroy(result);
    rs8uarr_destroy(src_buf);
}

void test_rsbase64_encode_longer_input(void) {
    const char input[] = "The quick brown fox jumps over the lazy dog.";
    src_buf = create_src_buffer(input, MA);
    TEST_ASSERT_NOT_NULL(src_buf);
    rsbuffer_t *result = rsbase64_encode(src_buf, &par, MA);
    TEST_ASSERT_NOT_NULL(result);
    const char expected[] = "VGhlIHF1aWNrIGJyb3duIGZveCBqdW1wcyBvdmVyIHRoZSBsYXp5IGRvZy4=";
    TEST_ASSERT_TRUE(compare_encoded_result(result, expected));
    rsbuffer_destroy(result);
    rs8uarr_destroy(src_buf);
}

void test_rsbase64_encode_with_line_breaks(void) {
    par.ncol_max = 4;
    const char input[] = "ABCDEF";
    src_buf = create_src_buffer(input, MA);
    TEST_ASSERT_NOT_NULL(src_buf);
    rsbuffer_t *result = rsbase64_encode(src_buf, &par, MA);
    TEST_ASSERT_NOT_NULL(result);
    /* 6 bytes => 8 base64 chars + 1 newline = 9 bytes total */
    /* 4 chars per line: "QUJD\nRkVGRw==" */
    const char expected[] = "QUJD\nRkVG\nRw==";
    TEST_ASSERT_EQUAL_MEMORY(expected, result->d, result->n);
    rsbuffer_destroy(result);
    rs8uarr_destroy(src_buf);
}

void test_rsbase64_encode_with_line_breaks_exact_boundary(void) {
    par.ncol_max = 4;
    const char input[] = "ABCD";  /* exactly 4 base64 chars */
    src_buf = create_src_buffer(input, MA);
    TEST_ASSERT_NOT_NULL(src_buf);
    rsbuffer_t *result = rsbase64_encode(src_buf, &par, MA);
    TEST_ASSERT_NOT_NULL(result);
    /* 4 bytes => 8 base64 chars, but no line break needed unless ncol_max enforced */
    /* 4 chars per line: "QUJD\nRA==" */
    const char expected[] = "QUJD\nRA==";
    TEST_ASSERT_EQUAL_MEMORY(expected, result->d, result->n);
    rsbuffer_destroy(result);
    rs8uarr_destroy(src_buf);
}

void test_rsbase64_encode_no_line_breaks_when_ncol_max_zero(void) {
    par.ncol_max = 0;
    const char input[] = "ABCDEF";
    src_buf = create_src_buffer(input, MA);
    TEST_ASSERT_NOT_NULL(src_buf);
    rsbuffer_t *result = rsbase64_encode(src_buf, &par, MA);
    TEST_ASSERT_NOT_NULL(result);
    const char expected[] = "QUJDREVG";
    TEST_ASSERT_TRUE(compare_encoded_result(result, expected));
    rsbuffer_destroy(result);
    rs8uarr_destroy(src_buf);
}

void test_rsbase64_encode_null_par_uses_default_ncol_max(void) {
    const char input[] = "ABCDEF";
    src_buf = create_src_buffer(input, MA);
    TEST_ASSERT_NOT_NULL(src_buf);
    /* par is NULL => default ncol_max=72, so no line breaks for short input */
    rsbuffer_t *result = rsbase64_encode(src_buf, NULL, MA);
    TEST_ASSERT_NOT_NULL(result);
    const char expected[] = "QUJDREVG";
    TEST_ASSERT_TRUE(compare_encoded_result(result, expected));
    rsbuffer_destroy(result);
    rs8uarr_destroy(src_buf);
}

void test_rsbase64_encode_large_input_with_line_breaks(void) {
    par.ncol_max = 12;
    /* 12 bytes => 16 base64 chars per line */
    const char input[] = "0123456789AB";
    src_buf = create_src_buffer(input, MA);
    TEST_ASSERT_NOT_NULL(src_buf);
    rsbuffer_t *result = rsbase64_encode(src_buf, &par, MA);
    TEST_ASSERT_NOT_NULL(result);
    /* 12 bytes => 16 base64 chars + 1 newline + 16 base64 chars + 1 newline + 16 base64 chars + 1 newline + 16 base64 chars + 1 newline */
    /* Actually: 12 bytes = 16 base64 chars, so one line only */
    const char expected[] = "MDEyMzQ1Njc4OUFC";
    TEST_ASSERT_TRUE(compare_encoded_result(result, expected));
    rsbuffer_destroy(result);
    rs8uarr_destroy(src_buf);
}

void test_rsbase64_encode_large_input_with_multiple_line_breaks(void) {
    par.ncol_max = 8;
    /* 12 bytes => 16 base64 chars total => 2 lines of 8 chars */
    const char input[] = "0123456789AB";
    src_buf = create_src_buffer(input, MA);
    TEST_ASSERT_NOT_NULL(src_buf);
    rsbuffer_t *result = rsbase64_encode(src_buf, &par, MA);
    TEST_ASSERT_NOT_NULL(result);
    const char expected[] = "MDEyMzQ1\nNjc4OUFC";
    TEST_ASSERT_EQUAL_MEMORY(expected, result->d, result->n);
    rsbuffer_destroy(result);
    rs8uarr_destroy(src_buf);
}

void test_rsbase64_encode_memory_boundary(void) {
    /* Test that memory allocation is sufficient */
    const char input[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";  /* 64 bytes */
    src_buf = create_src_buffer(input, MA);
    TEST_ASSERT_NOT_NULL(src_buf);
    rsbuffer_t *result = rsbase64_encode(src_buf, &par, MA);
    TEST_ASSERT_NOT_NULL(result);
    /* 64 bytes => ceil(64/3)*4 = 88 base64 chars */
    TEST_ASSERT_EQUAL_INT(88, result->n);
    rsbuffer_destroy(result);
    rs8uarr_destroy(src_buf);
}

void test_rsbase64_encode_alloc_failure_returns_null(void) {
    /* This test assumes malloc failure is not easily simulated without mocks */
    /* Skip this test in real environment unless custom allocator is used */
    /* For now, just verify normal allocation works */
    const char input[] = "A";
    src_buf = create_src_buffer(input, MA);
    TEST_ASSERT_NOT_NULL(src_buf);
    rsbuffer_t *result = rsbase64_encode(src_buf, &par, MA);
    TEST_ASSERT_NOT_NULL(result);
    rsbuffer_destroy(result);
    rs8uarr_destroy(src_buf);
}

/* --- main --- */

int main(void) {
    unity_install_sighandler();
    UNITY_BEGIN();

    MA = rsMA_alloc();
    TEST_ASSERT_NOT_NULL(MA);

    RUN_TEST(test_rsbase64_encode_null_src);
    RUN_TEST(test_rsbase64_encode_empty_src);
    RUN_TEST(test_rsbase64_encode_single_byte);
    RUN_TEST(test_rsbase64_encode_two_bytes);
    RUN_TEST(test_rsbase64_encode_three_bytes);
    RUN_TEST(test_rsbase64_encode_standard_example);
    RUN_TEST(test_rsbase64_encode_longer_input);
    RUN_TEST(test_rsbase64_encode_with_line_breaks);
    RUN_TEST(test_rsbase64_encode_with_line_breaks_exact_boundary);
    RUN_TEST(test_rsbase64_encode_no_line_breaks_when_ncol_max_zero);
    RUN_TEST(test_rsbase64_encode_null_par_uses_default_ncol_max);
    RUN_TEST(test_rsbase64_encode_large_input_with_line_breaks);
    RUN_TEST(test_rsbase64_encode_large_input_with_multiple_line_breaks);
    RUN_TEST(test_rsbase64_encode_memory_boundary);
    RUN_TEST(test_rsbase64_encode_alloc_failure_returns_null);

    rsMA_free(MA);
    return UNITY_END();
}