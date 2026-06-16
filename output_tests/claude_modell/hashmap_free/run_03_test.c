#include "hashmap.c"
#include "unity.h"
#include "hashmap.h"
#include <stdlib.h>
#include <string.h>

/* ── fixtures ─────────────────────────────────────────────────────────────── */

typedef struct {
    int key;
    int value;
} int_item;

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

/* Track allocations for custom-allocator tests */
static int g_alloc_count = 0;
static int g_free_count  = 0;

static void *counting_malloc(size_t size) {
    g_alloc_count++;
    return malloc(size);
}

static void counting_free(void *ptr) {
    if (ptr) g_free_count++;
    free(ptr);
}

/* ── setUp / tearDown ─────────────────────────────────────────────────────── */

void setUp(void) {
    g_alloc_count = 0;
    g_free_count  = 0;
}

void tearDown(void) {
    /* nothing */
}

/* ── helpers ──────────────────────────────────────────────────────────────── */

static struct hashmap *make_map(void) {
    return hashmap_new(sizeof(int_item), 0, 0, 0,
                       int_hash, int_compare, NULL, NULL);
}

static struct hashmap *make_map_with_allocator(void) {
    return hashmap_new_with_allocator(counting_malloc, NULL, counting_free,
                                      sizeof(int_item), 0, 0, 0,
                                      int_hash, int_compare, NULL, NULL);
}

/* ── test cases ───────────────────────────────────────────────────────────── */

/* hashmap_free(NULL) must not crash */
void test_free_null_does_not_crash(void) {
    hashmap_free(NULL);
    TEST_ASSERT_TRUE(1); /* reached here without crash */
}

/* free an empty map */
void test_free_empty_map(void) {
    struct hashmap *map = make_map();
    TEST_ASSERT_NOT_NULL(map);
    hashmap_free(map);
    TEST_ASSERT_TRUE(1);
}

/* free a map that has one element */
void test_free_map_with_one_element(void) {
    struct hashmap *map = make_map();
    TEST_ASSERT_NOT_NULL(map);

    int_item item = {.key = 42, .value = 100};
    hashmap_set(map, &item);

    hashmap_free(map);
    TEST_ASSERT_TRUE(1);
}

/* free a map that has many elements */
void test_free_map_with_many_elements(void) {
    struct hashmap *map = make_map();
    TEST_ASSERT_NOT_NULL(map);

    for (int i = 0; i < 1000; i++) {
        int_item item = {.key = i, .value = i * 2};
        hashmap_set(map, &item);
    }

    TEST_ASSERT_EQUAL_UINT((size_t)1000, hashmap_count(map));
    hashmap_free(map);
    TEST_ASSERT_TRUE(1);
}

/* free a map created with a custom allocator — free must be called */
void test_free_calls_custom_free(void) {
    g_alloc_count = 0;
    g_free_count  = 0;

    struct hashmap *map = make_map_with_allocator();
    TEST_ASSERT_NOT_NULL(map);

    int allocs_before_free = g_alloc_count;
    (void)allocs_before_free;

    hashmap_free(map);

    /* At minimum the map struct itself and the bucket array were freed */
    TEST_ASSERT_TRUE(g_free_count >= 2);
}

/* free a map with custom allocator and several elements */
void test_free_with_allocator_and_elements(void) {
    g_alloc_count = 0;
    g_free_count  = 0;

    struct hashmap *map = make_map_with_allocator();
    TEST_ASSERT_NOT_NULL(map);

    for (int i = 0; i < 50; i++) {
        int_item item = {.key = i, .value = i};
        hashmap_set(map, &item);
    }

    hashmap_free(map);

    /* All allocations must have a corresponding free */
    TEST_ASSERT_EQUAL_INT(g_alloc_count, g_free_count);
}

/* after freeing one map, another map is still usable */
void test_free_does_not_affect_other_maps(void) {
    struct hashmap *map1 = make_map();
    struct hashmap *map2 = make_map();
    TEST_ASSERT_NOT_NULL(map1);
    TEST_ASSERT_NOT_NULL(map2);

    int_item item = {.key = 7, .value = 77};
    hashmap_set(map2, &item);

    hashmap_free(map1);

    const int_item *found = hashmap_get(map2, &(int_item){.key = 7});
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_INT(77, found->value);

    hashmap_free(map2);
}

/* free a map that had elements deleted from it */
void test_free_map_after_deletions(void) {
    struct hashmap *map = make_map();
    TEST_ASSERT_NOT_NULL(map);

    for (int i = 0; i < 100; i++) {
        int_item item = {.key = i, .value = i};
        hashmap_set(map, &item);
    }
    for (int i = 0; i < 50; i++) {
        hashmap_delete(map, &(int_item){.key = i});
    }

    TEST_ASSERT_EQUAL_UINT((size_t)50, hashmap_count(map));
    hashmap_free(map);
    TEST_ASSERT_TRUE(1);
}

/* free a map that was cleared */
void test_free_map_after_clear(void) {
    struct hashmap *map = make_map();
    TEST_ASSERT_NOT_NULL(map);

    for (int i = 0; i < 20; i++) {
        int_item item = {.key = i, .value = i};
        hashmap_set(map, &item);
    }
    hashmap_clear(map, false);
    TEST_ASSERT_EQUAL_UINT((size_t)0, hashmap_count(map));

    hashmap_free(map);
    TEST_ASSERT_TRUE(1);
}

/* free a map that triggered a resize (many insertions) */
void test_free_map_after_resize(void) {
    struct hashmap *map = make_map();
    TEST_ASSERT_NOT_NULL(map);

    /* Insert enough items to force multiple resizes */
    for (int i = 0; i < 5000; i++) {
        int_item item = {.key = i, .value = i};
        hashmap_set(map, &item);
    }

    TEST_ASSERT_EQUAL_UINT((size_t)5000, hashmap_count(map));
    hashmap_free(map);
    TEST_ASSERT_TRUE(1);
}

/* ── main ─────────────────────────────────────────────────────────────────── */

int main(void) {
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_free_null_does_not_crash);
    RUN_TEST(test_free_empty_map);
    RUN_TEST(test_free_map_with_one_element);
    RUN_TEST(test_free_map_with_many_elements);
    RUN_TEST(test_free_calls_custom_free);
    RUN_TEST(test_free_with_allocator_and_elements);
    RUN_TEST(test_free_does_not_affect_other_maps);
    RUN_TEST(test_free_map_after_deletions);
    RUN_TEST(test_free_map_after_clear);
    RUN_TEST(test_free_map_after_resize);
    return UNITY_END();
}