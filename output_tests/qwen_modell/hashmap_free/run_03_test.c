#include "hashmap.c"
#include "unity.h"
#include <stdlib.h>
#include <string.h>

// File-scope static variables / fixtures
static struct hashmap *map = NULL;
static int element_free_call_count = 0;
static void *freed_element = NULL;

// Helper functions
static uint64_t dummy_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    (void)seed0;
    (void)seed1;
    return *(uint32_t *)item;
}

static int dummy_compare(const void *a, const void *b, void *udata) {
    (void)udata;
    return *(uint32_t *)a - *(uint32_t *)b;
}

static void dummy_elfree(void *item) {
    element_free_call_count++;
    freed_element = item;
}

void setUp(void) {
    element_free_call_count = 0;
    freed_element = NULL;
    map = NULL;
}

void tearDown(void) {
    if (map) {
        hashmap_free(map);
        map = NULL;
    }
}

// Test cases
void test_hashmap_free_with_NULL_returns_early(void) {
    // Act
    hashmap_free(NULL);

    // Assert: no crash, no memory leak
}

void test_hashmap_free_frees_buckets_and_map(void) {
    // Arrange
    map = hashmap_new(sizeof(uint32_t), 0, 0, 0, dummy_hash, dummy_compare, NULL, NULL);
    TEST_ASSERT_NOT_NULL(map);

    // Act
    hashmap_free(map);

    // Assert
    // map pointer should be freed (no crash on access would be undefined behavior, but we rely on free behavior)
    // Since we cannot inspect freed memory, we verify by ensuring no memory leak via valgrind in real use
    // In unit test context, we just ensure no crash
}

void test_hashmap_free_calls_elfree_for_each_element(void) {
    // Arrange
    uint32_t key1 = 10;
    uint32_t key2 = 20;
    uint32_t key3 = 30;

    map = hashmap_new(sizeof(uint32_t), 0, 0, 0, dummy_hash, dummy_compare, dummy_elfree, NULL);
    TEST_ASSERT_NOT_NULL(map);

    hashmap_set(map, &key1);
    hashmap_set(map, &key2);
    hashmap_set(map, &key3);

    // Act
    hashmap_free(map);

    // Assert
    TEST_ASSERT_EQUAL_INT(3, element_free_call_count);
}

void test_hashmap_free_with_custom_allocator_frees_buckets_and_map(void) {
    // Arrange
    void *custom_malloc(size_t size) { return malloc(size); }
    void *custom_realloc(void *ptr, size_t size) { return realloc(ptr, size); }
    void custom_free(void *ptr) { free(ptr); }

    map = hashmap_new_with_allocator(custom_malloc, custom_realloc, custom_free,
                                     sizeof(uint32_t), 0, 0, 0,
                                     dummy_hash, dummy_compare, NULL, NULL);
    TEST_ASSERT_NOT_NULL(map);

    // Act
    hashmap_free(map);

    // Assert: no crash, no memory leak
}

void test_hashmap_free_with_elements_and_elfree_frees_all_elements_and_map(void) {
    // Arrange
    uint32_t key1 = 100;
    uint32_t key2 = 200;

    map = hashmap_new(sizeof(uint32_t), 0, 0, 0, dummy_hash, dummy_compare, dummy_elfree, NULL);
    TEST_ASSERT_NOT_NULL(map);

    hashmap_set(map, &key1);
    hashmap_set(map, &key2);

    // Act
    hashmap_free(map);

    // Assert
    TEST_ASSERT_EQUAL_INT(2, element_free_call_count);
    // freed_element points to last freed item (key2), but we can't verify address reliably
}

// main function
int main(void) {
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_hashmap_free_with_NULL_returns_early);
    RUN_TEST(test_hashmap_free_frees_buckets_and_map);
    RUN_TEST(test_hashmap_free_calls_elfree_for_each_element);
    RUN_TEST(test_hashmap_free_with_custom_allocator_frees_buckets_and_map);
    RUN_TEST(test_hashmap_free_with_elements_and_elfree_frees_all_elements_and_map);
    return UNITY_END();
}