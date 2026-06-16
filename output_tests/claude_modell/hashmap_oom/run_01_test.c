#include "hashmap.c"
#include "unity.h"
#include "hashmap.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

/* Fixture data */
static struct hashmap *map;

/* Helper: simple integer key item */
typedef struct {
    int key;
    int value;
} int_item;

static uint64_t int_item_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    const int_item *it = (const int_item *)item;
    return hashmap_murmur(&it->key, sizeof(it->key), seed0, seed1);
}

static int int_item_compare(const void *a, const void *b, void *udata) {
    (void)udata;
    const int_item *ia = (const int_item *)a;
    const int_item *ib = (const int_item *)b;
    return ia->key - ib->key;
}

void setUp(void) {
    map = hashmap_new(sizeof(int_item), 0, 0, 0,
                      int_item_hash, int_item_compare, NULL, NULL);
}

void tearDown(void) {
    if (map) {
        hashmap_free(map);
        map = NULL;
    }
}

/* Test: oom is false on a freshly created map */
static void test_hashmap_oom_false_on_new_map(void) {
    TEST_ASSERT_NOT_NULL(map);
    TEST_ASSERT_FALSE(hashmap_oom(map));
}

/* Test: oom remains false after a successful set */
static void test_hashmap_oom_false_after_successful_set(void) {
    TEST_ASSERT_NOT_NULL(map);
    int_item item = {1, 100};
    hashmap_set(map, &item);
    TEST_ASSERT_FALSE(hashmap_oom(map));
}

/* Test: oom remains false after multiple successful sets */
static void test_hashmap_oom_false_after_multiple_sets(void) {
    TEST_ASSERT_NOT_NULL(map);
    for (int i = 0; i < 64; i++) {
        int_item item = {i, i * 10};
        hashmap_set(map, &item);
        TEST_ASSERT_FALSE(hashmap_oom(map));
    }
}

/* Test: oom field reflects the actual map->oom value when set to true manually */
static void test_hashmap_oom_true_when_field_set(void) {
    TEST_ASSERT_NOT_NULL(map);
    /* Directly set the oom field to simulate an OOM condition */
    map->oom = true;
    TEST_ASSERT_TRUE(hashmap_oom(map));
}

/* Test: oom field reflects the actual map->oom value when set to false manually */
static void test_hashmap_oom_false_when_field_cleared(void) {
    TEST_ASSERT_NOT_NULL(map);
    map->oom = true;
    TEST_ASSERT_TRUE(hashmap_oom(map));
    map->oom = false;
    TEST_ASSERT_FALSE(hashmap_oom(map));
}

/* Test: oom returns false on a map created with a large initial capacity */
static void test_hashmap_oom_false_large_initial_capacity(void) {
    struct hashmap *large_map = hashmap_new(sizeof(int_item), 1024, 0, 0,
                                            int_item_hash, int_item_compare,
                                            NULL, NULL);
    TEST_ASSERT_NOT_NULL(large_map);
    TEST_ASSERT_FALSE(hashmap_oom(large_map));
    hashmap_free(large_map);
}

/* Test: oom returns false after clearing the map */
static void test_hashmap_oom_false_after_clear(void) {
    TEST_ASSERT_NOT_NULL(map);
    for (int i = 0; i < 10; i++) {
        int_item item = {i, i};
        hashmap_set(map, &item);
    }
    hashmap_clear(map, false);
    TEST_ASSERT_FALSE(hashmap_oom(map));
}

/* Test: oom returns false after deleting items */
static void test_hashmap_oom_false_after_delete(void) {
    TEST_ASSERT_NOT_NULL(map);
    int_item item = {42, 420};
    hashmap_set(map, &item);
    int_item key = {42, 0};
    hashmap_delete(map, &key);
    TEST_ASSERT_FALSE(hashmap_oom(map));
}

/* Test: oom returns true consistently when field is set */
static void test_hashmap_oom_consistent_true(void) {
    TEST_ASSERT_NOT_NULL(map);
    map->oom = true;
    TEST_ASSERT_TRUE(hashmap_oom(map));
    TEST_ASSERT_TRUE(hashmap_oom(map));
    TEST_ASSERT_TRUE(hashmap_oom(map));
}

/* Test: oom returns false consistently on fresh map */
static void test_hashmap_oom_consistent_false(void) {
    TEST_ASSERT_NOT_NULL(map);
    TEST_ASSERT_FALSE(hashmap_oom(map));
    TEST_ASSERT_FALSE(hashmap_oom(map));
    TEST_ASSERT_FALSE(hashmap_oom(map));
}

int main(void) {
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_hashmap_oom_false_on_new_map);
    RUN_TEST(test_hashmap_oom_false_after_successful_set);
    RUN_TEST(test_hashmap_oom_false_after_multiple_sets);
    RUN_TEST(test_hashmap_oom_true_when_field_set);
    RUN_TEST(test_hashmap_oom_false_when_field_cleared);
    RUN_TEST(test_hashmap_oom_false_large_initial_capacity);
    RUN_TEST(test_hashmap_oom_false_after_clear);
    RUN_TEST(test_hashmap_oom_false_after_delete);
    RUN_TEST(test_hashmap_oom_consistent_true);
    RUN_TEST(test_hashmap_oom_consistent_false);
    return UNITY_END();
}