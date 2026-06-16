#include "hashmap.c"
#include "unity.h"
#include "hashmap.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

/* ── fixtures ── */

typedef struct {
    int   key;
    int   value;
} IntItem;

static struct hashmap *g_map;

/* ── helpers ── */

static uint64_t int_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
    const IntItem *it = (const IntItem *)item;
    return hashmap_murmur(&it->key, sizeof(it->key), seed0, seed1);
}

static int int_compare(const void *a, const void *b, void *udata)
{
    (void)udata;
    return ((const IntItem *)a)->key - ((const IntItem *)b)->key;
}

static struct hashmap *make_map(size_t cap)
{
    return hashmap_new(sizeof(IntItem), cap, 0, 0,
                       int_hash, int_compare, NULL, NULL);
}

/* ── setUp / tearDown ── */

void setUp(void)
{
    g_map = make_map(0);
    TEST_ASSERT_NOT_NULL(g_map);
}

void tearDown(void)
{
    hashmap_free(g_map);
    g_map = NULL;
}

/* ── test cases ── */

/* inserting a new item returns NULL (no previous value) */
void test_set_new_item_returns_null(void)
{
    IntItem item = {42, 100};
    uint64_t h = int_hash(&item, 0, 0);
    const void *prev = hashmap_set_with_hash(g_map, &item, h);
    TEST_ASSERT_NULL(prev);
}

/* count increases after a new insertion */
void test_set_new_item_increments_count(void)
{
    IntItem item = {1, 10};
    uint64_t h = int_hash(&item, 0, 0);
    hashmap_set_with_hash(g_map, &item, h);
    TEST_ASSERT_EQUAL_UINT(1, hashmap_count(g_map));
}

/* replacing an existing item returns the old value */
void test_set_existing_item_returns_old_value(void)
{
    IntItem item1 = {7, 111};
    IntItem item2 = {7, 222};
    uint64_t h = int_hash(&item1, 0, 0);

    hashmap_set_with_hash(g_map, &item1, h);
    const void *prev = hashmap_set_with_hash(g_map, &item2, h);

    TEST_ASSERT_NOT_NULL(prev);
    TEST_ASSERT_EQUAL_INT(111, ((const IntItem *)prev)->value);
}

/* replacing an existing item does NOT increase count */
void test_set_existing_item_does_not_increment_count(void)
{
    IntItem item1 = {7, 111};
    IntItem item2 = {7, 222};
    uint64_t h = int_hash(&item1, 0, 0);

    hashmap_set_with_hash(g_map, &item1, h);
    hashmap_set_with_hash(g_map, &item2, h);

    TEST_ASSERT_EQUAL_UINT(1, hashmap_count(g_map));
}

/* after replacement the map stores the new value */
void test_set_existing_item_stores_new_value(void)
{
    IntItem item1 = {7, 111};
    IntItem item2 = {7, 222};
    uint64_t h = int_hash(&item1, 0, 0);

    hashmap_set_with_hash(g_map, &item1, h);
    hashmap_set_with_hash(g_map, &item2, h);

    const IntItem *found = (const IntItem *)hashmap_get(g_map, &item2);
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_INT(222, found->value);
}

/* multiple distinct items can be inserted */
void test_set_multiple_distinct_items(void)
{
    for (int i = 0; i < 50; i++) {
        IntItem item = {i, i * 10};
        uint64_t h = int_hash(&item, 0, 0);
        const void *prev = hashmap_set_with_hash(g_map, &item, h);
        TEST_ASSERT_NULL(prev);
    }
    TEST_ASSERT_EQUAL_UINT(50, hashmap_count(g_map));
}

/* all inserted items are retrievable after bulk insert */
void test_set_multiple_items_all_retrievable(void)
{
    for (int i = 0; i < 50; i++) {
        IntItem item = {i, i * 10};
        uint64_t h = int_hash(&item, 0, 0);
        hashmap_set_with_hash(g_map, &item, h);
    }
    for (int i = 0; i < 50; i++) {
        IntItem key = {i, 0};
        const IntItem *found = (const IntItem *)hashmap_get(g_map, &key);
        TEST_ASSERT_NOT_NULL(found);
        TEST_ASSERT_EQUAL_INT(i * 10, found->value);
    }
}

