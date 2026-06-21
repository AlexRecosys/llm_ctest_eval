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

/* After iterating an empty map, i must be >= nbuckets (loop exhausted) */
void test_iter_empty_map_advances_index_past_buckets(void) {
    size_t i = 0;
    void  *item = NULL;

    hashmap_iter(g_map, &i, &item);

    /* i must be > 0 because the loop increments before checking dib */
    TEST_ASSERT_GREATER_THAN(0, i);
}

/* Starting with i already past the bucket count returns false immediately */
void test_iter_with_i_beyond_nbuckets_returns_false(void) {
    size_t i = (size_t)-1; /* very large value */
    void  *item = NULL;

    bool result = hashmap_iter(g_map, &i, &item);

    TEST_ASSERT_FALSE(result);
}

/* Single item: first call returns true and yields the item */
void test_iter_single_item_first_call_returns_true(void) {
    int_item entry = {42, 100};
    hashmap_set(g_map, &entry);
    TEST_ASSERT_FALSE(hashmap_oom(g_map));

    size_t i = 0;
    void  *item = NULL;

    bool result = hashmap_iter(g_map, &i, &item);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(item);
}

/* Single item: the yielded item has the correct key and value */
void test_iter_single_item_correct_content(void) {
    int_item entry = {42, 100};
    hashmap_set(g_map, &entry);

    size_t i = 0;
    void  *item = NULL;

    bool result = hashmap_iter(g_map, &i, &item);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(item);

    int_item *retrieved = (int_item *)item;
    TEST_ASSERT_EQUAL_INT(42,  retrieved->key);
    TEST_ASSERT_EQUAL_INT(100, retrieved->value);
}

/* Single item: second call returns false (no more items) */
void test_iter_single_item_second_call_returns_false(void) {
    int_item entry = {7, 77};
    hashmap_set(g_map, &entry);

    size_t i = 0;
    void  *item = NULL;

    hashmap_iter(g_map, &i, &item); /* first: consumes the one item */
    bool result = hashmap_iter(g_map, &i, &item); /* second: exhausted */

    TEST_ASSERT_FALSE(result);
}

/* Multiple items: iteration count matches hashmap_count */
void test_iter_multiple_items_count_matches(void) {
    const int N = 20;
    for (int k = 0; k < N; k++) {
        int_item entry = {k, k * 2};
        hashmap_set(g_map, &entry);
    }
    TEST_ASSERT_EQUAL_UINT(N, hashmap_count(g_map));

    size_t i = 0;
    void  *item = NULL;
    int    count = 0;

    while (hashmap_iter(g_map, &i, &item)) {
        TEST_ASSERT_NOT_NULL(item);
        count++;
    }

    TEST_ASSERT_EQUAL_INT(N, count);
}

/* Multiple items: every inserted key is visited exactly once */
void test_iter_multiple_items_all_keys_visited(void) {
    const int N = 15;
    bool seen[15];
    memset(seen, 0, sizeof(seen));

    for (int k = 0; k < N; k++) {
        int_item entry = {k, k + 1};
        hashmap_set(g_map, &entry);
    }

    size_t i = 0;
    void  *item = NULL;

    while (hashmap_iter(g_map, &i, &item)) {
        int_item *it = (int_item *)item;
        TEST_ASSERT_TRUE_MESSAGE(it->key >= 0 && it->key < N,
                                 "key out of expected range");
        TEST_ASSERT_FALSE_MESSAGE(seen[it->key], "key visited more than once");
        seen[it->key] = true;
    }

    for (int k = 0; k < N; k++) {
        TEST_ASSERT_TRUE_MESSAGE(seen[k], "key was never visited");
    }
}

/* i is incremented on every call (monotonically non-decreasing) */
void test_iter_index_monotonically_increases(void) {
    const int N = 10;
    for (int k = 0; k < N; k++) {
        int_item entry = {k, k};
        hashmap_set(g_map, &entry);
    }

    size_t i = 0;
    size_t prev_i = 0;
    void  *item = NULL;

    while (hashmap_iter(g_map, &i, &item)) {
        TEST_ASSERT_GREATER_THAN(prev_i, i);
        prev_i = i;
    }
}

