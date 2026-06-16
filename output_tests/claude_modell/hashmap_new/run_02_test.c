#include "hashmap.c"
#include "unity.h"
#include "hashmap.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

/* -------------------------------------------------------------------------
 * File-scope fixtures
 * ------------------------------------------------------------------------- */

static struct hashmap *g_map = NULL;

/* -------------------------------------------------------------------------
 * Helper types and callbacks
 * ------------------------------------------------------------------------- */

typedef struct {
    int   key;
    int   value;
} IntItem;

typedef struct {
    char  key[64];
    int   value;
} StrItem;

static uint64_t int_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
    const IntItem *it = (const IntItem *)item;
    return hashmap_sip(&it->key, sizeof(it->key), seed0, seed1);
}

static int int_compare(const void *a, const void *b, void *udata)
{
    (void)udata;
    const IntItem *ia = (const IntItem *)a;
    const IntItem *ib = (const IntItem *)b;
    return ia->key - ib->key;
}

static uint64_t str_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
    const StrItem *it = (const StrItem *)item;
    return hashmap_sip(it->key, strlen(it->key), seed0, seed1);
}

static int str_compare(const void *a, const void *b, void *udata)
{
    (void)udata;
    const StrItem *sa = (const StrItem *)a;
    const StrItem *sb = (const StrItem *)b;
    return strcmp(sa->key, sb->key);
}

static int g_elfree_count = 0;

static void counting_elfree(void *item)
{
    (void)item;
    g_elfree_count++;
}

/* -------------------------------------------------------------------------
 * setUp / tearDown
 * ------------------------------------------------------------------------- */

void setUp(void)
{
    g_map = NULL;
    g_elfree_count = 0;
}

void tearDown(void)
{
    if (g_map) {
        hashmap_free(g_map);
        g_map = NULL;
    }
}

/* -------------------------------------------------------------------------
 * Test cases
 * ------------------------------------------------------------------------- */

void test_hashmap_new_returns_non_null(void)
{
    g_map = hashmap_new(sizeof(IntItem), 0, 0, 0,
                        int_hash, int_compare, NULL, NULL);
    TEST_ASSERT_NOT_NULL(g_map);
}

void test_hashmap_new_initial_count_is_zero(void)
{
    g_map = hashmap_new(sizeof(IntItem), 0, 0, 0,
                        int_hash, int_compare, NULL, NULL);
    TEST_ASSERT_NOT_NULL(g_map);
    TEST_ASSERT_EQUAL_UINT(0, hashmap_count(g_map));
}

void test_hashmap_new_with_nonzero_cap(void)
{
    g_map = hashmap_new(sizeof(IntItem), 64, 0, 0,
                        int_hash, int_compare, NULL, NULL);
    TEST_ASSERT_NOT_NULL(g_map);
    TEST_ASSERT_EQUAL_UINT(0, hashmap_count(g_map));
}

void test_hashmap_new_with_seeds(void)
{
    g_map = hashmap_new(sizeof(IntItem), 0,
                        0xDEADBEEFDEADBEEFULL,
                        0xCAFEBABECAFEBABEULL,
                        int_hash, int_compare, NULL, NULL);
    TEST_ASSERT_NOT_NULL(g_map);
}

void test_hashmap_new_set_and_get(void)
{
    g_map = hashmap_new(sizeof(IntItem), 0, 0, 0,
                        int_hash, int_compare, NULL, NULL);
    TEST_ASSERT_NOT_NULL(g_map);

    IntItem item = {.key = 42, .value = 100};
    hashmap_set(g_map, &item);

    IntItem lookup = {.key = 42};
    const IntItem *found = (const IntItem *)hashmap_get(g_map, &lookup);
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_INT(42,  found->key);
    TEST_ASSERT_EQUAL_INT(100, found->value);
}

