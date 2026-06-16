#include "hashmap.c"
#include "unity.h"
#include "hashmap.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* ── fixtures ─────────────────────────────────────────────────────────────── */

struct item {
    int   key;
    int   value;
};

static struct hashmap *map;

/* ── helpers ──────────────────────────────────────────────────────────────── */

static uint64_t item_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
    const struct item *it = (const struct item *)item;
    return hashmap_murmur(&it->key, sizeof(it->key), seed0, seed1);
}

static int item_compare(const void *a, const void *b, void *udata)
{
    (void)udata;
    return ((const struct item *)a)->key - ((const struct item *)b)->key;
}

static struct hashmap *make_map(void)
{
    return hashmap_new(sizeof(struct item), 0, 0, 0,
                       item_hash, item_compare, NULL, NULL);
}

static void insert(struct hashmap *m, int key, int value)
{
    struct item it = { key, value };
    hashmap_set(m, &it);
}

/* ── setUp / tearDown ─────────────────────────────────────────────────────── */

void setUp(void)
{
    map = make_map();
    TEST_ASSERT_NOT_NULL(map);
}

void tearDown(void)
{
    hashmap_free(map);
    map = NULL;
}

/* ── test cases ───────────────────────────────────────────────────────────── */

/* Deleting a key that was inserted returns a non-NULL pointer */
void test_delete_existing_key_returns_non_null(void)
{
    insert(map, 42, 100);

    struct item key = { 42, 0 };
    const void *result = hashmap_delete(map, &key);

    TEST_ASSERT_NOT_NULL(result);
}

/* The returned item carries the correct key and value */
void test_delete_returns_correct_item(void)
{
    insert(map, 7, 999);

    struct item key = { 7, 0 };
    const struct item *result = (const struct item *)hashmap_delete(map, &key);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(7,   result->key);
    TEST_ASSERT_EQUAL_INT(999, result->value);
}

/* After deletion the key is no longer retrievable */
void test_delete_removes_key_from_map(void)
{
    insert(map, 5, 55);

    struct item key = { 5, 0 };
    hashmap_delete(map, &key);

    const struct item *found = (const struct item *)hashmap_get(map, &key);
    TEST_ASSERT_NULL(found);
}

/* Deleting a key that does not exist returns NULL */
void test_delete_nonexistent_key_returns_null(void)
{
    struct item key = { 123, 0 };
    const void *result = hashmap_delete(map, &key);

    TEST_ASSERT_NULL(result);
}

/* Deleting from an empty map returns NULL */
void test_delete_from_empty_map_returns_null(void)
{
    struct item key = { 1, 0 };
    const void *result = hashmap_delete(map, &key);

    TEST_ASSERT_NULL(result);
}

/* Map count decreases by one after a successful deletion */
void test_delete_decrements_count(void)
{
    insert(map, 10, 1);
    insert(map, 20, 2);
    insert(map, 30, 3);

    size_t before = hashmap_count(map);

    struct item key = { 20, 0 };
    hashmap_delete(map, &key);

    size_t after = hashmap_count(map);
    TEST_ASSERT_EQUAL_UINT(before - 1, after);
}

/* Map count does NOT change when deleting a non-existent key */
void test_delete_nonexistent_does_not_change_count(void)
{
    insert(map, 1, 1);
    insert(map, 2, 2);

    size_t before = hashmap_count(map);

    struct item key = { 99, 0 };
    hashmap_delete(map, &key);

    TEST_ASSERT_EQUAL_UINT(before, hashmap_count(map));
}

/* Deleting the same key twice: second call returns NULL */
void test_delete_same_key_twice_second_returns_null(void)
{
    insert(map, 8, 8);

    struct item key = { 8, 0 };
    hashmap_delete(map, &key);

    const void *second = hashmap_delete(map, &key);
    TEST_ASSERT_NULL(second);
}

/* Remaining keys are still accessible after an unrelated deletion */
void test_delete_does_not_disturb_other_keys(void)
{
    insert(map, 1, 11);
    insert(map, 2, 22);
    insert(map, 3, 33);

    struct item del_key = { 2, 0 };
    hashmap_delete(map, &del_key);

    struct item k1 = { 1, 0 };
    struct item k3 = { 3, 0 };

    const struct item *r1 = (const struct item *)hashmap_get(map, &k1);
    const struct item *r3 = (const struct item *)hashmap_get(map, &k3);

    TEST_ASSERT_NOT_NULL(r1);
    TEST_ASSERT_EQUAL_INT(11, r1->value);

    TEST_ASSERT_NOT_NULL(r3);
    TEST_ASSERT_EQUAL_INT(33, r3->value);
}

/* A key can be re-inserted after deletion and then deleted again */
void test_reinsert_after_delete_then_delete_again(void)
{
    insert(map, 55, 100);

    struct item key = { 55, 0 };
    hashmap_delete(map, &key);

    insert(map, 55, 200);

    const struct item *result = (const struct item *)hashmap_delete(map, &key);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(55,  result->key);
    TEST_ASSERT_EQUAL_INT(200, result->value);
}

/* Delete many items one by one; count reaches zero */
void test_delete_all_items_count_reaches_zero(void)
{
    int keys[] = { 10, 20, 30, 40, 50 };
    int n = (int)(sizeof(keys) / sizeof(keys[0]));

    for (int i = 0; i < n; i++) {
        insert(map, keys[i], keys[i]);
    }

    for (int i = 0; i < n; i++) {
        struct item k = { keys[i], 0 };
        const void *r = hashmap_delete(map, &k);
        TEST_ASSERT_NOT_NULL_MESSAGE(r, "expected non-NULL on valid delete");
    }

    TEST_ASSERT_EQUAL_UINT(0, hashmap_count(map));
}

/* ── main ─────────────────────────────────────────────────────────────────── */

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_delete_existing_key_returns_non_null);
    RUN_TEST(test_delete_returns_correct_item);
    RUN_TEST(test_delete_removes_key_from_map);
    RUN_TEST(test_delete_nonexistent_key_returns_null);
    RUN_TEST(test_delete_from_empty_map_returns_null);
    RUN_TEST(test_delete_decrements_count);
    RUN_TEST(test_delete_nonexistent_does_not_change_count);
    RUN_TEST(test_delete_same_key_twice_second_returns_null);
    RUN_TEST(test_delete_does_not_disturb_other_keys);
    RUN_TEST(test_reinsert_after_delete_then_delete_again);
    RUN_TEST(test_delete_all_items_count_reaches_zero);
    return UNITY_END();
}