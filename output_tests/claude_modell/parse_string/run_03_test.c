#include "cJSON.c"
#include "unity.h"
#include "cJSON.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* -------------------------------------------------------------------------
 * Helper utilities
 * ---------------------------------------------------------------------- */

/* Build a parse_buffer that wraps a C string (including the surrounding
 * double-quotes that parse_string expects).  The caller owns the memory
 * pointed to by buf->content – it must NOT be freed through the hooks
 * because we pass a stack / static buffer.  We therefore set the hooks to
 * the real malloc/free so that the internal allocation inside parse_string
 * works correctly.
 */
static void init_parse_buffer(parse_buffer *buf,
                               const unsigned char *content,
                               size_t length,
                               size_t offset)
{
    memset(buf, 0, sizeof(*buf));
    buf->content = content;
    buf->length  = length;
    buf->offset  = offset;
    buf->hooks.allocate   = malloc;
    buf->hooks.deallocate = free;
    buf->hooks.reallocate = realloc;
}

/* Convenience: wrap a quoted JSON string literal into a parse_buffer
 * positioned at offset 0 so that buffer_at_offset() returns the opening '"'.
 */
static void init_parse_buffer_str(parse_buffer *buf,
                                   const char *json_string)
{
    init_parse_buffer(buf,
                      (const unsigned char *)json_string,
                      strlen(json_string),
                      0);
}

/* Create a zeroed cJSON item on the stack. */
static cJSON make_item(void)
{
    cJSON item;
    memset(&item, 0, sizeof(item));
    return item;
}

/* -------------------------------------------------------------------------
 * setUp / tearDown
 * ---------------------------------------------------------------------- */

void setUp(void)
{
    /* Reset cJSON hooks to defaults before every test. */
    cJSON_InitHooks(NULL);
}

void tearDown(void)
{
}

/* =========================================================================
 * Test cases
 * ========================================================================= */

/* --- 1. Simple ASCII string -------------------------------------------- */
void test_parse_string_simple_ascii(void)
{
    cJSON item = make_item();
    parse_buffer buf;
    const char *json = "\"hello\"";

    init_parse_buffer_str(&buf, json);

    cJSON_bool result = parse_string(&item, &buf);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_NOT_NULL(item.valuestring);
    TEST_ASSERT_EQUAL_STRING("hello", item.valuestring);

    /* offset should point one past the closing '"' */
    TEST_ASSERT_EQUAL_UINT(strlen(json), buf.offset);

    free(item.valuestring);
}

/* --- 2. Empty string ---------------------------------------------------- */
void test_parse_string_empty(void)
{
    cJSON item = make_item();
    parse_buffer buf;
    const char *json = "\"\"";

    init_parse_buffer_str(&buf, json);

    cJSON_bool result = parse_string(&item, &buf);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);
    TEST_ASSERT_NOT_NULL(item.valuestring);
    TEST_ASSERT_EQUAL_STRING("", item.valuestring);

    free(item.valuestring);
}

/* --- 3. Fail: input does not start with '"' ----------------------------- */
void test_parse_string_not_a_string(void)
{
    cJSON item = make_item();
    parse_buffer buf;
    const char *json = "hello";

    init_parse_buffer_str(&buf, json);

    cJSON_bool result = parse_string(&item, &buf);

    TEST_ASSERT_FALSE(result);
    /* item type must remain untouched */
    TEST_ASSERT_EQUAL_INT(0, item.type);
    TEST_ASSERT_NULL(item.valuestring);
}

/* --- 4. Fail: unterminated string (no closing '"') --------------------- */
void test_parse_string_unterminated(void)
{
    cJSON item = make_item();
    parse_buffer buf;
    const char *json = "\"hello";

    init_parse_buffer_str(&buf, json);

    cJSON_bool result = parse_string(&item, &buf);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(item.valuestring);
}

/* --- 5. Escape sequence: backslash-n ----------------------------------- */
void test_parse_string_escape_newline(void)
{
    cJSON item = make_item();
    parse_buffer buf;
    const char *json = "\"line1\\nline2\"";

    init_parse_buffer_str(&buf, json);

    cJSON_bool result = parse_string(&item, &buf);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(item.valuestring);
    TEST_ASSERT_EQUAL_STRING("line1\nline2", item.valuestring);

    free(item.valuestring);
}

/* --- 6. Escape sequence: backslash-t ----------------------------------- */
void test_parse_string_escape_tab(void)
{
    cJSON item = make_item();
    parse_buffer buf;
    const char *json = "\"col1\\tcol2\"";

    init_parse_buffer_str(&buf, json);

    cJSON_bool result = parse_string(&item, &buf);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(item.valuestring);
    TEST_ASSERT_EQUAL_STRING("col1\tcol2", item.valuestring);

    free(item.valuestring);
}

