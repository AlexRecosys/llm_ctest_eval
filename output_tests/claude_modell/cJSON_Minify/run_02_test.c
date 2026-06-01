#include "cJSON.c"
#include "unity.h"
#include "cJSON.h"
#include <stdlib.h>
#include <string.h>
#include <signal.h>

/* File-scope variables */
static volatile int segfault_occurred = 0;

/* Signal handler for SIGSEGV */
static void sigsegv_handler(int sig)
{
    (void)sig;
    segfault_occurred = 1;
    /* We cannot safely longjmp here in all cases, so just mark and return */
    TEST_FAIL_MESSAGE("Segmentation fault occurred during test");
}

void setUp(void)
{
    segfault_occurred = 0;
    signal(SIGSEGV, sigsegv_handler);
}

void tearDown(void)
{
    signal(SIGSEGV, SIG_DFL);
}

/* Helper: duplicate a string literal into a mutable heap buffer */
static char *make_mutable(const char *src)
{
    size_t len = strlen(src) + 1;
    char *buf = (char *)malloc(len);
    if (buf != NULL)
    {
        memcpy(buf, src, len);
    }
    return buf;
}

/* -------------------------------------------------------------------------
 * Test 1: NULL input — must not crash
 * ---------------------------------------------------------------------- */
void test_minify_null_input(void)
{
    /* Should return immediately without crashing */
    cJSON_Minify(NULL);
    TEST_ASSERT_TRUE_MESSAGE(1, "cJSON_Minify(NULL) should not crash");
}

/* -------------------------------------------------------------------------
 * Test 2: Whitespace-only string becomes empty string
 * ---------------------------------------------------------------------- */
void test_minify_whitespace_only(void)
{
    char *json = make_mutable("   \t\r\n   ");
    TEST_ASSERT_NOT_NULL(json);

    cJSON_Minify(json);

    TEST_ASSERT_EQUAL_STRING_MESSAGE("", json, "Whitespace-only input should become empty string");
    free(json);
}

/* -------------------------------------------------------------------------
 * Test 3: Typical JSON object with whitespace is compacted
 * ---------------------------------------------------------------------- */
void test_minify_typical_json_object(void)
{
    char *json = make_mutable(
        "{\n"
        "    \"key\" : \"value\",\n"
        "    \"number\" : 42\n"
        "}"
    );
    TEST_ASSERT_NOT_NULL(json);

    cJSON_Minify(json);

    TEST_ASSERT_EQUAL_STRING_MESSAGE(
        "{\"key\":\"value\",\"number\":42}",
        json,
        "Typical JSON object should be minified correctly"
    );
    free(json);
}

/* -------------------------------------------------------------------------
 * Test 4: Single-line (//) and multi-line (/* *\/) comments are stripped
 * ---------------------------------------------------------------------- */
void test_minify_comments_removed(void)
{
    /* Single-line comment */
    char *json_sl = make_mutable("{\"a\":1}// this is a comment\n");
    TEST_ASSERT_NOT_NULL(json_sl);
    cJSON_Minify(json_sl);
    TEST_ASSERT_EQUAL_STRING_MESSAGE(
        "{\"a\":1}",
        json_sl,
        "Single-line comment should be stripped"
    );
    free(json_sl);

    /* Multi-line comment */
    char *json_ml = make_mutable("{/* comment */\"b\":2}");
    TEST_ASSERT_NOT_NULL(json_ml);
    cJSON_Minify(json_ml);
    TEST_ASSERT_EQUAL_STRING_MESSAGE(
        "{\"b\":2}",
        json_ml,
        "Multi-line comment should be stripped"
    );
    free(json_ml);
}

/* -------------------------------------------------------------------------
 * Test 5: Whitespace inside string literals is preserved
 * ---------------------------------------------------------------------- */
void test_minify_preserves_whitespace_in_strings(void)
{
    char *json = make_mutable(
        "{ \"greeting\" : \"hello world\" }"
    );
    TEST_ASSERT_NOT_NULL(json);

    cJSON_Minify(json);

    TEST_ASSERT_EQUAL_STRING_MESSAGE(
        "{\"greeting\":\"hello world\"}",
        json,
        "Whitespace inside string values must be preserved"
    );
    free(json);
}

/* -------------------------------------------------------------------------
 * main
 * ---------------------------------------------------------------------- */
int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_minify_null_input);
    RUN_TEST(test_minify_whitespace_only);
    RUN_TEST(test_minify_typical_json_object);
    RUN_TEST(test_minify_comments_removed);
    RUN_TEST(test_minify_preserves_whitespace_in_strings);
    return UNITY_END();
}