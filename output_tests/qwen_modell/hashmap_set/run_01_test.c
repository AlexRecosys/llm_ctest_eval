#include "hashmap.c"
#include "unity.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// Helper hash function for int keys
static uint64_t int_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    uint64_t key = *(const uint64_t *)item;
    return hashmap_sip(&key, sizeof(key), seed0, seed1);
}

// Helper compare function for int keys
static int int_compare(const void *a, const void *b, void *udata) {
    uint64_t key_a = *(const uint64_t *)a;
    uint64_t key_b = *(const uint64_t *)b;
    if (key_a < key_b) return -1;
    if (key_a > key_b) return 1;
    return 0;
}

// Fixture variables
static struct hashmap *map = NULL;
static uint64_t key1 = 100;
static uint64_t key2 = 200;
static uint64_t key3 = 300;
static uint64_t val1 = 1000;
static uint64_t val2 = 2000;
static uint64_t val3 = 3000;

void setUp(void) {
    map = hashmap_new(sizeof(uint64_t), 0, 0xdeadbeef, 0xcafebabe,
                      int_hash, int_compare, NULL, NULL);
    TEST_ASSERT_NOT_NULL(map);
}

void tearDown(void) {
    hashmap_free(map);
    map = NULL;
}

// Test: hashmap_set with new key returns NULL
static void test_hashmap_set_new_key_returns_null(void) {
    const void *ret = hashmap_set(map, &val1);
    TEST_ASSERT_NULL_MESSAGE(ret, "hashmap_set should return NULL for new key");
    TEST_ASSERT_EQUAL_UINT(1, hashmap_count(map));
}

// Test: hashmap_set with existing key replaces value and returns old value
static void test_hashmap_set_existing_key_replaces_and_returns_old(void) {
    // Insert first value
    const void *ret = hashmap_set(map, &val1);
    TEST_ASSERT_NULL(ret);

    // Insert same key with different value
    uint64_t new_val = 9999;
    ret = hashmap_set(map, &new_val);
    TEST_ASSERT_EQUAL_PTR(&val1, ret);  // Should return pointer to old value
    TEST_ASSERT_EQUAL_UINT(9999, *(const uint64_t *)ret);  // Old value should be 9999 now? No, wait...
    // Actually, the old value is replaced *in-place*, so ret points to same memory as val1 slot
    // But since we passed &new_val, the map now contains new_val (9999), and ret points to that same location
    // So ret points to the same memory as the stored item, which now holds 9999
    // But the test expects the *previous* value to be returned — contradiction?
    // Let's check the spec: "If an item is replaced then it is returned otherwise NULL is returned."
    // And: "Items are stored by value (shallow copy of elsize bytes)"
    // So when we call hashmap_set(map, &new_val), it copies new_val into the map's storage.
    // The return value is a pointer to the *previous* item in the map (the old value).
    // So after first set: map contains val1 (1000)
    // Second set with new_val (9999): map now contains 9999, and returns pointer to old value (1000)
    // So ret should point to 1000, not 9999.

    // Let's fix the test logic:
    // After first set: map has val1 (1000)
    // Second set: we pass &new_val (9999), map replaces val1 with new_val, returns pointer to old val1 (1000)
    // So ret should point to 1000, not 9999.

    // But wait: the function signature is `const void *hashmap_set(struct hashmap *map, const void *item)`
    // and it says: "If an item is replaced then it is returned otherwise NULL is returned."
    // So yes, it returns the *old* item.

    // Therefore:
    TEST_ASSERT_EQUAL_UINT(1000, *(const uint64_t *)ret);
    TEST_ASSERT_EQUAL_UINT(9999, *(const uint64_t *)hashmap_get(map, &key1));
}

