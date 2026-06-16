#include "hashmap.c"
#include "unity.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// Helper hash function using SipHash
static uint64_t test_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    return hashmap_sip(item, sizeof(int), seed0, seed1);
}

// Helper compare function for integers
static int test_compare(const void *a, const void *b, void *udata) {
    int ia = *(const int *)a;
    int ib = *(const int *)b;
    return (ia > ib) - (ia < ib);
}

// Fixture variables
static struct hashmap *map = NULL;
static int key1 = 42;
static int key2 = 100;
static int key3 = 999;
static int val1 = 1000;
static int val2 = 2000;
static int val3 = 3000;

void setUp(void) {
    map = hashmap_new(sizeof(int), 0, 0xdeadbeef, 0xcafebabe,
                      test_hash, test_compare, NULL, NULL);
    TEST_ASSERT_NOT_NULL(map);
}

void tearDown(void) {
    if (map) {
        hashmap_free(map);
        map = NULL;
    }
}

static void populate_map_with_keys(void) {
    // Insert three key-value pairs (keys and values are the same for simplicity)
    TEST_ASSERT_NULL(hashmap_set(map, &key1));
    TEST_ASSERT_NULL(hashmap_set(map, &key2));
    TEST_ASSERT_NULL(hashmap_set(map, &key3));
    TEST_ASSERT_EQUAL_SIZE_T(3, hashmap_count(map));
}

// Test: delete existing key returns the deleted value
void test_hashmap_delete_returns_deleted_value(void) {
    populate_map_with_keys();

    const void *deleted = hashmap_delete(map, &key1);
    TEST_ASSERT_NOT_NULL(deleted);
    TEST_ASSERT_EQUAL_INT(key1, *(const int *)deleted);
    TEST_ASSERT_EQUAL_SIZE_T(2, hashmap_count(map));
}

// Test: delete non-existing key returns NULL
void test_hashmap_delete_non_existing_key_returns_null(void) {
    populate_map_with_keys();

    int non_existing = 777;
    const void *deleted = hashmap_delete(map, &non_existing);
    TEST_ASSERT_NULL(deleted);
    TEST_ASSERT_EQUAL_SIZE_T(3, hashmap_count(map));
}

// Test: delete key after insertion and verify it's gone
void test_hashmap_delete_removes_key(void) {
    populate_map_with_keys();

    // Delete key1
    const void *deleted = hashmap_delete(map, &key1);
    TEST_ASSERT_NOT_NULL(deleted);
    TEST_ASSERT_EQUAL_INT(key1, *(const int *)deleted);

    // Verify key1 is no longer in map
    const void *retrieved = hashmap_get(map, &key1);
    TEST_ASSERT_NULL(retrieved);

    // Verify other keys still present
    retrieved = hashmap_get(map, &key2);
    TEST_ASSERT_NOT_NULL(retrieved);
    TEST_ASSERT_EQUAL_INT(key2, *(const int *)retrieved);

    retrieved = hashmap_get(map, &key3);
    TEST_ASSERT_NOT_NULL(retrieved);
    TEST_ASSERT_EQUAL_INT(key3, *(const int *)retrieved);
}

// Test: delete all keys one by one
void test_hashmap_delete_all_keys(void) {
    populate_map_with_keys();

    // Delete all keys
    TEST_ASSERT_NOT_NULL(hashmap_delete(map, &key1));
    TEST_ASSERT_EQUAL_SIZE_T(2, hashmap_count(map));
    TEST_ASSERT_NOT_NULL(hashmap_delete(map, &key2));
    TEST_ASSERT_EQUAL_SIZE_T(1, hashmap_count(map));
    TEST_ASSERT_NOT_NULL(hashmap_delete(map, &key3));
    TEST_ASSERT_EQUAL_SIZE_T(0, hashmap_count(map));

    // Map should be empty
    TEST_ASSERT_NULL(hashmap_get(map, &key1));
    TEST_ASSERT_NULL(hashmap_get(map, &key2));
    TEST_ASSERT_NULL(hashmap_get(map, &key3));
}

// Test: delete same key twice returns NULL second time
void test_hashmap_delete_twice_second_returns_null(void) {
    populate_map_with_keys();

    // First delete
    const void *deleted1 = hashmap_delete(map, &key1);
    TEST_ASSERT_NOT_NULL(deleted1);
    TEST_ASSERT_EQUAL_INT(key1, *(const int *)deleted1);
    TEST_ASSERT_EQUAL_SIZE_T(2, hashmap_count(map));

    // Second delete of same key
    const void *deleted2 = hashmap_delete(map, &key1);
    TEST_ASSERT_NULL(deleted2);
    TEST_ASSERT_EQUAL_SIZE_T(2, hashmap_count(map));
}

// Test: delete with empty map returns NULL
void test_hashmap_delete_empty_map_returns_null(void) {
    TEST_ASSERT_EQUAL_SIZE_T(0, hashmap_count(map));
    int key = 123;
    const void *deleted = hashmap_delete(map, &key);
    TEST_ASSERT_NULL(deleted);
}

// Test: delete with NULL map (undefined behavior, but ensure no crash in test harness)
// Note: This test is intentionally omitted because passing NULL to hashmap_delete
// is undefined behavior per the API contract. We only test valid usage.

int main(void) {
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_hashmap_delete_returns_deleted_value);
    RUN_TEST(test_hashmap_delete_non_existing_key_returns_null);
    RUN_TEST(test_hashmap_delete_removes_key);
    RUN_TEST(test_hashmap_delete_all_keys);
    RUN_TEST(test_hashmap_delete_twice_second_returns_null);
    RUN_TEST(test_hashmap_delete_empty_map_returns_null);
    return UNITY_END();
}