#include "hashmap.c"
#include "unity.h"
#include "hashmap.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* ── fixtures ─────────────────────────────────────────────────────────────── */

struct item {
    int   key;
    int   value;
};

static struct hashmap *g_map;

/* ── helpers ──────────────────────────────────────────────────────────────── */

static uint64_t item_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    const struct item *it = (const struct item *)item;
    return hashmap_murmur(&it->key, sizeof(it->key), seed0, seed1);
}

static int item_compare(const void *a, const void *b, void *udata) {
    (void)udata;
    const struct item *ia = (const struct item *)a;
    const struct item *ib = (const struct item *)b;
    return ia->key - ib->key;
}

static struct hashmap *make_map(size_t cap) {
    return hashmap_new(sizeof(struct item), cap, 0, 0,
                       item_hash, item_compare, NULL, NULL);
}

static void populate_map(struct hashmap *map, int n) {
    for (int i = 0; i < n; i++) {
        struct item it = { .key = i, .value = i * 10 };
        hashmap_set(map, &it);
    }
}

/* ── setUp / tearDown ─────────────────────────────────────────────────────── */

void setUp(void) {
    g_map = make_map(16);
    TEST_ASSERT_NOT_NULL(g_map);
}

void tearDown(void) {
    if (g_map) {
        hashmap_free(g_map);
        g_map = NULL;
    }
}

/* ── test cases ───────────────────────────────────────────────────────────── */

/* After clear the count must be zero */
void test_clear_resets_count_to_zero(void) {
    populate_map(g_map, 20);
    TEST_ASSERT_EQUAL_UINT(20, hashmap_count(g_map));

    hashmap_clear(g_map, false);

    TEST_ASSERT_EQUAL_UINT(0, hashmap_count(g_map));
}

/* After clear with update_cap=false, previously inserted items are gone */
void test_clear_no_update_cap_items_not_found(void) {
    populate_map(g_map, 10);

    hashmap_clear(g_map, false);

    for (int i = 0; i < 10; i++) {
        struct item key = { .key = i };
        TEST_ASSERT_NULL(hashmap_get(g_map, &key));
    }
}

/* After clear with update_cap=true, previously inserted items are gone */
void test_clear_update_cap_items_not_found(void) {
    populate_map(g_map, 10);

    hashmap_clear(g_map, true);

    for (int i = 0; i < 10; i++) {
        struct item key = { .key = i };
        TEST_ASSERT_NULL(hashmap_get(g_map, &key));
    }
}

/* Map is usable (set + get) after clear with update_cap=false */
void test_clear_no_update_cap_map_still_usable(void) {
    populate_map(g_map, 5);
    hashmap_clear(g_map, false);

    struct item it = { .key = 42, .value = 420 };
    hashmap_set(g_map, &it);

    const struct item *found = hashmap_get(g_map, &it);
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_INT(420, found->value);
    TEST_ASSERT_EQUAL_UINT(1, hashmap_count(g_map));
}

/* Map is usable (set + get) after clear with update_cap=true */
void test_clear_update_cap_map_still_usable(void) {
    populate_map(g_map, 5);
    hashmap_clear(g_map, true);

    struct item it = { .key = 99, .value = 990 };
    hashmap_set(g_map, &it);

    const struct item *found = hashmap_get(g_map, &it);
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_INT(990, found->value);
    TEST_ASSERT_EQUAL_UINT(1, hashmap_count(g_map));
}

/* Clearing an already-empty map must not crash and count stays 0 */
void test_clear_empty_map_no_crash(void) {
    TEST_ASSERT_EQUAL_UINT(0, hashmap_count(g_map));

    hashmap_clear(g_map, false);

    TEST_ASSERT_EQUAL_UINT(0, hashmap_count(g_map));
}

/* Clearing an already-empty map with update_cap=true must not crash */
void test_clear_empty_map_update_cap_no_crash(void) {
    TEST_ASSERT_EQUAL_UINT(0, hashmap_count(g_map));

    hashmap_clear(g_map, true);

    TEST_ASSERT_EQUAL_UINT(0, hashmap_count(g_map));
}

