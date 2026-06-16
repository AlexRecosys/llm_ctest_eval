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

static struct hashmap *g_map;

/* ── helpers ──────────────────────────────────────────────────────────────── */

static uint64_t item_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
    const struct item *it = (const struct item *)item;
    return hashmap_murmur(&it->key, sizeof(it->key), seed0, seed1);
}

static int item_compare(const void *a, const void *b, void *udata)
{
    (void)udata;
    const struct item *ia = (const struct item *)a;
    const struct item *ib = (const struct item *)b;
    return ia->key - ib->key;
}

static struct hashmap *make_map(void)
{
    return hashmap_new(sizeof(struct item), 0, 0, 0,
                       item_hash, item_compare, NULL, NULL);
}

static void insert(struct hashmap *map, int key, int value)
{
    struct item it = { key, value };
    hashmap_set(map, &it);
}

/* ── setUp / tearDown ─────────────────────────────────────────────────────── */

void setUp(void)
{
    g_map = make_map();
    TEST_ASSERT_NOT_NULL(g_map);
}

void tearDown(void)
{
    hashmap_free(g_map);
    g_map = NULL;
}

/* ── test cases ───────────────────────────────────────────────────────────── */

/* deleting from an empty map returns NULL */
void test_delete_empty_map_returns_null(void)
{
    struct item key = { 42, 0 };
    const void *result = hashmap_delete(g_map, &key);
    TEST_ASSERT_NULL(result);
}

/* deleting an existing key returns a non-NULL pointer */
void test_delete_existing_key_returns_non_null(void)
{
    insert(g_map, 10, 100);
    struct item key = { 10, 0 };
    const void *result = hashmap_delete(g_map, &key);
    TEST_ASSERT_NOT_NULL(result);
}

/* the returned item carries the correct key and value */
void test_delete_returns_correct_item(void)
{
    insert(g_map, 7, 77);
    struct item key = { 7, 0 };
    const struct item *ret = (const struct item *)hashmap_delete(g_map, &key);
    TEST_ASSERT_NOT_NULL(ret);
    TEST_ASSERT_EQUAL_INT(7,  ret->key);
    TEST_ASSERT_EQUAL_INT(77, ret->value);
}

/* after deletion the key is no longer retrievable */
void test_delete_removes_key_from_map(void)
{
    insert(g_map, 5, 50);
    struct item key = { 5, 0 };
    hashmap_delete(g_map, &key);
    const void *found = hashmap_get(g_map, &key);
    TEST_ASSERT_NULL(found);
}

/* deleting a key that was never inserted returns NULL */
void test_delete_nonexistent_key_returns_null(void)
{
    insert(g_map, 1, 10);
    struct item key = { 999, 0 };
    const void *result = hashmap_delete(g_map, &key);
    TEST_ASSERT_NULL(result);
}

/* deleting the same key twice: second call returns NULL */
void test_delete_same_key_twice(void)
{
    insert(g_map, 3, 30);
    struct item key = { 3, 0 };
    const void *first  = hashmap_delete(g_map, &key);
    TEST_ASSERT_NOT_NULL(first);
    const void *second = hashmap_delete(g_map, &key);
    TEST_ASSERT_NULL(second);
}

/* map count decreases by one after a successful delete */
void test_delete_decrements_count(void)
{
    insert(g_map, 1, 10);
    insert(g_map, 2, 20);
    insert(g_map, 3, 30);
    size_t before = hashmap_count(g_map);
    struct item key = { 2, 0 };
    hashmap_delete(g_map, &key);
    size_t after = hashmap_count(g_map);
    TEST_ASSERT_EQUAL_UINT(before - 1, after);
}

/* map count does NOT decrease when deleting a nonexistent key */
void test_delete_nonexistent_does_not_change_count(void)
{
    insert(g_map, 1, 10);
    size_t before = hashmap_count(g_map);
    struct item key = { 999, 0 };
    hashmap_delete(g_map, &key);
    size_t after = hashmap_count(g_map);
    TEST_ASSERT_EQUAL_UINT(before, after);
}

