#include "hashmap.c"
#include "unity.h"
#include "hashmap.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* -------------------------------------------------------------------------
 * Fixture types and file-scope variables
 * ---------------------------------------------------------------------- */

typedef struct {
    int   key;
    int   value;
} IntItem;

static struct hashmap *g_map;

/* -------------------------------------------------------------------------
 * Helper functions
 * ---------------------------------------------------------------------- */

static uint64_t int_item_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
    const IntItem *it = (const IntItem *)item;
    return hashmap_sip(&it->key, sizeof(it->key), seed0, seed1);
}

static int int_item_compare(const void *a, const void *b, void *udata)
{
    (void)udata;
    const IntItem *ia = (const IntItem *)a;
    const IntItem *ib = (const IntItem *)b;
    return ia->key - ib->key;
}

static struct hashmap *create_map(void)
{
    return hashmap_new(sizeof(IntItem), 0, 0, 0,
                       int_item_hash, int_item_compare, NULL, NULL);
}

/* -------------------------------------------------------------------------
 * setUp / tearDown
 * ---------------------------------------------------------------------- */

void setUp(void)
{
    g_map = create_map();
    TEST_ASSERT_NOT_NULL(g_map);
}

void tearDown(void)
{
    hashmap_free(g_map);
    g_map = NULL;
}

/* -------------------------------------------------------------------------
 * Test cases
 * ---------------------------------------------------------------------- */

/* Inserting a new item returns NULL (no previous item existed). */
void test_set_new_item_returns_null(void)
{
    IntItem item = {42, 100};
    const void *prev = hashmap_set(g_map, &item);
    TEST_ASSERT_NULL(prev);
}

/* After insertion the item can be retrieved with the correct value. */
void test_set_new_item_can_be_retrieved(void)
{
    IntItem item = {7, 999};
    hashmap_set(g_map, &item);

    IntItem key = {7, 0};
    const IntItem *found = (const IntItem *)hashmap_get(g_map, &key);
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_INT(7,   found->key);
    TEST_ASSERT_EQUAL_INT(999, found->value);
}

/* Replacing an existing item returns a pointer to the OLD item data. */
void test_set_existing_item_returns_old_item(void)
{
    IntItem first  = {10, 1};
    IntItem second = {10, 2};

    hashmap_set(g_map, &first);
    const IntItem *prev = (const IntItem *)hashmap_set(g_map, &second);

    TEST_ASSERT_NOT_NULL(prev);
    TEST_ASSERT_EQUAL_INT(10, prev->key);
    TEST_ASSERT_EQUAL_INT(1,  prev->value);
}

/* After replacement the map stores the NEW value. */
void test_set_existing_item_updates_value(void)
{
    IntItem first  = {10, 1};
    IntItem second = {10, 2};

    hashmap_set(g_map, &first);
    hashmap_set(g_map, &second);

    IntItem key = {10, 0};
    const IntItem *found = (const IntItem *)hashmap_get(g_map, &key);
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_INT(2, found->value);
}

/* Map count increases by one when a new item is inserted. */
void test_set_increments_count(void)
{
    TEST_ASSERT_EQUAL_INT(0, (int)hashmap_count(g_map));

    IntItem item = {1, 10};
    hashmap_set(g_map, &item);
    TEST_ASSERT_EQUAL_INT(1, (int)hashmap_count(g_map));
}

/* Map count does NOT increase when an existing item is replaced. */
void test_set_replace_does_not_increment_count(void)
{
    IntItem first  = {5, 50};
    IntItem second = {5, 55};

    hashmap_set(g_map, &first);
    TEST_ASSERT_EQUAL_INT(1, (int)hashmap_count(g_map));

    hashmap_set(g_map, &second);
    TEST_ASSERT_EQUAL_INT(1, (int)hashmap_count(g_map));
}

/* Multiple distinct items can be inserted and all are retrievable. */
void test_set_multiple_distinct_items(void)
{
    int n = 50;
    for (int i = 0; i < n; i++) {
        IntItem item = {i, i * 2};
        const void *prev = hashmap_set(g_map, &item);
        TEST_ASSERT_NULL(prev);
    }

    TEST_ASSERT_EQUAL_INT(n, (int)hashmap_count(g_map));

    for (int i = 0; i < n; i++) {
        IntItem key = {i, 0};
        const IntItem *found = (const IntItem *)hashmap_get(g_map, &key);
        TEST_ASSERT_NOT_NULL_MESSAGE(found, "item not found after bulk insert");
        TEST_ASSERT_EQUAL_INT(i,     found->key);
        TEST_ASSERT_EQUAL_INT(i * 2, found->value);
    }
}

/* Inserting an item with key 0 works correctly. */
void test_set_zero_key(void)
{
    IntItem item = {0, 77};
    const void *prev = hashmap_set(g_map, &item);
    TEST_ASSERT_NULL(prev);

    IntItem key = {0, 0};
    const IntItem *found = (const IntItem *)hashmap_get(g_map, &key);
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_INT(77, found->value);
}

/* Inserting an item with a negative key works correctly. */
void test_set_negative_key(void)
{
    IntItem item = {-1, 42};
    const void *prev = hashmap_set(g_map, &item);
    TEST_ASSERT_NULL(prev);

    IntItem key = {-1, 0};
    const IntItem *found = (const IntItem *)hashmap_get(g_map, &key);
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_INT(42, found->value);
}

/* Replacing the same key many times keeps count at 1. */
void test_set_repeated_replace_keeps_count_one(void)
{
    for (int i = 0; i < 100; i++) {
        IntItem item = {99, i};
        hashmap_set(g_map, &item);
    }
    TEST_ASSERT_EQUAL_INT(1, (int)hashmap_count(g_map));

    IntItem key = {99, 0};
    const IntItem *found = (const IntItem *)hashmap_get(g_map, &key);
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_INT(99, found->value);
}

/* hashmap_set triggers a resize when many items are inserted; map stays
 * consistent after the resize. */
void test_set_triggers_resize_and_remains_consistent(void)
{
    int n = 200;
    for (int i = 0; i < n; i++) {
        IntItem item = {i, i + 1};
        hashmap_set(g_map, &item);
    }

    TEST_ASSERT_EQUAL_INT(n, (int)hashmap_count(g_map));

    for (int i = 0; i < n; i++) {
        IntItem key = {i, 0};
        const IntItem *found = (const IntItem *)hashmap_get(g_map, &key);
        TEST_ASSERT_NOT_NULL_MESSAGE(found, "item missing after resize");
        TEST_ASSERT_EQUAL_INT(i + 1, found->value);
    }
}

/* -------------------------------------------------------------------------
 * main
 * ---------------------------------------------------------------------- */

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_set_new_item_returns_null);
    RUN_TEST(test_set_new_item_can_be_retrieved);
    RUN_TEST(test_set_existing_item_returns_old_item);
    RUN_TEST(test_set_existing_item_updates_value);
    RUN_TEST(test_set_increments_count);
    RUN_TEST(test_set_replace_does_not_increment_count);
    RUN_TEST(test_set_multiple_distinct_items);
    RUN_TEST(test_set_zero_key);
    RUN_TEST(test_set_negative_key);
    RUN_TEST(test_set_repeated_replace_keeps_count_one);
    RUN_TEST(test_set_triggers_resize_and_remains_consistent);
    return UNITY_END();
}