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

/* Helper: duplicate a string literal into a writable heap buffer */
static char *make_writable(const char *src)
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
 * Test 2: Whitespace-only string becomes empty
 * ---------------------------------------------------------------------- */
void test_minify_whitespace_only(void)
{
    char *json = make_writable("   \t\r\n   ");
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
    char *json = make_writable(
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
 * Test 4: Single-line comment is stripped
 * ---------------------------------------------------------------------- */
void test_minify_single_line_comment(void)
{
    char *json = make_writable(
        "{"
        "// this is a comment\n"
        "\"key\":\"value\""
        "}"
    );
    TEST_ASSERT_NOT_NULL(json);

    cJSON_Minify(json);

    TEST_ASSERT_EQUAL_STRING_MESSAGE(
        "{\"key\":\"value\"}",
        json,
        "Single-line comment should be stripped during minification"
    );
    free(json);
}

/* -------------------------------------------------------------------------
 * Test 5: String containing whitespace and escape sequences is preserved
 * ---------------------------------------------------------------------- */
void test_minify_string_with_whitespace_preserved(void)
{
    /* Whitespace inside a quoted string must NOT be removed */
    char *json = make_writable(
        "{ \"greeting\" : \"hello world\" }"
    );
    TEST_ASSERT_NOT_NULL(json);

    cJSON_Minify(json);

    TEST_ASSERT_EQUAL_STRING_MESSAGE(
        "{\"greeting\":\"hello world\"}",
        json,
        "Whitespace inside quoted strings must be preserved"
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
    RUN_TEST(test_minify_single_line_comment);
    RUN_TEST(test_minify_string_with_whitespace_preserved);
    return UNITY_END();
}