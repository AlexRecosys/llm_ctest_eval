#include "hashmap.c"
#include "unity.h"
#include <stdlib.h>
#include <string.h>

// Helper to create a simple integer-based hashmap for testing
static struct hashmap *create_int_hashmap(void) {
    return hashmap_new(sizeof(int), 0, 0, 0,
        hashmap_sip,
        (int (*)(const void *, const void *, void *))memcmp,
        NULL, NULL);
}

// Helper to create a simple string-based hashmap for testing
static struct hashmap *create_str_hashmap(void) {
    return hashmap_new(sizeof(char *), 0, 0, 0,
        hashmap_sip,
        (int (*)(const void *, const void *, void *))strcmp,
        NULL, NULL);
}

// Test: hashmap_iter returns false on empty map
static void test_hashmap_iter_empty_map(void) {
    struct hashmap *map = create_int_hashmap();
    size_t i = 0;
    void *item = NULL;
    bool result = hashmap_iter(map, &i, &item);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL_SIZE(0, i);
    TEST_ASSERT_NULL(item);

    hashmap_free(map);
}

// Test: hashmap_iter returns true for single item
static void test_hashmap_iter_single_item(void) {
    struct hashmap *map = create_int_hashmap();
    int val = 42;
    hashmap_set(map, &val);

    size_t i = 0;
    void *item = NULL;
    bool result = hashmap_iter(map, &i, &item);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(item);
    TEST_ASSERT_EQUAL_INT(42, *(int *)item);
    TEST_ASSERT_EQUAL_SIZE(1, i);

    // Next call should return false
    result = hashmap_iter(map, &i, &item);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL_SIZE(1, i); // i should not advance beyond end

    hashmap_free(map);
}

// Test: hashmap_iter iterates over multiple items
static void test_hashmap_iter_multiple_items(void) {
    struct hashmap *map = create_int_hashmap();

    int vals[] = {10, 20, 30, 40, 50};
    for (size_t i = 0; i < 5; i++) {
        hashmap_set(map, &vals[i]);
    }

    size_t i = 0;
    void *item = NULL;
    int found[5] = {0};
    int count = 0;

    while (hashmap_iter(map, &i, &item)) {
        int v = *(int *)item;
        for (int j = 0; j < 5; j++) {
            if (v == vals[j]) {
                found[j] = 1;
                break;
            }
        }
        count++;
    }

    TEST_ASSERT_EQUAL_INT(5, count);
    for (int j = 0; j < 5; j++) {
        TEST_ASSERT_TRUE(found[j]);
    }

    hashmap_free(map);
}

// Test: hashmap_iter continues iteration after partial iteration
static void test_hashmap_iter_partial_then_continue(void) {
    struct hashmap *map = create_int_hashmap();

    int vals[] = {100, 200, 300};
    for (size_t i = 0; i < 3; i++) {
        hashmap_set(map, &vals[i]);
    }

    size_t i = 0;
    void *item = NULL;
    int found_first = 0;

    // First iteration: stop after first item
    if (hashmap_iter(map, &i, &item)) {
        found_first = *(int *)item;
    }

    TEST_ASSERT_TRUE(found_first == 100 || found_first == 200 || found_first == 300);

    // Continue iteration from where left off
    int found_rest[3] = {0};
    int count_rest = 0;
    while (hashmap_iter(map, &i, &item)) {
        int v = *(int *)item;
        for (int j = 0; j < 3; j++) {
            if (v == vals[j]) {
                found_rest[j] = 1;
                break;
            }
        }
        count_rest++;
    }

    TEST_ASSERT_EQUAL_INT(2, count_rest);
    int found_total = 0;
    for (int j = 0; j < 3; j++) {
        if (found_first == vals[j]) found_total++;
        if (found_rest[j]) found_total++;
    }
    TEST_ASSERT_EQUAL_INT(3, found_total);

    hashmap_free(map);
}

// Test: hashmap_iter returns false when i >= nbuckets (no items)
static void test_hashmap_iter_no_items_after_initial_scan(void) {
    struct hashmap *map = create_int_hashmap();

    size_t i = 0;
    void *item = NULL;

    // First call should return false
    bool result = hashmap_iter(map, &i, &item);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_NULL(item);

    // Even after multiple calls
    result = hashmap_iter(map, &i, &item);
    TEST_ASSERT_FALSE(result);

    hashmap_free(map);
}

// Test: hashmap_iter works after deletion (with reset)
static void test_hashmap_iter_after_deletion_with_reset(void) {
    struct hashmap *map = create_int_hashmap();

    int vals[] = {111, 222, 333};
    for (size_t i = 0; i < 3; i++) {
        hashmap_set(map, &vals[i]);
    }

    // Delete one item
    hashmap_delete(map, &vals[1]);

    // Must reset iterator after deletion
    size_t i = 0;
    void *item = NULL;
    int found[3] = {0};
    int count = 0;

    while (hashmap_iter(map, &i, &item)) {
        int v = *(int *)item;
        for (int j = 0; j < 3; j++) {
            if (v == vals[j]) {
                found[j] = 1;
                break;
            }
        }
        count++;
    }

    // Should have 2 items left
    TEST_ASSERT_EQUAL_INT(2, count);
    TEST_ASSERT_FALSE(found[1]); // vals[1] should be gone
    TEST_ASSERT_TRUE(found[0]);
    TEST_ASSERT_TRUE(found[2]);

    hashmap_free(map);
}

// Test: hashmap_iter returns same item on repeated calls with same i (no advancement)
static void test_hashmap_iter_no_side_effects_on_same_i(void) {
    struct hashmap *map = create_int_hashmap();

    int val = 777;
    hashmap_set(map, &val);

    size_t i = 0;
    void *item1 = NULL;
    void *item2 = NULL;

    bool r1 = hashmap_iter(map, &i, &item1);
    bool r2 = hashmap_iter(map, &i, &item2);

    TEST_ASSERT_TRUE(r1);
    TEST_ASSERT_TRUE(r2);
    TEST_ASSERT_EQUAL_PTR(item1, item2);
    TEST_ASSERT_EQUAL_INT(777, *(int *)item1);

    hashmap_free(map);
}

int main(void) {
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_hashmap_iter_empty_map);
    RUN_TEST(test_hashmap_iter_single_item);
    RUN_TEST(test_hashmap_iter_multiple_items);
    RUN_TEST(test_hashmap_iter_partial_then_continue);
    RUN_TEST(test_hashmap_iter_no_items_after_initial_scan);
    RUN_TEST(test_hashmap_iter_after_deletion_with_reset);
    RUN_TEST(test_hashmap_iter_no_side_effects_on_same_i);
    return UNITY_END();
}