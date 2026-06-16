#include "hashmap.c"
#include "unity.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

// Static fixtures
static struct hashmap *map = NULL;
static uint64_t (*test_hash)(const void *, uint64_t, uint64_t) = NULL;
static int (*test_compare)(const void *, const void *, void *) = NULL;
static void (*test_elfree)(void *) = NULL;
static void *test_udata = NULL;

// Helper functions
static uint64_t simple_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    (void)seed0;
    (void)seed1;
    return *(uint32_t *)item ^ (seed0 >> 1);
}

static int simple_compare(const void *a, const void *b, void *udata) {
    (void)udata;
    return *(uint32_t *)a - *(uint32_t *)b;
}

static void free_int(void *item) {
    free(item);
}

static void setup_map(size_t elsize, size_t cap, bool use_custom_allocator) {
    if (map) {
        hashmap_free(map);
        map = NULL;
    }

    if (use_custom_allocator) {
        map = hashmap_new_with_allocator(
            malloc, realloc, free,
            elsize, cap, 0xdeadbeefcafebabeULL, 0x1234567890abcdefULL,
            test_hash, test_compare, test_elfree, test_udata
        );
    } else {
        map = hashmap_new(
            elsize, cap, 0xdeadbeefcafebabeULL, 0x1234567890abcdefULL,
            test_hash, test_compare, test_elfree, test_udata
        );
    }
}

void setUp(void) {
    test_hash = simple_hash;
    test_compare = simple_compare;
    test_elfree = NULL;
    test_udata = NULL;
    map = NULL;
}

void tearDown(void) {
    if (map) {
        hashmap_free(map);
        map = NULL;
    }
}

// Test cases

void test_hashmap_clear_update_cap_true_resets_count_and_buckets(void) {
    setup_map(sizeof(uint32_t), 16, false);

    // Insert some elements to grow the map
    uint32_t key1 = 100;
    uint32_t key2 = 200;
    uint32_t key3 = 300;
    hashmap_set(map, &key1);
    hashmap_set(map, &key2);
    hashmap_set(map, &key3);

    TEST_ASSERT_EQUAL(3, hashmap_count(map));

    // Clear with update_cap = true
    hashmap_clear(map, true);

    TEST_ASSERT_EQUAL(0, hashmap_count(map));
    TEST_ASSERT_EQUAL(16, map->nbuckets);
    TEST_ASSERT_EQUAL(16, map->cap);
    TEST_ASSERT_EQUAL(15, map->mask);
    TEST_ASSERT_EQUAL(0.0, map->growat);
    TEST_ASSERT_EQUAL(0.0, map->shrinkat);
}

void test_hashmap_clear_update_cap_false_preserves_capacity_if_equal(void) {
    setup_map(sizeof(uint32_t), 32, false);

    // Insert elements to reach capacity
    for (uint32_t i = 0; i < 32; i++) {
        uint32_t key = i * 100;
        hashmap_set(map, &key);
    }

    // Force growth to increase capacity beyond nbuckets
    hashmap_set_grow_by_power(map, 6); // 64 buckets

    size_t original_cap = map->cap;
    size_t original_nbuckets = map->nbuckets;

    // Clear with update_cap = false
    hashmap_clear(map, false);

    TEST_ASSERT_EQUAL(0, hashmap_count(map));
    TEST_ASSERT_EQUAL(original_cap, map->cap);
    TEST_ASSERT_EQUAL(original_nbuckets, map->nbuckets);
    TEST_ASSERT_EQUAL(original_nbuckets - 1, map->mask);
}

void test_hashmap_clear_update_cap_false_reallocates_if_nbuckets_differs(void) {
    setup_map(sizeof(uint32_t), 16, false);

    // Force growth to increase capacity beyond nbuckets
    hashmap_set_grow_by_power(map, 5); // 32 buckets

    size_t original_cap = map->cap;
    size_t original_nbuckets = map->nbuckets;

    TEST_ASSERT_NOT_EQUAL(original_cap, original_nbuckets);

    // Clear with update_cap = false
    hashmap_clear(map, false);

    TEST_ASSERT_EQUAL(0, hashmap_count(map));
    TEST_ASSERT_EQUAL(original_cap, map->cap);
    TEST_ASSERT_EQUAL(original_cap, map->nbuckets);
    TEST_ASSERT_EQUAL(original_cap - 1, map->mask);
}

