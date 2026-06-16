#include "hashmap.c"
#include "unity.h"
#include "hashmap.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

/* -------------------------------------------------------------------------
 * File-scope fixtures
 * ---------------------------------------------------------------------- */
static struct hashmap *g_map = NULL;

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

static void destroy_map(struct hashmap *map)
{
    if (map) hashmap_free(map);
}

/* Compute the hash the same way the library does so we can call
 * hashmap_get_with_hash() with a matching hash value.                     */
static uint64_t compute_hash(struct hashmap *map, const item_t *it)
{
    return item_hash(it, map->seed0, map->seed1);
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
    destroy_map(g_map);
    g_map = NULL;
}

/* -------------------------------------------------------------------------
 * Test cases
 * ---------------------------------------------------------------------- */

/* 1. Get an item that was inserted — should return a non-NULL pointer whose
 *    key matches the inserted item.                                        */
void test_get_with_hash_existing_item(void)
{
    item_t inserted = {42, "hello"};
    hashmap_set(g_map, &inserted);

    item_t key = {42, ""};
    uint64_t hash = compute_hash(g_map, &key);

    const void *result = hashmap_get_with_hash(g_map, &key, hash);
    TEST_ASSERT_NOT_NULL(result);

    const item_t *found = (const item_t *)result;
    TEST_ASSERT_EQUAL_INT(42, found->key);
    TEST_ASSERT_EQUAL_STRING("hello", found->value);
}

/* 2. Get an item that was never inserted — should return NULL.             */
void test_get_with_hash_missing_item(void)
{
    item_t key = {99, ""};
    uint64_t hash = compute_hash(g_map, &key);

    const void *result = hashmap_get_with_hash(g_map, &key, hash);
    TEST_ASSERT_NULL(result);
}

/* 3. Get from an empty map — should return NULL without crashing.         */
void test_get_with_hash_empty_map(void)
{
    item_t key = {1, ""};
    uint64_t hash = compute_hash(g_map, &key);

    const void *result = hashmap_get_with_hash(g_map, &key, hash);
    TEST_ASSERT_NULL(result);
}

/* 4. Insert multiple items and retrieve each one correctly.               */
void test_get_with_hash_multiple_items(void)
{
    for (int i = 0; i < 20; i++) {
        item_t it;
        it.key = i;
        snprintf(it.value, sizeof(it.value), "val_%d", i);
        hashmap_set(g_map, &it);
    }

    for (int i = 0; i < 20; i++) {
        item_t key = {i, ""};
        uint64_t hash = compute_hash(g_map, &key);
        const void *result = hashmap_get_with_hash(g_map, &key, hash);
        TEST_ASSERT_NOT_NULL_MESSAGE(result, "Expected to find inserted item");
        const item_t *found = (const item_t *)result;
        TEST_ASSERT_EQUAL_INT(i, found->key);
    }
}

/* 5. After deleting an item, get should return NULL for that item.        */
void test_get_with_hash_after_delete(void)
{
    item_t inserted = {7, "seven"};
    hashmap_set(g_map, &inserted);

    item_t key = {7, ""};
    uint64_t hash = compute_hash(g_map, &key);

    /* Confirm it exists first */
    const void *before = hashmap_get_with_hash(g_map, &key, hash);
    TEST_ASSERT_NOT_NULL(before);

    hashmap_delete(g_map, &key);

    const void *after = hashmap_get_with_hash(g_map, &key, hash);
    TEST_ASSERT_NULL(after);
}

/* 6. Providing a wrong (mismatched) hash for an existing key should NOT
 *    find the item (the hash is used as a fast-path discriminator).       */
void test_get_with_hash_wrong_hash_returns_null(void)
{
    item_t inserted = {5, "five"};
    hashmap_set(g_map, &inserted);

    item_t key = {5, ""};
    /* Use a deliberately wrong hash — pick a value that differs from the
     * real one.  We XOR with a large constant to make a collision unlikely. */
    uint64_t real_hash  = compute_hash(g_map, &key);
    uint64_t wrong_hash = real_hash ^ 0xDEADBEEFCAFEBABEULL;

    /* clip_hash strips the top bit; make sure wrong_hash is also clipped
     * differently from real_hash after clipping.  If by bad luck they
     * collide after clipping, just skip the assertion — but in practice
     * this will not happen with the XOR constant above.                   */
    uint64_t clipped_real  = clip_hash(real_hash);
    uint64_t clipped_wrong = clip_hash(wrong_hash);

    if (clipped_real != clipped_wrong) {
        const void *result = hashmap_get_with_hash(g_map, &key, wrong_hash);
        TEST_ASSERT_NULL(result);
    } else {
        /* Hashes collided after clipping — test is vacuously passing.    */
        TEST_ASSERT_TRUE(1);
    }
}

