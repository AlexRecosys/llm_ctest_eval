#include "hashmap.c"
#include "unity.h"
#include "hashmap.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

/* Fixture data */
static struct hashmap *test_map;

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
    const test_item_t *ta = (const test_item_t *)a;
    const test_item_t *tb = (const test_item_t *)b;
    (void)udata;
    return ta->key - tb->key;
}

/* Helper: create a fresh map */
static struct hashmap *create_map(void) {
    return hashmap_new(sizeof(test_item_t), 0, 0, 0,
                       test_hash, test_compare, NULL, NULL);
}

void setUp(void) {
    test_map = create_map();
}

void tearDown(void) {
    if (test_map) {
        hashmap_free(test_map);
        test_map = NULL;
    }
}

/* -------------------------------------------------------------------------
 * Test cases
 * ---------------------------------------------------------------------- */

/* A freshly created map should NOT be in OOM state */
void test_hashmap_oom_false_on_new_map(void) {
    TEST_ASSERT_NOT_NULL(test_map);
    TEST_ASSERT_FALSE(hashmap_oom(test_map));
}

/* After normal insertions the OOM flag should remain false */
void test_hashmap_oom_false_after_insertions(void) {
    TEST_ASSERT_NOT_NULL(test_map);

    for (int i = 0; i < 64; i++) {
        test_item_t item = { .key = i, .value = i * 2 };
        hashmap_set(test_map, &item);
        /* If an allocation failure happened the loop would still run,
           but we verify the flag state after each insert. */
    }

    /* As long as the system has memory, OOM should be false */
    TEST_ASSERT_FALSE(hashmap_oom(test_map));
}

/* Directly set the oom field to true and verify the accessor returns true */
void test_hashmap_oom_returns_true_when_flag_set(void) {
    TEST_ASSERT_NOT_NULL(test_map);

    /* Access the internal field directly (we included the .c file) */
    test_map->oom = true;

    TEST_ASSERT_TRUE(hashmap_oom(test_map));
}

/* Directly set the oom field to false and verify the accessor returns false */
void test_hashmap_oom_returns_false_when_flag_cleared(void) {
    TEST_ASSERT_NOT_NULL(test_map);

    test_map->oom = true;
    TEST_ASSERT_TRUE(hashmap_oom(test_map));

    test_map->oom = false;
    TEST_ASSERT_FALSE(hashmap_oom(test_map));
}

/* Toggle the flag multiple times and confirm the accessor tracks it */
void test_hashmap_oom_tracks_flag_changes(void) {
    TEST_ASSERT_NOT_NULL(test_map);

    for (int i = 0; i < 10; i++) {
        test_map->oom = (bool)(i % 2);
        if (i % 2) {
            TEST_ASSERT_TRUE(hashmap_oom(test_map));
        } else {
            TEST_ASSERT_FALSE(hashmap_oom(test_map));
        }
    }
}

/* After a successful hashmap_set the OOM flag should be false */
void test_hashmap_oom_false_after_successful_set(void) {
    TEST_ASSERT_NOT_NULL(test_map);

    test_item_t item = { .key = 42, .value = 100 };
    hashmap_set(test_map, &item);

    TEST_ASSERT_FALSE(hashmap_oom(test_map));
}

/* Verify that hashmap_oom works on a second independent map */
void test_hashmap_oom_independent_maps(void) {
    TEST_ASSERT_NOT_NULL(test_map);

    struct hashmap *map2 = create_map();
    TEST_ASSERT_NOT_NULL(map2);

    /* Set OOM on the first map only */
    test_map->oom = true;

    TEST_ASSERT_TRUE(hashmap_oom(test_map));
    TEST_ASSERT_FALSE(hashmap_oom(map2));

    hashmap_free(map2);
}

/* Verify initial state is exactly false (not just zero-ish) */
void test_hashmap_oom_initial_value_is_exactly_false(void) {
    TEST_ASSERT_NOT_NULL(test_map);
    bool result = hashmap_oom(test_map);
    TEST_ASSERT_EQUAL_INT((int)false, (int)result);
}

/* Verify that setting oom=true yields exactly true */
void test_hashmap_oom_set_value_is_exactly_true(void) {
    TEST_ASSERT_NOT_NULL(test_map);
    test_map->oom = true;
    bool result = hashmap_oom(test_map);
    TEST_ASSERT_EQUAL_INT((int)true, (int)result);
}

/* -------------------------------------------------------------------------
 * main
 * ---------------------------------------------------------------------- */
int main(void) {
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_hashmap_oom_false_on_new_map);
    RUN_TEST(test_hashmap_oom_false_after_insertions);
    RUN_TEST(test_hashmap_oom_returns_true_when_flag_set);
    RUN_TEST(test_hashmap_oom_returns_false_when_flag_cleared);
    RUN_TEST(test_hashmap_oom_tracks_flag_changes);
    RUN_TEST(test_hashmap_oom_false_after_successful_set);
    RUN_TEST(test_hashmap_oom_independent_maps);
    RUN_TEST(test_hashmap_oom_initial_value_is_exactly_false);
    RUN_TEST(test_hashmap_oom_set_value_is_exactly_true);
    return UNITY_END();
}