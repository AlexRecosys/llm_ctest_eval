#include "hashmap.c"
#include "unity.h"
#include "hashmap.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* ── fixtures ─────────────────────────────────────────────────────────────── */

static struct hashmap *map;

/* ── helpers ──────────────────────────────────────────────────────────────── */

typedef struct {
    int key;
    int value;
} item_t;

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
    map = create_map();
    TEST_ASSERT_NOT_NULL(map);
}

void tearDown(void) {
    hashmap_free(map);
    map = NULL;
}

/* ── test cases ───────────────────────────────────────────────────────────── */

/* Setting a normal load factor (0.75) should be stored as 75 in loadfactor
   and growat should equal nbuckets * 0.75. */
void test_set_load_factor_normal(void) {
    hashmap_set_load_factor(map, 0.75);
    TEST_ASSERT_EQUAL_INT(75, map->loadfactor);
    size_t expected_growat = (size_t)(map->nbuckets * 0.75);
    TEST_ASSERT_EQUAL_UINT(expected_growat, map->growat);
}

/* Setting load factor to 0.5 */
void test_set_load_factor_half(void) {
    hashmap_set_load_factor(map, 0.5);
    TEST_ASSERT_EQUAL_INT(50, map->loadfactor);
    size_t expected_growat = (size_t)(map->nbuckets * 0.5);
    TEST_ASSERT_EQUAL_UINT(expected_growat, map->growat);
}

/* Setting load factor to 0.9 */
void test_set_load_factor_high(void) {
    hashmap_set_load_factor(map, 0.9);
    TEST_ASSERT_EQUAL_INT(90, map->loadfactor);
    size_t expected_growat = (size_t)(map->nbuckets * 0.9);
    TEST_ASSERT_EQUAL_UINT(expected_growat, map->growat);
}

/* A value > 1.0 should be clamped to the maximum allowed by clamp_load_factor.
   We verify that loadfactor does not exceed 100 (i.e. 1.0). */
void test_set_load_factor_above_one_clamped(void) {
    hashmap_set_load_factor(map, 2.0);
    /* clamp_load_factor will cap it; loadfactor must be <= 100 */
    TEST_ASSERT_TRUE(map->loadfactor <= 100);
    TEST_ASSERT_TRUE(map->loadfactor > 0);
}

/* A value <= 0 should be clamped to at least the current load factor
   (or the minimum). The result must be positive. */
void test_set_load_factor_zero_clamped(void) {
    hashmap_set_load_factor(map, 0.0);
    TEST_ASSERT_TRUE(map->loadfactor > 0);
}

/* A negative value should also be clamped to a positive result. */
void test_set_load_factor_negative_clamped(void) {
    hashmap_set_load_factor(map, -1.0);
    TEST_ASSERT_TRUE(map->loadfactor > 0);
}

/* growat must always be consistent with nbuckets and loadfactor after the call. */
void test_growat_consistent_with_nbuckets_and_loadfactor(void) {
    hashmap_set_load_factor(map, 0.6);
    size_t expected = (size_t)(map->nbuckets * (map->loadfactor / 100.0));
    TEST_ASSERT_EQUAL_UINT(expected, map->growat);
}

/* Calling set_load_factor twice: second call should override the first. */
void test_set_load_factor_twice(void) {
    hashmap_set_load_factor(map, 0.5);
    TEST_ASSERT_EQUAL_INT(50, map->loadfactor);

    hashmap_set_load_factor(map, 0.8);
    TEST_ASSERT_EQUAL_INT(80, map->loadfactor);
    size_t expected_growat = (size_t)(map->nbuckets * 0.8);
    TEST_ASSERT_EQUAL_UINT(expected_growat, map->growat);
}

/* After setting load factor, inserting items should still work correctly. */
void test_set_load_factor_then_insert(void) {
    hashmap_set_load_factor(map, 0.75);
    item_t it = {42, 100};
    hashmap_set(map, &it);
    item_t key = {42, 0};
    const item_t *found = (const item_t *)hashmap_get(map, &key);
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_INT(100, found->value);
}

/* growat must be <= nbuckets (cannot grow at more than 100% capacity). */
void test_growat_not_exceeding_nbuckets(void) {
    hashmap_set_load_factor(map, 0.99);
    TEST_ASSERT_TRUE(map->growat <= map->nbuckets);
}

/* loadfactor field is stored as integer percentage (factor * 100). */
void test_loadfactor_stored_as_percentage(void) {
    hashmap_set_load_factor(map, 0.65);
    TEST_ASSERT_EQUAL_INT(65, map->loadfactor);
}

/* ── main ─────────────────────────────────────────────────────────────────── */

int main(void) {
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_set_load_factor_normal);
    RUN_TEST(test_set_load_factor_half);
    RUN_TEST(test_set_load_factor_high);
    RUN_TEST(test_set_load_factor_above_one_clamped);
    RUN_TEST(test_set_load_factor_zero_clamped);
    RUN_TEST(test_set_load_factor_negative_clamped);
    RUN_TEST(test_growat_consistent_with_nbuckets_and_loadfactor);
    RUN_TEST(test_set_load_factor_twice);
    RUN_TEST(test_set_load_factor_then_insert);
    RUN_TEST(test_growat_not_exceeding_nbuckets);
    RUN_TEST(test_loadfactor_stored_as_percentage);
    return UNITY_END();
}