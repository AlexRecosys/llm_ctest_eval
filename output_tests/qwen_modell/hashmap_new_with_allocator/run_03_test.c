#include "hashmap.c"
#include "unity.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

// Static fixtures
static void *custom_malloc(size_t size);
static void *custom_realloc(void *ptr, size_t size);
static void custom_free(void *ptr);

static void *last_malloc_call = NULL;
static size_t last_malloc_size = 0;
static int malloc_call_count = 0;

static void *last_realloc_call = NULL;
static size_t last_realloc_size = 0;
static int realloc_call_count = 0;

static void *last_free_call = NULL;
static int free_call_count = 0;

// Helper functions
static void *custom_malloc(size_t size) {
    malloc_call_count++;
    last_malloc_call = malloc(size);
    last_malloc_size = size;
    return last_malloc_call;
}

static void *custom_realloc(void *ptr, size_t size) {
    realloc_call_count++;
    last_realloc_call = ptr;
    last_realloc_size = size;
    return realloc(ptr, size);
}

static void custom_free(void *ptr) {
    free_call_count++;
    last_free_call = ptr;
    free(ptr);
}

static uint64_t test_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    const int *key = (const int *)item;
    return hashmap_sip(key, sizeof(int), seed0, seed1);
}

static int test_compare(const void *a, const void *b, void *udata) {
    const int *ia = (const int *)a;
    const int *ib = (const int *)b;
    if (*ia < *ib) return -1;
    if (*ia > *ib) return 1;
    return 0;
}

static void test_elfree(void *item) {
    // Do nothing for simple int keys
    (void)item;
}

// Test cases

void test_hashmap_new_with_allocator_returns_non_null_with_valid_params(void) {
    struct hashmap *map = hashmap_new_with_allocator(
        custom_malloc, custom_realloc, custom_free,
        sizeof(int), 0, 0x123456789ABCDEF0ULL, 0xFEDCBA9876543210ULL,
        test_hash, test_compare, test_elfree, NULL
    );
    TEST_ASSERT_NOT_NULL(map);
    TEST_ASSERT_EQUAL_SIZE(16, map->nbuckets);
    TEST_ASSERT_EQUAL_SIZE(16, map->cap);
    TEST_ASSERT_EQUAL_UINT64(0x123456789ABCDEF0ULL, map->seed0);
    TEST_ASSERT_EQUAL_UINT64(0xFEDCBA9876543210ULL, map->seed1);
    TEST_ASSERT_EQUAL_PTR(test_hash, map->hash);
    TEST_ASSERT_EQUAL_PTR(test_compare, map->compare);
    TEST_ASSERT_EQUAL_PTR(test_elfree, map->elfree);
    TEST_ASSERT_EQUAL_PTR(NULL, map->udata);
    TEST_ASSERT_EQUAL_SIZE(sizeof(int), map->elsize);
    TEST_ASSERT_NOT_NULL(map->buckets);
    TEST_ASSERT_NOT_NULL(map->spare);
    TEST_ASSERT_NOT_NULL(map->edata);
    TEST_ASSERT_EQUAL_PTR(custom_malloc, map->malloc);
    TEST_ASSERT_EQUAL_PTR(custom_realloc, map->realloc);
    TEST_ASSERT_EQUAL_PTR(custom_free, map->free);
    hashmap_free(map);
}

void test_hashmap_new_with_allocator_uses_default_allocator_when_null(void) {
    struct hashmap *map = hashmap_new_with_allocator(
        NULL, NULL, NULL,
        sizeof(int), 0, 0, 0, test_hash, test_compare, test_elfree, NULL
    );
    TEST_ASSERT_NOT_NULL(map);
    TEST_ASSERT_EQUAL_PTR(malloc, map->malloc);
    TEST_ASSERT_EQUAL_PTR(realloc, map->realloc);
    TEST_ASSERT_EQUAL_PTR(free, map->free);
    hashmap_free(map);
}

