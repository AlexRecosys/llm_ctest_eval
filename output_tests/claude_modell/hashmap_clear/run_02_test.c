#include "hashmap.c"
#include "unity.h"
#include "hashmap.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* ── fixtures ── */

struct test_item {
    int key;
    int value;
};

static uint64_t item_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    const struct test_item *ti = (const struct test_item *)item;
    return hashmap_murmur(&ti->key, sizeof(ti->key), seed0, seed1);
}

static int item_compare(const void *a, const void *b, void *udata) {
    (void)udata;
    const struct test_item *ta = (const struct test_item *)a;
    const struct test_item *tb = (const struct test_item *)b;
    return ta->key - tb->key;
}

static struct hashmap *g_map = NULL;

/* ── helpers ── */

static struct hashmap *create_map(void) {
    return hashmap_new(sizeof(struct test_item), 0, 0, 0,
                       item_hash, item_compare, NULL, NULL);
}

static void populate_map(struct hashmap *map, int count) {
    for (int i = 0; i < count; i++) {
        struct test_item item = { .key = i, .value = i * 10 };
        hashmap_set(map, &item);
    }
}

/* ── setUp / tearDown ── */

void setUp(void) {
    g_map = create_map();
}

void tearDown(void) {
    if (g_map) {
        hashmap_free(g_map);
        g_map = NULL;
    }
}

/* ── test cases ── */

void test_hashmap_clear_sets_count_to_zero(void) {
    populate_map(g_map, 10);
    TEST_ASSERT_EQUAL_INT(10, (int)hashmap_count(g_map));
    hashmap_clear(g_map, false);
    TEST_ASSERT_EQUAL_INT(0, (int)hashmap_count(g_map));
}

void test_hashmap_clear_empty_map_count_stays_zero(void) {
    TEST_ASSERT_EQUAL_INT(0, (int)hashmap_count(g_map));
    hashmap_clear(g_map, false);
    TEST_ASSERT_EQUAL_INT(0, (int)hashmap_count(g_map));
}

void test_hashmap_clear_items_not_found_after_clear(void) {
    populate_map(g_map, 5);
    hashmap_clear(g_map, false);
    for (int i = 0; i < 5; i++) {
        struct test_item key = { .key = i };
        const struct test_item *found = hashmap_get(g_map, &key);
        TEST_ASSERT_NULL(found);
    }
}

void test_hashmap_clear_update_cap_true_sets_cap_to_nbuckets(void) {
    populate_map(g_map, 20);
    size_t nbuckets_before = g_map->nbuckets;
    hashmap_clear(g_map, true);
    /* After update_cap=true, cap should equal nbuckets */
    TEST_ASSERT_EQUAL_UINT(g_map->nbuckets, g_map->cap);
    TEST_ASSERT_EQUAL_UINT(nbuckets_before, g_map->nbuckets);
}

void test_hashmap_clear_update_cap_false_preserves_nbuckets(void) {
    populate_map(g_map, 20);
    size_t nbuckets_before = g_map->nbuckets;
    hashmap_clear(g_map, false);
    TEST_ASSERT_EQUAL_UINT(nbuckets_before, g_map->nbuckets);
}

void test_hashmap_clear_mask_updated_correctly(void) {
    populate_map(g_map, 10);
    hashmap_clear(g_map, false);
    TEST_ASSERT_EQUAL_UINT(g_map->nbuckets - 1, g_map->mask);
}

void test_hashmap_clear_mask_updated_after_update_cap_true(void) {
    populate_map(g_map, 10);
    hashmap_clear(g_map, true);
    TEST_ASSERT_EQUAL_UINT(g_map->nbuckets - 1, g_map->mask);
}

void test_hashmap_clear_growat_updated_correctly(void) {
    populate_map(g_map, 10);
    hashmap_clear(g_map, false);
    size_t expected_growat = (size_t)(g_map->nbuckets * (g_map->loadfactor / 100.0));
    TEST_ASSERT_EQUAL_UINT(expected_growat, g_map->growat);
}

void test_hashmap_clear_shrinkat_updated_correctly(void) {
    populate_map(g_map, 10);
    hashmap_clear(g_map, false);
    size_t expected_shrinkat = (size_t)(g_map->nbuckets * SHRINK_AT);
    TEST_ASSERT_EQUAL_UINT(expected_shrinkat, g_map->shrinkat);
}

void test_hashmap_clear_buckets_zeroed(void) {
    populate_map(g_map, 10);
    hashmap_clear(g_map, false);
    /* All bucket memory should be zero */
    size_t total = g_map->bucketsz * g_map->nbuckets;
    unsigned char *buckets = (unsigned char *)g_map->buckets;
    int all_zero = 1;
    for (size_t i = 0; i < total; i++) {
        if (buckets[i] != 0) {
            all_zero = 0;
            break;
        }
    }
    TEST_ASSERT_TRUE(all_zero);
}

void test_hashmap_clear_can_insert_after_clear(void) {
    populate_map(g_map, 10);
    hashmap_clear(g_map, false);
    struct test_item item = { .key = 42, .value = 420 };
    hashmap_set(g_map, &item);
    TEST_ASSERT_EQUAL_INT(1, (int)hashmap_count(g_map));
    struct test_item key = { .key = 42 };
    const struct test_item *found = hashmap_get(g_map, &key);
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_INT(420, found->value);
}

