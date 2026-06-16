#include "hashmap.c"
#include "unity.h"
#include "hashmap.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* ── fixtures ─────────────────────────────────────────────────────────────── */

typedef struct {
    int   key;
    int   value;
} IntItem;

static struct hashmap *g_map;

/* ── helpers ──────────────────────────────────────────────────────────────── */

static uint64_t int_item_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
    const IntItem *it = (const IntItem *)item;
    return hashmap_murmur(&it->key, sizeof(it->key), seed0, seed1);
}

static int int_item_compare(const void *a, const void *b, void *udata)
{
    (void)udata;
    const IntItem *ia = (const IntItem *)a;
    const IntItem *ib = (const IntItem *)b;
    return ia->key - ib->key;
}

static struct hashmap *make_map(void)
{
    return hashmap_new(sizeof(IntItem), 0, 0, 0,
                       int_item_hash, int_item_compare, NULL, NULL);
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

/* Inserting a new item returns NULL (no previous value). */
void test_set_new_item_returns_null(void)
{
    IntItem item = {.key = 1, .value = 100};
    const void *prev = hashmap_set(g_map, &item);
    TEST_ASSERT_NULL(prev);
}

/* After insertion the item can be retrieved with the correct value. */
void test_set_new_item_can_be_retrieved(void)
{
    IntItem item = {.key = 42, .value = 999};
    hashmap_set(g_map, &item);

    const IntItem *found = (const IntItem *)hashmap_get(g_map, &item);
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_INT(42,  found->key);
    TEST_ASSERT_EQUAL_INT(999, found->value);
}

/* Replacing an existing item returns the OLD item (by value). */
void test_set_existing_item_returns_old_value(void)
{
    IntItem first  = {.key = 7, .value = 10};
    IntItem second = {.key = 7, .value = 20};

    hashmap_set(g_map, &first);
    const IntItem *prev = (const IntItem *)hashmap_set(g_map, &second);

    TEST_ASSERT_NOT_NULL(prev);
    TEST_ASSERT_EQUAL_INT(7,  prev->key);
    TEST_ASSERT_EQUAL_INT(10, prev->value);
}

/* After replacement the map holds the NEW value. */
void test_set_existing_item_updates_value(void)
{
    IntItem first  = {.key = 7, .value = 10};
    IntItem second = {.key = 7, .value = 20};

    hashmap_set(g_map, &first);
    hashmap_set(g_map, &second);

    const IntItem *found = (const IntItem *)hashmap_get(g_map, &second);
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_INT(20, found->value);
}

/* Count increases by one for each unique key inserted. */
void test_set_increments_count(void)
{
    IntItem a = {.key = 1, .value = 1};
    IntItem b = {.key = 2, .value = 2};
    IntItem c = {.key = 3, .value = 3};

    hashmap_set(g_map, &a);
    TEST_ASSERT_EQUAL_INT(1, (int)hashmap_count(g_map));

    hashmap_set(g_map, &b);
    TEST_ASSERT_EQUAL_INT(2, (int)hashmap_count(g_map));

    hashmap_set(g_map, &c);
    TEST_ASSERT_EQUAL_INT(3, (int)hashmap_count(g_map));
}

/* Replacing an existing key does NOT increase the count. */
void test_set_replace_does_not_increment_count(void)
{
    IntItem first  = {.key = 5, .value = 1};
    IntItem second = {.key = 5, .value = 2};

    hashmap_set(g_map, &first);
    hashmap_set(g_map, &second);

    TEST_ASSERT_EQUAL_INT(1, (int)hashmap_count(g_map));
}

/* Multiple distinct items can be inserted and all retrieved correctly. */
void test_set_multiple_items(void)
{
    int n = 50;
    for (int i = 0; i < n; i++) {
        IntItem item = {.key = i, .value = i * 10};
        const void *prev = hashmap_set(g_map, &item);
        TEST_ASSERT_NULL(prev);
    }

    TEST_ASSERT_EQUAL_INT(n, (int)hashmap_count(g_map));

    for (int i = 0; i < n; i++) {
        IntItem lookup = {.key = i};
        const IntItem *found = (const IntItem *)hashmap_get(g_map, &lookup);
        TEST_ASSERT_NOT_NULL(found);
        TEST_ASSERT_EQUAL_INT(i,      found->key);
        TEST_ASSERT_EQUAL_INT(i * 10, found->value);
    }
}

/* Inserting the same item twice in a row returns the first on the second call. */
void test_set_same_item_twice(void)
{
    IntItem item = {.key = 99, .value = 77};

    const void *first_prev  = hashmap_set(g_map, &item);
    const void *second_prev = hashmap_set(g_map, &item);

    TEST_ASSERT_NULL(first_prev);
    TEST_ASSERT_NOT_NULL(second_prev);
    TEST_ASSERT_EQUAL_INT(99, ((const IntItem *)second_prev)->key);
    TEST_ASSERT_EQUAL_INT(77, ((const IntItem *)second_prev)->value);
}

/* hashmap_set stores a copy: mutating the original does not affect the map. */
void test_set_stores_copy_not_pointer(void)
{
    IntItem item = {.key = 11, .value = 55};
    hashmap_set(g_map, &item);

    /* Mutate the local variable */
    item.value = 9999;

    IntItem lookup = {.key = 11};
    const IntItem *found = (const IntItem *)hashmap_get(g_map, &lookup);
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_INT(55, found->value);   /* original value preserved */
}

/* Inserting enough items to trigger a resize still works correctly. */
void test_set_survives_resize(void)
{
    int n = 200;
    for (int i = 0; i < n; i++) {
        IntItem item = {.key = i, .value = i + 1};
        hashmap_set(g_map, &item);
    }

    TEST_ASSERT_EQUAL_INT(n, (int)hashmap_count(g_map));

    for (int i = 0; i < n; i++) {
        IntItem lookup = {.key = i};
        const IntItem *found = (const IntItem *)hashmap_get(g_map, &lookup);
        TEST_ASSERT_NOT_NULL(found);
        TEST_ASSERT_EQUAL_INT(i + 1, found->value);
    }
}

/* A key that was never inserted is not found after unrelated insertions. */
void test_set_absent_key_not_found(void)
{
    IntItem a = {.key = 1, .value = 1};
    IntItem b = {.key = 2, .value = 2};
    hashmap_set(g_map, &a);
    hashmap_set(g_map, &b);

    IntItem lookup = {.key = 999};
    const void *found = hashmap_get(g_map, &lookup);
    TEST_ASSERT_NULL(found);
}

/* ── main ─────────────────────────────────────────────────────────────────── */

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_set_new_item_returns_null);
    RUN_TEST(test_set_new_item_can_be_retrieved);
    RUN_TEST(test_set_existing_item_returns_old_value);
    RUN_TEST(test_set_existing_item_updates_value);
    RUN_TEST(test_set_increments_count);
    RUN_TEST(test_set_replace_does_not_increment_count);
    RUN_TEST(test_set_multiple_items);
    RUN_TEST(test_set_same_item_twice);
    RUN_TEST(test_set_stores_copy_not_pointer);
    RUN_TEST(test_set_survives_resize);
    RUN_TEST(test_set_absent_key_not_found);
    return UNITY_END();
}