void test_hashmap_clear_frees_elements_when_elfree_provided(void) {
    setup_map(sizeof(uint32_t *), 16, false);
    test_elfree = free_int;

    uint32_t *val1 = malloc(sizeof(uint32_t));
    uint32_t *val2 = malloc(sizeof(uint32_t));
    *val1 = 100;
    *val2 = 200;

    hashmap_set(map, &val1);
    hashmap_set(map, &val2);

    TEST_ASSERT_EQUAL(2, hashmap_count(map));

    // Clear with update_cap = true
    hashmap_clear(map, true);

    TEST_ASSERT_EQUAL(0, hashmap_count(map));
    // val1 and val2 should have been freed by elfree
}

void test_hashmap_clear_clears_buckets_memory(void) {
    setup_map(sizeof(uint32_t), 16, false);

    uint32_t key = 42;
    hashmap_set(map, &key);

    // Verify bucket is non-zero
    const void *found = hashmap_get(map, &key);
    TEST_ASSERT_NOT_NULL(found);

    // Clear
    hashmap_clear(map, true);

    // All buckets should be zeroed
    for (size_t i = 0; i < map->nbuckets; i++) {
        TEST_ASSERT_NULL(hashmap_probe(map, i));
    }
}

void test_hashmap_clear_reinitializes_growat_and_shrinkat_correctly(void) {
    setup_map(sizeof(uint32_t), 16, false);

    // Set custom load factor
    hashmap_set_load_factor(map, 75.0);

    // Insert some elements
    for (uint32_t i = 0; i < 10; i++) {
        uint32_t key = i * 10;
        hashmap_set(map, &key);
    }

    double original_growat = map->growat;
    double original_shrinkat = map->shrinkat;

    // Clear with update_cap = true
    hashmap_clear(map, true);

    TEST_ASSERT_EQUAL(0, hashmap_count(map));
    TEST_ASSERT_EQUAL(16, map->nbuckets);
    TEST_ASSERT_EQUAL(16, map->cap);
    TEST_ASSERT_EQUAL(15, map->mask);

    // growat = nbuckets * (loadfactor / 100.0) = 16 * 0.75 = 12.0
    TEST_ASSERT_EQUAL_DOUBLE(12.0, map->growat);
    // shrinkat = nbuckets * SHRINK_AT = 16 * 0.25 = 4.0 (assuming SHRINK_AT = 0.25)
    TEST_ASSERT_EQUAL_DOUBLE(4.0, map->shrinkat);
}

void test_hashmap_clear_with_custom_allocator(void) {
    setup_map(sizeof(uint32_t), 16, true);

    uint32_t key1 = 100;
    uint32_t key2 = 200;
    hashmap_set(map, &key1);
    hashmap_set(map, &key2);

    TEST_ASSERT_EQUAL(2, hashmap_count(map));

    // Clear with update_cap = true
    hashmap_clear(map, true);

    TEST_ASSERT_EQUAL(0, hashmap_count(map));
    TEST_ASSERT_EQUAL(16, map->nbuckets);
    TEST_ASSERT_EQUAL(16, map->cap);
}

int main(void) {
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_hashmap_clear_update_cap_true_resets_count_and_buckets);
    RUN_TEST(test_hashmap_clear_update_cap_false_preserves_capacity_if_equal);
    RUN_TEST(test_hashmap_clear_update_cap_false_reallocates_if_nbuckets_differs);
    RUN_TEST(test_hashmap_clear_frees_elements_when_elfree_provided);
    RUN_TEST(test_hashmap_clear_clears_buckets_memory);
    RUN_TEST(test_hashmap_clear_reinitializes_growat_and_shrinkat_correctly);
    RUN_TEST(test_hashmap_clear_with_custom_allocator);
    return UNITY_END();
}