void test_hashmap_new_with_allocator_rounds_capacity_up_to_power_of_two(void) {
    struct hashmap *map = hashmap_new_with_allocator(
        custom_malloc, custom_realloc, custom_free,
        sizeof(int), 10, 0, 0, test_hash, test_compare, test_elfree, NULL
    );
    TEST_ASSERT_NOT_NULL(map);
    TEST_ASSERT_EQUAL_SIZE(16, map->nbuckets);
    TEST_ASSERT_EQUAL_SIZE(16, map->cap);
    hashmap_free(map);

    map = hashmap_new_with_allocator(
        custom_malloc, custom_realloc, custom_free,
        sizeof(int), 17, 0, 0, test_hash, test_compare, test_elfree, NULL
    );
    TEST_ASSERT_NOT_NULL(map);
    TEST_ASSERT_EQUAL_SIZE(32, map->nbuckets);
    TEST_ASSERT_EQUAL_SIZE(32, map->cap);
    hashmap_free(map);

    map = hashmap_new_with_allocator(
        custom_malloc, custom_realloc, custom_free,
        sizeof(int), 100, 0, 0, test_hash, test_compare, test_elfree, NULL
    );
    TEST_ASSERT_NOT_NULL(map);
    TEST_ASSERT_EQUAL_SIZE(128, map->nbuckets);
    TEST_ASSERT_EQUAL_SIZE(128, map->cap);
    hashmap_free(map);
}

void test_hashmap_new_with_allocator_returns_null_when_malloc_fails(void) {
    // Force malloc to fail by replacing malloc with a dummy that returns NULL
    struct hashmap *map = hashmap_new_with_allocator(
        NULL, custom_realloc, custom_free,
        sizeof(int), 0, 0, 0, test_hash, test_compare, test_elfree, NULL
    );
    // Since malloc is NULL, it should fall back to __malloc if available, but if not, to malloc
    // To test failure, we need to simulate malloc failure
    // Instead, we'll test with a custom allocator that fails on first call
    static int malloc_fail = 1;
    void *custom_malloc_fail(size_t size) {
        if (malloc_fail) {
            malloc_fail = 0;
            return NULL;
        }
        return malloc(size);
    }
    map = hashmap_new_with_allocator(
        custom_malloc_fail, custom_realloc, custom_free,
        sizeof(int), 0, 0, 0, test_hash, test_compare, test_elfree, NULL
    );
    TEST_ASSERT_NULL(map);
}

void test_hashmap_new_with_allocator_frees_map_when_buckets_malloc_fails(void) {
    static int malloc_count = 0;
    void *custom_malloc_fail_after_2(size_t size) {
        malloc_count++;
        if (malloc_count > 2) {
            return NULL;
        }
        return malloc(size);
    }
    malloc_count = 0;
    struct hashmap *map = hashmap_new_with_allocator(
        custom_malloc_fail_after_2, custom_realloc, custom_free,
        sizeof(int), 0, 0, 0, test_hash, test_compare, test_elfree, NULL
    );
    TEST_ASSERT_NULL(map);
    // Verify that the first map allocation was freed
    TEST_ASSERT_EQUAL_INT(2, free_call_count);
}

void test_hashmap_new_with_allocator_sets_bucketsz_correctly(void) {
    struct hashmap *map = hashmap_new_with_allocator(
        custom_malloc, custom_realloc, custom_free,
        sizeof(int), 0, 0, 0, test_hash, test_compare, test_elfree, NULL
    );
    TEST_ASSERT_NOT_NULL(map);
    // bucketsz = sizeof(bucket) + elsize, padded to uintptr_t alignment
    size_t expected_bucketsz = sizeof(struct bucket) + sizeof(int);
    // Round up to next multiple of sizeof(uintptr_t)
    while (expected_bucketsz & (sizeof(uintptr_t) - 1)) {
        expected_bucketsz++;
    }
    TEST_ASSERT_EQUAL_SIZE(expected_bucketsz, map->bucketsz);
    hashmap_free(map);
}