/* --- 7. Escape sequence: backslash-r ----------------------------------- */
void test_parse_string_escape_carriage_return(void)
{
    cJSON item = make_item();
    parse_buffer buf;
    const char *json = "\"foo\\rbar\"";

    init_parse_buffer_str(&buf, json);

    cJSON_bool result = parse_string(&item, &buf);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(item.valuestring);
    TEST_ASSERT_EQUAL_STRING("foo\rbar", item.valuestring);

    free(item.valuestring);
}

/* --- 8. Escape sequence: backslash-b ----------------------------------- */
void test_parse_string_escape_backspace(void)
{
    cJSON item = make_item();
    parse_buffer buf;
    const char *json = "\"a\\bb\"";

    init_parse_buffer_str(&buf, json);

    cJSON_bool result = parse_string(&item, &buf);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(item.valuestring);
    TEST_ASSERT_EQUAL_STRING("a\bb", item.valuestring);

    free(item.valuestring);
}

/* --- 9. Escape sequence: backslash-f ----------------------------------- */
void test_parse_string_escape_formfeed(void)
{
    cJSON item = make_item();
    parse_buffer buf;
    const char *json = "\"a\\fb\"";

    init_parse_buffer_str(&buf, json);

    cJSON_bool result = parse_string(&item, &buf);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(item.valuestring);
    TEST_ASSERT_EQUAL_STRING("a\fb", item.valuestring);

    free(item.valuestring);
}

/* --- 10. Escape sequence: escaped double-quote ------------------------- */
void test_parse_string_escape_double_quote(void)
{
    cJSON item = make_item();
    parse_buffer buf;
    const char *json = "\"say \\\"hi\\\"\"";

    init_parse_buffer_str(&buf, json);

    cJSON_bool result = parse_string(&item, &buf);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(item.valuestring);
    TEST_ASSERT_EQUAL_STRING("say \"hi\"", item.valuestring);

    free(item.valuestring);
}

/* --- 11. Escape sequence: escaped backslash ---------------------------- */
void test_parse_string_escape_backslash(void)
{
    cJSON item = make_item();
    parse_buffer buf;
    const char *json = "\"a\\\\b\"";

    init_parse_buffer_str(&buf, json);

    cJSON_bool result = parse_string(&item, &buf);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(item.valuestring);
    TEST_ASSERT_EQUAL_STRING("a\\b", item.valuestring);

    free(item.valuestring);
}

/* --- 12. Escape sequence: escaped forward-slash ------------------------ */
void test_parse_string_escape_forward_slash(void)
{
    cJSON item = make_item();
    parse_buffer buf;
    const char *json = "\"a\\/b\"";

    init_parse_buffer_str(&buf, json);

    cJSON_bool result = parse_string(&item, &buf);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(item.valuestring);
    TEST_ASSERT_EQUAL_STRING("a/b", item.valuestring);

    free(item.valuestring);
}

/* --- 13. Fail: invalid escape sequence --------------------------------- */
void test_parse_string_invalid_escape(void)
{
    cJSON item = make_item();
    parse_buffer buf;
    /* \q is not a valid JSON escape */
    const char *json = "\"bad\\qescape\"";

    init_parse_buffer_str(&buf, json);

    cJSON_bool result = parse_string(&item, &buf);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(item.valuestring);
}

/* --- 14. Fail: backslash at end of buffer (no char after it) ----------- */
void test_parse_string_backslash_at_end(void)
{
    cJSON item = make_item();
    parse_buffer buf;
    /* The backslash is the last character before the closing quote would be,
     * but there is no character after it inside the string. */
    const unsigned char content[] = { '"', '\\' };  /* no closing '"' */
    init_parse_buffer(&buf, content, sizeof(content), 0);

    cJSON_bool result = parse_string(&item, &buf);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(item.valuestring);
}

/* --- 15. UTF-16 escape: basic BMP character \u0041 == 'A' ------------- */
void test_parse_string_utf16_basic(void)
{
    cJSON item = make_item();
    parse_buffer buf;
    const char *json = "\"\\u0041\"";   /* \u0041 == 'A' */

    init_parse_buffer_str(&buf, json);

    cJSON_bool result = parse_string(&item, &buf);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(item.valuestring);
    TEST_ASSERT_EQUAL_STRING("A", item.valuestring);

    free(item.valuestring);
}

