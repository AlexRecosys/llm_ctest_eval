#include "hashmap.c"
#include "unity.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Fixture variables
static struct hashmap *map = NULL;
static int free_call_count = 0;
static void *test_buckets = NULL;
static size_t test_bucketsz = 0;
static size_t test_cap = 0;
static size_t test_nbuckets = 0;
static double test_loadfactor = 0.0;

// Mock allocator functions
static void *test_malloc(size_t size) {
    return malloc(size);
}

static void test_free(void *ptr) {
    free(ptr);
}

static void *test_realloc(void *ptr, size_t size) {
    return realloc(ptr, size);
}

// Custom free_elements implementation for testing
static void free_elements(struct hashmap *map) {
    // In the real implementation, this would free each element in buckets
    // For testing, we just track that it was called
    free_call_count++;
}

// Helper to initialize map with test values
static void init_map(struct hashmap *map, size_t initial_cap, size_t bucketsz, double loadfactor) {
    map->buckets = NULL;
    map->cap = initial_cap;
    map->nbuckets = initial_cap;
    map->count = 0;
    map->bucketsz = bucketsz;
    map->loadfactor = loadfactor;
    map->malloc = test_malloc;
    map->free = test_free;
    map->realloc = test_realloc;
    map->buckets = map->malloc(map->bucketsz * map->cap);
    memset(map->buckets, 0, map->bucketsz * map->cap);
    map->mask = map->nbuckets - 1;
    map->growat = map->nbuckets * (map->loadfactor / 100.0);
    map->shrinkat = map->nbuckets * SHRINK_AT;
}

// setUp and tearDown must be non-static per requirements
void setUp(void) {
    free_call_count = 0;
    map = (struct hashmap *)malloc(sizeof(struct hashmap));
    TEST_ASSERT_NOT_NULL(map);
    init_map(map, 16, sizeof(void *), 75.0);
}

void tearDown(void) {
    if (map != NULL) {
        if (map->buckets != NULL) {
            map->free(map->buckets);
        }
        free(map);
        map = NULL;
    }
}

// Test: clear with update_cap = true (should reset nbuckets to nbuckets)
void test_hashmap_clear_update_cap_true(void) {
    // Precondition: set nbuckets != cap to verify reset
    map->nbuckets = 8;
    map->cap = 16;
    map->count = 5;
    map->buckets = map->malloc(map->bucketsz * map->cap);
    memset(map->buckets, 0, map->bucketsz * map->cap);
    map->mask = map->nbuckets - 1;
    map->growat = map->nbuckets * (map->loadfactor / 100.0);
    map->shrinkat = map->nbuckets * SHRINK_AT;

    hashmap_clear(map, true);

    TEST_ASSERT_EQUAL_INT(0, map->count);
    TEST_ASSERT_EQUAL_INT(16, map->nbuckets);  // nbuckets reset to original cap (16)
    TEST_ASSERT_EQUAL_INT(16, map->cap);       // cap reset to nbuckets (16)
    TEST_ASSERT_EQUAL_INT(15, map->mask);      // mask = nbuckets - 1 = 15
    TEST_ASSERT_EQUAL_DOUBLE(12.0, map->growat);  // 16 * 0.75 = 12.0
    TEST_ASSERT_EQUAL_DOUBLE(4.0, map->shrinkat); // 16 * 0.25 = 4.0 (assuming SHRINK_AT=0.25)
    TEST_ASSERT_EQUAL_INT(1, free_call_count); // free_elements called once
}

// Test: clear with update_cap = false and nbuckets == cap (no reallocation)
void test_hashmap_clear_update_cap_false_equal_buckets(void) {
    map->nbuckets = 16;
    map->cap = 16;
    map->count = 10;
    map->buckets = map->malloc(map->bucketsz * map->cap);
    memset(map->buckets, 0, map->bucketsz * map->cap);
    map->mask = map->nbuckets - 1;
    map->growat = map->nbuckets * (map->loadfactor / 100.0);
    map->shrinkat = map->nbuckets * SHRINK_AT;

    hashmap_clear(map, false);

    TEST_ASSERT_EQUAL_INT(0, map->count);
    TEST_ASSERT_EQUAL_INT(16, map->nbuckets);
    TEST_ASSERT_EQUAL_INT(16, map->cap);
    TEST_ASSERT_EQUAL_INT(15, map->mask);
    TEST_ASSERT_EQUAL_DOUBLE(12.0, map->growat);
    TEST_ASSERT_EQUAL_DOUBLE(4.0, map->shrinkat);
    TEST_ASSERT_EQUAL_INT(1, free_call_count);
}

// Test: clear with update_cap = false and nbuckets != cap (realloc to cap)
void test_hashmap_clear_update_cap_false_unequal_buckets(void) {
    map->nbuckets = 8;
    map->cap = 16;
    map->count = 10;
    map->buckets = map->malloc(map->bucketsz * map->cap);
    memset(map->buckets, 0, map->bucketsz * map->cap);
    map->mask = map->nbuckets - 1;
    map->growat = map->nbuckets * (map->loadfactor / 100.0);
    map->shrinkat = map->nbuckets * SHRINK_AT;

    hashmap_clear(map, false);

    TEST_ASSERT_EQUAL_INT(0, map->count);
    TEST_ASSERT_EQUAL_INT(16, map->nbuckets);  // nbuckets updated to cap
    TEST_ASSERT_EQUAL_INT(16, map->cap);
    TEST_ASSERT_EQUAL_INT(15, map->mask);
    TEST_ASSERT_EQUAL_DOUBLE(12.0, map->growat);
    TEST_ASSERT_EQUAL_DOUBLE(4.0, map->shrinkat);
    TEST_ASSERT_EQUAL_INT(1, free_call_count);
}

