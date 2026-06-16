#include "hashmap.c"
#include "unity.h"
#include "hashmap.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

/* -------------------------------------------------------------------------
 * Fixtures / file-scope variables
 * ---------------------------------------------------------------------- */
static struct hashmap *g_map;

/* -------------------------------------------------------------------------
 * Helper types and functions
 * ---------------------------------------------------------------------- */
typedef struct {
    int   key;
    char  value[32];
} item_t;

static uint64_t item_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
    const item_t *it = (const item_t *)item;
    return hashmap_murmur(&it->key, sizeof(it->key), seed0, seed1);
}

static int item_compare(const void *a, const void *b, void *udata)
{
    (void)udata;
    const item_t *ia = (const item_t *)a;
    const item_t *ib = (const item_t *)b;
    return ia->key - ib->key;
}

static struct hashmap *create_map(void)
{
    return hashmap_new(sizeof(item_t), 0, 0, 0,
                       item_hash, item_compare, NULL, NULL);
}

static void insert_item(struct hashmap *map, int key, const char *value)
{
    item_t it;
    it.key = key;
    strncpy(it.value, value, sizeof(it.value) - 1);
    it.value[sizeof(it.value) - 1] = '\0';
    hashmap_set(map, &it);
}

static uint64_t compute_hash(struct hashmap *map, int key)
{
    item_t it;
    it.key = key;
    /* Use the same seeds the map was created with (0, 0) */
    return item_hash(&it, 0, 0);
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
    if (g_map) {
        hashmap_free(g_map);
        g_map = NULL;
    }
}

/* -------------------------------------------------------------------------
 * Test cases
 * ---------------------------------------------------------------------- */

/* Deleting from an empty map returns NULL */
void test_delete_with_hash_empty_map_returns_null(void)
{
    item_t key_item;
    key_item.key = 42;
    uint64_t h = compute_hash(g_map, 42);
    const void *result = hashmap_delete_with_hash(g_map, &key_item, h);
    TEST_ASSERT_NULL(result);
}

/* Deleting a key that was never inserted returns NULL */
void test_delete_with_hash_nonexistent_key_returns_null(void)
{
    insert_item(g_map, 1, "one");
    insert_item(g_map, 2, "two");

    item_t key_item;
    key_item.key = 99;
    uint64_t h = compute_hash(g_map, 99);
    const void *result = hashmap_delete_with_hash(g_map, &key_item, h);
    TEST_ASSERT_NULL(result);
}

/* Deleting an existing key returns a non-NULL pointer */
void test_delete_with_hash_existing_key_returns_nonnull(void)
{
    insert_item(g_map, 10, "ten");
    item_t key_item;
    key_item.key = 10;
    uint64_t h = compute_hash(g_map, 10);
    const void *result = hashmap_delete_with_hash(g_map, &key_item, h);
    TEST_ASSERT_NOT_NULL(result);
}

/* The returned pointer contains the deleted item's data */
void test_delete_with_hash_returns_correct_item(void)
{
    insert_item(g_map, 7, "seven");
    item_t key_item;
    key_item.key = 7;
    uint64_t h = compute_hash(g_map, 7);
    const item_t *result = (const item_t *)hashmap_delete_with_hash(g_map, &key_item, h);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(7, result->key);
    TEST_ASSERT_EQUAL_STRING("seven", result->value);
}

/* After deletion the count decreases by one */
void test_delete_with_hash_decrements_count(void)
{
    insert_item(g_map, 1, "one");
    insert_item(g_map, 2, "two");
    insert_item(g_map, 3, "three");

    size_t before = hashmap_count(g_map);
    item_t key_item;
    key_item.key = 2;
    uint64_t h = compute_hash(g_map, 2);
    hashmap_delete_with_hash(g_map, &key_item, h);
    size_t after = hashmap_count(g_map);
    TEST_ASSERT_EQUAL_UINT(before - 1, after);
}

/* After deletion the key can no longer be found */
void test_delete_with_hash_key_no_longer_found(void)
{
    insert_item(g_map, 5, "five");
    item_t key_item;
    key_item.key = 5;
    uint64_t h = compute_hash(g_map, 5);
    hashmap_delete_with_hash(g_map, &key_item, h);

    const item_t *found = (const item_t *)hashmap_get(g_map, &key_item);
    TEST_ASSERT_NULL(found);
}

/* Deleting the same key twice: second call returns NULL */
void test_delete_with_hash_double_delete_returns_null(void)
{
    insert_item(g_map, 3, "three");
    item_t key_item;
    key_item.key = 3;
    uint64_t h = compute_hash(g_map, 3);

    const void *first  = hashmap_delete_with_hash(g_map, &key_item, h);
    TEST_ASSERT_NOT_NULL(first);

    const void *second = hashmap_delete_with_hash(g_map, &key_item, h);
    TEST_ASSERT_NULL(second);
}

/* Remaining items are still accessible after a deletion */
void test_delete_with_hash_other_items_intact(void)
{
    insert_item(g_map, 10, "ten");
    insert_item(g_map, 20, "twenty");
    insert_item(g_map, 30, "thirty");

    item_t key_item;
    key_item.key = 20;
    uint64_t h = compute_hash(g_map, 20);
    hashmap_delete_with_hash(g_map, &key_item, h);

    item_t k10; k10.key = 10;
    item_t k30; k30.key = 30;
    const item_t *r10 = (const item_t *)hashmap_get(g_map, &k10);
    const item_t *r30 = (const item_t *)hashmap_get(g_map, &k30);

    TEST_ASSERT_NOT_NULL(r10);
    TEST_ASSERT_EQUAL_INT(10, r10->key);
    TEST_ASSERT_EQUAL_STRING("ten", r10->value);

    TEST_ASSERT_NOT_NULL(r30);
    TEST_ASSERT_EQUAL_INT(30, r30->key);
    TEST_ASSERT_EQUAL_STRING("thirty", r30->value);
}

