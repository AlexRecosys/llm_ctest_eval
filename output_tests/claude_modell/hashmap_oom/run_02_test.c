#include "hashmap.c"
#include "unity.h"
#include "hashmap.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

/* Fixture data */
static struct hashmap *map;

/* Minimal item type for testing */
typedef struct {
    int key;
    int value;
} test_item_t;

/* Hash callback */
static uint64_t test_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    const test_item_t *ti = (const test_item_t *)item;
    return hashmap_murmur(&ti->key, sizeof(ti->key), seed0, seed1);
}

/* Compare callback */
static int test_compare(const void *a, const void *b, void *udata) {
    (void)udata;
    const test_item_t *ta = (const test_item_t *)a;
    const test_item_t *tb = (const test_item_t *)b;
    return ta->key - tb->key;
}

/* setUp and tearDown */
void setUp(void) {
    map = hashmap_new(sizeof(test_item_t), 0, 0, 0,
                      test_hash, test_compare, NULL, NULL);
}

void tearDown(void) {
    if (map) {
        hashmap_free(map);
        map = NULL;
    }
}

/* ---- Test cases ---- */

/* A freshly created map should not be in OOM state */
static void test_hashmap_oom_false_on_new_map(void) {
    TEST_ASSERT_NOT_NULL(map);
    TEST_ASSERT_FALSE(hashmap_oom(map));
}

/* After normal set operations the OOM flag should remain false */
static void test_hashmap_oom_false_after_set(void) {
    TEST_ASSERT_NOT_NULL(map);
    test_item_t item = {1, 100};
    hashmap_set(map, &item);
    TEST_ASSERT_FALSE(hashmap_oom(map));
}

/* After multiple set operations the OOM flag should remain false */
static void test_hashmap_oom_false_after_multiple_sets(void) {
    TEST_ASSERT_NOT_NULL(map);
    for (int i = 0; i < 64; i++) {
        test_item_t item = {i, i * 2};
        hashmap_set(map, &item);
    }
    TEST_ASSERT_FALSE(hashmap_oom(map));
}

/* After a get on an existing key the OOM flag should remain false */
static void test_hashmap_oom_false_after_get(void) {
    TEST_ASSERT_NOT_NULL(map);
    test_item_t item = {42, 999};
    hashmap_set(map, &item);
    test_item_t key = {42, 0};
    hashmap_get(map, &key);
    TEST_ASSERT_FALSE(hashmap_oom(map));
}

/* After a delete operation the OOM flag should remain false */
static void test_hashmap_oom_false_after_delete(void) {
    TEST_ASSERT_NOT_NULL(map);
    test_item_t item = {7, 77};
    hashmap_set(map, &item);
    test_item_t key = {7, 0};
    hashmap_delete(map, &key);
    TEST_ASSERT_FALSE(hashmap_oom(map));
}

/* Directly set the oom field to true and verify hashmap_oom returns true */
static void test_hashmap_oom_true_when_field_set(void) {
    TEST_ASSERT_NOT_NULL(map);
    map->oom = true;
    TEST_ASSERT_TRUE(hashmap_oom(map));
}

/* Directly set the oom field to false and verify hashmap_oom returns false */
static void test_hashmap_oom_false_when_field_cleared(void) {
    TEST_ASSERT_NOT_NULL(map);
    map->oom = true;
    map->oom = false;
    TEST_ASSERT_FALSE(hashmap_oom(map));
}

/* Toggle oom field multiple times and verify each state */
static void test_hashmap_oom_toggle(void) {
    TEST_ASSERT_NOT_NULL(map);

    map->oom = false;
    TEST_ASSERT_FALSE(hashmap_oom(map));

    map->oom = true;
    TEST_ASSERT_TRUE(hashmap_oom(map));

    map->oom = false;
    TEST_ASSERT_FALSE(hashmap_oom(map));

    map->oom = true;
    TEST_ASSERT_TRUE(hashmap_oom(map));
}

/* Verify return type is bool: true maps to 1 */
static void test_hashmap_oom_returns_bool_true(void) {
    TEST_ASSERT_NOT_NULL(map);
    map->oom = true;
    bool result = hashmap_oom(map);
    TEST_ASSERT_EQUAL_INT(1, (int)result);
}

/* Verify return type is bool: false maps to 0 */
static void test_hashmap_oom_returns_bool_false(void) {
    TEST_ASSERT_NOT_NULL(map);
    map->oom = false;
    bool result = hashmap_oom(map);
    TEST_ASSERT_EQUAL_INT(0, (int)result);
}

/* A second independent map should also start with oom == false */
static void test_hashmap_oom_independent_maps(void) {
    struct hashmap *map2 = hashmap_new(sizeof(test_item_t), 0, 0, 0,
                                       test_hash, test_compare, NULL, NULL);
    TEST_ASSERT_NOT_NULL(map2);

    /* Set oom on the first map */
    map->oom = true;

    /* Second map should be unaffected */
    TEST_ASSERT_FALSE(hashmap_oom(map2));
    TEST_ASSERT_TRUE(hashmap_oom(map));

    hashmap_free(map2);
}

int main(void) {
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_hashmap_oom_false_on_new_map);
    RUN_TEST(test_hashmap_oom_false_after_set);
    RUN_TEST(test_hashmap_oom_false_after_multiple_sets);
    RUN_TEST(test_hashmap_oom_false_after_get);
    RUN_TEST(test_hashmap_oom_false_after_delete);
    RUN_TEST(test_hashmap_oom_true_when_field_set);
    RUN_TEST(test_hashmap_oom_false_when_field_cleared);
    RUN_TEST(test_hashmap_oom_toggle);
    RUN_TEST(test_hashmap_oom_returns_bool_true);
    RUN_TEST(test_hashmap_oom_returns_bool_false);
    RUN_TEST(test_hashmap_oom_independent_maps);
    return UNITY_END();
}