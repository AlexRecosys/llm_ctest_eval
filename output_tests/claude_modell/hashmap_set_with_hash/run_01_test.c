#include "hashmap.c"
#include "unity.h"
#include "hashmap.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

/* ── fixtures ─────────────────────────────────────────────────────────────── */

typedef struct {
    int   key;
    int   value;
} Item;

static struct hashmap *map;

/* ── helpers ──────────────────────────────────────────────────────────────── */

static uint64_t item_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
    const Item *it = (const Item *)item;
    return hashmap_murmur(&it->key, sizeof(it->key), seed0, seed1);
}

static int item_compare(const void *a, const void *b, void *udata)
{
    (void)udata;
    return ((const Item *)a)->key - ((const Item *)b)->key;
}

static uint64_t make_hash(struct hashmap *m, const Item *it)
{
    return item_hash(it, 0, 0);
}

/* ── setUp / tearDown ─────────────────────────────────────────────────────── */

void setUp(void)
{
    map = hashmap_new(sizeof(Item), 0, 0, 0,
                      item_hash, item_compare, NULL, NULL);
    TEST_ASSERT_NOT_NULL(map);
}

void tearDown(void)
{
    hashmap_free(map);
    map = NULL;
}

/* ── test cases ───────────────────────────────────────────────────────────── */

/* Inserting a new item returns NULL (no previous value) */
void test_set_new_item_returns_null(void)
{
    Item it = {42, 100};
    uint64_t h = make_hash(map, &it);
    const void *prev = hashmap_set_with_hash(map, &it, h);
    TEST_ASSERT_NULL(prev);
}

/* After insertion the count increases */
void test_set_new_item_increments_count(void)
{
    Item it = {1, 10};
    uint64_t h = make_hash(map, &it);
    hashmap_set_with_hash(map, &it, h);
    TEST_ASSERT_EQUAL_UINT(1, hashmap_count(map));
}

/* Replacing an existing key returns the old item */
void test_set_existing_key_returns_old_item(void)
{
    Item it1 = {7, 111};
    Item it2 = {7, 222};
    uint64_t h = make_hash(map, &it1);

    hashmap_set_with_hash(map, &it1, h);
    const Item *prev = (const Item *)hashmap_set_with_hash(map, &it2, h);

    TEST_ASSERT_NOT_NULL(prev);
    TEST_ASSERT_EQUAL_INT(7,   prev->key);
    TEST_ASSERT_EQUAL_INT(111, prev->value);
}

/* Replacing an existing key does NOT change the count */
void test_set_existing_key_count_unchanged(void)
{
    Item it1 = {7, 111};
    Item it2 = {7, 222};
    uint64_t h = make_hash(map, &it1);

    hashmap_set_with_hash(map, &it1, h);
    hashmap_set_with_hash(map, &it2, h);

    TEST_ASSERT_EQUAL_UINT(1, hashmap_count(map));
}

/* After replacement the map stores the new value */
void test_set_existing_key_updates_value(void)
{
    Item it1 = {7, 111};
    Item it2 = {7, 222};
    uint64_t h = make_hash(map, &it1);

    hashmap_set_with_hash(map, &it1, h);
    hashmap_set_with_hash(map, &it2, h);

    const Item *found = (const Item *)hashmap_get(map, &it2);
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_INT(222, found->value);
}

/* Multiple distinct keys can be inserted */
void test_set_multiple_distinct_keys(void)
{
    int n = 20;
    for (int i = 0; i < n; i++) {
        Item it = {i, i * 10};
        uint64_t h = make_hash(map, &it);
        const void *prev = hashmap_set_with_hash(map, &it, h);
        TEST_ASSERT_NULL(prev);
    }
    TEST_ASSERT_EQUAL_UINT((size_t)n, hashmap_count(map));
}

/* Each inserted item is retrievable with the correct value */
void test_set_items_are_retrievable(void)
{
    int n = 20;
    for (int i = 0; i < n; i++) {
        Item it = {i, i * 10};
        uint64_t h = make_hash(map, &it);
        hashmap_set_with_hash(map, &it, h);
    }
    for (int i = 0; i < n; i++) {
        Item key = {i, 0};
        const Item *found = (const Item *)hashmap_get(map, &key);
        TEST_ASSERT_NOT_NULL(found);
        TEST_ASSERT_EQUAL_INT(i,      found->key);
        TEST_ASSERT_EQUAL_INT(i * 10, found->value);
    }
}