/* oom flag is cleared on entry */
void test_delete_with_hash_clears_oom_flag(void)
{
    /* Force oom to true by attempting a set that triggers OOM is hard,
     * so we directly set the field and verify it is cleared after the call. */
    g_map->oom = true;

    item_t key_item;
    key_item.key = 999;
    uint64_t h = compute_hash(g_map, 999);
    hashmap_delete_with_hash(g_map, &key_item, h);

    TEST_ASSERT_FALSE(g_map->oom);
}

/* Delete with a manually supplied hash that matches the real hash */
void test_delete_with_hash_manual_hash_matches(void)
{
    insert_item(g_map, 42, "fortytwo");

    item_t key_item;
    key_item.key = 42;
    /* Compute hash the same way the map does internally */
    uint64_t h = item_hash(&key_item, 0, 0);

    const item_t *result = (const item_t *)hashmap_delete_with_hash(g_map, &key_item, h);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(42, result->key);
    TEST_ASSERT_EQUAL_STRING("fortytwo", result->value);
}

/* Delete all items one by one; count reaches zero */
void test_delete_with_hash_delete_all_items(void)
{
    int keys[] = {1, 2, 3, 4, 5};
    const char *vals[] = {"one", "two", "three", "four", "five"};
    int n = (int)(sizeof(keys) / sizeof(keys[0]));

    for (int i = 0; i < n; i++) {
        insert_item(g_map, keys[i], vals[i]);
    }

    for (int i = 0; i < n; i++) {
        item_t key_item;
        key_item.key = keys[i];
        uint64_t h = compute_hash(g_map, keys[i]);
        const void *result = hashmap_delete_with_hash(g_map, &key_item, h);
        TEST_ASSERT_NOT_NULL(result);
    }

    TEST_ASSERT_EQUAL_UINT(0, hashmap_count(g_map));
}

/* Insert after delete works correctly */
void test_delete_with_hash_reinsert_after_delete(void)
{
    insert_item(g_map, 55, "original");

    item_t key_item;
    key_item.key = 55;
    uint64_t h = compute_hash(g_map, 55);
    hashmap_delete_with_hash(g_map, &key_item, h);

    insert_item(g_map, 55, "reinserted");

    const item_t *found = (const item_t *)hashmap_get(g_map, &key_item);
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_INT(55, found->key);
    TEST_ASSERT_EQUAL_STRING("reinserted", found->value);
}

/* Large number of insertions and deletions to exercise Robin Hood
 * back-shift and potential shrink path */
void test_delete_with_hash_many_items(void)
{
    int n = 200;
    for (int i = 0; i < n; i++) {
        insert_item(g_map, i, "val");
    }

    /* Delete every other item */
    for (int i = 0; i < n; i += 2) {
        item_t key_item;
        key_item.key = i;
        uint64_t h = compute_hash(g_map, i);
        const void *result = hashmap_delete_with_hash(g_map, &key_item, h);
        TEST_ASSERT_NOT_NULL(result);
    }

    /* Verify odd keys still present */
    for (int i = 1; i < n; i += 2) {
        item_t key_item;
        key_item.key = i;
        const item_t *found = (const item_t *)hashmap_get(g_map, &key_item);
        TEST_ASSERT_NOT_NULL_MESSAGE(found, "Odd key should still be present");
        TEST_ASSERT_EQUAL_INT(i, found->key);
    }

    /* Verify even keys are gone */
    for (int i = 0; i < n; i += 2) {
        item_t key_item;
        key_item.key = i;
        const item_t *found = (const item_t *)hashmap_get(g_map, &key_item);
        TEST_ASSERT_NULL_MESSAGE(found, "Even key should have been deleted");
    }
}

/* Clip-hash edge case: pass UINT64_MAX as hash; function must not crash */
void test_delete_with_hash_max_hash_value(void)
{
    insert_item(g_map, 1, "one");

    item_t key_item;
    key_item.key = 1;
    /* Pass UINT64_MAX; clip_hash will mask it; key won't be found at that
     * bucket so NULL is expected (unless it happens to collide, which is
     * acceptable — we just check no crash). */
    const void *result = hashmap_delete_with_hash(g_map, &key_item, UINT64_MAX);
    /* Result is either NULL (not found at that hash) or the item (collision).
     * Either way the map must remain consistent. */
    size_t count = hashmap_count(g_map);
    TEST_ASSERT_TRUE(count == 0 || count == 1);
}

/* -------------------------------------------------------------------------
 * main
 * ---------------------------------------------------------------------- */
int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_delete_with_hash_empty_map_returns_null);
    RUN_TEST(test_delete_with_hash_nonexistent_key_returns_null);
    RUN_TEST(test_delete_with_hash_existing_key_returns_nonnull);
    RUN_TEST(test_delete_with_hash_returns_correct_item);
    RUN_TEST(test_delete_with_hash_decrements_count);
    RUN_TEST(test_delete_with_hash_key_no_longer_found);
    RUN_TEST(test_delete_with_hash_double_delete_returns_null);
    RUN_TEST(test_delete_with_hash_other_items_intact);
    RUN_TEST(test_delete_with_hash_clears_oom_flag);
    RUN_TEST(test_delete_with_hash_manual_hash_matches);
    RUN_TEST(test_delete_with_hash_delete_all_items);
    RUN_TEST(test_delete_with_hash_reinsert_after_delete);
    RUN_TEST(test_delete_with_hash_many_items);
    RUN_TEST(test_delete_with_hash_max_hash_value);
    return UNITY_END();
}