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

/* Compute the hash the same way the public API would */
static uint64_t compute_hash(struct hashmap *map, int key)
{
    item_t it;
    it.key = key;
    /* hashmap_get_with_hash is not public, so we replicate the hash
     * computation used internally: seed0/seed1 are stored in the map but
     * are not exposed.  We use hashmap_set/get to derive the hash via the
     * public delete helper that accepts a pre-computed hash.
     * Instead, we just call item_hash with seed0=0, seed1=0 and let
     * clip_hash normalise it — the map was created with seed0=seed1=0. */
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
    hashmap_free(g_map);
    g_map = NULL;
}

/* -------------------------------------------------------------------------
 * Test cases
 * ---------------------------------------------------------------------- */

/* Deleting from an empty map must return NULL */
void test_delete_with_hash_empty_map_returns_null(void)
{
    item_t key_item;
    key_item.key = 42;
    uint64_t h = compute_hash(g_map, 42);
    const void *result = hashmap_delete_with_hash(g_map, &key_item, h);
    TEST_ASSERT_NULL(result);
}

/* Deleting a key that was never inserted must return NULL */
void test_delete_with_hash_missing_key_returns_null(void)
{
    insert_item(g_map, 1, "one");
    insert_item(g_map, 2, "two");

    item_t key_item;
    key_item.key = 99;
    uint64_t h = compute_hash(g_map, 99);
    const void *result = hashmap_delete_with_hash(g_map, &key_item, h);
    TEST_ASSERT_NULL(result);
}

/* Deleting an existing key must return a non-NULL pointer */
void test_delete_with_hash_existing_key_returns_non_null(void)
{
    insert_item(g_map, 10, "ten");

    item_t key_item;
    key_item.key = 10;
    uint64_t h = compute_hash(g_map, 10);
    const void *result = hashmap_delete_with_hash(g_map, &key_item, h);
    TEST_ASSERT_NOT_NULL(result);
}

/* The returned pointer must contain the deleted item's data */
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

/* After deletion the count must decrease by one */
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

    TEST_ASSERT_EQUAL_UINT(before - 1, hashmap_count(g_map));
}

/* After deletion the key must no longer be retrievable */
void test_delete_with_hash_key_no_longer_present(void)
{
    insert_item(g_map, 5, "five");

    item_t key_item;
    key_item.key = 5;
    uint64_t h = compute_hash(g_map, 5);
    hashmap_delete_with_hash(g_map, &key_item, h);

    const item_t *found = (const item_t *)hashmap_get(g_map, &key_item);
    TEST_ASSERT_NULL(found);
}

/* Deleting the same key twice: second call must return NULL */
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

/* Remaining keys must still be accessible after an unrelated deletion */
void test_delete_with_hash_other_keys_intact(void)
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

/* oom flag must be cleared on entry */
void test_delete_with_hash_clears_oom_flag(void)
{
    /* Force oom to true by direct field access (map is opaque but we
     * included the .c so the struct layout is visible). */
    g_map->oom = true;

    item_t key_item;
    key_item.key = 999;
    uint64_t h = compute_hash(g_map, 999);
    hashmap_delete_with_hash(g_map, &key_item, h);

    TEST_ASSERT_FALSE(g_map->oom);
}

/* Delete all items one by one; count must reach zero */
void test_delete_with_hash_all_items_count_reaches_zero(void)
{
    int keys[] = {1, 2, 3, 4, 5};
    int n = (int)(sizeof(keys) / sizeof(keys[0]));

    for (int i = 0; i < n; i++) {
        char buf[16];
        snprintf(buf, sizeof(buf), "val%d", keys[i]);
        insert_item(g_map, keys[i], buf);
    }

    for (int i = 0; i < n; i++) {
        item_t key_item;
        key_item.key = keys[i];
        uint64_t h = compute_hash(g_map, keys[i]);
        const void *r = hashmap_delete_with_hash(g_map, &key_item, h);
        TEST_ASSERT_NOT_NULL(r);
    }

    TEST_ASSERT_EQUAL_UINT(0, hashmap_count(g_map));
}

/* A wrong hash (different from the stored one) must not find the item */
void test_delete_with_hash_wrong_hash_returns_null(void)
{
    insert_item(g_map, 42, "fortytwo");

    item_t key_item;
    key_item.key = 42;

    /* Use a deliberately wrong hash value */
    uint64_t wrong_hash = compute_hash(g_map, 42) ^ 0xDEADBEEFDEADBEEFULL;

    const void *result = hashmap_delete_with_hash(g_map, &key_item, wrong_hash);
    /* The item should NOT be found because the hash slot differs */
    TEST_ASSERT_NULL(result);

    /* The item must still be present */
    const item_t *found = (const item_t *)hashmap_get(g_map, &key_item);
    TEST_ASSERT_NOT_NULL(found);
}

/* Insert many items to trigger potential resize, then delete several */
void test_delete_with_hash_after_many_inserts(void)
{
    for (int i = 0; i < 200; i++) {
        char buf[32];
        snprintf(buf, sizeof(buf), "value%d", i);
        insert_item(g_map, i, buf);
    }

    TEST_ASSERT_EQUAL_UINT(200, hashmap_count(g_map));

    for (int i = 0; i < 200; i += 2) {
        item_t key_item;
        key_item.key = i;
        uint64_t h = compute_hash(g_map, i);
        const item_t *r = (const item_t *)hashmap_delete_with_hash(g_map, &key_item, h);
        TEST_ASSERT_NOT_NULL(r);
        TEST_ASSERT_EQUAL_INT(i, r->key);
    }

    TEST_ASSERT_EQUAL_UINT(100, hashmap_count(g_map));

    /* Odd keys must still be present */
    for (int i = 1; i < 200; i += 2) {
        item_t key_item;
        key_item.key = i;
        const item_t *found = (const item_t *)hashmap_get(g_map, &key_item);
        TEST_ASSERT_NOT_NULL_MESSAGE(found, "Odd key missing after deleting even keys");
        TEST_ASSERT_EQUAL_INT(i, found->key);
    }
}

/* Re-inserting a deleted key must work correctly */
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

/* -------------------------------------------------------------------------
 * main
 * ---------------------------------------------------------------------- */
int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_delete_with_hash_empty_map_returns_null);
    RUN_TEST(test_delete_with_hash_missing_key_returns_null);
    RUN_TEST(test_delete_with_hash_existing_key_returns_non_null);
    RUN_TEST(test_delete_with_hash_returns_correct_item);
    RUN_TEST(test_delete_with_hash_decrements_count);
    RUN_TEST(test_delete_with_hash_key_no_longer_present);
    RUN_TEST(test_delete_with_hash_double_delete_returns_null);
    RUN_TEST(test_delete_with_hash_other_keys_intact);
    RUN_TEST(test_delete_with_hash_clears_oom_flag);
    RUN_TEST(test_delete_with_hash_all_items_count_reaches_zero);
    RUN_TEST(test_delete_with_hash_wrong_hash_returns_null);
    RUN_TEST(test_delete_with_hash_after_many_inserts);
    RUN_TEST(test_delete_with_hash_reinsert_after_delete);
    return UNITY_END();
}