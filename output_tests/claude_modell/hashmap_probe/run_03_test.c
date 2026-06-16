#include "hashmap.c"
#include "unity.h"
#include "hashmap.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

/* -------------------------------------------------------------------------
 * Fixtures / file-scope variables
 * ---------------------------------------------------------------------- */

typedef struct {
    int key;
    int value;
} int_item;

static struct hashmap *g_map;

/* -------------------------------------------------------------------------
 * Helper functions
 * ---------------------------------------------------------------------- */

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

static struct hashmap *create_map(void) {
    return hashmap_new(sizeof(int_item), 0, 0, 0,
                       int_hash, int_compare, NULL, NULL);
}

/* -------------------------------------------------------------------------
 * setUp / tearDown
 * ---------------------------------------------------------------------- */

void setUp(void) {
    g_map = create_map();
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
 * ---------------------------------------------------------------------- */

/* Probing an empty map at position 0 should return NULL */
void test_probe_empty_map_returns_null(void) {
    const void *result = hashmap_probe(g_map, 0);
    TEST_ASSERT_NULL(result);
}

/* Probing an empty map at various positions should always return NULL */
void test_probe_empty_map_multiple_positions_return_null(void) {
    for (uint64_t pos = 0; pos < 16; pos++) {
        const void *result = hashmap_probe(g_map, pos);
        TEST_ASSERT_NULL_MESSAGE(result, "Expected NULL for empty map probe");
    }
}

/* After inserting one item, at least one position should be non-NULL */
void test_probe_after_single_insert_finds_item(void) {
    int_item item = {.key = 42, .value = 100};
    hashmap_set(g_map, &item);

    int found = 0;
    /* The map capacity after one insert is at least 1; scan all positions */
    size_t cap = hashmap_count(g_map) + 16; /* generous upper bound */
    for (uint64_t pos = 0; pos < (uint64_t)cap; pos++) {
        const void *result = hashmap_probe(g_map, pos);
        if (result != NULL) {
            const int_item *got = (const int_item *)result;
            if (got->key == 42 && got->value == 100) {
                found = 1;
                break;
            }
        }
    }
    TEST_ASSERT_TRUE(found);
}

/* Probing with position that wraps via mask should still work */
void test_probe_position_wraps_with_mask(void) {
    /* Insert several items so the map is non-trivially populated */
    for (int i = 0; i < 8; i++) {
        int_item item = {.key = i, .value = i * 10};
        hashmap_set(g_map, &item);
    }

    /* Use a very large position — it should be masked down safely */
    const void *result = hashmap_probe(g_map, UINT64_MAX);
    /* Result is either NULL or a valid pointer — just ensure no crash */
    (void)result; /* no assertion needed beyond "did not crash" */
    TEST_ASSERT_TRUE(1); /* reached here without crash */
}

/* After inserting multiple items, probing occupied slots returns non-NULL */
void test_probe_occupied_slots_return_non_null(void) {
    int inserted = 0;
    for (int i = 0; i < 16; i++) {
        int_item item = {.key = i, .value = i};
        hashmap_set(g_map, &item);
        inserted++;
    }

    int non_null_count = 0;
    for (uint64_t pos = 0; pos < 64; pos++) {
        const void *result = hashmap_probe(g_map, pos);
        if (result != NULL) {
            non_null_count++;
        }
    }
    TEST_ASSERT_EQUAL_INT(inserted, non_null_count);
}

/* Returned pointer content matches what was inserted */
void test_probe_returns_correct_item_content(void) {
    int_item item = {.key = 7, .value = 77};
    hashmap_set(g_map, &item);

    const int_item *found = NULL;
    for (uint64_t pos = 0; pos < 64; pos++) {
        const void *result = hashmap_probe(g_map, pos);
        if (result != NULL) {
            const int_item *candidate = (const int_item *)result;
            if (candidate->key == 7) {
                found = candidate;
                break;
            }
        }
    }
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_INT(7, found->key);
    TEST_ASSERT_EQUAL_INT(77, found->value);
}

/* After deleting an item, probing should not find it */
void test_probe_after_delete_does_not_find_item(void) {
    int_item item = {.key = 55, .value = 555};
    hashmap_set(g_map, &item);

    int_item key = {.key = 55};
    hashmap_delete(g_map, &key);

    int found = 0;
    for (uint64_t pos = 0; pos < 64; pos++) {
        const void *result = hashmap_probe(g_map, pos);
        if (result != NULL) {
            const int_item *candidate = (const int_item *)result;
            if (candidate->key == 55) {
                found = 1;
                break;
            }
        }
    }
    TEST_ASSERT_FALSE(found);
}

/* Probing position 0 on a freshly created map returns NULL */
void test_probe_position_zero_empty_map(void) {
    const void *result = hashmap_probe(g_map, 0);
    TEST_ASSERT_NULL(result);
}

/* Probing a large position on an empty map returns NULL */
void test_probe_large_position_empty_map(void) {
    const void *result = hashmap_probe(g_map, 1000000ULL);
    TEST_ASSERT_NULL(result);
}

/* Count of non-NULL probes equals hashmap_count */
void test_probe_non_null_count_equals_hashmap_count(void) {
    for (int i = 0; i < 10; i++) {
        int_item item = {.key = i * 3, .value = i};
        hashmap_set(g_map, &item);
    }

    size_t expected = hashmap_count(g_map);
    int non_null = 0;
    /* Scan enough positions to cover the internal capacity */
    for (uint64_t pos = 0; pos < 256; pos++) {
        if (hashmap_probe(g_map, pos) != NULL) {
            non_null++;
        }
    }
    TEST_ASSERT_EQUAL_UINT(expected, (size_t)non_null);
}

/* Probing same position twice returns consistent result */
void test_probe_same_position_consistent(void) {
    int_item item = {.key = 99, .value = 999};
    hashmap_set(g_map, &item);

    for (uint64_t pos = 0; pos < 64; pos++) {
        const void *r1 = hashmap_probe(g_map, pos);
        const void *r2 = hashmap_probe(g_map, pos);
        TEST_ASSERT_EQUAL_PTR(r1, r2);
    }
}

/* -------------------------------------------------------------------------
 * main
 * ---------------------------------------------------------------------- */

int main(void) {
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_probe_empty_map_returns_null);
    RUN_TEST(test_probe_empty_map_multiple_positions_return_null);
    RUN_TEST(test_probe_after_single_insert_finds_item);
    RUN_TEST(test_probe_position_wraps_with_mask);
    RUN_TEST(test_probe_occupied_slots_return_non_null);
    RUN_TEST(test_probe_returns_correct_item_content);
    RUN_TEST(test_probe_after_delete_does_not_find_item);
    RUN_TEST(test_probe_position_zero_empty_map);
    RUN_TEST(test_probe_large_position_empty_map);
    RUN_TEST(test_probe_non_null_count_equals_hashmap_count);
    RUN_TEST(test_probe_same_position_consistent);
    return UNITY_END();
}