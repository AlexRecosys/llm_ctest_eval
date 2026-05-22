#include "unity.h"
#include "cJSON.h"

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>

/* -------------------------------------------------------------------------
 * Internal types mirrored from cJSON.c so we can build parse_buffer structs
 * without touching the private source.  We only need the fields that
 * parse_number actually touches.
 * ---------------------------------------------------------------------- */

typedef struct internal_hooks
{
    void *(CJSON_CDECL *allocate)(size_t size);
    void  (CJSON_CDECL *deallocate)(void *pointer);
    void *(CJSON_CDECL *reallocate)(void *pointer, size_t size);
} internal_hooks;

typedef struct parse_buffer
{
    const unsigned char *content;
    size_t               length;
    size_t               offset;
    size_t               depth;
    internal_hooks       hooks;
} parse_buffer;

/* -------------------------------------------------------------------------
 * Helpers that parse_number uses internally – replicated here so the
 * translation unit compiles without the full cJSON.c source.
 * ---------------------------------------------------------------------- */

static unsigned char get_decimal_point(void)
{
    /* Use the standard '.' – locale-independent for test purposes */
    return (unsigned char)'.';
}

/* can_access_at_index / buffer_at_offset are macros in cJSON.c */
#define can_access_at_index(buf, idx) \
    (((buf)->offset + (idx)) < (buf)->length)

#define buffer_at_offset(buf) \
    ((buf)->content + (buf)->offset)

/* -------------------------------------------------------------------------
 * Paste the function under test verbatim
 * ---------------------------------------------------------------------- */

static cJSON_bool parse_number(cJSON * const item, parse_buffer * const input_buffer)
{
    double number = 0;
    unsigned char *after_end = NULL;
    unsigned char *number_c_string;
    unsigned char decimal_point = get_decimal_point();
    size_t i = 0;
    size_t number_string_length = 0;
    cJSON_bool has_decimal_point = 0;

    if ((input_buffer == NULL) || (input_buffer->content == NULL))
    {
        return 0;
    }

    for (i = 0; can_access_at_index(input_buffer, i); i++)
    {
        switch (buffer_at_offset(input_buffer)[i])
        {
            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9':
            case '+': case '-': case 'e': case 'E':
                number_string_length++;
                break;
            case '.':
                number_string_length++;
                has_decimal_point = 1;
                break;
            default:
                goto loop_end;
        }
    }
loop_end:
    number_c_string = (unsigned char *)input_buffer->hooks.allocate(number_string_length + 1);
    if (number_c_string == NULL)
    {
        return 0;
    }

    memcpy(number_c_string, buffer_at_offset(input_buffer), number_string_length);
    number_c_string[number_string_length] = '\0';

    if (has_decimal_point)
    {
        for (i = 0; i < number_string_length; i++)
        {
            if (number_c_string[i] == '.')
            {
                number_c_string[i] = decimal_point;
            }
        }
    }

    number = strtod((const char *)number_c_string, (char **)&after_end);
    if (number_c_string == after_end)
    {
        input_buffer->hooks.deallocate(number_c_string);
        return 0;
    }

    item->valuedouble = number;

    if (number >= INT_MAX)
    {
        item->valueint = INT_MAX;
    }
    else if (number <= (double)INT_MIN)
    {
        item->valueint = INT_MIN;
    }
    else
    {
        item->valueint = (int)number;
    }

    item->type = cJSON_Number;

    input_buffer->offset += (size_t)(after_end - number_c_string);
    input_buffer->hooks.deallocate(number_c_string);
    return 1;
}

/* -------------------------------------------------------------------------
 * File-scope fixtures
 * ---------------------------------------------------------------------- */

static cJSON        g_item;
static parse_buffer g_buf;

/* -------------------------------------------------------------------------
 * Helper: initialise a parse_buffer pointing at a C string
 * ---------------------------------------------------------------------- */
static void init_parse_buffer(parse_buffer *pb, const char *content)
{
    memset(pb, 0, sizeof(*pb));
    pb->content          = (const unsigned char *)content;
    pb->length           = strlen(content);
    pb->offset           = 0;
    pb->depth            = 0;
    pb->hooks.allocate   = malloc;
    pb->hooks.deallocate = free;
    pb->hooks.reallocate = realloc;
}

/* -------------------------------------------------------------------------
 * setUp / tearDown
 * ---------------------------------------------------------------------- */

