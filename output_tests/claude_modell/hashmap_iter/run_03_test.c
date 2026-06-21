#include "hashmap.c"
#include "unity.h"
#include "hashmap.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

/* -------------------------------------------------------------------------
 * Fixtures
 * ------------------------------------------------------------------------- */

static struct hashmap *g_map;

/* -------------------------------------------------------------------------
 * Helper types and functions
 * ------------------------------------------------------------------------- */

typedef struct {
    int   key;
    int   value;
} int_item;

static uint64_t int_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    const int_item *it = (const int_item *)item;
    return hashmap_murmur(&it->key, sizeof(it->key), seed0, seed1);
}

static int int_compare(const void *a, const void *b, void *udata) {
    (void)udata;
    const int_item *ia = (const int_item *)a;
    const int_item *ib = (const int_item *)b;
    return ia->key - ib->key;
}

static struct hashmap *create_map(size_t cap) {
    return hashmap_new(sizeof(int_item), cap, 0, 0,
                       int_hash, int_compare, NULL, NULL);
}

/* -------------------------------------------------------------------------
 * setUp / tearDown
 * ------------------------------------------------------------------------- */

void setUp(void) {
    g_map = create_map(0);
    TEST_ASSERT_NOT_NULL(g_map);
}

void tearDown(void) {
    if (g_map) {
        hashmap_free(g_map);
        g_map = NULL;
    }
}

/* -------------------------------------------------------------------------
 * Test cases
 * ------------------------------------------------------------------------- */

/* Iterating an empty map must return false immediately */
void test_iter_empty_map_returns_false(void) {
    size_t i = 0;
    void  *item = NULL;

    bool result = hashmap_iter(g_map, &i, &item);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(item);
}

/* After exhausting all items the function must return false */
void test_iter_returns_false_after_all_items_consumed(void) {
    int_item entry = {42, 100};
    hashmap_set(g_map, &entry);

    size_t i    = 0;
    void  *item = NULL;

    /* consume the single item */
    bool first = hashmap_iter(g_map, &i, &item);
    TEST_ASSERT_TRUE(first);

    /* next call must signal end-of-iteration */
    bool second = hashmap_iter(g_map, &i, &item);
    TEST_ASSERT_FALSE(second);
}

/* A single inserted item must be returned by the iterator */
void test_iter_single_item(void) {
    int_item entry = {7, 77};
    hashmap_set(g_map, &entry);

    size_t    i    = 0;
    void     *item = NULL;
    bool      ret  = hashmap_iter(g_map, &i, &item);

    TEST_ASSERT_TRUE(ret);
    TEST_ASSERT_NOT_NULL(item);

    int_item *got = (int_item *)item;
    TEST_ASSERT_EQUAL_INT(7,  got->key);
    TEST_ASSERT_EQUAL_INT(77, got->value);
}

/* Iterator must visit every inserted item exactly once */
void test_iter_visits_all_items(void) {
    const int N = 20;
    for (int k = 0; k < N; k++) {
        int_item e = {k, k * 10};
        hashmap_set(g_map, &e);
    }

    int visited[20] = {0};
    size_t i   = 0;
    void  *item = NULL;
    int    count = 0;

    while (hashmap_iter(g_map, &i, &item)) {
        TEST_ASSERT_NOT_NULL(item);
        int_item *got = (int_item *)item;
        TEST_ASSERT_TRUE(got->key >= 0 && got->key < N);
        visited[got->key]++;
        count++;
    }

    TEST_ASSERT_EQUAL_INT(N, count);
    for (int k = 0; k < N; k++) {
        TEST_ASSERT_EQUAL_INT_MESSAGE(1, visited[k], "item visited wrong number of times");
    }
}

/* Cursor i must be advanced after each successful call */
void test_iter_advances_cursor(void) {
    int_item e1 = {1, 10};
    int_item e2 = {2, 20};
    hashmap_set(g_map, &e1);
    hashmap_set(g_map, &e2);

    size_t i_before, i_after;
    void  *item = NULL;

    i_before = 0;
    hashmap_iter(g_map, &i_before, &item);
    i_after = i_before;

    /* cursor must have moved forward */
    TEST_ASSERT_GREATER_THAN(0, (int)i_after);
}