/* --- 16. UTF-16 escape: \u0000 (null character) ------------------------ */
void test_parse_string_utf16_null_char(void)
{
    cJSON item = make_item();
    parse_buffer buf;
    const char *json = "\"\\u0000\"";

    init_parse_buffer_str(&buf, json);

    cJSON_bool result = parse_string(&item, &buf);

    /* parse_string should succeed; the null byte is embedded in valuestring */
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(item.valuestring);
    /* First byte of valuestring should be '\0' */
    TEST_ASSERT_EQUAL_INT(0, (unsigned char)item.valuestring[0]);

    free(item.valuestring);
}

/* --- 17. UTF-16 surrogate pair: U+1F600 (😀) -------------------------- */
void test_parse_string_utf16_surrogate_pair(void)
{
    cJSON item = make_item();
    parse_buffer buf;
    /* U+1F600 encoded as surrogate pair \uD83D\uDE00 */
    const char *json = "\"\\uD83D\\uDE00\"";

    init_parse_buffer_str(&buf, json);

    cJSON_bool result = parse_string(&item, &buf);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(item.valuestring);

    /* UTF-8 encoding of U+1F600: F0 9F 98 80 */
    const unsigned char expected[] = { 0xF0, 0x9F, 0x98, 0x80, 0x00 };
    TEST_ASSERT_EQUAL_MEMORY(expected, item.valuestring, 4);

    free(item.valuestring);
}

/* --- 18. Fail: incomplete surrogate pair (high surrogate only) --------- */
void test_parse_string_utf16_incomplete_surrogate(void)
{
    cJSON item = make_item();
    parse_buffer buf;
    /* High surrogate without a following low surrogate */
    const char *json = "\"\\uD83D\"";

    init_parse_buffer_str(&buf, json);

    cJSON_bool result = parse_string(&item, &buf);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(item.valuestring);
}

/* --- 19. String with multiple escape sequences ------------------------- */
void test_parse_string_multiple_escapes(void)
{
    cJSON item = make_item();
    parse_buffer buf;
    const char *json = "\"\\t\\n\\r\"";

    init_parse_buffer_str(&buf, json);

    cJSON_bool result = parse_string(&item, &buf);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(item.valuestring);
    TEST_ASSERT_EQUAL_STRING("\t\n\r", item.valuestring);

    free(item.valuestring);
}

/* --- 20. Buffer offset is updated correctly after successful parse ----- */
void test_parse_string_offset_updated(void)
{
    cJSON item = make_item();
    parse_buffer buf;
    /* Embed the string inside a larger buffer; offset starts at 0 */
    const char *json = "\"abc\"xyz";

    init_parse_buffer_str(&buf, json);

    cJSON_bool result = parse_string(&item, &buf);

    TEST_ASSERT_TRUE(result);
    /* offset should be 5: past the closing '"' of "abc" */
    TEST_ASSERT_EQUAL_UINT(5u, buf.offset);

    free(item.valuestring);
}

/* --- 21. String with non-ASCII bytes (raw UTF-8 passthrough) ----------- */
void test_parse_string_raw_utf8(void)
{
    cJSON item = make_item();
    parse_buffer buf;
    /* "café" in UTF-8: c a f é(0xC3 0xA9) */
    const char *json = "\"caf\xC3\xA9\"";

    init_parse_buffer_str(&buf, json);

    cJSON_bool result = parse_string(&item, &buf);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(item.valuestring);
    TEST_ASSERT_EQUAL_STRING("caf\xC3\xA9", item.valuestring);

    free(item.valuestring);
}

/* --- 22. Fail: empty input buffer -------------------------------------- */
void test_parse_string_empty_buffer(void)
{
    cJSON item = make_item();
    parse_buffer buf;
    const char *json = "";

    init_parse_buffer_str(&buf, json);

    cJSON_bool result = parse_string(&item, &buf);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(item.valuestring);
}

/* --- 23. String containing only spaces --------------------------------- */
void test_parse_string_spaces_only(void)
{
    cJSON item = make_item();
    parse_buffer buf;
    const char *json = "\"   \"";

    init_parse_buffer_str(&buf, json);

    cJSON_bool result = parse_string(&item, &buf);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(item.valuestring);
    TEST_ASSERT_EQUAL_STRING("   ", item.valuestring);

    free(item.valuestring);
}

/* --- 24. Long string (stress) ------------------------------------------ */
void test_parse_string_long_string(void)
{
    cJSON item = make_item();
    parse_buffer buf;

    /* Build a 1000-character string */
    char json[1004];
    json[0] = '"';
    memset(json + 1, 'x', 1000);
    json[1001] = '"';
    json[1002] = '\0';

    init_parse_buffer_str(&buf, json);

    cJSON_bool result = parse_string(&item, &buf);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(item.valuestring);
    TEST_ASSERT_EQUAL_UINT(1000u, strlen(item.valuestring));

    free(item.valuestring);
}

