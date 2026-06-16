#include "hashmap.c"
#include "unity.h"
#include "hashmap.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* ── fixtures ── */

struct test_item {
    int   key;
    int   value;
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
        struct test_item ti = { .key = i, .value = i * 10 };
        hashmap_set(map, &ti);
    }
}

/* ── setUp / tearDown ── */

void setUp(void) {
    g_map = create_map();
    TEST_ASSERT_NOT_NULL(g_map);
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
    TEST_ASSERT_EQUAL_UINT(10, hashmap_count(g_map));

    hashmap_clear(g_map, false);

    TEST_ASSERT_EQUAL_UINT(0, hashmap_count(g_map));
}

void test_hashmap_clear_empty_map_count_stays_zero(void) {
    TEST_ASSERT_EQUAL_UINT(0, hashmap_count(g_map));

    hashmap_clear(g_map, false);

    TEST_ASSERT_EQUAL_UINT(0, hashmap_count(g_map));
}

void test_hashmap_clear_items_not_retrievable_after_clear(void) {
    populate_map(g_map, 5);

    hashmap_clear(g_map, false);

    for (int i = 0; i < 5; i++) {
        struct test_item key = { .key = i };
        const struct test_item *found =
            (const struct test_item *)hashmap_get(g_map, &key);
        TEST_ASSERT_NULL(found);
    }
}

void test_hashmap_clear_update_cap_true_cap_equals_nbuckets(void) {
    populate_map(g_map, 50);

    /* capture nbuckets before clear */
    size_t nbuckets_before = g_map->nbuckets;

    hashmap_clear(g_map, true);

    /* with update_cap=true, cap is set to nbuckets */
    TEST_ASSERT_EQUAL_UINT(nbuckets_before, (size_t)g_map->cap);
    TEST_ASSERT_EQUAL_UINT(g_map->cap, (size_t)g_map->nbuckets);
}

void test_hashmap_clear_update_cap_false_nbuckets_equals_cap(void) {
    populate_map(g_map, 50);

    /* After many inserts nbuckets may have grown; cap tracks original */
    size_t cap_before = (size_t)g_map->cap;

    hashmap_clear(g_map, false);

    /* with update_cap=false, nbuckets is reset to cap */
    TEST_ASSERT_EQUAL_UINT(cap_before, (size_t)g_map->nbuckets);
    TEST_ASSERT_EQUAL_UINT(g_map->cap, (size_t)g_map->nbuckets);
}

void test_hashmap_clear_mask_is_nbuckets_minus_one(void) {
    populate_map(g_map, 20);

    hashmap_clear(g_map, false);

    TEST_ASSERT_EQUAL_UINT((size_t)(g_map->nbuckets - 1), (size_t)g_map->mask);
}

void test_hashmap_clear_mask_is_nbuckets_minus_one_update_cap_true(void) {
    populate_map(g_map, 20);

    hashmap_clear(g_map, true);

    TEST_ASSERT_EQUAL_UINT((size_t)(g_map->nbuckets - 1), (size_t)g_map->mask);
}

void test_hashmap_clear_growat_is_set_correctly(void) {
    populate_map(g_map, 20);

    hashmap_clear(g_map, false);

    size_t expected_growat =
        (size_t)((double)g_map->nbuckets * (g_map->loadfactor / 100.0));
    TEST_ASSERT_EQUAL_UINT(expected_growat, (size_t)g_map->growat);
}

void test_hashmap_clear_shrinkat_is_set_correctly(void) {
    populate_map(g_map, 20);

    hashmap_clear(g_map, false);

    size_t expected_shrinkat =
        (size_t)((double)g_map->nbuckets * SHRINK_AT);
    TEST_ASSERT_EQUAL_UINT(expected_shrinkat, (size_t)g_map->shrinkat);
}

void test_hashmap_clear_map_usable_after_clear(void) {
    populate_map(g_map, 10);
    hashmap_clear(g_map, false);

    /* re-insert items and verify they are retrievable */
    for (int i = 0; i < 10; i++) {
        struct test_item ti = { .key = i, .value = i * 100 };
        hashmap_set(g_map, &ti);
    }

    TEST_ASSERT_EQUAL_UINT(10, hashmap_count(g_map));

    for (int i = 0; i < 10; i++) {
        struct test_item key = { .key = i };
        const struct test_item *found =
            (const struct test_item *)hashmap_get(g_map, &key);
        TEST_ASSERT_NOT_NULL(found);
        TEST_ASSERT_EQUAL_INT(i * 100, found->value);
    }
}