void test_hashmap_new_count_increments_on_set(void)
{
    g_map = hashmap_new(sizeof(IntItem), 0, 0, 0,
                        int_hash, int_compare, NULL, NULL);
    TEST_ASSERT_NOT_NULL(g_map);

    IntItem item1 = {.key = 1, .value = 10};
    IntItem item2 = {.key = 2, .value = 20};
    hashmap_set(g_map, &item1);
    TEST_ASSERT_EQUAL_UINT(1, hashmap_count(g_map));
    hashmap_set(g_map, &item2);
    TEST_ASSERT_EQUAL_UINT(2, hashmap_count(g_map));
}

void test_hashmap_new_get_missing_key_returns_null(void)
{
    g_map = hashmap_new(sizeof(IntItem), 0, 0, 0,
                        int_hash, int_compare, NULL, NULL);
    TEST_ASSERT_NOT_NULL(g_map);

    IntItem lookup = {.key = 999};
    const IntItem *found = (const IntItem *)hashmap_get(g_map, &lookup);
    TEST_ASSERT_NULL(found);
}

void test_hashmap_new_delete_existing_key(void)
{
    g_map = hashmap_new(sizeof(IntItem), 0, 0, 0,
                        int_hash, int_compare, NULL, NULL);
    TEST_ASSERT_NOT_NULL(g_map);

    IntItem item = {.key = 7, .value = 77};
    hashmap_set(g_map, &item);

    IntItem del = {.key = 7};
    const IntItem *deleted = (const IntItem *)hashmap_delete(g_map, &del);
    TEST_ASSERT_NOT_NULL(deleted);
    TEST_ASSERT_EQUAL_INT(7, deleted->key);
    TEST_ASSERT_EQUAL_UINT(0, hashmap_count(g_map));
}

void test_hashmap_new_elfree_called_on_free(void)
{
    g_map = hashmap_new(sizeof(IntItem), 0, 0, 0,
                        int_hash, int_compare, counting_elfree, NULL);
    TEST_ASSERT_NOT_NULL(g_map);

    IntItem item1 = {.key = 1, .value = 1};
    IntItem item2 = {.key = 2, .value = 2};
    IntItem item3 = {.key = 3, .value = 3};
    hashmap_set(g_map, &item1);
    hashmap_set(g_map, &item2);
    hashmap_set(g_map, &item3);

    hashmap_free(g_map);
    g_map = NULL;

    TEST_ASSERT_EQUAL_INT(3, g_elfree_count);
}

void test_hashmap_new_with_string_keys(void)
{
    g_map = hashmap_new(sizeof(StrItem), 0, 0, 0,
                        str_hash, str_compare, NULL, NULL);
    TEST_ASSERT_NOT_NULL(g_map);

    StrItem item;
    memset(&item, 0, sizeof(item));
    strncpy(item.key, "hello", sizeof(item.key) - 1);
    item.value = 123;
    hashmap_set(g_map, &item);

    StrItem lookup;
    memset(&lookup, 0, sizeof(lookup));
    strncpy(lookup.key, "hello", sizeof(lookup.key) - 1);
    const StrItem *found = (const StrItem *)hashmap_get(g_map, &lookup);
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_STRING("hello", found->key);
    TEST_ASSERT_EQUAL_INT(123, found->value);
}

void test_hashmap_new_overwrite_existing_key(void)
{
    g_map = hashmap_new(sizeof(IntItem), 0, 0, 0,
                        int_hash, int_compare, NULL, NULL);
    TEST_ASSERT_NOT_NULL(g_map);

    IntItem item1 = {.key = 5, .value = 50};
    IntItem item2 = {.key = 5, .value = 99};
    hashmap_set(g_map, &item1);
    hashmap_set(g_map, &item2);

    TEST_ASSERT_EQUAL_UINT(1, hashmap_count(g_map));

    IntItem lookup = {.key = 5};
    const IntItem *found = (const IntItem *)hashmap_get(g_map, &lookup);
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_INT(99, found->value);
}