/* --- 25. item->type is set to cJSON_String on success ------------------ */
void test_parse_string_sets_type(void)
{
    cJSON item = make_item();
    parse_buffer buf;
    const char *json = "\"test\"";

    init_parse_buffer_str(&buf, json);

    parse_string(&item, &buf);

    TEST_ASSERT_EQUAL_INT(cJSON_String, item.type);

    free(item.valuestring);
}

/* --- 26. Fail: input starts with single-quote (not valid JSON) --------- */
void test_parse_string_single_quote_fails(void)
{
    cJSON item = make_item();
    parse_buffer buf;
    const char *json = "'hello'";

    init_parse_buffer_str(&buf, json);

    cJSON_bool result = parse_string(&item, &buf);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(item.valuestring);
}

/* --- 27. UTF-16 \u0022 (escaped double-quote via unicode) -------------- */
void test_parse_string_utf16_double_quote(void)
{
    cJSON item = make_item();
    parse_buffer buf;
    const char *json = "\"\\u0022\"";   /* \u0022 == '"' */

    init_parse_buffer_str(&buf, json);

    cJSON_bool result = parse_string(&item, &buf);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(item.valuestring);
    TEST_ASSERT_EQUAL_STRING("\"", item.valuestring);

    free(item.valuestring);
}

/* --- 28. Fail: invalid UTF-16 sequence (non-hex digits) ---------------- */
void test_parse_string_utf16_invalid_hex(void)
{
    cJSON item = make_item();
    parse_buffer buf;
    const char *json = "\"\\uXYZW\"";

    init_parse_buffer_str(&buf, json);

    cJSON_bool result = parse_string(&item, &buf);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(item.valuestring);
}

/* --- 29. String with escaped slash in URL ------------------------------ */
void test_parse_string_url_with_escaped_slash(void)
{
    cJSON item = make_item();
    parse_buffer buf;
    const char *json = "\"http:\\/\\/example.com\"";

    init_parse_buffer_str(&buf, json);

    cJSON_bool result = parse_string(&item, &buf);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(item.valuestring);
    TEST_ASSERT_EQUAL_STRING("http://example.com", item.valuestring);

    free(item.valuestring);
}

/* --- 30. Verify via public API: cJSON_Parse round-trip ----------------- */
void test_parse_string_via_public_api(void)
{
    const char *json = "{\"key\":\"value\"}";
    cJSON *root = cJSON_Parse(json);

    TEST_ASSERT_NOT_NULL(root);

    cJSON *item = cJSON_GetObjectItem(root, "key");
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_EQUAL_INT(cJSON_String, item->type);
    TEST_ASSERT_EQUAL_STRING("value", item->valuestring);

    cJSON_Delete(root);
}

/* =========================================================================
 * main
 * ========================================================================= */

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();

    RUN_TEST(test_parse_string_simple_ascii);
    RUN_TEST(test_parse_string_empty);
    RUN_TEST(test_parse_string_not_a_string);
    RUN_TEST(test_parse_string_unterminated);
    RUN_TEST(test_parse_string_escape_newline);
    RUN_TEST(test_parse_string_escape_tab);
    RUN_TEST(test_parse_string_escape_carriage_return);
    RUN_TEST(test_parse_string_escape_backspace);
    RUN_TEST(test_parse_string_escape_formfeed);
    RUN_TEST(test_parse_string_escape_double_quote);
    RUN_TEST(test_parse_string_escape_backslash);
    RUN_TEST(test_parse_string_escape_forward_slash);
    RUN_TEST(test_parse_string_invalid_escape);
    RUN_TEST(test_parse_string_backslash_at_end);
    RUN_TEST(test_parse_string_utf16_basic);
    RUN_TEST(test_parse_string_utf16_null_char);
    RUN_TEST(test_parse_string_utf16_surrogate_pair);
    RUN_TEST(test_parse_string_utf16_incomplete_surrogate);
    RUN_TEST(test_parse_string_multiple_escapes);
    RUN_TEST(test_parse_string_offset_updated);
    RUN_TEST(test_parse_string_raw_utf8);
    RUN_TEST(test_parse_string_empty_buffer);
    RUN_TEST(test_parse_string_spaces_only);
    RUN_TEST(test_parse_string_long_string);
    RUN_TEST(test_parse_string_sets_type);
    RUN_TEST(test_parse_string_single_quote_fails);
    RUN_TEST(test_parse_string_utf16_double_quote);
    RUN_TEST(test_parse_string_utf16_invalid_hex);
    RUN_TEST(test_parse_string_url_with_escaped_slash);
    RUN_TEST(test_parse_string_via_public_api);

    return UNITY_END();
}