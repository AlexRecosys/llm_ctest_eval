#include "hashmap.c"
#include "unity.h"
#include "hashmap.h"
#include <stdlib.h>
#include <string.h>

/* ── fixtures ── */

struct item {
    int key;
    int value;
};

static uint64_t item_hash(const void *a, uint64_t seed0, uint64_t seed1) {
    const struct item *it = (const struct item *)a;
    return hashmap_murmur(&it->key, sizeof(it->key), seed0, seed1);
}

static int item_compare(const void *a, const void *b, void *udata) {
    (void)udata;
    const struct item *ia = (const struct item *)a;
    const struct item *ib = (const struct item *)b;
    return ia->key - ib->key;
}

/* ── helpers ── */

static struct hashmap *make_map(void) {
    return hashmap_new(sizeof(struct item), 0, 0, 0,
                       item_hash, item_compare, NULL, NULL);
}

static struct hashmap *make_map_with_items(int count) {
    struct hashmap *map = make_map();
    if (!map) return NULL;
    for (int i = 0; i < count; i++) {
        struct item it = { .key = i, .value = i * 10 };
        hashmap_set(map, &it);
    }
    return map;
}

/* ── test cases ── */

void setUp(void) { /* nothing */ }
void tearDown(void) { /* nothing */ }

void test_hashmap_free_null_does_not_crash(void) {
    /* Must not crash or abort when passed NULL */
    hashmap_free(NULL);
    TEST_ASSERT_TRUE(1); /* reached here without crash */
}

void test_hashmap_free_empty_map(void) {
    struct hashmap *map = make_map();
    TEST_ASSERT_NOT_NULL(map);
    /* Should not crash on an empty map */
    hashmap_free(map);
    TEST_ASSERT_TRUE(1);
}

void test_hashmap_free_map_with_one_item(void) {
    struct hashmap *map = make_map();
    TEST_ASSERT_NOT_NULL(map);
    struct item it = { .key = 42, .value = 420 };
    hashmap_set(map, &it);
    TEST_ASSERT_EQUAL_INT(1, (int)hashmap_count(map));
    hashmap_free(map);
    TEST_ASSERT_TRUE(1);
}

void test_hashmap_free_map_with_many_items(void) {
    struct hashmap *map = make_map_with_items(100);
    TEST_ASSERT_NOT_NULL(map);
    TEST_ASSERT_EQUAL_INT(100, (int)hashmap_count(map));
    hashmap_free(map);
    TEST_ASSERT_TRUE(1);
}

void test_hashmap_free_map_after_delete_all(void) {
    struct hashmap *map = make_map_with_items(10);
    TEST_ASSERT_NOT_NULL(map);
    for (int i = 0; i < 10; i++) {
        struct item key = { .key = i };
        hashmap_delete(map, &key);
    }
    TEST_ASSERT_EQUAL_INT(0, (int)hashmap_count(map));
    hashmap_free(map);
    TEST_ASSERT_TRUE(1);
}

void test_hashmap_free_map_after_clear(void) {
    struct hashmap *map = make_map_with_items(20);
    TEST_ASSERT_NOT_NULL(map);
    hashmap_clear(map, false);
    TEST_ASSERT_EQUAL_INT(0, (int)hashmap_count(map));
    hashmap_free(map);
    TEST_ASSERT_TRUE(1);
}

void test_hashmap_free_map_after_clear_with_resize(void) {
    struct hashmap *map = make_map_with_items(20);
    TEST_ASSERT_NOT_NULL(map);
    hashmap_clear(map, true);
    TEST_ASSERT_EQUAL_INT(0, (int)hashmap_count(map));
    hashmap_free(map);
    TEST_ASSERT_TRUE(1);
}

void test_hashmap_free_large_map(void) {
    struct hashmap *map = make_map_with_items(10000);
    TEST_ASSERT_NOT_NULL(map);
    TEST_ASSERT_EQUAL_INT(10000, (int)hashmap_count(map));
    hashmap_free(map);
    TEST_ASSERT_TRUE(1);
}

void test_hashmap_free_map_with_custom_allocator(void) {
    struct hashmap *map = hashmap_new_with_allocator(
        malloc, realloc, free,
        sizeof(struct item), 0, 0, 0,
        item_hash, item_compare, NULL, NULL);
    TEST_ASSERT_NOT_NULL(map);
    struct item it = { .key = 7, .value = 77 };
    hashmap_set(map, &it);
    TEST_ASSERT_EQUAL_INT(1, (int)hashmap_count(map));
    hashmap_free(map);
    TEST_ASSERT_TRUE(1);
}

void test_hashmap_free_map_with_initial_capacity(void) {
    struct hashmap *map = hashmap_new(sizeof(struct item), 64, 0, 0,
                                      item_hash, item_compare, NULL, NULL);
    TEST_ASSERT_NOT_NULL(map);
    for (int i = 0; i < 50; i++) {
        struct item it = { .key = i, .value = i };
        hashmap_set(map, &it);
    }
    hashmap_free(map);
    TEST_ASSERT_TRUE(1);
}

void test_hashmap_free_map_after_multiple_sets_same_key(void) {
    struct hashmap *map = make_map();
    TEST_ASSERT_NOT_NULL(map);
    for (int i = 0; i < 50; i++) {
        struct item it = { .key = 1, .value = i };
        hashmap_set(map, &it);
    }
    TEST_ASSERT_EQUAL_INT(1, (int)hashmap_count(map));
    hashmap_free(map);
    TEST_ASSERT_TRUE(1);
}

void test_hashmap_free_map_triggers_no_oom(void) {
    /* Verify that after free, a fresh map can still be allocated */
    struct hashmap *map = make_map_with_items(5);
    TEST_ASSERT_NOT_NULL(map);
    hashmap_free(map);

    struct hashmap *map2 = make_map_with_items(5);
    TEST_ASSERT_NOT_NULL(map2);
    TEST_ASSERT_EQUAL_INT(5, (int)hashmap_count(map2));
    hashmap_free(map2);
    TEST_ASSERT_TRUE(1);
}

/* ── main ── */

int main(void) {
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_hashmap_free_null_does_not_crash);
    RUN_TEST(test_hashmap_free_empty_map);
    RUN_TEST(test_hashmap_free_map_with_one_item);
    RUN_TEST(test_hashmap_free_map_with_many_items);
    RUN_TEST(test_hashmap_free_map_after_delete_all);
    RUN_TEST(test_hashmap_free_map_after_clear);
    RUN_TEST(test_hashmap_free_map_after_clear_with_resize);
    RUN_TEST(test_hashmap_free_large_map);
    RUN_TEST(test_hashmap_free_map_with_custom_allocator);
    RUN_TEST(test_hashmap_free_map_with_initial_capacity);
    RUN_TEST(test_hashmap_free_map_after_multiple_sets_same_key);
    RUN_TEST(test_hashmap_free_map_triggers_no_oom);
    return UNITY_END();
}