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

/* Inserting a brand-new item returns NULL (no previous value). */
void test_set_new_item_returns_null(void)
{
    Item it = {42, 100};
    uint64_t h = make_hash(map, &it);
    const void *prev = hashmap_set_with_hash(map, &it, h);
    TEST_ASSERT_NULL(prev);
}

/* After insertion the count increases by one. */
void test_set_new_item_increments_count(void)
{
    Item it = {1, 2};
    uint64_t h = make_hash(map, &it);
    hashmap_set_with_hash(map, &it, h);
    TEST_ASSERT_EQUAL_UINT(1, hashmap_count(map));
}

/* Inserting a duplicate key returns a pointer to the OLD item. */
void test_set_duplicate_key_returns_old_item(void)
{
    Item first  = {7, 10};
    Item second = {7, 20};
    uint64_t h = make_hash(map, &first);

    hashmap_set_with_hash(map, &first, h);
    const void *prev = hashmap_set_with_hash(map, &second, h);

    TEST_ASSERT_NOT_NULL(prev);
    TEST_ASSERT_EQUAL_INT(10, ((const Item *)prev)->value);
}

/* Inserting a duplicate key does NOT increase the count. */
void test_set_duplicate_key_count_unchanged(void)
{
    Item first  = {7, 10};
    Item second = {7, 20};
    uint64_t h = make_hash(map, &first);

    hashmap_set_with_hash(map, &first, h);
    hashmap_set_with_hash(map, &second, h);

    TEST_ASSERT_EQUAL_UINT(1, hashmap_count(map));
}

/* After updating a duplicate key the map stores the NEW value. */
void test_set_duplicate_key_updates_value(void)
{
    Item first  = {7, 10};
    Item second = {7, 20};
    uint64_t h = make_hash(map, &first);

    hashmap_set_with_hash(map, &first, h);
    hashmap_set_with_hash(map, &second, h);

    const Item *stored = (const Item *)hashmap_get(map, &second);
    TEST_ASSERT_NOT_NULL(stored);
    TEST_ASSERT_EQUAL_INT(20, stored->value);
}

/* Multiple distinct keys can be inserted; count reflects all of them. */
void test_set_multiple_distinct_keys(void)
{
    int n = 50;
    for (int i = 0; i < n; i++) {
        Item it = {i, i * 2};
        uint64_t h = make_hash(map, &it);
        const void *prev = hashmap_set_with_hash(map, &it, h);
        TEST_ASSERT_NULL(prev);
    }
    TEST_ASSERT_EQUAL_UINT((size_t)n, hashmap_count(map));
}

/* Each inserted item can be retrieved with the correct value. */
void test_set_items_retrievable(void)
{
    int n = 20;
    for (int i = 0; i < n; i++) {
        Item it = {i, i + 100};
        uint64_t h = make_hash(map, &it);
        hashmap_set_with_hash(map, &it, h);
    }
    for (int i = 0; i < n; i++) {
        Item key = {i, 0};
        const Item *found = (const Item *)hashmap_get(map, &key);
        TEST_ASSERT_NOT_NULL(found);
        TEST_ASSERT_EQUAL_INT(i + 100, found->value);
    }
}

/* oom flag is cleared at the start of each call. */
void test_set_clears_oom_flag(void)
{
    /* Force oom to true by direct field access (map is opaque but we included
       the .c so the struct layout is visible). */
    map->oom = true;
    Item it = {99, 1};
    uint64_t h = make_hash(map, &it);
    hashmap_set_with_hash(map, &it, h);
    TEST_ASSERT_FALSE(map->oom);
}

/* Passing a pre-computed hash that differs from the default hash function
   still inserts the item (the map uses the supplied hash). */
void test_set_with_explicit_hash_value(void)
{
    Item it = {55, 77};
    /* Use a deliberately different hash seed. */
    uint64_t h = hashmap_murmur(&it.key, sizeof(it.key), 12345, 67890);
    const void *prev = hashmap_set_with_hash(map, &it, h);
    TEST_ASSERT_NULL(prev);
    TEST_ASSERT_EQUAL_UINT(1, hashmap_count(map));
}

/* Inserting many items triggers a resize; the map remains consistent. */
void test_set_triggers_resize_and_remains_consistent(void)
{
    int n = 200;
    for (int i = 0; i < n; i++) {
        Item it = {i, i};
        uint64_t h = make_hash(map, &it);
        hashmap_set_with_hash(map, &it, h);
        TEST_ASSERT_FALSE(map->oom);
    }
    TEST_ASSERT_EQUAL_UINT((size_t)n, hashmap_count(map));

    for (int i = 0; i < n; i++) {
        Item key = {i, 0};
        const Item *found = (const Item *)hashmap_get(map, &key);
        TEST_ASSERT_NOT_NULL_MESSAGE(found, "item missing after resize");
        TEST_ASSERT_EQUAL_INT(i, found->value);
    }
}

/* Returned pointer from a duplicate insert points to the spare buffer
   (not to the live bucket), so the value is the old one. */
void test_set_returned_pointer_holds_old_value(void)
{
    Item a = {3, 111};
    Item b = {3, 222};
    uint64_t h = make_hash(map, &a);

    hashmap_set_with_hash(map, &a, h);
    const Item *old = (const Item *)hashmap_set_with_hash(map, &b, h);

    TEST_ASSERT_NOT_NULL(old);
    TEST_ASSERT_EQUAL_INT(3,   old->key);
    TEST_ASSERT_EQUAL_INT(111, old->value);
}

/* Inserting the same item twice in a row (identical struct) returns the
   first copy and keeps count at 1. */
void test_set_same_item_twice(void)
{
    Item it = {8, 8};
    uint64_t h = make_hash(map, &it);

    hashmap_set_with_hash(map, &it, h);
    const void *prev = hashmap_set_with_hash(map, &it, h);

    TEST_ASSERT_NOT_NULL(prev);
    TEST_ASSERT_EQUAL_UINT(1, hashmap_count(map));
}

/* hash = 0 is a valid hash (clip_hash maps 0 to 1 internally, but the
   public API must still work). */
void test_set_with_hash_zero(void)
{
    Item it = {0, 0};
    const void *prev = hashmap_set_with_hash(map, &it, 0);
    TEST_ASSERT_NULL(prev);
    TEST_ASSERT_EQUAL_UINT(1, hashmap_count(map));
}

/* ── main ─────────────────────────────────────────────────────────────────── */

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_set_new_item_returns_null);
    RUN_TEST(test_set_new_item_increments_count);
    RUN_TEST(test_set_duplicate_key_returns_old_item);
    RUN_TEST(test_set_duplicate_key_count_unchanged);
    RUN_TEST(test_set_duplicate_key_updates_value);
    RUN_TEST(test_set_multiple_distinct_keys);
    RUN_TEST(test_set_items_retrievable);
    RUN_TEST(test_set_clears_oom_flag);
    RUN_TEST(test_set_with_explicit_hash_value);
    RUN_TEST(test_set_triggers_resize_and_remains_consistent);
    RUN_TEST(test_set_returned_pointer_holds_old_value);
    RUN_TEST(test_set_same_item_twice);
    RUN_TEST(test_set_with_hash_zero);
    return UNITY_END();
}