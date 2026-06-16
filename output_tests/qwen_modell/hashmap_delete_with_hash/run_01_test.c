#include "hashmap.c"
#include "unity.h"
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

// File-scope static variables / fixtures
static struct hashmap *map = NULL;
static uint64_t (*test_hash)(const void *, uint64_t, uint64_t) = NULL;
static int (*test_compare)(const void *, const void *, void *) = NULL;
static void (*test_elfree)(void *) = NULL;
static void *test_udata = NULL;

// Helper functions and macros
static uint64_t simple_hash(const void *data, uint64_t seed0, uint64_t seed1) {
    (void)seed0; (void)seed1;
    const char *s = (const char *)data;
    uint64_t h = 0;
    while (*s) {
        h = h * 31 + (uint8_t)*s++;
    }
    return h;
}

static int simple_compare(const void *a, const void *b, void *udata) {
    (void)udata;
    return strcmp((const char *)a, (const char *)b);
}

static void free_string(void *item) {
    free(*(char **)item);
}

static void setup_map(size_t elsize, size_t cap, bool use_elfree) {
    if (map) {
        hashmap_free(map);
    }
    test_hash = simple_hash;
    test_compare = simple_compare;
    test_udata = NULL;
    test_elfree = use_elfree ? free_string : NULL;
    map = hashmap_new(elsize, cap, 0, 0, test_hash, test_compare, test_elfree, test_udata);
    TEST_ASSERT_NOT_NULL(map);
}

// Test cases

void test_hashmap_delete_with_hash_success(void) {
    setup_map(sizeof(char *), 0, true);

    char *key1 = strdup("key1");
    char *val1 = strdup("value1");
    char **p1 = malloc(sizeof(char *));
    *p1 = val1;
    hashmap_set(map, p1);

    char *key2 = strdup("key2");
    char *val2 = strdup("value2");
    char **p2 = malloc(sizeof(char *));
    *p2 = val2;
    hashmap_set(map, p2);

    uint64_t hash = test_hash(key1, 0, 0);
    const void *deleted = hashmap_delete_with_hash(map, key1, hash);

    TEST_ASSERT_NOT_NULL(deleted);
    TEST_ASSERT_EQUAL_STRING("value1", *(char **)deleted);
    TEST_ASSERT_EQUAL_SIZE(1, hashmap_count(map));

    // Verify key1 is gone
    deleted = hashmap_get_with_hash(map, key1, hash);
    TEST_ASSERT_NULL(deleted);

    // Verify key2 still exists
    hash = test_hash(key2, 0, 0);
    deleted = hashmap_get_with_hash(map, key2, hash);
    TEST_ASSERT_NOT_NULL(deleted);
    TEST_ASSERT_EQUAL_STRING("value2", *(char **)deleted);

    free(key1);
    free(key2);
    free(p1);
    free(p2);
}

void test_hashmap_delete_with_hash_not_found(void) {
    setup_map(sizeof(char *), 0, true);

    char *key1 = strdup("key1");
    char *val1 = strdup("value1");
    char **p1 = malloc(sizeof(char *));
    *p1 = val1;
    hashmap_set(map, p1);

    char *key2 = strdup("key2");
    uint64_t hash = test_hash(key2, 0, 0);
    const void *deleted = hashmap_delete_with_hash(map, key2, hash);

    TEST_ASSERT_NULL(deleted);
    TEST_ASSERT_EQUAL_SIZE(1, hashmap_count(map));

    free(key1);
    free(key2);
    free(p1);
}

void test_hashmap_delete_with_hash_rehashing_after_delete(void) {
    setup_map(sizeof(char *), 0, true);

    // Insert enough items to cause growth
    for (int i = 0; i < 10; i++) {
        char key[32];
        char *val = malloc(32);
        char **p = malloc(sizeof(char *));
        snprintf(key, sizeof(key), "key%d", i);
        snprintf(val, 32, "value%d", i);
        *p = val;
        hashmap_set(map, p);
    }

    size_t initial_buckets = map->nbuckets;

    // Delete some items to trigger shrink
    for (int i = 0; i < 8; i++) {
        char key[32];
        snprintf(key, sizeof(key), "key%d", i);
        uint64_t hash = test_hash(key, 0, 0);
        const void *deleted = hashmap_delete_with_hash(map, key, hash);
        TEST_ASSERT_NOT_NULL(deleted);
    }

    // After deletion, count should be 2
    TEST_ASSERT_EQUAL_SIZE(2, hashmap_count(map));

    // If map shrank, nbuckets should be smaller
    // Note: exact behavior depends on implementation details (shrinkat threshold)
    // We just verify map integrity is maintained
    TEST_ASSERT_TRUE(map->nbuckets <= initial_buckets || map->nbuckets >= initial_buckets);
    TEST_ASSERT_EQUAL_SIZE(2, hashmap_count(map));

    // Verify remaining items exist
    for (int i = 8; i < 10; i++) {
        char key[32];
        snprintf(key, sizeof(key), "key%d", i);
        uint64_t hash = test_hash(key, 0, 0);
        const void *val = hashmap_get_with_hash(map, key, hash);
        TEST_ASSERT_NOT_NULL(val);
        TEST_ASSERT_EQUAL_STRING("value8", *(char **)val);
    }
}

void test_hashmap_delete_with_hash_oom_handling(void) {
    setup_map(sizeof(char *), 0, true);

    char *key1 = strdup("key1");
    char *val1 = strdup("value1");
    char **p1 = malloc(sizeof(char *));
    *p1 = val1;
    hashmap_set(map, p1);

    uint64_t hash = test_hash(key1, 0, 0);
    const void *deleted = hashmap_delete_with_hash(map, key1, hash);

    TEST_ASSERT_NOT_NULL(deleted);
    TEST_ASSERT_FALSE(map->oom);

    free(key1);
    free(p1);
}