/* map triggers resize when load factor is exceeded */
void test_set_triggers_resize_on_high_load(void)
{
    /* start with a tiny capacity to force resize */
    struct hashmap *m = make_map(4);
    TEST_ASSERT_NOT_NULL(m);

    for (int i = 0; i < 200; i++) {
        IntItem item = {i, i};
        uint64_t h = int_hash(&item, 0, 0);
        hashmap_set_with_hash(m, &item, h);
        TEST_ASSERT_FALSE(m->oom);
    }
    TEST_ASSERT_EQUAL_UINT(200, hashmap_count(m));

    /* verify a sample of items survived the resize */
    for (int i = 0; i < 200; i++) {
        IntItem key = {i, 0};
        const IntItem *found = (const IntItem *)hashmap_get(m, &key);
        TEST_ASSERT_NOT_NULL(found);
        TEST_ASSERT_EQUAL_INT(i, found->value);
    }
    hashmap_free(m);
}

/* oom flag is cleared at the start of each call */
void test_set_clears_oom_flag(void)
{
    g_map->oom = true;
    IntItem item = {99, 99};
    uint64_t h = int_hash(&item, 0, 0);
    hashmap_set_with_hash(g_map, &item, h);
    TEST_ASSERT_FALSE(g_map->oom);
}

/* hash value 0 is handled correctly (clip_hash maps 0 → 1) */
void test_set_with_zero_hash(void)
{
    IntItem item = {55, 55};
    const void *prev = hashmap_set_with_hash(g_map, &item, 0);
    TEST_ASSERT_NULL(prev);
    TEST_ASSERT_EQUAL_UINT(1, hashmap_count(g_map));

    const IntItem *found = (const IntItem *)hashmap_get(g_map, &item);
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_INT(55, found->value);
}

/* inserting the same item twice (same pointer) replaces correctly */
void test_set_same_pointer_twice(void)
{
    IntItem item = {3, 30};
    uint64_t h = int_hash(&item, 0, 0);

    hashmap_set_with_hash(g_map, &item, h);

    item.value = 300;
    const void *prev = hashmap_set_with_hash(g_map, &item, h);

    TEST_ASSERT_NOT_NULL(prev);
    TEST_ASSERT_EQUAL_INT(30, ((const IntItem *)prev)->value);
    TEST_ASSERT_EQUAL_UINT(1, hashmap_count(g_map));
}

/* returned old-value pointer is the spare buffer (not the live bucket) */
void test_set_returned_pointer_is_spare(void)
{
    IntItem item1 = {8, 80};
    IntItem item2 = {8, 800};
    uint64_t h = int_hash(&item1, 0, 0);

    hashmap_set_with_hash(g_map, &item1, h);
    const void *prev = hashmap_set_with_hash(g_map, &item2, h);

    TEST_ASSERT_NOT_NULL(prev);
    /* spare buffer must equal map->spare */
    TEST_ASSERT_EQUAL_PTR(g_map->spare, prev);
}

/* robin-hood displacement: many collisions still produce correct results */
void test_set_robin_hood_displacement(void)
{
    /* Force many items into the same initial bucket by using the same hash
       but different keys (compare will distinguish them). */
    struct hashmap *m = make_map(8);
    TEST_ASSERT_NOT_NULL(m);

    /* Use a fixed hash so all items start at the same slot */
    uint64_t fixed_hash = 1; /* clip_hash(1) == 1 */

    for (int i = 0; i < 20; i++) {
        IntItem item = {i, i * 5};
        hashmap_set_with_hash(m, &item, fixed_hash);
    }
    TEST_ASSERT_EQUAL_UINT(20, hashmap_count(m));

    for (int i = 0; i < 20; i++) {
        IntItem key = {i, 0};
        const IntItem *found = (const IntItem *)hashmap_get(m, &key);
        TEST_ASSERT_NOT_NULL(found);
        TEST_ASSERT_EQUAL_INT(i * 5, found->value);
    }
    hashmap_free(m);
}

/* ── main ── */

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_set_new_item_returns_null);
    RUN_TEST(test_set_new_item_increments_count);
    RUN_TEST(test_set_existing_item_returns_old_value);
    RUN_TEST(test_set_existing_item_does_not_increment_count);
    RUN_TEST(test_set_existing_item_stores_new_value);
    RUN_TEST(test_set_multiple_distinct_items);
    RUN_TEST(test_set_multiple_items_all_retrievable);
    RUN_TEST(test_set_triggers_resize_on_high_load);
    RUN_TEST(test_set_clears_oom_flag);
    RUN_TEST(test_set_with_zero_hash);
    RUN_TEST(test_set_same_pointer_twice);
    RUN_TEST(test_set_returned_pointer_is_spare);
    RUN_TEST(test_set_robin_hood_displacement);
    return UNITY_END();
}