void test_hashmap_new_udata_passed_to_compare(void)
{
    /* We use a simple compare that ignores udata; just verify the map
       works correctly when udata is a non-NULL pointer. */
    int udata_val = 0xABCD;
    g_map = hashmap_new(sizeof(IntItem), 0, 0, 0,
                        int_hash, int_compare, NULL, &udata_val);
    TEST_ASSERT_NOT_NULL(g_map);

    IntItem item = {.key = 3, .value = 33};
    hashmap_set(g_map, &item);

    IntItem lookup = {.key = 3};
    const IntItem *found = (const IntItem *)hashmap_get(g_map, &lookup);
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_INT(33, found->value);
}

void test_hashmap_new_large_cap_hint(void)
{
    g_map = hashmap_new(sizeof(IntItem), 1024, 0, 0,
                        int_hash, int_compare, NULL, NULL);
    TEST_ASSERT_NOT_NULL(g_map);
    TEST_ASSERT_EQUAL_UINT(0, hashmap_count(g_map));
}

void test_hashmap_new_many_insertions(void)
{
    g_map = hashmap_new(sizeof(IntItem), 0, 0, 0,
                        int_hash, int_compare, NULL, NULL);
    TEST_ASSERT_NOT_NULL(g_map);

    const int N = 200;
    for (int i = 0; i < N; i++) {
        IntItem item = {.key = i, .value = i * 2};
        hashmap_set(g_map, &item);
    }
    TEST_ASSERT_EQUAL_UINT((size_t)N, hashmap_count(g_map));

    for (int i = 0; i < N; i++) {
        IntItem lookup = {.key = i};
        const IntItem *found = (const IntItem *)hashmap_get(g_map, &lookup);
        TEST_ASSERT_NOT_NULL_MESSAGE(found, "Expected to find inserted item");
        TEST_ASSERT_EQUAL_INT(i * 2, found->value);
    }
}

void test_hashmap_new_elfree_called_on_overwrite(void)
{
    g_map = hashmap_new(sizeof(IntItem), 0, 0, 0,
                        int_hash, int_compare, counting_elfree, NULL);
    TEST_ASSERT_NOT_NULL(g_map);

    IntItem item1 = {.key = 10, .value = 1};
    IntItem item2 = {.key = 10, .value = 2};
    hashmap_set(g_map, &item1);
    hashmap_set(g_map, &item2);

    /* elfree should have been called once for the replaced item */
    TEST_ASSERT_EQUAL_INT(1, g_elfree_count);
}

void test_hashmap_new_clear_resets_count(void)
{
    g_map = hashmap_new(sizeof(IntItem), 0, 0, 0,
                        int_hash, int_compare, NULL, NULL);
    TEST_ASSERT_NOT_NULL(g_map);

    IntItem item = {.key = 1, .value = 1};
    hashmap_set(g_map, &item);
    TEST_ASSERT_EQUAL_UINT(1, hashmap_count(g_map));

    hashmap_clear(g_map, false);
    TEST_ASSERT_EQUAL_UINT(0, hashmap_count(g_map));
}

/* -------------------------------------------------------------------------
 * main
 * ------------------------------------------------------------------------- */

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_hashmap_new_returns_non_null);
    RUN_TEST(test_hashmap_new_initial_count_is_zero);
    RUN_TEST(test_hashmap_new_with_nonzero_cap);
    RUN_TEST(test_hashmap_new_with_seeds);
    RUN_TEST(test_hashmap_new_set_and_get);
    RUN_TEST(test_hashmap_new_count_increments_on_set);
    RUN_TEST(test_hashmap_new_get_missing_key_returns_null);
    RUN_TEST(test_hashmap_new_delete_existing_key);
    RUN_TEST(test_hashmap_new_elfree_called_on_free);
    RUN_TEST(test_hashmap_new_with_string_keys);
    RUN_TEST(test_hashmap_new_overwrite_existing_key);
    RUN_TEST(test_hashmap_new_udata_passed_to_compare);
    RUN_TEST(test_hashmap_new_large_cap_hint);
    RUN_TEST(test_hashmap_new_many_insertions);
    RUN_TEST(test_hashmap_new_elfree_called_on_overwrite);
    RUN_TEST(test_hashmap_new_clear_resets_count);
    return UNITY_END();
}