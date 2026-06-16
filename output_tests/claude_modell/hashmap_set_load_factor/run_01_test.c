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
    if (map) {
        hashmap_free(map);
        map = NULL;
    }
}

/* ── test cases ───────────────────────────────────────────────────────────── */

/* Setting a normal valid load factor (0.75) should be stored correctly */
void test_set_load_factor_normal(void) {
    hashmap_set_load_factor(map, 0.75);
    /* loadfactor is stored as factor * 100, so expect 75 */
    TEST_ASSERT_EQUAL_INT(75, map->loadfactor);
}

/* growat must equal nbuckets * (loadfactor / 100.0) after the call */
void test_set_load_factor_growat_updated(void) {
    hashmap_set_load_factor(map, 0.75);
    size_t expected_growat = (size_t)(map->nbuckets * (map->loadfactor / 100.0));
    TEST_ASSERT_EQUAL_UINT(expected_growat, map->growat);
}

/* Setting load factor to 0.5 */
void test_set_load_factor_half(void) {
    hashmap_set_load_factor(map, 0.50);
    TEST_ASSERT_EQUAL_INT(50, map->loadfactor);
    size_t expected_growat = (size_t)(map->nbuckets * (map->loadfactor / 100.0));
    TEST_ASSERT_EQUAL_UINT(expected_growat, map->growat);
}

/* Setting load factor to 0.9 */
void test_set_load_factor_high(void) {
    hashmap_set_load_factor(map, 0.90);
    TEST_ASSERT_EQUAL_INT(90, map->loadfactor);
    size_t expected_growat = (size_t)(map->nbuckets * (map->loadfactor / 100.0));
    TEST_ASSERT_EQUAL_UINT(expected_growat, map->growat);
}

/* A value above 1.0 should be clamped to the maximum allowed by clamp_load_factor */
void test_set_load_factor_above_one_clamped(void) {
    /* Store the current loadfactor before the call */
    int before = map->loadfactor;
    hashmap_set_load_factor(map, 2.0);
    /* After clamping, loadfactor must not exceed 100 (i.e. 1.0 * 100) */
    TEST_ASSERT_TRUE(map->loadfactor <= 100);
    /* growat must still be consistent */
    size_t expected_growat = (size_t)(map->nbuckets * (map->loadfactor / 100.0));
    TEST_ASSERT_EQUAL_UINT(expected_growat, map->growat);
    (void)before;
}

/* A value of 0.0 or negative should be clamped to at least the current loadfactor */
void test_set_load_factor_zero_clamped(void) {
    int before = map->loadfactor;
    hashmap_set_load_factor(map, 0.0);
    /* clamp_load_factor should keep it at least at the previous value */
    TEST_ASSERT_TRUE(map->loadfactor >= before);
    size_t expected_growat = (size_t)(map->nbuckets * (map->loadfactor / 100.0));
    TEST_ASSERT_EQUAL_UINT(expected_growat, map->growat);
}

/* Negative factor should be clamped similarly to zero */
void test_set_load_factor_negative_clamped(void) {
    int before = map->loadfactor;
    hashmap_set_load_factor(map, -1.0);
    TEST_ASSERT_TRUE(map->loadfactor >= before);
    size_t expected_growat = (size_t)(map->nbuckets * (map->loadfactor / 100.0));
    TEST_ASSERT_EQUAL_UINT(expected_growat, map->growat);
}

/* Calling set_load_factor twice: second call should overwrite the first */
void test_set_load_factor_twice(void) {
    hashmap_set_load_factor(map, 0.60);
    TEST_ASSERT_EQUAL_INT(60, map->loadfactor);

    hashmap_set_load_factor(map, 0.80);
    TEST_ASSERT_EQUAL_INT(80, map->loadfactor);

    size_t expected_growat = (size_t)(map->nbuckets * (map->loadfactor / 100.0));
    TEST_ASSERT_EQUAL_UINT(expected_growat, map->growat);
}

/* growat must be consistent with nbuckets after setting load factor */
void test_set_load_factor_growat_consistent_with_nbuckets(void) {
    hashmap_set_load_factor(map, 0.75);
    double ratio = (double)map->growat / (double)map->nbuckets;
    /* ratio should be close to loadfactor/100 */
    TEST_ASSERT_DOUBLE_WITHIN(0.01, map->loadfactor / 100.0, ratio);
}

/* Setting load factor to exactly 1.0 — boundary test */
void test_set_load_factor_exactly_one(void) {
    hashmap_set_load_factor(map, 1.0);
    /* loadfactor stored as integer percentage; may be clamped to <100 */
    TEST_ASSERT_TRUE(map->loadfactor <= 100);
    TEST_ASSERT_TRUE(map->loadfactor > 0);
    size_t expected_growat = (size_t)(map->nbuckets * (map->loadfactor / 100.0));
    TEST_ASSERT_EQUAL_UINT(expected_growat, map->growat);
}

/* After inserting items and resizing, set_load_factor still keeps growat consistent */
void test_set_load_factor_after_inserts(void) {
    /* Insert enough items to trigger at least one resize */
    for (int i = 0; i < 64; i++) {
        item_t it = { .key = i, .value = i * 2 };
        hashmap_set(map, &it);
    }
    hashmap_set_load_factor(map, 0.75);
    size_t expected_growat = (size_t)(map->nbuckets * (map->loadfactor / 100.0));
    TEST_ASSERT_EQUAL_UINT(expected_growat, map->growat);
    TEST_ASSERT_EQUAL_INT(75, map->loadfactor);
}

/* ── main ─────────────────────────────────────────────────────────────────── */

int main(void) {
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_set_load_factor_normal);
    RUN_TEST(test_set_load_factor_growat_updated);
    RUN_TEST(test_set_load_factor_half);
    RUN_TEST(test_set_load_factor_high);
    RUN_TEST(test_set_load_factor_above_one_clamped);
    RUN_TEST(test_set_load_factor_zero_clamped);
    RUN_TEST(test_set_load_factor_negative_clamped);
    RUN_TEST(test_set_load_factor_twice);
    RUN_TEST(test_set_load_factor_growat_consistent_with_nbuckets);
    RUN_TEST(test_set_load_factor_exactly_one);
    RUN_TEST(test_set_load_factor_after_inserts);
    return UNITY_END();
}