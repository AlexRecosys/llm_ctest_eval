#include "hashmap.c"
#include "unity.h"
#include "hashmap.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

/* ── fixtures ─────────────────────────────────────────────────────────────── */

typedef struct {
    int   key;
    int   value;
} item_t;

static struct hashmap *g_map;

/* ── helpers ──────────────────────────────────────────────────────────────── */

static uint64_t item_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    const item_t *it = (const item_t *)item;
    return hashmap_murmur(&it->key, sizeof(it->key), seed0, seed1);
}

static int item_compare(const void *a, const void *b, void *udata) {
    (void)udata;
    return ((const item_t *)a)->key - ((const item_t *)b)->key;
}

static struct hashmap *create_map(void) {
    return hashmap_new(sizeof(item_t), 0, 0, 0,
                       item_hash, item_compare, NULL, NULL);
}

/* ── setUp / tearDown ─────────────────────────────────────────────────────── */

void setUp(void) {
    g_map = create_map();
    TEST_ASSERT_NOT_NULL(g_map);
}

void tearDown(void) {
    hashmap_free(g_map);
    g_map = NULL;
}

/* ── test cases ───────────────────────────────────────────────────────────── */

/* Probing any position in an empty map must return NULL */
void test_probe_empty_map_returns_null(void) {
    const void *result = hashmap_probe(g_map, 0);
    TEST_ASSERT_NULL(result);
}

/* Probing every position in an empty map returns NULL */
void test_probe_all_positions_empty_map(void) {
    uint64_t cap = (uint64_t)(g_map->mask + 1);
    for (uint64_t i = 0; i < cap; i++) {
        TEST_ASSERT_NULL_MESSAGE(hashmap_probe(g_map, i),
                                 "expected NULL for empty bucket");
    }
}

/* After inserting one item, at least one position must return non-NULL */
void test_probe_finds_inserted_item(void) {
    item_t it = {.key = 42, .value = 100};
    hashmap_set(g_map, &it);

    int found = 0;
    uint64_t cap = (uint64_t)(g_map->mask + 1);
    for (uint64_t i = 0; i < cap; i++) {
        const void *r = hashmap_probe(g_map, i);
        if (r != NULL) {
            const item_t *got = (const item_t *)r;
            TEST_ASSERT_EQUAL_INT(42,  got->key);
            TEST_ASSERT_EQUAL_INT(100, got->value);
            found++;
        }
    }
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, found, "exactly one occupied bucket expected");
}

/* The returned pointer content matches what was inserted */
void test_probe_returned_item_content_correct(void) {
    item_t it = {.key = 7, .value = 77};
    hashmap_set(g_map, &it);

    uint64_t cap = (uint64_t)(g_map->mask + 1);
    const item_t *found = NULL;
    for (uint64_t i = 0; i < cap; i++) {
        const void *r = hashmap_probe(g_map, i);
        if (r) { found = (const item_t *)r; break; }
    }
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_INT(7,  found->key);
    TEST_ASSERT_EQUAL_INT(77, found->value);
}

/* Multiple items: occupied bucket count equals hashmap_count */
void test_probe_occupied_count_matches_hashmap_count(void) {
    int n = 10;
    for (int i = 0; i < n; i++) {
        item_t it = {.key = i * 3, .value = i};
        hashmap_set(g_map, &it);
    }

    int occupied = 0;
    uint64_t cap = (uint64_t)(g_map->mask + 1);
    for (uint64_t i = 0; i < cap; i++) {
        if (hashmap_probe(g_map, i) != NULL) {
            occupied++;
        }
    }
    TEST_ASSERT_EQUAL_INT((int)hashmap_count(g_map), occupied);
}

/* Probing with position larger than mask wraps correctly (mask applied) */
void test_probe_position_wraps_with_mask(void) {
    /* position 0 and position (mask+1) should refer to the same bucket */
    const void *r0   = hashmap_probe(g_map, 0);
    const void *rwrap = hashmap_probe(g_map, (uint64_t)(g_map->mask + 1));
    TEST_ASSERT_EQUAL_PTR(r0, rwrap);
}

/* After deleting the only item, all probes return NULL again */
void test_probe_after_delete_returns_null(void) {
    item_t it = {.key = 55, .value = 5};
    hashmap_set(g_map, &it);
    hashmap_delete(g_map, &it);

    uint64_t cap = (uint64_t)(g_map->mask + 1);
    for (uint64_t i = 0; i < cap; i++) {
        TEST_ASSERT_NULL_MESSAGE(hashmap_probe(g_map, i),
                                 "expected NULL after deletion");
    }
}

/* Probe returns NULL for positions that are genuinely empty even when
   other positions are occupied */
void test_probe_empty_bucket_among_occupied(void) {
    /* Insert several items then verify that at least one position is still
       empty (returns NULL) — valid because load factor < 1 */
    for (int i = 0; i < 5; i++) {
        item_t it = {.key = i + 1, .value = i};
        hashmap_set(g_map, &it);
    }

    int empty_found = 0;
    uint64_t cap = (uint64_t)(g_map->mask + 1);
    for (uint64_t i = 0; i < cap; i++) {
        if (hashmap_probe(g_map, i) == NULL) {
            empty_found++;
        }
    }
    TEST_ASSERT_TRUE(empty_found > 0);
}

/* Probe on a freshly-grown map (trigger resize) still works */
void test_probe_after_resize(void) {
    /* Insert enough items to force at least one resize */
    int n = 200;
    for (int i = 0; i < n; i++) {
        item_t it = {.key = i, .value = i * 2};
        hashmap_set(g_map, &it);
    }
    TEST_ASSERT_EQUAL_INT(n, (int)hashmap_count(g_map));

    int occupied = 0;
    uint64_t cap = (uint64_t)(g_map->mask + 1);
    for (uint64_t i = 0; i < cap; i++) {
        if (hashmap_probe(g_map, i) != NULL) {
            occupied++;
        }
    }
    TEST_ASSERT_EQUAL_INT(n, occupied);
}

/* ── main ─────────────────────────────────────────────────────────────────── */

int main(void) {
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_probe_empty_map_returns_null);
    RUN_TEST(test_probe_all_positions_empty_map);
    RUN_TEST(test_probe_finds_inserted_item);
    RUN_TEST(test_probe_returned_item_content_correct);
    RUN_TEST(test_probe_occupied_count_matches_hashmap_count);
    RUN_TEST(test_probe_position_wraps_with_mask);
    RUN_TEST(test_probe_after_delete_returns_null);
    RUN_TEST(test_probe_empty_bucket_among_occupied);
    RUN_TEST(test_probe_after_resize);
    return UNITY_END();
}