// Test: clear with update_cap = false and realloc fails (should keep old buckets)
void test_hashmap_clear_update_cap_false_realloc_fails(void) {
    // Temporarily override malloc to fail
    void *original_malloc = map->malloc;
    map->malloc = (void *(*)(size_t))NULL; // Force failure

    map->nbuckets = 8;
    map->cap = 16;
    map->count = 10;
    map->buckets = map->malloc(map->bucketsz * map->cap);
    if (map->buckets == NULL) {
        // If initial allocation failed, use fallback
        map->buckets = malloc(map->bucketsz * map->cap);
        memset(map->buckets, 0, map->bucketsz * map->cap);
    }
    map->mask = map->nbuckets - 1;
    map->growat = map->nbuckets * (map->loadfactor / 100.0);
    map->shrinkat = map->nbuckets * SHRINK_AT;

    hashmap_clear(map, false);

    // Restore malloc
    map->malloc = original_malloc;

    TEST_ASSERT_EQUAL_INT(0, map->count);
    TEST_ASSERT_EQUAL_INT(8, map->nbuckets);  // nbuckets unchanged (realloc failed)
    TEST_ASSERT_EQUAL_INT(16, map->cap);
    TEST_ASSERT_EQUAL_INT(7, map->mask);
    TEST_ASSERT_EQUAL_DOUBLE(6.0, map->growat);  // 8 * 0.75 = 6.0
    TEST_ASSERT_EQUAL_DOUBLE(2.0, map->shrinkat); // 8 * 0.25 = 2.0
    TEST_ASSERT_EQUAL_INT(1, free_call_count);
}

// Test: clear with update_cap = false and realloc succeeds (buckets reallocated)
void test_hashmap_clear_update_cap_false_realloc_succeeds(void) {
    map->nbuckets = 8;
    map->cap = 16;
    map->count = 10;
    map->buckets = map->malloc(map->bucketsz * map->cap);
    memset(map->buckets, 0, map->bucketsz * map->cap);
    map->mask = map->nbuckets - 1;
    map->growat = map->nbuckets * (map->loadfactor / 100.0);
    map->shrinkat = map->nbuckets * SHRINK_AT;

    hashmap_clear(map, false);

    TEST_ASSERT_EQUAL_INT(0, map->count);
    TEST_ASSERT_EQUAL_INT(16, map->nbuckets);
    TEST_ASSERT_EQUAL_INT(16, map->cap);
    TEST_ASSERT_EQUAL_INT(15, map->mask);
    TEST_ASSERT_EQUAL_DOUBLE(12.0, map->growat);
    TEST_ASSERT_EQUAL_DOUBLE(4.0, map->shrinkat);
    TEST_ASSERT_EQUAL_INT(1, free_call_count);
}

// Test: clear with zero capacity (edge case)
void test_hashmap_clear_zero_capacity(void) {
    map->cap = 0;
    map->nbuckets = 0;
    map->count = 0;
    map->buckets = NULL;
    map->mask = 0;
    map->growat = 0.0;
    map->shrinkat = 0.0;

    hashmap_clear(map, true);

    TEST_ASSERT_EQUAL_INT(0, map->count);
    TEST_ASSERT_EQUAL_INT(0, map->nbuckets);
    TEST_ASSERT_EQUAL_INT(0, map->cap);
    TEST_ASSERT_EQUAL_INT(0, map->mask);
    TEST_ASSERT_EQUAL_DOUBLE(0.0, map->growat);
    TEST_ASSERT_EQUAL_DOUBLE(0.0, map->shrinkat);
    TEST_ASSERT_EQUAL_INT(1, free_call_count);
}

// Test: clear with empty map (count=0)
void test_hashmap_clear_empty_map(void) {
    map->count = 0;
    map->nbuckets = 16;
    map->cap = 16;
    map->buckets = map->malloc(map->bucketsz * map->cap);
    memset(map->buckets, 0, map->bucketsz * map->cap);
    map->mask = map->nbuckets - 1;
    map->growat = map->nbuckets * (map->loadfactor / 100.0);
    map->shrinkat = map->nbuckets * SHRINK_AT;

    hashmap_clear(map, true);

    TEST_ASSERT_EQUAL_INT(0, map->count);
    TEST_ASSERT_EQUAL_INT(16, map->nbuckets);
    TEST_ASSERT_EQUAL_INT(16, map->cap);
    TEST_ASSERT_EQUAL_INT(15, map->mask);
    TEST_ASSERT_EQUAL_DOUBLE(12.0, map->growat);
    TEST_ASSERT_EQUAL_DOUBLE(4.0, map->shrinkat);
    TEST_ASSERT_EQUAL_INT(1, free_call_count);
}

int main(void) {
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_hashmap_clear_update_cap_true);
    RUN_TEST(test_hashmap_clear_update_cap_false_equal_buckets);
    RUN_TEST(test_hashmap_clear_update_cap_false_unequal_buckets);
    RUN_TEST(test_hashmap_clear_update_cap_false_realloc_fails);
    RUN_TEST(test_hashmap_clear_update_cap_false_realloc_succeeds);
    RUN_TEST(test_hashmap_clear_zero_capacity);
    RUN_TEST(test_hashmap_clear_empty_map);
    return UNITY_END();
}