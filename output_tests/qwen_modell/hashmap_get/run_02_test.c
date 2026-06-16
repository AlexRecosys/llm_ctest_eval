#include "hashmap.c"
#include "unity.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// Helper hash function using SipHash for integer keys
static uint64_t int_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    return hashmap_sip(item, sizeof(int), seed0, seed1);
}

// Compare function for integer keys
static int int_compare(const void *a, const void *b, void *udata) {
    int ia = *(const int *)a;
    int ib = *(const int *)b;
    return (ia > ib) - (ia < ib);
}

// Fixture variables
static struct hashmap *map = NULL;
static int key1 = 42;
static int key2 = 100;
static int key3 = 999;
static int val1 = 123;
static int val2 = 456;
static int val3 = 789;

void setUp(void) {
    map = hashmap_new(sizeof(int), 0, 0xdeadbeef, 0xcafebabe,
                      int_hash, int_compare, NULL, NULL);
    TEST_ASSERT_NOT_NULL(map);
}

void tearDown(void) {
    if (map) {
        hashmap_free(map);
        map = NULL;
    }
}

// Test: hashmap_get returns NULL for empty map
static void test_hashmap_get_empty_map_returns_null(void) {
    const void *result = hashmap_get(map, &key1);
    TEST_ASSERT_NULL(result);
}

// Test: hashmap_get returns value after insertion
static void test_hashmap_get_after_insert_returns_value(void) {
    // Insert key1 -> val1
    const void *old = hashmap_set(map, &val1);
    TEST_ASSERT_NULL(old);

    // Retrieve using key1
    const void *result = hashmap_get(map, &key1);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(val1, *(const int *)result);
}

// Test: hashmap_get returns NULL for non-existent key
static void test_hashmap_get_nonexistent_key_returns_null(void) {
    // Insert key1 -> val1
    const void *old = hashmap_set(map, &val1);
    TEST_ASSERT_NULL(old);

    // Try to get key2 (not inserted)
    const void *result = hashmap_get(map, &key2);
    TEST_ASSERT_NULL(result);
}

// Test: hashmap_get returns updated value after replacement
static void test_hashmap_get_after_replacement_returns_new_value(void) {
    // Insert key1 -> val1
    const void *old = hashmap_set(map, &val1);
    TEST_ASSERT_NULL(old);

    // Replace key1 -> val2
    old = hashmap_set(map, &val2);
    TEST_ASSERT_NOT_NULL(old);
    TEST_ASSERT_EQUAL_INT(val1, *(const int *)old);

    // Retrieve key1 should now return val2
    const void *result = hashmap_get(map, &key1);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(val2, *(const int *)result);
}

// Test: hashmap_get works with multiple keys
static void test_hashmap_get_multiple_keys(void) {
    // Insert multiple entries
    const void *old = hashmap_set(map, &val1);
    TEST_ASSERT_NULL(old);

    old = hashmap_set(map, &val2);
    TEST_ASSERT_NULL(old);

    old = hashmap_set(map, &val3);
    TEST_ASSERT_NULL(old);

    // Verify all values
    const void *r1 = hashmap_get(map, &key1);
    TEST_ASSERT_NOT_NULL(r1);
    TEST_ASSERT_EQUAL_INT(val1, *(const int *)r1);

    const void *r2 = hashmap_get(map, &key2);
    TEST_ASSERT_NOT_NULL(r2);
    TEST_ASSERT_EQUAL_INT(val2, *(const int *)r2);

    const void *r3 = hashmap_get(map, &key3);
    TEST_ASSERT_NOT_NULL(r3);
    TEST_ASSERT_EQUAL_INT(val3, *(const int *)r3);

    // Verify non-existent key
    int key4 = 1000;
    const void *r4 = hashmap_get(map, &key4);
    TEST_ASSERT_NULL(r4);
}

// Test: hashmap_get returns correct value after deletion
static void test_hashmap_get_after_delete_returns_null(void) {
    // Insert key1 -> val1
    const void *old = hashmap_set(map, &val1);
    TEST_ASSERT_NULL(old);

    // Delete key1
    old = hashmap_delete(map, &key1);
    TEST_ASSERT_NOT_NULL(old);
    TEST_ASSERT_EQUAL_INT(val1, *(const int *)old);

    // Verify get returns NULL
    const void *result = hashmap_get(map, &key1);
    TEST_ASSERT_NULL(result);
}

// Test: hashmap_get works after clear
static void test_hashmap_get_after_clear_returns_null(void) {
    // Insert key1 -> val1
    const void *old = hashmap_set(map, &val1);
    TEST_ASSERT_NULL(old);

    // Clear map
    hashmap_clear(map, false);

    // Verify get returns NULL
    const void *result = hashmap_get(map, &key1);
    TEST_ASSERT_NULL(result);
}

int main(void) {
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_hashmap_get_empty_map_returns_null);
    RUN_TEST(test_hashmap_get_after_insert_returns_value);
    RUN_TEST(test_hashmap_get_nonexistent_key_returns_null);
    RUN_TEST(test_hashmap_get_after_replacement_returns_new_value);
    RUN_TEST(test_hashmap_get_multiple_keys);
    RUN_TEST(test_hashmap_get_after_delete_returns_null);
    RUN_TEST(test_hashmap_get_after_clear_returns_null);
    return UNITY_END();
}