#include "cJSON.c"
#include "unity.h"
#include "cJSON.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <float.h>
#include <signal.h>

/* File-scope fixtures */
static cJSON item;
static parse_buffer buf;

/* Signal handler for SIGSEGV */
static void sigsegv_handler(int sig)
{
    (void)sig;
    TEST_FAIL_MESSAGE("Segmentation fault (SIGSEGV) caught during test execution");
}

/* Helper: initialize a parse_buffer with a given string */
static void init_parse_buffer(parse_buffer *pb, const unsigned char *content, size_t length)
{
    memset(pb, 0, sizeof(parse_buffer));
    pb->content          = content;
    pb->length           = length;
    pb->offset           = 0;
    pb->depth            = 0;
    pb->hooks.allocate   = malloc;
    pb->hooks.deallocate = free;
    pb->hooks.reallocate = realloc;
}

/* Helper: initialize a cJSON item */
static void init_cjson_item(cJSON *it)
{
    memset(it, 0, sizeof(cJSON));
}

void setUp(void)
{
    signal(SIGSEGV, sigsegv_handler);
    init_cjson_item(&item);
    memset(&buf, 0, sizeof(parse_buffer));
}

void tearDown(void)
{
    /* Reset signal handler */
    signal(SIGSEGV, SIG_DFL);
}

/* Test 1: Parse a simple positive integer */
void test_parse_number_simple_integer(void)
{
    const unsigned char *content = (const unsigned char *)"42";
    init_parse_buffer(&buf, content, strlen("42"));
    init_cjson_item(&item);

    cJSON_bool result = parse_number(&item, &buf);

    TEST_ASSERT_TRUE_MESSAGE(result, "parse_number should return true for '42'");
    TEST_ASSERT_EQUAL_INT(cJSON_Number, item.type);
    TEST_ASSERT_EQUAL_DOUBLE(42.0, item.valuedouble);
    TEST_ASSERT_EQUAL_INT(42, item.valueint);
    TEST_ASSERT_EQUAL_UINT((size_t)2, buf.offset);
}

/* Test 2: Parse a floating-point number with decimal point */
void test_parse_number_floating_point(void)
{
    const unsigned char *content = (const unsigned char *)"3.14";
    init_parse_buffer(&buf, content, strlen("3.14"));
    init_cjson_item(&item);

    cJSON_bool result = parse_number(&item, &buf);

    TEST_ASSERT_TRUE_MESSAGE(result, "parse_number should return true for '3.14'");
    TEST_ASSERT_EQUAL_INT(cJSON_Number, item.type);
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 3.14, item.valuedouble);
    TEST_ASSERT_EQUAL_INT(3, item.valueint);
    TEST_ASSERT_EQUAL_UINT((size_t)4, buf.offset);
}

/* Test 3: Parse a negative number */
void test_parse_number_negative(void)
{
    const unsigned char *content = (const unsigned char *)"-100";
    init_parse_buffer(&buf, content, strlen("-100"));
    init_cjson_item(&item);

    cJSON_bool result = parse_number(&item, &buf);

    TEST_ASSERT_TRUE_MESSAGE(result, "parse_number should return true for '-100'");
    TEST_ASSERT_EQUAL_INT(cJSON_Number, item.type);
    TEST_ASSERT_EQUAL_DOUBLE(-100.0, item.valuedouble);
    TEST_ASSERT_EQUAL_INT(-100, item.valueint);
    TEST_ASSERT_EQUAL_UINT((size_t)4, buf.offset);
}

/* Test 4: Parse a number in scientific notation */
void test_parse_number_scientific_notation(void)
{
    const unsigned char *content = (const unsigned char *)"1.5e2";
    init_parse_buffer(&buf, content, strlen("1.5e2"));
    init_cjson_item(&item);

    cJSON_bool result = parse_number(&item, &buf);

    TEST_ASSERT_TRUE_MESSAGE(result, "parse_number should return true for '1.5e2'");
    TEST_ASSERT_EQUAL_INT(cJSON_Number, item.type);
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 150.0, item.valuedouble);
    TEST_ASSERT_EQUAL_INT(150, item.valueint);
    TEST_ASSERT_EQUAL_UINT((size_t)5, buf.offset);
}

/* Test 5: NULL input_buffer returns false */
void test_parse_number_null_input_buffer(void)
{
    init_cjson_item(&item);

    cJSON_bool result = parse_number(&item, NULL);

    TEST_ASSERT_FALSE_MESSAGE(result, "parse_number should return false when input_buffer is NULL");
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_parse_number_simple_integer);
    RUN_TEST(test_parse_number_floating_point);
    RUN_TEST(test_parse_number_negative);
    RUN_TEST(test_parse_number_scientific_notation);
    RUN_TEST(test_parse_number_null_input_buffer);
    return UNITY_END();
}