/* deleting one key leaves other keys intact */
void test_delete_does_not_affect_other_keys(void)
{
    insert(g_map, 10, 100);
    insert(g_map, 20, 200);
    insert(g_map, 30, 300);

    struct item del_key = { 20, 0 };
    hashmap_delete(g_map, &del_key);

    struct item k10 = { 10, 0 };
    struct item k30 = { 30, 0 };
    const struct item *r10 = (const struct item *)hashmap_get(g_map, &k10);
    const struct item *r30 = (const struct item *)hashmap_get(g_map, &k30);

    TEST_ASSERT_NOT_NULL(r10);
    TEST_ASSERT_EQUAL_INT(100, r10->value);
    TEST_ASSERT_NOT_NULL(r30);
    TEST_ASSERT_EQUAL_INT(300, r30->value);
}

/* re-inserting a deleted key works correctly */
void test_reinsert_after_delete(void)
{
    insert(g_map, 42, 1);
    struct item key = { 42, 0 };
    hashmap_delete(g_map, &key);

    insert(g_map, 42, 2);
    const struct item *found = (const struct item *)hashmap_get(g_map, &key);
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_INT(2, found->value);
}

/* delete many items — stress test to exercise Robin Hood probe sequences */
void test_delete_many_items(void)
{
    const int N = 200;
    for (int i = 0; i < N; i++) {
        insert(g_map, i, i * 10);
    }
    /* delete even keys */
    for (int i = 0; i < N; i += 2) {
        struct item key = { i, 0 };
        const struct item *ret = (const struct item *)hashmap_delete(g_map, &key);
        TEST_ASSERT_NOT_NULL_MESSAGE(ret, "expected non-NULL for even key delete");
        TEST_ASSERT_EQUAL_INT(i, ret->key);
    }
    /* odd keys must still be present */
    for (int i = 1; i < N; i += 2) {
        struct item key = { i, 0 };
        const struct item *found = (const struct item *)hashmap_get(g_map, &key);
        TEST_ASSERT_NOT_NULL_MESSAGE(found, "odd key should still exist");
        TEST_ASSERT_EQUAL_INT(i * 10, found->value);
    }
    /* even keys must be gone */
    for (int i = 0; i < N; i += 2) {
        struct item key = { i, 0 };
        const void *gone = hashmap_get(g_map, &key);
        TEST_ASSERT_NULL_MESSAGE(gone, "even key should be deleted");
    }
}

/* delete with key whose value field is irrelevant (only key field matters) */
void test_delete_ignores_value_field_of_key_argument(void)
{
    insert(g_map, 55, 555);
    /* pass a different value in the lookup key — compare only uses key field */
    struct item key = { 55, 9999 };
    const struct item *ret = (const struct item *)hashmap_delete(g_map, &key);
    TEST_ASSERT_NOT_NULL(ret);
    TEST_ASSERT_EQUAL_INT(55,  ret->key);
    TEST_ASSERT_EQUAL_INT(555, ret->value);
}

/* ── main ─────────────────────────────────────────────────────────────────── */

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_delete_empty_map_returns_null);
    RUN_TEST(test_delete_existing_key_returns_non_null);
    RUN_TEST(test_delete_returns_correct_item);
    RUN_TEST(test_delete_removes_key_from_map);
    RUN_TEST(test_delete_nonexistent_key_returns_null);
    RUN_TEST(test_delete_same_key_twice);
    RUN_TEST(test_delete_decrements_count);
    RUN_TEST(test_delete_nonexistent_does_not_change_count);
    RUN_TEST(test_delete_does_not_affect_other_keys);
    RUN_TEST(test_reinsert_after_delete);
    RUN_TEST(test_delete_many_items);
    RUN_TEST(test_delete_ignores_value_field_of_key_argument);
    return UNITY_END();
}