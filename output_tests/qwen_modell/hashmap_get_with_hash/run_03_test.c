#include "hashmap.c"
#include "unity.h"
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

// Static fixtures
static struct hashmap *map = NULL;
static int compare_calls = 0;
static uint64_t last_hash = 0;

// Helper functions
static uint64_t test_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    const int *key = (const int *)item;
    return *key ^ seed0 ^ (seed1 << 1);
}

static int test_compare(const void *a, const void *b, void *udata) {
    compare_calls++;
    const int *ia = (const int *)a;
    const int *ib = (const int *)b;
    if (*ia < *ib) return -1;
    if (*ia > *ib) return 1;
    return 0;
}

static void setup_map(void) {
    map = hashmap_new(sizeof(int), 0, 0x123456789abcdef0ULL, 0xfedcba9876543210ULL,
                      test_hash, test_compare, NULL, NULL);
    TEST_ASSERT_NOT_NULL(map);
}

static void teardown_map(void) {
    if (map) {
        hashmap_free(map);
        map = NULL;
    }
    compare_calls = 0;
    last_hash = 0;
}

// Test cases

void test_hashmap_get_with_hash_returns_existing_item(void) {
    setup_map();
    
    int key = 42;
    int value = 100;
    const void *result;
    
    // Insert item
    result = hashmap_set(map, &value);
    TEST_ASSERT_NULL(result);
    
    // Get using hash
    uint64_t hash = test_hash(&key, 0x123456789abcdef0ULL, 0xfedcba9876543210ULL);
    const void *retrieved = hashmap_get_with_hash(map, &key, hash);
    
    TEST_ASSERT_NOT_NULL(retrieved);
    TEST_ASSERT_EQUAL_INT(100, *(const int *)retrieved);
    
    teardown_map();
}

void test_hashmap_get_with_hash_returns_null_for_missing_key(void) {
    setup_map();
    
    int key = 999;
    uint64_t hash = test_hash(&key, 0x123456789abcdef0ULL, 0xfedcba9876543210ULL);
    const void *result = hashmap_get_with_hash(map, &key, hash);
    
    TEST_ASSERT_NULL(result);
    
    teardown_map();
}

void test_hashmap_get_with_hash_uses_provided_hash_not_map_hash(void) {
    setup_map();
    
    int key = 42;
    int value = 100;
    
    // Insert item
    const void *result = hashmap_set(map, &value);
    TEST_ASSERT_NULL(result);
    
    // Get using wrong hash (should not find item)
    uint64_t wrong_hash = test_hash(&key, 0x123456789abcdef0ULL, 0xfedcba9876543210ULL) ^ 0xFFFFFFFFFFFFFFFFULL;
    const void *retrieved = hashmap_get_with_hash(map, &key, wrong_hash);
    
    TEST_ASSERT_NULL(retrieved);
    
    // Get using correct hash (should find item)
    uint64_t correct_hash = test_hash(&key, 0x123456789abcdef0ULL, 0xfedcba9876543210ULL);
    retrieved = hashmap_get_with_hash(map, &key, correct_hash);
    
    TEST_ASSERT_NOT_NULL(retrieved);
    TEST_ASSERT_EQUAL_INT(100, *(const int *)retrieved);
    
    teardown_map();
}

void test_hashmap_get_with_hash_uses_custom_compare_function(void) {
    setup_map();
    
    int key1 = 42;
    int key2 = 42; // same value, different address
    int value = 100;
    
    // Insert item
    const void *result = hashmap_set(map, &value);
    TEST_ASSERT_NULL(result);
    
    // Get using same hash but different key pointer
    uint64_t hash = test_hash(&key1, 0x123456789abcdef0ULL, 0xfedcba9876543210ULL);
    const void *retrieved = hashmap_get_with_hash(map, &key2, hash);
    
    TEST_ASSERT_NOT_NULL(retrieved);
    TEST_ASSERT_EQUAL_INT(100, *(const int *)retrieved);
    TEST_ASSERT_EQUAL_INT(1, compare_calls);
    
    teardown_map();
}

void test_hashmap_get_with_hash_ignores_compare_when_null(void) {
    // Create map with NULL compare function
    struct hashmap *map2 = hashmap_new(sizeof(int), 0, 0x123456789abcdef0ULL, 0xfedcba9876543210ULL,
                                       test_hash, NULL, NULL, NULL);
    TEST_ASSERT_NOT_NULL(map2);
    
    int key = 42;
    int value = 100;
    
    // Insert item
    const void *result = hashmap_set(map2, &value);
    TEST_ASSERT_NULL(result);
    
    // Get using hash
    uint64_t hash = test_hash(&key, 0x123456789abcdef0ULL, 0xfedcba9876543210ULL);
    const void *retrieved = hashmap_get_with_hash(map2, &key, hash);
    
    TEST_ASSERT_NOT_NULL(retrieved);
    TEST_ASSERT_EQUAL_INT(100, *(const int *)retrieved);
    
    hashmap_free(map2);
}

