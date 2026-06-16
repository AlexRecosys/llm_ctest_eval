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

    IntItem lookup = {.key = 7};
    const IntItem *deleted = (const IntItem *)hashmap_delete(g_map, &lookup);
    TEST_ASSERT_NOT_NULL(deleted);
    TEST_ASSERT_EQUAL_INT(7, deleted->key);
    TEST_ASSERT_EQUAL_UINT(0, hashmap_count(g_map));
}

void test_hashmap_new_with_elfree_callback(void)
{
    g_map = hashmap_new(sizeof(IntItem), 0, 0, 0,
                        int_hash, int_compare, counting_elfree, NULL);
    TEST_ASSERT_NOT_NULL(g_map);

    IntItem item1 = {.key = 1, .value = 1};
    IntItem item2 = {.key = 2, .value = 2};
    hashmap_set(g_map, &item1);
    hashmap_set(g_map, &item2);

    hashmap_free(g_map);
    g_map = NULL;

    TEST_ASSERT_EQUAL_INT(2, g_elfree_count);
}

void test_hashmap_new_with_udata(void)
{
    int udata_value = 12345;
    g_map = hashmap_new(sizeof(IntItem), 0, 0, 0,
                        int_hash, int_compare, NULL, &udata_value);
    TEST_ASSERT_NOT_NULL(g_map);

    IntItem item = {.key = 3, .value = 30};
    hashmap_set(g_map, &item);

    IntItem lookup = {.key = 3};
    const IntItem *found = (const IntItem *)hashmap_get(g_map, &lookup);
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_INT(30, found->value);
}

void test_hashmap_new_string_key_type(void)
{
    g_map = hashmap_new(sizeof(StrItem), 0, 0, 0,
                        str_hash, str_compare, NULL, NULL);
    TEST_ASSERT_NOT_NULL(g_map);

    StrItem item;
    memset(&item, 0, sizeof(item));
    strncpy(item.key, "hello", sizeof(item.key) - 1);
    item.value = 55;
    hashmap_set(g_map, &item);

    StrItem lookup;
    memset(&lookup, 0, sizeof(lookup));
    strncpy(lookup.key, "hello", sizeof(lookup.key) - 1);
    const StrItem *found = (const StrItem *)hashmap_get(g_map, &lookup);
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_STRING("hello", found->key);
    TEST_ASSERT_EQUAL_INT(55, found->value);
}

void test_hashmap_new_overwrite_existing_key(void)
{
    g_map = hashmap_new(sizeof(IntItem), 0, 0, 0,
                        int_hash, int_compare, NULL, NULL);
    TEST_ASSERT_NOT_NULL(g_map);

    IntItem item1 = {.key = 10, .value = 100};
    IntItem item2 = {.key = 10, .value = 200};
    hashmap_set(g_map, &item1);
    hashmap_set(g_map, &item2);

    TEST_ASSERT_EQUAL_UINT(1, hashmap_count(g_map));

    IntItem lookup = {.key = 10};
    const IntItem *found = (const IntItem *)hashmap_get(g_map, &lookup);
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_INT(200, found->value);
}

void test_hashmap_new_many_items_triggers_resize(void)
{
    g_map = hashmap_new(sizeof(IntItem), 0, 0, 0,
                        int_hash, int_compare, NULL, NULL);
    TEST_ASSERT_NOT_NULL(g_map);

    int n = 200;
    for (int i = 0; i < n; i++) {
        IntItem item = {.key = i, .value = i * 2};
        hashmap_set(g_map, &item);
    }

    TEST_ASSERT_EQUAL_UINT((size_t)n, hashmap_count(g_map));

    for (int i = 0; i < n; i++) {
        IntItem lookup = {.key = i};
        const IntItem *found = (const IntItem *)hashmap_get(g_map, &lookup);
        TEST_ASSERT_NOT_NULL_MESSAGE(found, "item not found after resize");
        TEST_ASSERT_EQUAL_INT(i * 2, found->value);
    }
}

void test_hashmap_new_large_cap_hint(void)
{
    g_map = hashmap_new(sizeof(IntItem), 1024, 0, 0,
                        int_hash, int_compare, NULL, NULL);
    TEST_ASSERT_NOT_NULL(g_map);
    TEST_ASSERT_EQUAL_UINT(0, hashmap_count(g_map));
}

void test_hashmap_new_elfree_called_on_clear(void)
{
    g_map = hashmap_new(sizeof(IntItem), 0, 0, 0,
                        int_hash, int_compare, counting_elfree, NULL);
    TEST_ASSERT_NOT_NULL(g_map);

    for (int i = 0; i < 5; i++) {
        IntItem item = {.key = i, .value = i};
        hashmap_set(g_map, &item);
    }

    hashmap_clear(g_map, true);
    TEST_ASSERT_EQUAL_INT(5, g_elfree_count);
    TEST_ASSERT_EQUAL_UINT(0, hashmap_count(g_map));
}

void test_hashmap_new_different_seeds_produce_valid_map(void)
{
    struct hashmap *map1 = hashmap_new(sizeof(IntItem), 0,
                                       0x0000000000000001ULL,
                                       0x0000000000000002ULL,
                                       int_hash, int_compare, NULL, NULL);
    struct hashmap *map2 = hashmap_new(sizeof(IntItem), 0,
                                       0xFFFFFFFFFFFFFFFFULL,
                                       0xFFFFFFFFFFFFFFFEULL,
                                       int_hash, int_compare, NULL, NULL);
    TEST_ASSERT_NOT_NULL(map1);
    TEST_ASSERT_NOT_NULL(map2);

    IntItem item = {.key = 1, .value = 99};
    hashmap_set(map1, &item);
    hashmap_set(map2, &item);

    IntItem lookup = {.key = 1};
    const IntItem *f1 = (const IntItem *)hashmap_get(map1, &lookup);
    const IntItem *f2 = (const IntItem *)hashmap_get(map2, &lookup);
    TEST_ASSERT_NOT_NULL(f1);
    TEST_ASSERT_NOT_NULL(f2);
    TEST_ASSERT_EQUAL_INT(99, f1->value);
    TEST_ASSERT_EQUAL_INT(99, f2->value);

    hashmap_free(map1);
    hashmap_free(map2);
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
    RUN_TEST(test_hashmap_new_with_elfree_callback);
    RUN_TEST(test_hashmap_new_with_udata);
    RUN_TEST(test_hashmap_new_string_key_type);
    RUN_TEST(test_hashmap_new_overwrite_existing_key);
    RUN_TEST(test_hashmap_new_many_items_triggers_resize);
    RUN_TEST(test_hashmap_new_large_cap_hint);
    RUN_TEST(test_hashmap_new_elfree_called_on_clear);
    RUN_TEST(test_hashmap_new_different_seeds_produce_valid_map);
    return UNITY_END();
}