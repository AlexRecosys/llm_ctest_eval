#include "crs_string.h"
#include "unity.h"
#include "crs_core.h"
#include "crs_dir.h"

static rsMA_t *MA = NULL;

void setUp(void) {
    MA = rs_get_MA();
    TEST_ASSERT_NOT_NULL(MA);
}

void tearDown(void) {
    // No explicit cleanup needed for MA as it's a shared allocator
}

static rscstring_t *make_test_string(const char *str, rsMA_t *MA) {
    rscstring_t *copy = (rscstring_t *)rs_alloc(strlen(str) + 1, MA);
    TEST_ASSERT_NOT_NULL(copy);
    strcpy(copy, str);
    return copy;
}

static void test_reverse_ascii(void) {
    rscstring_t *src = make_test_string("hello", MA);
    rscstring_t *dest = rscstring_reverse(src, MA);
    TEST_ASSERT_NOT_NULL(dest);
    TEST_ASSERT_EQUAL_STRING("olleh", dest);
    rs_free(dest, MA);
    rs_free(src, MA);
}

static void test_reverse_empty_string(void) {
    rscstring_t *src = make_test_string("", MA);
    rscstring_t *dest = rscstring_reverse(src, MA);
    TEST_ASSERT_NOT_NULL(dest);
    TEST_ASSERT_EQUAL_STRING("", dest);
    rs_free(dest, MA);
    rs_free(src, MA);
}

static void test_reverse_null_input(void) {
    rscstring_t *dest = rscstring_reverse(NULL, MA);
    TEST_ASSERT_NULL(dest);
}

static void test_reverse_single_char(void) {
    rscstring_t *src = make_test_string("a", MA);
    rscstring_t *dest = rscstring_reverse(src, MA);
    TEST_ASSERT_NOT_NULL(dest);
    TEST_ASSERT_EQUAL_STRING("a", dest);
    rs_free(dest, MA);
    rs_free(src, MA);
}

static void test_reverse_multi_byte_utf8_2byte(void) {
    // "ä" is 0xC3 0xA4 in UTF-8 (2 bytes)
    rscstring_t src_bytes[] = {0xC3, 0xA4, 0x00};
    rscstring_t *src = (rscstring_t *)src_bytes;
    rscstring_t *dest = rscstring_reverse(src, MA);
    TEST_ASSERT_NOT_NULL(dest);
    TEST_ASSERT_EQUAL_MEMORY(src_bytes, dest, 2);
    TEST_ASSERT_EQUAL_STRING("ä", dest);
    rs_free(dest, MA);
}

static void test_reverse_multi_byte_utf8_3byte(void) {
    // "€" is 0xE2 0x82 0xAC in UTF-8 (3 bytes)
    rscstring_t src_bytes[] = {0xE2, 0x82, 0xAC, 0x00};
    rscstring_t *src = (rscstring_t *)src_bytes;
    rscstring_t *dest = rscstring_reverse(src, MA);
    TEST_ASSERT_NOT_NULL(dest);
    TEST_ASSERT_EQUAL_MEMORY(src_bytes, dest, 3);
    TEST_ASSERT_EQUAL_STRING("€", dest);
    rs_free(dest, MA);
}

static void test_reverse_multi_byte_utf8_4byte(void) {
    // "😀" is 0xF0 0x9F 0x98 0x80 in UTF-8 (4 bytes)
    rscstring_t src_bytes[] = {0xF0, 0x9F, 0x98, 0x80, 0x00};
    rscstring_t *src = (rscstring_t *)src_bytes;
    rscstring_t *dest = rscstring_reverse(src, MA);
    TEST_ASSERT_NOT_NULL(dest);
    TEST_ASSERT_EQUAL_MEMORY(src_bytes, dest, 4);
    TEST_ASSERT_EQUAL_STRING("😀", dest);
    rs_free(dest, MA);
}

