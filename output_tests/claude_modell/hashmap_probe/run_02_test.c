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
    const item_t *ia = (const item_t *)a;
    const item_t *ib = (const item_t *)b;
    return ia->key - ib->key;
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
    for (uint64_t pos = 0; pos < 16; pos++) {
        const void *result = hashmap_probe(g_map, pos);
        TEST_ASSERT_NULL_MESSAGE(result, "Expected NULL for empty map probe");
    }
}

/* After inserting one item, at least one position must be non-NULL */
void test_probe_after_single_insert_finds_item(void) {
    item_t it = { .key = 42, .value = 100 };
    hashmap_set(g_map, &it);

    int found = 0;
    uint64_t cap = (uint64_t)(g_map->mask + 1);
    for (uint64_t pos = 0; pos < cap; pos++) {
        const void *result = hashmap_probe(g_map, pos);
        if (result != NULL) {
            const item_t *ret = (const item_t *)result;
            TEST_ASSERT_EQUAL_INT(42,  ret->key);
            TEST_ASSERT_EQUAL_INT(100, ret->value);
            found++;
        }
    }
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, found, "Expected exactly one occupied bucket");
}

/* Probe returns the correct item pointer (same content as inserted) */
void test_probe_returns_correct_item_content(void) {
    item_t it = { .key = 7, .value = 77 };
    hashmap_set(g_map, &it);

    uint64_t cap = (uint64_t)(g_map->mask + 1);
    const item_t *found_item = NULL;
    for (uint64_t pos = 0; pos < cap; pos++) {
        const void *result = hashmap_probe(g_map, pos);
        if (result != NULL) {
            found_item = (const item_t *)result;
            break;
        }
    }
    TEST_ASSERT_NOT_NULL(found_item);
    TEST_ASSERT_EQUAL_INT(7,  found_item->key);
    TEST_ASSERT_EQUAL_INT(77, found_item->value);
}

/* After inserting N items, the number of non-NULL probes equals N */
void test_probe_occupied_count_matches_count(void) {
    int n = 8;
    for (int i = 0; i < n; i++) {
        item_t it = { .key = i * 3 + 1, .value = i };
        hashmap_set(g_map, &it);
    }

    uint64_t cap = (uint64_t)(g_map->mask + 1);
    int occupied = 0;
    for (uint64_t pos = 0; pos < cap; pos++) {
        if (hashmap_probe(g_map, pos) != NULL) {
            occupied++;
        }
    }
    TEST_ASSERT_EQUAL_INT((int)hashmap_count(g_map), occupied);
}

/* Probing a position that wraps around (position >= capacity) uses mask */
void test_probe_position_wraps_with_mask(void) {
    item_t it = { .key = 5, .value = 55 };
    hashmap_set(g_map, &it);

    uint64_t cap = (uint64_t)(g_map->mask + 1);

    /* probe(pos) and probe(pos + cap) must agree */
    for (uint64_t pos = 0; pos < cap; pos++) {
        const void *r1 = hashmap_probe(g_map, pos);
        const void *r2 = hashmap_probe(g_map, pos + cap);
        if (r1 == NULL) {
            TEST_ASSERT_NULL(r2);
        } else {
            TEST_ASSERT_NOT_NULL(r2);
            const item_t *i1 = (const item_t *)r1;
            const item_t *i2 = (const item_t *)r2;
            TEST_ASSERT_EQUAL_INT(i1->key,   i2->key);
            TEST_ASSERT_EQUAL_INT(i1->value, i2->value);
        }
    }
}

/* After deleting the only item, all probes return NULL */
void test_probe_after_delete_returns_null(void) {
    item_t it = { .key = 99, .value = 999 };
    hashmap_set(g_map, &it);
    hashmap_delete(g_map, &it);

    uint64_t cap = (uint64_t)(g_map->mask + 1);
    for (uint64_t pos = 0; pos < cap; pos++) {
        TEST_ASSERT_NULL(hashmap_probe(g_map, pos));
    }
}

/* Probe returns a pointer into the map's internal storage (not a copy) */
void test_probe_returns_pointer_inside_map(void) {
    item_t it = { .key = 11, .value = 22 };
    hashmap_set(g_map, &it);

    uint64_t cap = (uint64_t)(g_map->mask + 1);
    const void *found = NULL;
    for (uint64_t pos = 0; pos < cap; pos++) {
        found = hashmap_probe(g_map, pos);
        if (found != NULL) break;
    }
    TEST_ASSERT_NOT_NULL(found);

    /* The pointer returned by hashmap_get must equal the one from probe */
    const void *via_get = hashmap_get(g_map, &it);
    TEST_ASSERT_NOT_NULL(via_get);
    TEST_ASSERT_EQUAL_PTR(via_get, found);
}

/* Multiple items: every item inserted can be found via probe */
void test_probe_finds_all_inserted_items(void) {
    int n = 12;
    for (int i = 0; i < n; i++) {
        item_t it = { .key = i + 100, .value = i };
        hashmap_set(g_map, &it);
    }

    uint64_t cap = (uint64_t)(g_map->mask + 1);
    int keys_found[12] = {0};

    for (uint64_t pos = 0; pos < cap; pos++) {
        const void *result = hashmap_probe(g_map, pos);
        if (result != NULL) {
            const item_t *ret = (const item_t *)result;
            int idx = ret->key - 100;
            TEST_ASSERT_TRUE(idx >= 0 && idx < n);
            keys_found[idx]++;
        }
    }

    for (int i = 0; i < n; i++) {
        TEST_ASSERT_EQUAL_INT_MESSAGE(1, keys_found[i],
            "Each inserted key must appear exactly once");
    }
}

/* Probe on position 0 of an empty map returns NULL */
void test_probe_position_zero_empty_map(void) {
    const void *result = hashmap_probe(g_map, 0);
    TEST_ASSERT_NULL(result);
}

/* ── main ─────────────────────────────────────────────────────────────────── */

int main(void) {
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_probe_empty_map_returns_null);
    RUN_TEST(test_probe_after_single_insert_finds_item);
    RUN_TEST(test_probe_returns_correct_item_content);
    RUN_TEST(test_probe_occupied_count_matches_count);
    RUN_TEST(test_probe_position_wraps_with_mask);
    RUN_TEST(test_probe_after_delete_returns_null);
    RUN_TEST(test_probe_returns_pointer_inside_map);
    RUN_TEST(test_probe_finds_all_inserted_items);
    RUN_TEST(test_probe_position_zero_empty_map);
    return UNITY_END();
}