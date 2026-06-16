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
void test_hashmap_set_new_item_returns_null(void)
{
    IntItem item = {42, 100};
    const void *prev = hashmap_set(g_map, &item);
    TEST_ASSERT_NULL(prev);
}

/* After insertion the item count increases by one. */
void test_hashmap_set_increases_count(void)
{
    IntItem item = {1, 10};
    hashmap_set(g_map, &item);
    TEST_ASSERT_EQUAL_INT(1, (int)hashmap_count(g_map));
}

/* Inserting multiple distinct keys increases count correctly. */
void test_hashmap_set_multiple_distinct_keys(void)
{
    for (int i = 0; i < 10; i++) {
        IntItem item = {i, i * 2};
        const void *prev = hashmap_set(g_map, &item);
        TEST_ASSERT_NULL(prev);
    }
    TEST_ASSERT_EQUAL_INT(10, (int)hashmap_count(g_map));
}

/* Replacing an existing key returns the old item (non-NULL). */
void test_hashmap_set_replace_returns_old_item(void)
{
    IntItem item1 = {7, 111};
    hashmap_set(g_map, &item1);

    IntItem item2 = {7, 222};
    const void *prev = hashmap_set(g_map, &item2);
    TEST_ASSERT_NOT_NULL(prev);
}

/* Replacing an existing key returns the correct old value. */
void test_hashmap_set_replace_returns_correct_old_value(void)
{
    IntItem item1 = {7, 111};
    hashmap_set(g_map, &item1);

    IntItem item2 = {7, 222};
    const IntItem *prev = (const IntItem *)hashmap_set(g_map, &item2);
    TEST_ASSERT_NOT_NULL(prev);
    TEST_ASSERT_EQUAL_INT(7,   prev->key);
    TEST_ASSERT_EQUAL_INT(111, prev->value);
}

/* Replacing an existing key does NOT increase the count. */
void test_hashmap_set_replace_does_not_increase_count(void)
{
    IntItem item1 = {5, 50};
    hashmap_set(g_map, &item1);

    IntItem item2 = {5, 99};
    hashmap_set(g_map, &item2);

    TEST_ASSERT_EQUAL_INT(1, (int)hashmap_count(g_map));
}

/* After replacement, hashmap_get returns the new value. */
void test_hashmap_set_replace_updates_stored_value(void)
{
    IntItem item1 = {3, 30};
    hashmap_set(g_map, &item1);

    IntItem item2 = {3, 99};
    hashmap_set(g_map, &item2);

    IntItem key = {3, 0};
    const IntItem *found = (const IntItem *)hashmap_get(g_map, &key);
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_INT(99, found->value);
}

/* Inserting a single item and then retrieving it works correctly. */
void test_hashmap_set_then_get_returns_item(void)
{
    IntItem item = {100, 200};
    hashmap_set(g_map, &item);

    IntItem key = {100, 0};
    const IntItem *found = (const IntItem *)hashmap_get(g_map, &key);
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_INT(100, found->key);
    TEST_ASSERT_EQUAL_INT(200, found->value);
}

/* Inserting many items (forces resize) and all are retrievable. */
void test_hashmap_set_many_items_all_retrievable(void)
{
    const int N = 200;
    for (int i = 0; i < N; i++) {
        IntItem item = {i, i + 1000};
        hashmap_set(g_map, &item);
    }
    TEST_ASSERT_EQUAL_INT(N, (int)hashmap_count(g_map));

    for (int i = 0; i < N; i++) {
        IntItem key = {i, 0};
        const IntItem *found = (const IntItem *)hashmap_get(g_map, &key);
        TEST_ASSERT_NOT_NULL_MESSAGE(found, "item not found after bulk insert");
        TEST_ASSERT_EQUAL_INT(i + 1000, found->value);
    }
}

/* Inserting key 0 (edge case) works correctly. */
void test_hashmap_set_key_zero(void)
{
    IntItem item = {0, 42};
    const void *prev = hashmap_set(g_map, &item);
    TEST_ASSERT_NULL(prev);

    IntItem key = {0, 0};
    const IntItem *found = (const IntItem *)hashmap_get(g_map, &key);
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_INT(42, found->value);
}

/* Inserting negative keys works correctly. */
void test_hashmap_set_negative_key(void)
{
    IntItem item = {-1, 77};
    const void *prev = hashmap_set(g_map, &item);
    TEST_ASSERT_NULL(prev);

    IntItem key = {-1, 0};
    const IntItem *found = (const IntItem *)hashmap_get(g_map, &key);
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_INT(77, found->value);
}

/* Double-replace: second replacement also returns the intermediate value. */
void test_hashmap_set_double_replace(void)
{
    IntItem item1 = {9, 10};
    IntItem item2 = {9, 20};
    IntItem item3 = {9, 30};

    hashmap_set(g_map, &item1);
    hashmap_set(g_map, &item2);

    const IntItem *prev = (const IntItem *)hashmap_set(g_map, &item3);
    TEST_ASSERT_NOT_NULL(prev);
    TEST_ASSERT_EQUAL_INT(20, prev->value);
    TEST_ASSERT_EQUAL_INT(1, (int)hashmap_count(g_map));
}

/* -------------------------------------------------------------------------
 * main
 * ---------------------------------------------------------------------- */

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_hashmap_set_new_item_returns_null);
    RUN_TEST(test_hashmap_set_increases_count);
    RUN_TEST(test_hashmap_set_multiple_distinct_keys);
    RUN_TEST(test_hashmap_set_replace_returns_old_item);
    RUN_TEST(test_hashmap_set_replace_returns_correct_old_value);
    RUN_TEST(test_hashmap_set_replace_does_not_increase_count);
    RUN_TEST(test_hashmap_set_replace_updates_stored_value);
    RUN_TEST(test_hashmap_set_then_get_returns_item);
    RUN_TEST(test_hashmap_set_many_items_all_retrievable);
    RUN_TEST(test_hashmap_set_key_zero);
    RUN_TEST(test_hashmap_set_negative_key);
    RUN_TEST(test_hashmap_set_double_replace);
    return UNITY_END();
}