void test_hashmap_new_with_allocator_sets_load_factor_and_growat_correctly(void) {
    struct hashmap *map = hashmap_new_with_allocator(
        custom_malloc, custom_realloc, custom_free,
        sizeof(int), 0, 0, 0, test_hash, test_compare, test_elfree, NULL
    );
    TEST_ASSERT_NOT_NULL(map);
    // Default load factor is HASHMAP_LOAD_FACTOR (0.75) and GROW_AT (0.75)
    // loadfactor = clamp_load_factor(HASHMAP_LOAD_FACTOR, GROW_AT) * 100 = 75
    TEST_ASSERT_EQUAL_INT(75, map->loadfactor);
    // growat = nbuckets * (loadfactor / 100.0) = 16 * 0.75 = 12
    TEST_ASSERT_EQUAL_DOUBLE(12.0, map->growat);
    TEST_ASSERT_EQUAL_DOUBLE(16 * 0.125, map->shrinkat); // SHRINK_AT = 0.125
    hashmap_free(map);
}

void test_hashmap_new_with_allocator_sets_growpower_correctly(void) {
    struct hashmap *map = hashmap_new_with_allocator(
        custom_malloc, custom_realloc, custom_free,
        sizeof(int), 0, 0, 0, test_hash, test_compare, test_elfree, NULL
    );
    TEST_ASSERT_NOT_NULL(map);
    TEST_ASSERT_EQUAL_INT(1, map->growpower);
    hashmap_free(map);
}

void test_hashmap_new_with_allocator_sets_mask_correctly(void) {
    struct hashmap *map = hashmap_new_with_allocator(
        custom_malloc, custom_realloc, custom_free,
        sizeof(int), 0, 0, 0, test_hash, test_compare, test_elfree, NULL
    );
    TEST_ASSERT_NOT_NULL(map);
    TEST_ASSERT_EQUAL_UINT64(15, map->mask); // nbuckets - 1 = 16 - 1 = 15
    hashmap_free(map);
}

void test_hashmap_new_with_allocator_initializes_all_fields_to_zero_or_null(void) {
    struct hashmap *map = hashmap_new_with_allocator(
        custom_malloc, custom_realloc, custom_free,
        sizeof(int), 0, 0, 0, test_hash, test_compare, test_elfree, NULL
    );
    TEST_ASSERT_NOT_NULL(map);
    TEST_ASSERT_EQUAL_SIZE(0, map->count);
    TEST_ASSERT_EQUAL_PTR(NULL, map->buckets);
    // Wait, buckets is allocated in the function, so it should be non-null
    // Let's check the actual state
    TEST_ASSERT_NOT_NULL(map->buckets);
    // All buckets should be zeroed
    size_t i;
    for (i = 0; i < map->nbuckets; i++) {
        const void *item = hashmap_probe(map, i);
        TEST_ASSERT_NULL(item);
    }
    hashmap_free(map);
}

void test_hashmap_new_with_allocator_with_custom_capacity_uses_exact_power_of_two(void) {
    struct hashmap *map = hashmap_new_with_allocator(
        custom_malloc, custom_realloc, custom_free,
        sizeof(int), 32, 0, 0, test_hash, test_compare, test_elfree, NULL
    );
    TEST_ASSERT_NOT_NULL(map);
    TEST_ASSERT_EQUAL_SIZE(32, map->nbuckets);
    TEST_ASSERT_EQUAL_SIZE(32, map->cap);
    TEST_ASSERT_EQUAL_UINT64(31, map->mask);
    hashmap_free(map);
}