void test_hashmap_clear_map_usable_after_clear_update_cap_true(void) {
    populate_map(g_map, 10);
    hashmap_clear(g_map, true);

    for (int i = 0; i < 10; i++) {
        struct test_item ti = { .key = i, .value = i * 200 };
        hashmap_set(g_map, &ti);
    }

    TEST_ASSERT_EQUAL_UINT(10, hashmap_count(g_map));

    for (int i = 0; i < 10; i++) {
        struct test_item key = { .key = i };
        const struct test_item *found =
            (const struct test_item *)hashmap_get(g_map, &key);
        TEST_ASSERT_NOT_NULL(found);
        TEST_ASSERT_EQUAL_INT(i * 200, found->value);
    }
}

void test_hashmap_clear_buckets_zeroed(void) {
    populate_map(g_map, 5);

    hashmap_clear(g_map, false);

    /* All bucket memory should be zero */
    size_t total = g_map->bucketsz * g_map->nbuckets;
    unsigned char *raw = (unsigned char *)g_map->buckets;
    int all_zero = 1;
    for (size_t i = 0; i < total; i++) {
        if (raw[i] != 0) { all_zero = 0; break; }
    }
    TEST_ASSERT_TRUE(all_zero);
}

void test_hashmap_clear_multiple_times(void) {
    populate_map(g_map, 15);
    hashmap_clear(g_map, false);
    TEST_ASSERT_EQUAL_UINT(0, hashmap_count(g_map));

    populate_map(g_map, 15);
    hashmap_clear(g_map, false);
    TEST_ASSERT_EQUAL_UINT(0, hashmap_count(g_map));

    populate_map(g_map, 15);
    hashmap_clear(g_map, true);
    TEST_ASSERT_EQUAL_UINT(0, hashmap_count(g_map));
}

void test_hashmap_clear_large_population_update_cap_false(void) {
    populate_map(g_map, 200);
    TEST_ASSERT_EQUAL_UINT(200, hashmap_count(g_map));

    size_t cap_before = (size_t)g_map->cap;

    hashmap_clear(g_map, false);

    TEST_ASSERT_EQUAL_UINT(0, hashmap_count(g_map));
    TEST_ASSERT_EQUAL_UINT(cap_before, (size_t)g_map->nbuckets);
}

void test_hashmap_clear_large_population_update_cap_true(void) {
    populate_map(g_map, 200);
    TEST_ASSERT_EQUAL_UINT(200, hashmap_count(g_map));

    size_t nbuckets_before = g_map->nbuckets;

    hashmap_clear(g_map, true);

    TEST_ASSERT_EQUAL_UINT(0, hashmap_count(g_map));
    TEST_ASSERT_EQUAL_UINT(nbuckets_before, (size_t)g_map->cap);
    TEST_ASSERT_EQUAL_UINT(nbuckets_before, (size_t)g_map->nbuckets);
}

/* ── main ── */

int main(void) {
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_hashmap_clear_sets_count_to_zero);
    RUN_TEST(test_hashmap_clear_empty_map_count_stays_zero);
    RUN_TEST(test_hashmap_clear_items_not_retrievable_after_clear);
    RUN_TEST(test_hashmap_clear_update_cap_true_cap_equals_nbuckets);
    RUN_TEST(test_hashmap_clear_update_cap_false_nbuckets_equals_cap);
    RUN_TEST(test_hashmap_clear_mask_is_nbuckets_minus_one);
    RUN_TEST(test_hashmap_clear_mask_is_nbuckets_minus_one_update_cap_true);
    RUN_TEST(test_hashmap_clear_growat_is_set_correctly);
    RUN_TEST(test_hashmap_clear_shrinkat_is_set_correctly);
    RUN_TEST(test_hashmap_clear_map_usable_after_clear);
    RUN_TEST(test_hashmap_clear_map_usable_after_clear_update_cap_true);
    RUN_TEST(test_hashmap_clear_buckets_zeroed);
    RUN_TEST(test_hashmap_clear_multiple_times);
    RUN_TEST(test_hashmap_clear_large_population_update_cap_false);
    RUN_TEST(test_hashmap_clear_large_population_update_cap_true);
    return UNITY_END();
}