/* Restarting iteration (i=0) after full traversal yields same count */
void test_iter_restart_yields_same_count(void) {
    const int N = 8;
    for (int k = 0; k < N; k++) {
        int_item entry = {k * 3, k};
        hashmap_set(g_map, &entry);
    }

    /* First pass */
    size_t i = 0;
    void  *item = NULL;
    int    count1 = 0;
    while (hashmap_iter(g_map, &i, &item)) count1++;

    /* Second pass — reset i */
    i = 0;
    int count2 = 0;
    while (hashmap_iter(g_map, &i, &item)) count2++;

    TEST_ASSERT_EQUAL_INT(count1, count2);
}

/* item pointer points inside the map (not a copy) — modifying it is visible */
void test_iter_item_pointer_is_direct_reference(void) {
    int_item entry = {55, 0};
    hashmap_set(g_map, &entry);

    size_t i = 0;
    void  *item = NULL;

    bool result = hashmap_iter(g_map, &i, &item);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(item);

    /* Mutate through the pointer */
    ((int_item *)item)->value = 999;

    /* Re-fetch via hashmap_get and verify the mutation persisted */
    int_item key = {55, 0};
    const int_item *fetched = (const int_item *)hashmap_get(g_map, &key);
    TEST_ASSERT_NOT_NULL(fetched);
    TEST_ASSERT_EQUAL_INT(999, fetched->value);
}

/* Large map: iteration visits exactly hashmap_count items */
void test_iter_large_map_count(void) {
    const int N = 200;
    for (int k = 0; k < N; k++) {
        int_item entry = {k, k};
        hashmap_set(g_map, &entry);
    }
    TEST_ASSERT_EQUAL_UINT(N, hashmap_count(g_map));

    size_t i = 0;
    void  *item = NULL;
    int    count = 0;

    while (hashmap_iter(g_map, &i, &item)) {
        count++;
    }

    TEST_ASSERT_EQUAL_INT(N, count);
}

/* Map with explicit capacity hint: iteration still works correctly */
void test_iter_with_explicit_capacity(void) {
    struct hashmap *map = create_map(64);
    TEST_ASSERT_NOT_NULL(map);

    const int N = 30;
    for (int k = 0; k < N; k++) {
        int_item entry = {k, k * 5};
        hashmap_set(map, &entry);
    }

    size_t i = 0;
    void  *item = NULL;
    int    count = 0;

    while (hashmap_iter(map, &i, &item)) {
        count++;
    }

    TEST_ASSERT_EQUAL_INT(N, count);
    hashmap_free(map);
}

/* After clear, iteration on the same map returns false */
void test_iter_after_clear_returns_false(void) {
    int_item entry = {1, 2};
    hashmap_set(g_map, &entry);

    hashmap_clear(g_map, false);

    size_t i = 0;
    void  *item = NULL;

    bool result = hashmap_iter(g_map, &i, &item);
    TEST_ASSERT_FALSE(result);
}

/* -------------------------------------------------------------------------
 * main
 * ------------------------------------------------------------------------- */

int main(void) {
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_iter_empty_map_returns_false);
    RUN_TEST(test_iter_empty_map_advances_index_past_buckets);
    RUN_TEST(test_iter_with_i_beyond_nbuckets_returns_false);
    RUN_TEST(test_iter_single_item_first_call_returns_true);
    RUN_TEST(test_iter_single_item_correct_content);
    RUN_TEST(test_iter_single_item_second_call_returns_false);
    RUN_TEST(test_iter_multiple_items_count_matches);
    RUN_TEST(test_iter_multiple_items_all_keys_visited);
    RUN_TEST(test_iter_index_monotonically_increases);
    RUN_TEST(test_iter_restart_yields_same_count);
    RUN_TEST(test_iter_item_pointer_is_direct_reference);
    RUN_TEST(test_iter_large_map_count);
    RUN_TEST(test_iter_with_explicit_capacity);
    RUN_TEST(test_iter_after_clear_returns_false);
    return UNITY_END();
}