/* After clear with update_cap=true, nbuckets equals cap (stored in map).
   We verify indirectly: the map must accept new insertions without error. */
void test_clear_update_cap_nbuckets_equals_cap(void) {
    populate_map(g_map, 8);
    hashmap_clear(g_map, true);

    /* cap is now whatever nbuckets was; inserting many items should still work */
    for (int i = 0; i < 50; i++) {
        struct item it = { .key = i, .value = i };
        hashmap_set(g_map, &it);
    }
    TEST_ASSERT_EQUAL_UINT(50, hashmap_count(g_map));
}

/* Multiple consecutive clears work correctly */
void test_multiple_clears_work_correctly(void) {
    for (int round = 0; round < 3; round++) {
        populate_map(g_map, 15);
        TEST_ASSERT_EQUAL_UINT(15, hashmap_count(g_map));

        hashmap_clear(g_map, false);
        TEST_ASSERT_EQUAL_UINT(0, hashmap_count(g_map));
    }
}

/* After clear, mask is consistent with nbuckets (power-of-two property) */
void test_clear_mask_consistent_with_nbuckets(void) {
    populate_map(g_map, 10);
    hashmap_clear(g_map, false);

    /* Insert and retrieve to exercise the mask; if mask were wrong the
       lookup would fail or crash. */
    struct item it = { .key = 7, .value = 77 };
    hashmap_set(g_map, &it);
    const struct item *found = hashmap_get(g_map, &it);
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_INT(77, found->value);
}

/* Clear with update_cap=false on a map that has grown beyond initial cap
   should resize buckets back to cap and still work */
void test_clear_no_update_cap_after_growth(void) {
    /* Force the map to grow by inserting many items */
    populate_map(g_map, 100);
    TEST_ASSERT_EQUAL_UINT(100, hashmap_count(g_map));

    hashmap_clear(g_map, false);
    TEST_ASSERT_EQUAL_UINT(0, hashmap_count(g_map));

    /* Map must still be functional */
    struct item it = { .key = 1000, .value = 9999 };
    hashmap_set(g_map, &it);
    const struct item *found = hashmap_get(g_map, &it);
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_INT(9999, found->value);
}

/* Verify growat is positive after clear (map can grow again) */
void test_clear_growat_is_positive(void) {
    populate_map(g_map, 10);
    hashmap_clear(g_map, false);

    /* If growat were 0 the map would try to grow on every insert.
       We verify by inserting items without crashing. */
    for (int i = 0; i < 30; i++) {
        struct item it = { .key = i, .value = i };
        hashmap_set(g_map, &it);
    }
    TEST_ASSERT_EQUAL_UINT(30, hashmap_count(g_map));
}

/* Clear with a map created with explicit capacity hint */
void test_clear_map_with_explicit_capacity(void) {
    struct hashmap *m = make_map(64);
    TEST_ASSERT_NOT_NULL(m);

    populate_map(m, 30);
    hashmap_clear(m, true);
    TEST_ASSERT_EQUAL_UINT(0, hashmap_count(m));

    struct item it = { .key = 5, .value = 55 };
    hashmap_set(m, &it);
    const struct item *found = hashmap_get(m, &it);
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_INT(55, found->value);

    hashmap_free(m);
}

/* ── main ─────────────────────────────────────────────────────────────────── */

int main(void) {
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_clear_resets_count_to_zero);
    RUN_TEST(test_clear_no_update_cap_items_not_found);
    RUN_TEST(test_clear_update_cap_items_not_found);
    RUN_TEST(test_clear_no_update_cap_map_still_usable);
    RUN_TEST(test_clear_update_cap_map_still_usable);
    RUN_TEST(test_clear_empty_map_no_crash);
    RUN_TEST(test_clear_empty_map_update_cap_no_crash);
    RUN_TEST(test_clear_update_cap_nbuckets_equals_cap);
    RUN_TEST(test_multiple_clears_work_correctly);
    RUN_TEST(test_clear_mask_consistent_with_nbuckets);
    RUN_TEST(test_clear_no_update_cap_after_growth);
    RUN_TEST(test_clear_growat_is_positive);
    RUN_TEST(test_clear_map_with_explicit_capacity);
    return UNITY_END();
}