void test_hashmap_new_with_allocator_with_large_capacity_rounds_up_correctly(void) {
    struct hashmap *map = hashmap_new_with_allocator(
        custom_malloc, custom_realloc, custom_free,
        sizeof(int), 1000, 0, 0, test_hash, test_compare, test_elfree, NULL
    );
    TEST_ASSERT_NOT_NULL(map);
    TEST_ASSERT_EQUAL_SIZE(1024, map->nbuckets);
    TEST_ASSERT_EQUAL_SIZE(1024, map->cap);
    TEST_ASSERT_EQUAL_UINT64(1023, map->mask);
    hashmap_free(map);
}

void test_hashmap_new_with_allocator_with_udata_sets_udata_correctly(void) {
    int udata = 42;
    struct hashmap *map = hashmap_new_with_allocator(
        custom_malloc, custom_realloc, custom_free,
        sizeof(int), 0, 0, 0, test_hash, test_compare, test_elfree, &udata
    );
    TEST_ASSERT_NOT_NULL(map);
    TEST_ASSERT_EQUAL_PTR(&udata, map->udata);
    hashmap_free(map);
}

void test_hashmap_new_with_allocator_with_custom_hash_and_compare_sets_callbacks_correctly(void) {
    uint64_t custom_hash(const void *item, uint64_t seed0, uint64_t seed1) {
        return 12345;
    }
    int custom_compare(const void *a, const void *b, void *udata) {
        return 0;
    }
    struct hashmap *map = hashmap_new_with_allocator(
        custom_malloc, custom_realloc, custom_free,
        sizeof(int), 0, 0, 0, custom_hash, custom_compare, test_elfree, NULL
    );
    TEST_ASSERT_NOT_NULL(map);
    TEST_ASSERT_EQUAL_PTR(custom_hash, map->hash);
    TEST_ASSERT_EQUAL_PTR(custom_compare, map->compare);
    hashmap_free(map);
}

void test_hashmap_new_with_allocator_with_elfree_sets_elfree_correctly(void) {
    void custom_elfree(void *item) {
        // Do nothing
        (void)item;
    }
    struct hashmap *map = hashmap_new_with_allocator(
        custom_malloc, custom_realloc, custom_free,
        sizeof(int), 0, 0, 0, test_hash, test_compare, custom_elfree, NULL
    );
    TEST_ASSERT_NOT_NULL(map);
    TEST_ASSERT_EQUAL_PTR(custom_elfree, map->elfree);
    hashmap_free(map);
}

int main(void) {
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_hashmap_new_with_allocator_returns_non_null_with_valid_params);
    RUN_TEST(test_hashmap_new_with_allocator_uses_default_allocator_when_null);
    RUN_TEST(test_hashmap_new_with_allocator_rounds_capacity_up_to_power_of_two);
    RUN_TEST(test_hashmap_new_with_allocator_returns_null_when_malloc_fails);
    RUN_TEST(test_hashmap_new_with_allocator_frees_map_when_buckets_malloc_fails);
    RUN_TEST(test_hashmap_new_with_allocator_sets_bucketsz_correctly);
    RUN_TEST(test_hashmap_new_with_allocator_sets_load_factor_and_growat_correctly);
    RUN_TEST(test_hashmap_new_with_allocator_sets_growpower_correctly);
    RUN_TEST(test_hashmap_new_with_allocator_sets_mask_correctly);
    RUN_TEST(test_hashmap_new_with_allocator_initializes_all_fields_to_zero_or_null);
    RUN_TEST(test_hashmap_new_with_allocator_with_custom_capacity_uses_exact_power_of_two);
    RUN_TEST(test_hashmap_new_with_allocator_with_large_capacity_rounds_up_correctly);
    RUN_TEST(test_hashmap_new_with_allocator_with_udata_sets_udata_correctly);
    RUN_TEST(test_hashmap_new_with_allocator_with_custom_hash_and_compare_sets_callbacks_correctly);
    RUN_TEST(test_hashmap_new_with_allocator_with_elfree_sets_elfree_correctly);
    return UNITY_END();
}