void test_hashmap_clear_can_insert_after_clear_update_cap_true(void) {
    populate_map(g_map, 10);
    hashmap_clear(g_map, true);
    struct test_item item = { .key = 99, .value = 990 };
    hashmap_set(g_map, &item);
    TEST_ASSERT_EQUAL_INT(1, (int)hashmap_count(g_map));
    struct test_item key = { .key = 99 };
    const struct test_item *found = hashmap_get(g_map, &key);
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_INT(990, found->value);
}

void test_hashmap_clear_multiple_times(void) {
    populate_map(g_map, 5);
    hashmap_clear(g_map, false);
    TEST_ASSERT_EQUAL_INT(0, (int)hashmap_count(g_map));
    populate_map(g_map, 5);
    hashmap_clear(g_map, false);
    TEST_ASSERT_EQUAL_INT(0, (int)hashmap_count(g_map));
}

void test_hashmap_clear_update_cap_false_nbuckets_equals_cap_no_realloc(void) {
    /* When nbuckets == cap already, no reallocation should happen */
    /* Ensure cap == nbuckets by doing update_cap=true first */
    populate_map(g_map, 5);
    hashmap_clear(g_map, true);
    /* Now cap == nbuckets, clear again with update_cap=false */
    void *buckets_before = g_map->buckets;
    size_t nbuckets_before = g_map->nbuckets;
    hashmap_clear(g_map, false);
    /* nbuckets should remain the same */
    TEST_ASSERT_EQUAL_UINT(nbuckets_before, g_map->nbuckets);
    TEST_ASSERT_EQUAL_INT(0, (int)hashmap_count(g_map));
}

void test_hashmap_clear_large_population(void) {
    populate_map(g_map, 1000);
    TEST_ASSERT_EQUAL_INT(1000, (int)hashmap_count(g_map));
    hashmap_clear(g_map, false);
    TEST_ASSERT_EQUAL_INT(0, (int)hashmap_count(g_map));
    /* Spot check a few items are gone */
    struct test_item key = { .key = 500 };
    TEST_ASSERT_NULL(hashmap_get(g_map, &key));
}

void test_hashmap_clear_large_population_update_cap_true(void) {
    populate_map(g_map, 1000);
    hashmap_clear(g_map, true);
    TEST_ASSERT_EQUAL_INT(0, (int)hashmap_count(g_map));
    TEST_ASSERT_EQUAL_UINT(g_map->nbuckets, g_map->cap);
    TEST_ASSERT_EQUAL_UINT(g_map->nbuckets - 1, g_map->mask);
}

void test_hashmap_clear_growat_shrinkat_consistent_after_update_cap_true(void) {
    populate_map(g_map, 50);
    hashmap_clear(g_map, true);
    size_t expected_growat = (size_t)(g_map->nbuckets * (g_map->loadfactor / 100.0));
    size_t expected_shrinkat = (size_t)(g_map->nbuckets * SHRINK_AT);
    TEST_ASSERT_EQUAL_UINT(expected_growat, g_map->growat);
    TEST_ASSERT_EQUAL_UINT(expected_shrinkat, g_map->shrinkat);
}

void test_hashmap_clear_buckets_not_null_after_clear(void) {
    populate_map(g_map, 10);
    hashmap_clear(g_map, false);
    TEST_ASSERT_NOT_NULL(g_map->buckets);
}

void test_hashmap_clear_buckets_not_null_after_clear_update_cap_true(void) {
    populate_map(g_map, 10);
    hashmap_clear(g_map, true);
    TEST_ASSERT_NOT_NULL(g_map->buckets);
}

/* ── main ── */

int main(void) {
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_hashmap_clear_sets_count_to_zero);
    RUN_TEST(test_hashmap_clear_empty_map_count_stays_zero);
    RUN_TEST(test_hashmap_clear_items_not_found_after_clear);
    RUN_TEST(test_hashmap_clear_update_cap_true_sets_cap_to_nbuckets);
    RUN_TEST(test_hashmap_clear_update_cap_false_preserves_nbuckets);
    RUN_TEST(test_hashmap_clear_mask_updated_correctly);
    RUN_TEST(test_hashmap_clear_mask_updated_after_update_cap_true);
    RUN_TEST(test_hashmap_clear_growat_updated_correctly);
    RUN_TEST(test_hashmap_clear_shrinkat_updated_correctly);
    RUN_TEST(test_hashmap_clear_buckets_zeroed);
    RUN_TEST(test_hashmap_clear_can_insert_after_clear);
    RUN_TEST(test_hashmap_clear_can_insert_after_clear_update_cap_true);
    RUN_TEST(test_hashmap_clear_multiple_times);
    RUN_TEST(test_hashmap_clear_update_cap_false_nbuckets_equals_cap_no_realloc);
    RUN_TEST(test_hashmap_clear_large_population);
    RUN_TEST(test_hashmap_clear_large_population_update_cap_true);
    RUN_TEST(test_hashmap_clear_growat_shrinkat_consistent_after_update_cap_true);
    RUN_TEST(test_hashmap_clear_buckets_not_null_after_clear);
    RUN_TEST(test_hashmap_clear_buckets_not_null_after_clear_update_cap_true);
    return UNITY_END();
}