/* 7. Correct hash but key that does not exist in the map.                 */
void test_get_with_hash_correct_hash_nonexistent_key(void)
{
    item_t inserted = {10, "ten"};
    hashmap_set(g_map, &inserted);

    /* Build a key with a different integer but compute its own hash.      */
    item_t key = {11, ""};
    uint64_t hash = compute_hash(g_map, &key);

    const void *result = hashmap_get_with_hash(g_map, &key, hash);
    TEST_ASSERT_NULL(result);
}

/* 8. Large number of insertions to force rehashing; all items still
 *    retrievable via hashmap_get_with_hash.                               */
void test_get_with_hash_after_rehash(void)
{
    const int N = 200;
    for (int i = 0; i < N; i++) {
        item_t it;
        it.key = i * 3 + 1;   /* spread keys */
        snprintf(it.value, sizeof(it.value), "item%d", i);
        hashmap_set(g_map, &it);
    }

    for (int i = 0; i < N; i++) {
        item_t key;
        key.key = i * 3 + 1;
        uint64_t hash = compute_hash(g_map, &key);
        const void *result = hashmap_get_with_hash(g_map, &key, hash);
        TEST_ASSERT_NOT_NULL_MESSAGE(result, "Item missing after rehash");
        const item_t *found = (const item_t *)result;
        TEST_ASSERT_EQUAL_INT(key.key, found->key);
    }
}

/* 9. Overwrite an existing item and verify the updated value is returned. */
void test_get_with_hash_after_overwrite(void)
{
    item_t first  = {3, "original"};
    item_t second = {3, "updated"};

    hashmap_set(g_map, &first);
    hashmap_set(g_map, &second);

    item_t key = {3, ""};
    uint64_t hash = compute_hash(g_map, &key);

    const void *result = hashmap_get_with_hash(g_map, &key, hash);
    TEST_ASSERT_NOT_NULL(result);
    const item_t *found = (const item_t *)result;
    TEST_ASSERT_EQUAL_INT(3, found->key);
    TEST_ASSERT_EQUAL_STRING("updated", found->value);
}

/* 10. Verify that the returned pointer actually points into the map's
 *     storage (not a copy), i.e. it is non-NULL and consistent with a
 *     normal hashmap_get() call.                                          */
void test_get_with_hash_matches_regular_get(void)
{
    item_t inserted = {55, "fiftyfive"};
    hashmap_set(g_map, &inserted);

    item_t key = {55, ""};
    uint64_t hash = compute_hash(g_map, &key);

    const void *via_hash   = hashmap_get_with_hash(g_map, &key, hash);
    const void *via_normal = hashmap_get(g_map, &key);

    TEST_ASSERT_NOT_NULL(via_hash);
    TEST_ASSERT_NOT_NULL(via_normal);
    /* Both calls should return the same storage address.                  */
    TEST_ASSERT_EQUAL_PTR(via_normal, via_hash);
}

/* -------------------------------------------------------------------------
 * main
 * ---------------------------------------------------------------------- */
int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_get_with_hash_existing_item);
    RUN_TEST(test_get_with_hash_missing_item);
    RUN_TEST(test_get_with_hash_empty_map);
    RUN_TEST(test_get_with_hash_multiple_items);
    RUN_TEST(test_get_with_hash_after_delete);
    RUN_TEST(test_get_with_hash_wrong_hash_returns_null);
    RUN_TEST(test_get_with_hash_correct_hash_nonexistent_key);
    RUN_TEST(test_get_with_hash_after_rehash);
    RUN_TEST(test_get_with_hash_after_overwrite);
    RUN_TEST(test_get_with_hash_matches_regular_get);
    return UNITY_END();
}