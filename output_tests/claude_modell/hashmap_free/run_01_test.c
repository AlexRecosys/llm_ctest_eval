#include "hashmap.c"
#include "unity.h"
#include "hashmap.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* ── fixtures ── */

struct test_item {
    int key;
    int value;
};

static uint64_t test_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    const struct test_item *ti = (const struct test_item *)item;
    return hashmap_murmur(&ti->key, sizeof(ti->key), seed0, seed1);
}

static int test_compare(const void *a, const void *b, void *udata) {
    (void)udata;
    const struct test_item *ta = (const struct test_item *)a;
    const struct test_item *tb = (const struct test_item *)b;
    return ta->key - tb->key;
}

/* ── helpers ── */

static struct hashmap *create_empty_map(void) {
    return hashmap_new(sizeof(struct test_item), 0, 0, 0,
                       test_hash, test_compare, NULL, NULL);
}

static struct hashmap *create_populated_map(int count) {
    struct hashmap *map = create_empty_map();
    if (!map) return NULL;
    for (int i = 0; i < count; i++) {
        struct test_item item = { .key = i, .value = i * 10 };
        hashmap_set(map, &item);
    }
    return map;
}

/* track custom allocator calls */
static int custom_free_call_count = 0;

static void *custom_malloc(size_t size) {
    return malloc(size);
}

static void custom_free(void *ptr) {
    custom_free_call_count++;
    free(ptr);
}

/* ── setUp / tearDown ── */

void setUp(void) {
    custom_free_call_count = 0;
}

void tearDown(void) {
    /* nothing */
}

/* ── test cases ── */

void test_hashmap_free_null_does_not_crash(void) {
    /* Must not crash or assert when passed NULL */
    hashmap_free(NULL);
    TEST_ASSERT_TRUE(1); /* reached here without crash */
}

void test_hashmap_free_empty_map(void) {
    struct hashmap *map = create_empty_map();
    TEST_ASSERT_NOT_NULL(map);
    /* Should not crash on an empty map */
    hashmap_free(map);
    TEST_ASSERT_TRUE(1);
}

void test_hashmap_free_populated_map(void) {
    struct hashmap *map = create_populated_map(10);
    TEST_ASSERT_NOT_NULL(map);
    /* Should not crash when freeing a map with elements */
    hashmap_free(map);
    TEST_ASSERT_TRUE(1);
}

void test_hashmap_free_large_map(void) {
    struct hashmap *map = create_populated_map(1000);
    TEST_ASSERT_NOT_NULL(map);
    hashmap_free(map);
    TEST_ASSERT_TRUE(1);
}

void test_hashmap_free_single_element(void) {
    struct hashmap *map = create_empty_map();
    TEST_ASSERT_NOT_NULL(map);
    struct test_item item = { .key = 42, .value = 420 };
    hashmap_set(map, &item);
    hashmap_free(map);
    TEST_ASSERT_TRUE(1);
}

void test_hashmap_free_with_custom_allocator_calls_custom_free(void) {
    custom_free_call_count = 0;
    struct hashmap *map = hashmap_new_with_allocator(
        custom_malloc, NULL, custom_free,
        sizeof(struct test_item), 0, 0, 0,
        test_hash, test_compare, NULL, NULL);
    TEST_ASSERT_NOT_NULL(map);

    struct test_item item = { .key = 1, .value = 10 };
    hashmap_set(map, &item);

    hashmap_free(map);

    /* custom_free must have been called at least twice:
       once for buckets, once for the map struct itself */
    TEST_ASSERT_GREATER_THAN(1, custom_free_call_count);
}

void test_hashmap_free_custom_allocator_empty_map(void) {
    custom_free_call_count = 0;
    struct hashmap *map = hashmap_new_with_allocator(
        custom_malloc, NULL, custom_free,
        sizeof(struct test_item), 0, 0, 0,
        test_hash, test_compare, NULL, NULL);
    TEST_ASSERT_NOT_NULL(map);

    hashmap_free(map);

    /* At minimum: buckets + map struct */
    TEST_ASSERT_GREATER_THAN(1, custom_free_call_count);
}