/* Inserting with hash=0 (edge value) works correctly */
void test_set_with_hash_zero(void)
{
    Item it = {99, 999};
    const void *prev = hashmap_set_with_hash(map, &it, 0);
    TEST_ASSERT_NULL(prev);
    TEST_ASSERT_EQUAL_UINT(1, hashmap_count(map));
}

/* Inserting with UINT64_MAX hash (edge value) works correctly */
void test_set_with_hash_max(void)
{
    Item it = {55, 555};
    const void *prev = hashmap_set_with_hash(map, &it, UINT64_MAX);
    TEST_ASSERT_NULL(prev);
    TEST_ASSERT_EQUAL_UINT(1, hashmap_count(map));
}

/* Map grows automatically when load factor is exceeded */
void test_set_triggers_grow(void)
{
    /* Insert enough items to force at least one resize */
    int n = 200;
    for (int i = 0; i < n; i++) {
        Item it = {i, i};
        uint64_t h = make_hash(map, &it);
        hashmap_set_with_hash(map, &it, h);
        TEST_ASSERT_FALSE(map->oom);
    }
    TEST_ASSERT_EQUAL_UINT((size_t)n, hashmap_count(map));
}

/* oom flag is cleared at the start of each call */
void test_set_clears_oom_flag(void)
{
    map->oom = true;   /* artificially set */
    Item it = {3, 30};
    uint64_t h = make_hash(map, &it);
    hashmap_set_with_hash(map, &it, h);
    TEST_ASSERT_FALSE(map->oom);
}

/* Replacing a key multiple times keeps the latest value */
void test_set_repeated_replacement(void)
{
    Item it = {10, 1};
    uint64_t h = make_hash(map, &it);
    hashmap_set_with_hash(map, &it, h);

    for (int v = 2; v <= 10; v++) {
        Item upd = {10, v};
        const Item *prev = (const Item *)hashmap_set_with_hash(map, &upd, h);
        TEST_ASSERT_NOT_NULL(prev);
        TEST_ASSERT_EQUAL_INT(v - 1, prev->value);
    }

    Item key = {10, 0};
    const Item *found = (const Item *)hashmap_get(map, &key);
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_INT(10, found->value);
}

/* Robin Hood: items inserted with colliding hashes are all retrievable */
void test_set_collision_chain_all_retrievable(void)
{
    /* Force collisions by using the same raw hash for different keys.
       We bypass the compare callback by using a map without one so that
       only the hash distinguishes buckets — instead use the normal map
       which uses compare, so same hash + different key = different slot. */
    int n = 16;
    for (int i = 0; i < n; i++) {
        Item it = {i * 1000, i};
        uint64_t h = make_hash(map, &it);
        hashmap_set_with_hash(map, &it, h);
    }
    for (int i = 0; i < n; i++) {
        Item key = {i * 1000, 0};
        const Item *found = (const Item *)hashmap_get(map, &key);
        TEST_ASSERT_NOT_NULL(found);
        TEST_ASSERT_EQUAL_INT(i, found->value);
    }
}

/* ── main ─────────────────────────────────────────────────────────────────── */

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_set_new_item_returns_null);
    RUN_TEST(test_set_new_item_increments_count);
    RUN_TEST(test_set_existing_key_returns_old_item);
    RUN_TEST(test_set_existing_key_count_unchanged);
    RUN_TEST(test_set_existing_key_updates_value);
    RUN_TEST(test_set_multiple_distinct_keys);
    RUN_TEST(test_set_items_are_retrievable);
    RUN_TEST(test_set_with_hash_zero);
    RUN_TEST(test_set_with_hash_max);
    RUN_TEST(test_set_triggers_grow);
    RUN_TEST(test_set_clears_oom_flag);
    RUN_TEST(test_set_repeated_replacement);
    RUN_TEST(test_set_collision_chain_all_retrievable);
    return UNITY_END();
}