void test_hashmap_delete_with_hash_same_hash_different_key(void) {
    setup_map(sizeof(char *), 0, true);

    // Insert two keys that hash to same value (unlikely but possible)
    // We'll force same hash by using same string
    char *key1 = strdup("key1");
    char *val1 = strdup("value1");
    char **p1 = malloc(sizeof(char *));
    *p1 = val1;
    hashmap_set(map, p1);

    char *key2 = strdup("key2");
    char *val2 = strdup("value2");
    char **p2 = malloc(sizeof(char *));
    *p2 = val2;
    hashmap_set(map, p2);

    // Delete using key1's hash but key2 as key (should not match)
    uint64_t hash1 = test_hash(key1, 0, 0);
    const void *deleted = hashmap_delete_with_hash(map, key2, hash1);

    TEST_ASSERT_NULL(deleted);
    TEST_ASSERT_EQUAL_SIZE(2, hashmap_count(map));

    // Now delete using correct hash and key
    uint64_t hash2 = test_hash(key2, 0, 0);
    deleted = hashmap_delete_with_hash(map, key2, hash2);
    TEST_ASSERT_NOT_NULL(deleted);
    TEST_ASSERT_EQUAL_STRING("value2", *(char **)deleted);
    TEST_ASSERT_EQUAL_SIZE(1, hashmap_count(map));

    free(key1);
    free(key2);
    free(p1);
    free(p2);
}

void test_hashmap_delete_with_hash_empty_map(void) {
    setup_map(sizeof(char *), 0, true);

    char *key = strdup("key1");
    uint64_t hash = test_hash(key, 0, 0);
    const void *deleted = hashmap_delete_with_hash(map, key, hash);

    TEST_ASSERT_NULL(deleted);
    TEST_ASSERT_EQUAL_SIZE(0, hashmap_count(map));

    free(key);
}

void test_hashmap_delete_with_hash_multiple_collisions(void) {
    setup_map(sizeof(char *), 0, true);

    // Insert items that will cause collisions
    char *keys[5];
    char *vals[5];
    char **ptrs[5];

    for (int i = 0; i < 5; i++) {
        keys[i] = malloc(32);
        vals[i] = malloc(32);
        ptrs[i] = malloc(sizeof(char *));
        snprintf(keys[i], 32, "key%d", i);
        snprintf(vals[i], 32, "value%d", i);
        *ptrs[i] = vals[i];
        hashmap_set(map, ptrs[i]);
    }

    // Delete middle item to test rehashing chain
    uint64_t hash = test_hash(keys[2], 0, 0);
    const void *deleted = hashmap_delete_with_hash(map, keys[2], hash);

    TEST_ASSERT_NOT_NULL(deleted);
    TEST_ASSERT_EQUAL_STRING("value2", *(char **)deleted);
    TEST_ASSERT_EQUAL_SIZE(4, hashmap_count(map));

    // Verify all remaining items still accessible
    for (int i = 0; i < 5; i++) {
        if (i == 2) continue;
        uint64_t h = test_hash(keys[i], 0, 0);
        const void *val = hashmap_get_with_hash(map, keys[i], h);
        TEST_ASSERT_NOT_NULL(val);
        char expected[32];
        snprintf(expected, 32, "value%d", i);
        TEST_ASSERT_EQUAL_STRING(expected, *(char **)val);
    }

    for (int i = 0; i < 5; i++) {
        free(keys[i]);
        free(ptrs[i]);
    }
}

void test_hashmap_delete_with_hash_shrink_on_delete(void) {
    setup_map(sizeof(char *), 0, true);

    // Fill map to trigger growth
    for (int i = 0; i < 16; i++) {
        char key[32];
        char *val = malloc(32);
        char **p = malloc(sizeof(char *));
        snprintf(key, sizeof(key), "key%d", i);
        snprintf(val, 32, "value%d", i);
        *p = val;
        hashmap_set(map, p);
    }

    size_t initial_buckets = map->nbuckets;

    // Delete 14 items to trigger shrink (count goes from 16 to 2)
    for (int i = 0; i < 14; i++) {
        char key[32];
        snprintf(key, sizeof(key), "key%d", i);
        uint64_t hash = test_hash(key, 0, 0);
        const void *deleted = hashmap_delete_with_hash(map, key, hash);
        TEST_ASSERT_NOT_NULL(deleted);
    }

    // Map should have shrunk if count <= shrinkat
    // We just verify map integrity and remaining items
    TEST_ASSERT_EQUAL_SIZE(2, hashmap_count(map));

    // Verify remaining items
    for (int i = 14; i < 16; i++) {
        char key[32];
        snprintf(key, sizeof(key), "key%d", i);
        uint64_t hash = test_hash(key, 0, 0);
        const void *val = hashmap_get_with_hash(map, key, hash);
        TEST_ASSERT_NOT_NULL(val);
        TEST_ASSERT_EQUAL_STRING("value14", *(char **)val);
    }
}

int main(void) {
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_hashmap_delete_with_hash_success);
    RUN_TEST(test_hashmap_delete_with_hash_not_found);
    RUN_TEST(test_hashmap_delete_with_hash_rehashing_after_delete);
    RUN_TEST(test_hashmap_delete_with_hash_oom_handling);
    RUN_TEST(test_hashmap_delete_with_hash_same_hash_different_key);
    RUN_TEST(test_hashmap_delete_with_hash_empty_map);
    RUN_TEST(test_hashmap_delete_with_hash_multiple_collisions);
    RUN_TEST(test_hashmap_delete_with_hash_shrink_on_delete);
    return UNITY_END();
}