void test_hashmap_get_with_hash_handles_collision_chain(void) {
    setup_map();
    
    // Force collisions by using keys that hash to same bucket
    int key1 = 1;
    int key2 = 2;
    int key3 = 3;
    int value1 = 10;
    int value2 = 20;
    int value3 = 30;
    
    // Insert items
    const void *result = hashmap_set(map, &value1);
    TEST_ASSERT_NULL(result);
    result = hashmap_set(map, &value2);
    TEST_ASSERT_NULL(result);
    result = hashmap_set(map, &value3);
    TEST_ASSERT_NULL(result);
    
    // Get using hash for key2 (middle item)
    uint64_t hash2 = test_hash(&key2, 0x123456789abcdef0ULL, 0xfedcba9876543210ULL);
    const void *retrieved2 = hashmap_get_with_hash(map, &key2, hash2);
    
    TEST_ASSERT_NOT_NULL(retrieved2);
    TEST_ASSERT_EQUAL_INT(20, *(const int *)retrieved2);
    
    // Get using hash for key3 (last item in chain)
    uint64_t hash3 = test_hash(&key3, 0x123456789abcdef0ULL, 0xfedcba9876543210ULL);
    const void *retrieved3 = hashmap_get_with_hash(map, &key3, hash3);
    
    TEST_ASSERT_NOT_NULL(retrieved3);
    TEST_ASSERT_EQUAL_INT(30, *(const int *)retrieved3);
    
    teardown_map();
}

void test_hashmap_get_with_hash_returns_null_when_bucket_empty(void) {
    setup_map();
    
    // Insert one item to establish some buckets
    int key1 = 1;
    int value1 = 10;
    const void *result = hashmap_set(map, &value1);
    TEST_ASSERT_NULL(result);
    
    // Try to get from a bucket that's empty (different hash)
    int key2 = 1000;
    uint64_t hash2 = test_hash(&key2, 0x123456789abcdef0ULL, 0xfedcba9876543210ULL);
    const void *retrieved = hashmap_get_with_hash(map, &key2, hash2);
    
    TEST_ASSERT_NULL(retrieved);
    
    teardown_map();
}

void test_hashmap_get_with_hash_with_large_hash_value(void) {
    setup_map();
    
    int key = 42;
    int value = 100;
    
    // Insert item
    const void *result = hashmap_set(map, &value);
    TEST_ASSERT_NULL(result);
    
    // Get using hash with high bits set
    uint64_t hash = test_hash(&key, 0x123456789abcdef0ULL, 0xfedcba9876543210ULL);
    hash |= 0xFFFFFFFF00000000ULL; // Set high bits
    const void *retrieved = hashmap_get_with_hash(map, &key, hash);
    
    TEST_ASSERT_NOT_NULL(retrieved);
    TEST_ASSERT_EQUAL_INT(100, *(const int *)retrieved);
    
    teardown_map();
}

void test_hashmap_get_with_hash_with_zero_hash(void) {
    setup_map();
    
    int key = 0;
    int value = 100;
    
    // Insert item
    const void *result = hashmap_set(map, &value);
    TEST_ASSERT_NULL(result);
    
    // Get using zero hash
    uint64_t hash = 0;
    const void *retrieved = hashmap_get_with_hash(map, &key, hash);
    
    // This should only succeed if the hash of key 0 is 0 (unlikely but possible)
    // In our test_hash, hash(0) = 0 ^ seed0 ^ (seed1 << 1) = 0x123456789abcdef0ULL ^ (0xfedcba9876543210ULL << 1)
    // which is not zero, so this should return NULL
    TEST_ASSERT_NULL(retrieved);
    
    teardown_map();
}

void test_hashmap_get_with_hash_with_empty_map(void) {
    setup_map();
    
    int key = 42;
    uint64_t hash = test_hash(&key, 0x123456789abcdef0ULL, 0xfedcba9876543210ULL);
    const void *retrieved = hashmap_get_with_hash(map, &key, hash);
    
    TEST_ASSERT_NULL(retrieved);
    
    teardown_map();
}

int main(void) {
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_hashmap_get_with_hash_returns_existing_item);
    RUN_TEST(test_hashmap_get_with_hash_returns_null_for_missing_key);
    RUN_TEST(test_hashmap_get_with_hash_uses_provided_hash_not_map_hash);
    RUN_TEST(test_hashmap_get_with_hash_uses_custom_compare_function);
    RUN_TEST(test_hashmap_get_with_hash_ignores_compare_when_null);
    RUN_TEST(test_hashmap_get_with_hash_handles_collision_chain);
    RUN_TEST(test_hashmap_get_with_hash_returns_null_when_bucket_empty);
    RUN_TEST(test_hashmap_get_with_hash_with_large_hash_value);
    RUN_TEST(test_hashmap_get_with_hash_with_zero_hash);
    RUN_TEST(test_hashmap_get_with_hash_with_empty_map);
    return UNITY_END();
}