void setUp(void)
{
    memset(&g_item, 0, sizeof(g_item));
    memset(&g_buf,  0, sizeof(g_buf));
}

void tearDown(void)
{
    /* Nothing heap-allocated survives across tests in these fixtures */
}

/* =========================================================================
 * Test cases
 * ========================================================================= */

/* 1. NULL input_buffer → returns false */
void test_parse_number_null_input_buffer_returns_false(void)
{
    cJSON_bool result = parse_number(&g_item, NULL);
    TEST_ASSERT_FALSE_MESSAGE(result, "Expected false when input_buffer is NULL");
}

/* 2. Parse a simple positive integer */
void test_parse_number_positive_integer(void)
{
    const char *json = "42";
    init_parse_buffer(&g_buf, json);

    cJSON_bool result = parse_number(&g_item, &g_buf);

    TEST_ASSERT_TRUE_MESSAGE(result, "Expected true for valid integer '42'");
    TEST_ASSERT_EQUAL_INT_MESSAGE(cJSON_Number, g_item.type,
                                  "Item type should be cJSON_Number");
    TEST_ASSERT_EQUAL_DOUBLE_MESSAGE(42.0, g_item.valuedouble,
                                     "valuedouble should be 42.0");
    TEST_ASSERT_EQUAL_INT_MESSAGE(42, g_item.valueint,
                                  "valueint should be 42");
    TEST_ASSERT_EQUAL_UINT_MESSAGE(2, g_buf.offset,
                                   "offset should advance by 2 characters");
}

/* 3. Parse a floating-point number */
void test_parse_number_floating_point(void)
{
    const char *json = "3.14";
    init_parse_buffer(&g_buf, json);

    cJSON_bool result = parse_number(&g_item, &g_buf);

    TEST_ASSERT_TRUE_MESSAGE(result, "Expected true for valid float '3.14'");
    TEST_ASSERT_EQUAL_INT_MESSAGE(cJSON_Number, g_item.type,
                                  "Item type should be cJSON_Number");
    TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(1e-9, 3.14, g_item.valuedouble,
                                      "valuedouble should be ~3.14");
    TEST_ASSERT_EQUAL_INT_MESSAGE(3, g_item.valueint,
                                  "valueint should be 3 (truncated)");
    TEST_ASSERT_EQUAL_UINT_MESSAGE(4, g_buf.offset,
                                   "offset should advance by 4 characters");
}

/* 4. Parse a negative number */
void test_parse_number_negative_integer(void)
{
    const char *json = "-100";
    init_parse_buffer(&g_buf, json);

    cJSON_bool result = parse_number(&g_item, &g_buf);

    TEST_ASSERT_TRUE_MESSAGE(result, "Expected true for valid negative integer '-100'");
    TEST_ASSERT_EQUAL_INT_MESSAGE(cJSON_Number, g_item.type,
                                  "Item type should be cJSON_Number");
    TEST_ASSERT_EQUAL_DOUBLE_MESSAGE(-100.0, g_item.valuedouble,
                                     "valuedouble should be -100.0");
    TEST_ASSERT_EQUAL_INT_MESSAGE(-100, g_item.valueint,
                                  "valueint should be -100");
    TEST_ASSERT_EQUAL_UINT_MESSAGE(4, g_buf.offset,
                                   "offset should advance by 4 characters");
}

/* 5. Non-numeric content → returns false, offset unchanged */
void test_parse_number_non_numeric_content_returns_false(void)
{
    const char *json = "abc";
    init_parse_buffer(&g_buf, json);

    cJSON_bool result = parse_number(&g_item, &g_buf);

    TEST_ASSERT_FALSE_MESSAGE(result,
                              "Expected false when content is non-numeric");
    /* offset must not have advanced */
    TEST_ASSERT_EQUAL_UINT_MESSAGE(0, g_buf.offset,
                                   "offset should remain 0 for non-numeric input");
    /* item type must not have been set to cJSON_Number */
    TEST_ASSERT_NOT_EQUAL_MESSAGE(cJSON_Number, g_item.type,
                                  "item type should not be cJSON_Number on failure");
}

/* =========================================================================
 * main
 * ========================================================================= */

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_parse_number_null_input_buffer_returns_false);
    RUN_TEST(test_parse_number_positive_integer);
    RUN_TEST(test_parse_number_floating_point);
    RUN_TEST(test_parse_number_negative_integer);
    RUN_TEST(test_parse_number_non_numeric_content_returns_false);

    return UNITY_END();
}