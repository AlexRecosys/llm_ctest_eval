#include "hashmap.c"
#include "unity.h"
#include "hashmap.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

/* ── fixtures ─────────────────────────────────────────────────────────────── */

struct item {
    int   key;
    int   value;
};

static struct hashmap *g_map;

/* ── helpers ──────────────────────────────────────────────────────────────── */

static uint64_t item_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
    const struct item *it = (const struct item *)item;
    return hashmap_murmur(&it->key, sizeof(it->key), seed0, seed1);
}

static int item_compare(const void *a, const void *b, void *udata)
{
    (void)udata;
    const struct item *ia = (const struct item *)a;
    const struct item *ib = (const struct item *)b;
    return ia->key - ib->key;
}

static struct hashmap *create_map(void)
{
    return hashmap_new(sizeof(struct item), 0, 0, 0,
                       item_hash, item_compare, NULL, NULL);
}

/* ── setUp / tearDown ─────────────────────────────────────────────────────── */

void setUp(void)
{
    g_map = create_map();
    TEST_ASSERT_NOT_NULL(g_map);
}

void tearDown(void)
{
    hashmap_free(g_map);
    g_map = NULL;
}

/* ── test cases ───────────────────────────────────────────────────────────── */

void test_hashmap_count_empty_map_returns_zero(void)
{
    size_t count = hashmap_count(g_map);
    TEST_ASSERT_EQUAL_UINT(0, count);
}

void test_hashmap_count_after_single_insert(void)
{
    struct item it = {1, 100};
    hashmap_set(g_map, &it);
    TEST_ASSERT_EQUAL_UINT(1, hashmap_count(g_map));
}

void test_hashmap_count_after_multiple_inserts(void)
{
    for (int i = 0; i < 10; i++) {
        struct item it = {i, i * 10};
        hashmap_set(g_map, &it);
    }
    TEST_ASSERT_EQUAL_UINT(10, hashmap_count(g_map));
}

void test_hashmap_count_duplicate_key_does_not_increase_count(void)
{
    struct item it1 = {42, 1};
    struct item it2 = {42, 2};
    hashmap_set(g_map, &it1);
    hashmap_set(g_map, &it2);
    TEST_ASSERT_EQUAL_UINT(1, hashmap_count(g_map));
}

void test_hashmap_count_after_delete_decreases(void)
{
    struct item it = {7, 70};
    hashmap_set(g_map, &it);
    TEST_ASSERT_EQUAL_UINT(1, hashmap_count(g_map));

    struct item key = {7, 0};
    hashmap_delete(g_map, &key);
    TEST_ASSERT_EQUAL_UINT(0, hashmap_count(g_map));
}

void test_hashmap_count_delete_nonexistent_key_unchanged(void)
{
    struct item it = {5, 50};
    hashmap_set(g_map, &it);

    struct item missing = {99, 0};
    hashmap_delete(g_map, &missing);
    TEST_ASSERT_EQUAL_UINT(1, hashmap_count(g_map));
}

void test_hashmap_count_after_clear_returns_zero(void)
{
    for (int i = 0; i < 5; i++) {
        struct item it = {i, i};
        hashmap_set(g_map, &it);
    }
    TEST_ASSERT_EQUAL_UINT(5, hashmap_count(g_map));

    hashmap_clear(g_map, false);
    TEST_ASSERT_EQUAL_UINT(0, hashmap_count(g_map));
}

void test_hashmap_count_large_number_of_inserts(void)
{
    const int N = 1000;
    for (int i = 0; i < N; i++) {
        struct item it = {i, i};
        hashmap_set(g_map, &it);
    }
    TEST_ASSERT_EQUAL_UINT((size_t)N, hashmap_count(g_map));
}

void test_hashmap_count_insert_then_delete_all(void)
{
    const int N = 20;
    for (int i = 0; i < N; i++) {
        struct item it = {i, i};
        hashmap_set(g_map, &it);
    }
    for (int i = 0; i < N; i++) {
        struct item key = {i, 0};
        hashmap_delete(g_map, &key);
    }
    TEST_ASSERT_EQUAL_UINT(0, hashmap_count(g_map));
}

void test_hashmap_count_reinsert_after_delete(void)
{
    struct item it = {3, 30};
    hashmap_set(g_map, &it);

    struct item key = {3, 0};
    hashmap_delete(g_map, &key);
    TEST_ASSERT_EQUAL_UINT(0, hashmap_count(g_map));

    struct item it2 = {3, 300};
    hashmap_set(g_map, &it2);
    TEST_ASSERT_EQUAL_UINT(1, hashmap_count(g_map));
}

void test_hashmap_count_independent_maps(void)
{
    struct hashmap *map2 = create_map();
    TEST_ASSERT_NOT_NULL(map2);

    struct item a = {1, 10};
    struct item b = {2, 20};
    struct item c = {3, 30};

    hashmap_set(g_map, &a);
    hashmap_set(g_map, &b);

    hashmap_set(map2, &c);

    TEST_ASSERT_EQUAL_UINT(2, hashmap_count(g_map));
    TEST_ASSERT_EQUAL_UINT(1, hashmap_count(map2));

    hashmap_free(map2);
}

/* ── main ─────────────────────────────────────────────────────────────────── */

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_hashmap_count_empty_map_returns_zero);
    RUN_TEST(test_hashmap_count_after_single_insert);
    RUN_TEST(test_hashmap_count_after_multiple_inserts);
    RUN_TEST(test_hashmap_count_duplicate_key_does_not_increase_count);
    RUN_TEST(test_hashmap_count_after_delete_decreases);
    RUN_TEST(test_hashmap_count_delete_nonexistent_key_unchanged);
    RUN_TEST(test_hashmap_count_after_clear_returns_zero);
    RUN_TEST(test_hashmap_count_large_number_of_inserts);
    RUN_TEST(test_hashmap_count_insert_then_delete_all);
    RUN_TEST(test_hashmap_count_reinsert_after_delete);
    RUN_TEST(test_hashmap_count_independent_maps);
    return UNITY_END();
}