static void test_reverse_mixed_ascii_and_utf8(void) {
    // "aäb€c😀d" -> "d😀c€bäa"
    rscstring_t src_bytes[] = {
        'a',
        0xC3, 0xA4,  // ä
        'b',
        0xE2, 0x82, 0xAC,  // €
        'c',
        0xF0, 0x9F, 0x98, 0x80,  // 😀
        'd',
        0x00
    };
    rscstring_t *src = (rscstring_t *)src_bytes;
    rscstring_t *dest = rscstring_reverse(src, MA);
    TEST_ASSERT_NOT_NULL(dest);

    // Expected: "d😀c€bäa"
    rscstring_t expected_bytes[] = {
        'd',
        0xF0, 0x9F, 0x98, 0x80,  // 😀
        'c',
        0xE2, 0x82, 0xAC,  // €
        'b',
        0xC3, 0xA4,  // ä
        'a',
        0x00
    };
    TEST_ASSERT_EQUAL_MEMORY(expected_bytes, dest, sizeof(expected_bytes) - 1);
    TEST_ASSERT_EQUAL_STRING((rscstring_t *)expected_bytes, dest);
    rs_free(dest, MA);
}

static void test_reverse_multiple_multi_byte_chars(void) {
    // "ä€😀" -> "😀€ä"
    rscstring_t src_bytes[] = {
        0xC3, 0xA4,  // ä
        0xE2, 0x82, 0xAC,  // €
        0xF0, 0x9F, 0x98, 0x80,  // 😀
        0x00
    };
    rscstring_t *src = (rscstring_t *)src_bytes;
    rscstring_t *dest = rscstring_reverse(src, MA);
    TEST_ASSERT_NOT_NULL(dest);

    rscstring_t expected_bytes[] = {
        0xF0, 0x9F, 0x98, 0x80,  // 😀
        0xE2, 0x82, 0xAC,  // €
        0xC3, 0xA4,  // ä
        0x00
    };
    TEST_ASSERT_EQUAL_MEMORY(expected_bytes, dest, sizeof(expected_bytes) - 1);
    TEST_ASSERT_EQUAL_STRING((rscstring_t *)expected_bytes, dest);
    rs_free(dest, MA);
}

static void test_reverse_long_string(void) {
    const char *src_str = "The quick brown fox jumps over the lazy dog.";
    rscstring_t *src = make_test_string(src_str, MA);
    rscstring_t *dest = rscstring_reverse(src, MA);
    TEST_ASSERT_NOT_NULL(dest);

    // Expected: ".god yzal eht revo spmuj xof nworb kciuq ehT"
    const char *expected = ".god yzal eht revo spmuj xof nworb kciuq ehT";
    TEST_ASSERT_EQUAL_STRING(expected, dest);
    rs_free(dest, MA);
    rs_free(src, MA);
}

static void test_reverse_with_null_in_middle_should_not_happen(void) {
    // This test ensures that the function handles strings with embedded nulls correctly
    // In practice, rscstring_t is null-terminated, so embedded nulls are not expected.
    // But we test that the function stops at the first null.
    rscstring_t src_bytes[] = {'a', 'b', 'c', 0, 'd', 'e', 0};
    rscstring_t *src = (rscstring_t *)src_bytes;
    rscstring_t *dest = rscstring_reverse(src, MA);
    TEST_ASSERT_NOT_NULL(dest);
    // Only "abc" should be reversed to "cba"
    TEST_ASSERT_EQUAL_STRING("cba", dest);
    rs_free(dest, MA);
}

int main(void) {
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_reverse_ascii);
    RUN_TEST(test_reverse_empty_string);
    RUN_TEST(test_reverse_null_input);
    RUN_TEST(test_reverse_single_char);
    RUN_TEST(test_reverse_multi_byte_utf8_2byte);
    RUN_TEST(test_reverse_multi_byte_utf8_3byte);
    RUN_TEST(test_reverse_multi_byte_utf8_4byte);
    RUN_TEST(test_reverse_mixed_ascii_and_utf8);
    RUN_TEST(test_reverse_multiple_multi_byte_chars);
    RUN_TEST(test_reverse_long_string);
    RUN_TEST(test_reverse_with_null_in_middle_should_not_happen);
    return UNITY_END();
}