void test_hashmap_free_after_delete_all_elements(void) {
    struct hashmap *map = create_populated_map(5);
    TEST_ASSERT_NOT_NULL(map);

    for (int i = 0; i < 5; i++) {
        struct test_item key = { .key = i };
        hashmap_delete(map, &key);
    }

    TEST_ASSERT_EQUAL_UINT(0, hashmap_count(map));
    hashmap_free(map);
    TEST_ASSERT_TRUE(1);
}

void test_hashmap_free_map_count_was_correct_before_free(void) {
    struct hashmap *map = create_populated_map(7);
    TEST_ASSERT_NOT_NULL(map);
    TEST_ASSERT_EQUAL_UINT(7, hashmap_count(map));
    hashmap_free(map);
    TEST_ASSERT_TRUE(1);
}

void test_hashmap_free_multiple_independent_maps(void) {
    struct hashmap *map1 = create_populated_map(5);
    struct hashmap *map2 = create_populated_map(10);
    struct hashmap *map3 = create_empty_map();

    TEST_ASSERT_NOT_NULL(map1);
    TEST_ASSERT_NOT_NULL(map2);
    TEST_ASSERT_NOT_NULL(map3);

    hashmap_free(map1);
    hashmap_free(map2);
    hashmap_free(map3);
    TEST_ASSERT_TRUE(1);
}

void test_hashmap_free_after_clear(void) {
    struct hashmap *map = create_populated_map(8);
    TEST_ASSERT_NOT_NULL(map);
    hashmap_clear(map, false);
    TEST_ASSERT_EQUAL_UINT(0, hashmap_count(map));
    hashmap_free(map);
    TEST_ASSERT_TRUE(1);
}

void test_hashmap_free_with_initial_capacity(void) {
    struct hashmap *map = hashmap_new(sizeof(struct test_item), 64, 0, 0,
                                      test_hash, test_compare, NULL, NULL);
    TEST_ASSERT_NOT_NULL(map);
    struct test_item item = { .key = 99, .value = 990 };
    hashmap_set(map, &item);
    hashmap_free(map);
    TEST_ASSERT_TRUE(1);
}

void test_hashmap_free_custom_allocator_many_elements(void) {
    custom_free_call_count = 0;
    struct hashmap *map = hashmap_new_with_allocator(
        custom_malloc, NULL, custom_free,
        sizeof(struct test_item), 0, 0, 0,
        test_hash, test_compare, NULL, NULL);
    TEST_ASSERT_NOT_NULL(map);

    for (int i = 0; i < 50; i++) {
        struct test_item item = { .key = i, .value = i * 2 };
        hashmap_set(map, &item);
    }

    int before = custom_free_call_count;
    hashmap_free(map);
    /* free calls must have increased */
    TEST_ASSERT_GREATER_THAN(before, custom_free_call_count);
}

/* ── main ── */

int main(void) {
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_hashmap_free_null_does_not_crash);
    RUN_TEST(test_hashmap_free_empty_map);
    RUN_TEST(test_hashmap_free_populated_map);
    RUN_TEST(test_hashmap_free_large_map);
    RUN_TEST(test_hashmap_free_single_element);
    RUN_TEST(test_hashmap_free_with_custom_allocator_calls_custom_free);
    RUN_TEST(test_hashmap_free_custom_allocator_empty_map);
    RUN_TEST(test_hashmap_free_after_delete_all_elements);
    RUN_TEST(test_hashmap_free_map_count_was_correct_before_free);
    RUN_TEST(test_hashmap_free_multiple_independent_maps);
    RUN_TEST(test_hashmap_free_after_clear);
    RUN_TEST(test_hashmap_free_with_initial_capacity);
    RUN_TEST(test_hashmap_free_custom_allocator_many_elements);
    return UNITY_END();
}