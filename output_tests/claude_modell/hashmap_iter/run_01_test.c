#include "hashmap.c"
#include "unity.h"
#include "hashmap.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

/* -------------------------------------------------------------------------
 * Fixtures
 * ------------------------------------------------------------------------- */

struct test_item {
    int   key;
    int   value;
};

static struct hashmap *g_map;

/* -------------------------------------------------------------------------
 * Helper functions
 * ------------------------------------------------------------------------- */

static uint64_t item_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    const struct test_item *ti = (const struct test_item *)item;
    return hashmap_murmur(&ti->key, sizeof(ti->key), seed0, seed1);
}

static int item_compare(const void *a, const void *b, void *udata) {
    (void)udata;
    const struct test_item *ta = (const struct test_item *)a;
    const struct test_item *tb = (const struct test_item *)b;
    return ta->key - tb->key;
}

static struct hashmap *create_map(size_t cap) {
    return hashmap_new(sizeof(struct test_item), cap, 0, 0,
                       item_hash, item_compare, NULL, NULL);
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
void test_iter_returns_false_when_exhausted(void) {
    struct test_item ti = {42, 100};
    hashmap_set(g_map, &ti);

    size_t i    = 0;
    void  *item = NULL;

    /* consume the single item */
    bool got_one = hashmap_iter(g_map, &i, &item);
    TEST_ASSERT_TRUE(got_one);

    /* next call must signal end-of-iteration */
    item = NULL;
    bool got_two = hashmap_iter(g_map, &i, &item);
    TEST_ASSERT_FALSE(got_two);
    TEST_ASSERT_NULL(item);
}

/* A single inserted item must be returned by the iterator */
void test_iter_single_item(void) {
    struct test_item ti = {7, 77};
    hashmap_set(g_map, &ti);

    size_t i    = 0;
    void  *item = NULL;

    bool result = hashmap_iter(g_map, &i, &item);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(item);

    struct test_item *got = (struct test_item *)item;
    TEST_ASSERT_EQUAL_INT(7,  got->key);
    TEST_ASSERT_EQUAL_INT(77, got->value);
}

/* The iterator must visit every inserted item exactly once */
void test_iter_visits_all_items(void) {
    const int N = 10;
    for (int k = 0; k < N; k++) {
        struct test_item ti = {k, k * 10};
        hashmap_set(g_map, &ti);
    }

    int visited[10] = {0};
    size_t i    = 0;
    void  *item = NULL;
    int   count = 0;

    while (hashmap_iter(g_map, &i, &item)) {
        TEST_ASSERT_NOT_NULL(item);
        struct test_item *got = (struct test_item *)item;
        TEST_ASSERT_TRUE(got->key >= 0 && got->key < N);
        visited[got->key]++;
        count++;
    }

    TEST_ASSERT_EQUAL_INT(N, count);
    for (int k = 0; k < N; k++) {
        TEST_ASSERT_EQUAL_INT_MESSAGE(1, visited[k], "each key must be visited exactly once");
    }
}

/* The iterator must work correctly after items are inserted with a larger cap */
void test_iter_large_map(void) {
    const int N = 100;
    struct hashmap *map = create_map(128);
    TEST_ASSERT_NOT_NULL(map);

    for (int k = 0; k < N; k++) {
        struct test_item ti = {k, k + 1};
        hashmap_set(map, &ti);
    }

    int   count = 0;
    size_t i    = 0;
    void  *item = NULL;

    while (hashmap_iter(map, &i, &item)) {
        TEST_ASSERT_NOT_NULL(item);
        count++;
    }

    TEST_ASSERT_EQUAL_INT(N, count);
    hashmap_free(map);
}

/* Starting iteration with i > nbuckets must return false immediately */
void test_iter_i_beyond_nbuckets_returns_false(void) {
    struct test_item ti = {1, 1};
    hashmap_set(g_map, &ti);

    /* Set i to a very large value to simulate past-end cursor */
    size_t i    = (size_t)-1;
    void  *item = NULL;

    bool result = hashmap_iter(g_map, &i, &item);
    TEST_ASSERT_FALSE(result);
}

/* The item pointer returned is a direct reference into the map (not a copy) */
void test_iter_item_is_direct_reference(void) {
    struct test_item ti = {55, 550};
    hashmap_set(g_map, &ti);

    size_t i    = 0;
    void  *item = NULL;

    bool result = hashmap_iter(g_map, &i, &item);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(item);

    /* Verify the pointer points into the map by checking the get result */
    const struct test_item *from_get =
        (const struct test_item *)hashmap_get(g_map, &ti);
    TEST_ASSERT_NOT_NULL(from_get);
    TEST_ASSERT_EQUAL_PTR(from_get, item);
}

/* Iterating twice from i=0 must yield the same set of items */
void test_iter_restart_from_zero(void) {
    const int N = 5;
    for (int k = 0; k < N; k++) {
        struct test_item ti = {k, k};
        hashmap_set(g_map, &ti);
    }

    int count1 = 0, count2 = 0;
    size_t i = 0;
    void  *item = NULL;

    while (hashmap_iter(g_map, &i, &item)) count1++;

    i    = 0;
    item = NULL;
    while (hashmap_iter(g_map, &i, &item)) count2++;

    TEST_ASSERT_EQUAL_INT(N, count1);
    TEST_ASSERT_EQUAL_INT(N, count2);
}

/* After deleting all items the iterator must return false */
void test_iter_after_clear(void) {
    struct test_item ti = {3, 30};
    hashmap_set(g_map, &ti);

    hashmap_clear(g_map, false);

    size_t i    = 0;
    void  *item = NULL;
    bool result = hashmap_iter(g_map, &i, &item);
    TEST_ASSERT_FALSE(result);
}

/* The cursor i must be advanced after each successful call */
void test_iter_cursor_advances(void) {
    const int N = 3;
    for (int k = 0; k < N; k++) {
        struct test_item ti = {k, k};
        hashmap_set(g_map, &ti);
    }

    size_t i_prev = 0;
    size_t i      = 0;
    void  *item   = NULL;

    while (hashmap_iter(g_map, &i, &item)) {
        TEST_ASSERT_GREATER_THAN(i_prev, i);
        i_prev = i;
    }
}

/* -------------------------------------------------------------------------
 * main
 * ------------------------------------------------------------------------- */

int main(void) {
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_iter_empty_map_returns_false);
    RUN_TEST(test_iter_returns_false_when_exhausted);
    RUN_TEST(test_iter_single_item);
    RUN_TEST(test_iter_visits_all_items);
    RUN_TEST(test_iter_large_map);
    RUN_TEST(test_iter_i_beyond_nbuckets_returns_false);
    RUN_TEST(test_iter_item_is_direct_reference);
    RUN_TEST(test_iter_restart_from_zero);
    RUN_TEST(test_iter_after_clear);
    RUN_TEST(test_iter_cursor_advances);
    return UNITY_END();
}