/* Restarting the cursor (i=0) must restart iteration from the beginning */
void test_iter_restart_from_zero(void) {
    int_item e = {99, 999};
    hashmap_set(g_map, &e);

    size_t i   = 0;
    void  *item = NULL;

    bool first_run = hashmap_iter(g_map, &i, &item);
    TEST_ASSERT_TRUE(first_run);

    /* restart */
    i    = 0;
    item = NULL;
    bool second_run = hashmap_iter(g_map, &i, &item);
    TEST_ASSERT_TRUE(second_run);
    TEST_ASSERT_NOT_NULL(item);

    int_item *got = (int_item *)item;
    TEST_ASSERT_EQUAL_INT(99,  got->key);
    TEST_ASSERT_EQUAL_INT(999, got->value);
}

/* Item pointer returned must point into the map (not a copy) */
void test_iter_returns_pointer_into_map(void) {
    int_item e = {55, 550};
    hashmap_set(g_map, &e);

    size_t i   = 0;
    void  *item = NULL;
    hashmap_iter(g_map, &i, &item);
    TEST_ASSERT_NOT_NULL(item);

    /* Verify the pointer is consistent with hashmap_get */
    int_item key = {55, 0};
    const void *from_get = hashmap_get(g_map, &key);
    TEST_ASSERT_EQUAL_PTR(from_get, item);
}

/* Iterating a larger map must yield exactly hashmap_count() items */
void test_iter_count_matches_hashmap_count(void) {
    const int N = 50;
    for (int k = 0; k < N; k++) {
        int_item e = {k, k};
        hashmap_set(g_map, &e);
    }

    size_t expected = hashmap_count(g_map);
    TEST_ASSERT_EQUAL_UINT(N, (unsigned)expected);

    size_t i     = 0;
    void  *item  = NULL;
    size_t count = 0;

    while (hashmap_iter(g_map, &i, &item)) {
        count++;
    }

    TEST_ASSERT_EQUAL_UINT(expected, count);
}

/* When i already equals nbuckets the function must return false */
void test_iter_with_i_at_nbuckets_returns_false(void) {
    int_item e = {3, 30};
    hashmap_set(g_map, &e);

    /* exhaust the iterator to find the bucket count */
    size_t i   = 0;
    void  *item = NULL;
    while (hashmap_iter(g_map, &i, &item));

    /* i is now >= nbuckets; another call must still return false */
    bool ret = hashmap_iter(g_map, &i, &item);
    TEST_ASSERT_FALSE(ret);
}

/* Iterating after clear must return false immediately */
void test_iter_after_clear_returns_false(void) {
    int_item e = {10, 100};
    hashmap_set(g_map, &e);

    hashmap_clear(g_map, false);

    size_t i   = 0;
    void  *item = NULL;
    bool   ret  = hashmap_iter(g_map, &i, &item);
    TEST_ASSERT_FALSE(ret);
}

/* Duplicate inserts (same key) must not inflate the iteration count */
void test_iter_duplicate_keys_counted_once(void) {
    int_item e1 = {5, 50};
    int_item e2 = {5, 99}; /* same key, different value */
    hashmap_set(g_map, &e1);
    hashmap_set(g_map, &e2);

    size_t i     = 0;
    void  *item  = NULL;
    size_t count = 0;

    while (hashmap_iter(g_map, &i, &item)) {
        count++;
    }

    TEST_ASSERT_EQUAL_UINT(1, count);
    TEST_ASSERT_EQUAL_INT(99, ((int_item *)item)->value);
}

/* -------------------------------------------------------------------------
 * main
 * ------------------------------------------------------------------------- */

int main(void) {
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_iter_empty_map_returns_false);
    RUN_TEST(test_iter_returns_false_after_all_items_consumed);
    RUN_TEST(test_iter_single_item);
    RUN_TEST(test_iter_visits_all_items);
    RUN_TEST(test_iter_advances_cursor);
    RUN_TEST(test_iter_restart_from_zero);
    RUN_TEST(test_iter_returns_pointer_into_map);
    RUN_TEST(test_iter_count_matches_hashmap_count);
    RUN_TEST(test_iter_with_i_at_nbuckets_returns_false);
    RUN_TEST(test_iter_after_clear_returns_false);
    RUN_TEST(test_iter_duplicate_keys_counted_once);
    return UNITY_END();
}