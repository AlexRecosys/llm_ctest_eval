#include "hashmap.c"
#include "unity.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

static struct hashmap *map = NULL;
static uint64_t (*test_hash)(const void *, uint64_t, uint64_t) = NULL;
static int (*test_compare)(const void *, const void *, void *) = NULL;

static uint64_t simple_hash(const void *data, uint64_t seed0, uint64_t seed1) {
    (void)seed0;
    (void)seed1;
    uint64_t h = 0;
    const uint8_t *p = (const uint8_t *)data;
    for (size_t i = 0; i < 8; i++) {
        h = h * 31 + p[i];
    }
    return h;
}

static int simple_compare(const void *a, const void *b, void *udata) {
    (void)udata;
    return memcmp(a, b, 8);
}

void setUp(void) {
    map = hashmap_new(8, 0, 0, 0, simple_hash, simple_compare, NULL, NULL);
    TEST_ASSERT_NOT_NULL(map);
}

void tearDown(void) {
    if (map) {
        hashmap_free(map);
        map = NULL;
    }
}

static void populate_map_with_items(int count) {
    for (int i = 0; i < count; i++) {
        uint64_t key = (uint64_t)i;
        const void *ret = hashmap_set(map, &key);
        TEST_ASSERT_NULL(ret);
    }
}

static void test_hashmap_probe_empty_bucket(void) {
    // First populate the map to ensure buckets exist
    populate_map_with_items(10);

    // Probe a position that maps to an empty bucket
    // We need to find a position where the bucket is empty
    // Since we inserted 10 items, and the map grows to power-of-two buckets,
    // the mask will be at least 15 (16 buckets), so position 100 should map to bucket 4 (100 & 15 = 4)
    // But we don't know which buckets are occupied. Instead, we'll use a position that's guaranteed to be empty:
    // After clearing, all buckets are empty.

    // Clear the map to ensure all buckets are empty
    hashmap_clear(map, false);

    // Probe any position — should return NULL
    const void *result = hashmap_probe(map, 0);
    TEST_ASSERT_NULL(result);

    result = hashmap_probe(map, 123456789ULL);
    TEST_ASSERT_NULL(result);
}

static void test_hashmap_probe_with_item(void) {
    // Insert a known item
    uint64_t key = 42;
    const void *ret = hashmap_set(map, &key);
    TEST_ASSERT_NULL(ret);

    // Compute the bucket index for position 42 (same as key)
    // But note: hashmap_probe uses the position directly modulo mask, not the hash
    // So we need to probe at a position that maps to the bucket where key 42 is stored.

    // Instead, let's probe at position = hash(42) & mask, but we don't have direct access to mask.
    // Alternative: use hashmap_iter to find a non-empty bucket and probe there.

    size_t i = 0;
    void *item = NULL;
    bool found = false;
    while (hashmap_iter(map, &i, &item)) {
        found = true;
        break;
    }
    TEST_ASSERT_TRUE(found);

    // Now we know there's at least one item. Probe at a position that maps to its bucket.
    // But we don't know the hash of *item, so we can't compute the position directly.

    // Better approach: use the fact that hashmap_probe(position) uses position & mask.
    // We can probe at position = 0, 1, 2, ... until we find a non-NULL bucket, but that's fragile.

    // Instead, let's insert a known item and then probe at a position that we know maps to its bucket.
    // Since we control the hash function, and we know the key, we can compute the hash.

    // Actually, the simplest reliable test: insert one item, then clear and reinsert at a known position.
    // But hashmap_probe doesn't let us control the position directly — it's based on the hash.

    // Revised plan: use the internal structure to our advantage.
    // We'll insert one item, then probe at position = hash(key) & mask.
    // But we don't have access to mask or hash function internals.

    // Alternative: use the fact that if we insert N items, at least one bucket is non-empty.
    // Then probe at positions 0, 1, 2, ... until we find a non-NULL bucket.
    // But that's not deterministic.

    // Let's use a different strategy: insert an item, then use hashmap_get to get its address,
    // and verify that hashmap_probe returns the same address when probed at the correct position.
    // But we don't know the correct position.

    // Actually, the test should be: if a bucket has an item, hashmap_probe should return it.
    // So let's insert an item, then probe at a position that we know will hit that bucket.

    // Since we can't control the hash, let's use the following:
    // Insert one item, then probe at position 0. If it's not there, try position 1, etc.
    // But that's not a good test.

    // Better: use the internal mask. We can access map->mask if we include the .c file,
    // but the .c file defines struct bucket and struct hashmap internally.

    // Wait — the test file includes "hashmap.c", so it has access to the full struct definitions.
    // Let's check the actual struct in hashmap.c to see if mask is exposed.

    // From the header, struct hashmap is opaque, but in hashmap.c it's defined.
    // Since we include hashmap.c, we can access map->mask.

    // Let's assume the struct has a mask field (it does, for Robin Hood hashing).

    // So:
    uint64_t key2 = 100;
    hashmap_set(map, &key2);

    // Compute the bucket index for key2
    uint64_t hash_val = simple_hash(&key2, 0, 0);
    size_t idx = hash_val & map->mask;
    struct bucket *bucket = bucket_at(map, idx);

    // Now probe at position = hash_val (since position & mask = idx)
    const void *result = hashmap_probe(map, hash_val);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_PTR(&key2, result);
}

static void test_hashmap_probe_after_clear(void) {
    populate_map_with_items(5);
    hashmap_clear(map, false);

    const void *result = hashmap_probe(map, 0);
    TEST_ASSERT_NULL(result);
}

static void test_hashmap_probe_with_collision(void) {
    // Insert two items that hash to the same bucket (or close)
    // We can force a collision by using keys that hash to the same value.
    // But our hash function is simple and doesn't have obvious collisions.

    // Instead, insert many items to force collisions.
    populate_map_with_items(100);

    // Now probe at a position that we know has an item.
    // Find a non-empty bucket by scanning.
    size_t i = 0;
    void *item = NULL;
    bool found = false;
    while (hashmap_iter(map, &i, &item)) {
        found = true;
        break;
    }
    TEST_ASSERT_TRUE(found);

    // We still don't know the hash of *item, so we can't compute the position.
    // But we can use the fact that if we insert an item, then delete it, then reinsert,
    // the position should be the same (assuming no resize).

    // Actually, let's just test that a non-empty bucket returns the item.
    // We'll use the bucket index directly.

    // Since we have access to map->buckets and map->mask, we can scan buckets.
    size_t bucket_count = map->mask + 1;
    for (size_t j = 0; j < bucket_count; j++) {
        struct bucket *b = bucket_at(map, j);
        if (b->dib > 0) {
            // This bucket has an item
            // The position that maps to this bucket is any p such that p & mask = j
            uint64_t pos = j;  // since j <= mask (as mask = 2^k - 1), j & mask = j
            const void *result = hashmap_probe(map, pos);
            TEST_ASSERT_NOT_NULL(result);
            // Verify it's the same item as in the bucket
            TEST_ASSERT_EQUAL_MEMORY(bucket_item(b), result, map->elsize);
            return;  // Test passed for one non-empty bucket
        }
    }

    // If we get here, no buckets were non-empty — but we inserted 100 items, so this shouldn't happen.
    TEST_FAIL_MESSAGE("No non-empty buckets found after inserting 100 items");
}

int main(void) {
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_hashmap_probe_empty_bucket);
    RUN_TEST(test_hashmap_probe_with_item);
    RUN_TEST(test_hashmap_probe_after_clear);
    RUN_TEST(test_hashmap_probe_with_collision);
    return UNITY_END();
}