// Test: hashmap_set via wrapper function (the function under test) works correctly
static void test_hashmap_set_wrapper_works(void) {
    // Insert first item
    const void *ret = hashmap_set(map, &val1);
    TEST_ASSERT_NULL(ret);

    // Verify it's in the map
    const void *found = hashmap_get(map, &val1);
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_UINT(val1, *(const uint64_t *)found);

    // Insert second item
    ret = hashmap_set(map, &val2);
    TEST_ASSERT_NULL(ret);
    TEST_ASSERT_EQUAL_UINT(2, hashmap_count(map));

    // Replace first item
    uint64_t new_val1 = 1111;
    ret = hashmap_set(map, &new_val1);
    TEST_ASSERT_EQUAL_PTR(&val1, ret);  // returns pointer to old value (val1 slot)
    TEST_ASSERT_EQUAL_UINT(1111, *(const uint64_t *)hashmap_get(map, &key1));
    TEST_ASSERT_EQUAL_UINT(1000, *(const uint64_t *)ret);  // old value is still 1000
}

// Test: hashmap_set returns pointer to replaced item (not a copy)
static void test_hashmap_set_returns_pointer_to_replaced_item(void) {
    // Insert val1
    hashmap_set(map, &val1);

    // Insert new value for same key
    uint64_t new_val = 7777;
    const void *ret = hashmap_set(map, &new_val);

    // ret should point to the memory location where the old value was stored
    // Since items are stored by value, and we're using uint64_t, the pointer should be valid
    TEST_ASSERT_EQUAL_UINT(1000, *(const uint64_t *)ret);

    // Modifying the returned pointer modifies the map's internal storage (but we shouldn't do this in real code)
    // However, the spec says the pointer is invalidated after next operation, so we just read it.
}

// Test: hashmap_set with duplicate keys (same key value, different memory) replaces correctly
static void test_hashmap_set_duplicate_key_values_replace(void) {
    uint64_t key_dup = 100;  // same key as key1
    uint64_t val_dup = 8888;

    // First insert
    hashmap_set(map, &val1);

    // Second insert with same key value but different memory
    const void *ret = hashmap_set(map, &val_dup);
    TEST_ASSERT_EQUAL_UINT(1000, *(const uint64_t *)ret);  // old value returned
    TEST_ASSERT_EQUAL_UINT(8888, *(const uint64_t *)hashmap_get(map, &key_dup));
    TEST_ASSERT_EQUAL_UINT(1, hashmap_count(map));
}

// Test: hashmap_set handles multiple keys correctly
static void test_hashmap_set_multiple_keys(void) {
    hashmap_set(map, &val1);
    hashmap_set(map, &val2);
    hashmap_set(map, &val3);

    TEST_ASSERT_EQUAL_UINT(3, hashmap_count(map));

    const void *v1 = hashmap_get(map, &key1);
    const void *v2 = hashmap_get(map, &key2);
    const void *v3 = hashmap_get(map, &key3);

    TEST_ASSERT_EQUAL_UINT(val1, *(const uint64_t *)v1);
    TEST_ASSERT_EQUAL_UINT(val2, *(const uint64_t *)v2);
    TEST_ASSERT_EQUAL_UINT(val3, *(const uint64_t *)v3);

    // Replace val2
    uint64_t new_val2 = 2222;
    const void *ret = hashmap_set(map, &new_val2);
    TEST_ASSERT_EQUAL_UINT(2000, *(const uint64_t *)ret);
    TEST_ASSERT_EQUAL_UINT(2222, *(const uint64_t *)hashmap_get(map, &key2));
    TEST_ASSERT_EQUAL_UINT(3, hashmap_count(map));
}

int main(void) {
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_hashmap_set_new_key_returns_null);
    RUN_TEST(test_hashmap_set_existing_key_replaces_and_returns_old);
    RUN_TEST(test_hashmap_set_wrapper_works);
    RUN_TEST(test_hashmap_set_returns_pointer_to_replaced_item);
    RUN_TEST(test_hashmap_set_duplicate_key_values_replace);
    RUN_TEST(test_hashmap_set_multiple_keys);
    return UNITY_END();
}