#include "hashmap.c"
#include "unity.h"
#include <stdlib.h>
#include <string.h>

// File-scope static variables / fixtures
static struct hashmap *map = NULL;
static int iter_call_count = 0;
static int iter_stop_at = -1;
static int *test_items = NULL;
static size_t test_items_count = 0;

// Helper functions and macros
static uint64_t simple_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    (void)seed0;
    (void)seed1;
    return *(const int *)item;
}

static int simple_compare(const void *a, const void *b, void *udata) {
    (void)udata;
    int ia = *(const int *)a;
    int ib = *(const int *)b;
    if (ia < ib) return -1;
    if (ia > ib) return 1;
    return 0;
}

static bool iter_callback(const void *item, void *udata) {
    (void)udata;
    iter_call_count++;
    if (iter_stop_at >= 0 && iter_call_count > iter_stop_at) {
        return false;
    }
    return true;
}

static void setup_map_with_items(size_t count) {
    map = hashmap_new(sizeof(int), 0, 0, 0, simple_hash, simple_compare, NULL, NULL);
    TEST_ASSERT_NOT_NULL(map);

    test_items = malloc(count * sizeof(int));
    TEST_ASSERT_NOT_NULL(test_items);
    test_items_count = count;

    for (size_t i = 0; i < count; i++) {
        int val = (int)i * 10;
        test_items[i] = val;
        const void *ret = hashmap_set(map, &val);
        TEST_ASSERT_NULL(ret); // Should not replace existing
    }
}

static void teardown_map(void) {
    if (map) {
        hashmap_free(map);
        map = NULL;
    }
    if (test_items) {
        free(test_items);
        test_items = NULL;
        test_items_count = 0;
    }
    iter_call_count = 0;
    iter_stop_at = -1;
}

// Test cases

void test_hashmap_scan_empty_map_returns_true(void) {
    map = hashmap_new(sizeof(int), 0, 0, 0, simple_hash, simple_compare, NULL, NULL);
    TEST_ASSERT_NOT_NULL(map);

    bool result = hashmap_scan(map, iter_callback, NULL);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(0, iter_call_count);

    hashmap_free(map);
}

void test_hashmap_scan_single_item_map_calls_iter_once_and_returns_true(void) {
    map = hashmap_new(sizeof(int), 0, 0, 0, simple_hash, simple_compare, NULL, NULL);
    TEST_ASSERT_NOT_NULL(map);

    int val = 42;
    hashmap_set(map, &val);

    iter_call_count = 0;
    bool result = hashmap_scan(map, iter_callback, NULL);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(1, iter_call_count);

    hashmap_free(map);
}

void test_hashmap_scan_multiple_items_calls_iter_for_each_item_and_returns_true(void) {
    setup_map_with_items(5);

    iter_call_count = 0;
    bool result = hashmap_scan(map, iter_callback, NULL);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(5, iter_call_count);

    teardown_map();
}

void test_hashmap_scan_stops_early_when_iter_returns_false(void) {
    setup_map_with_items(5);

    iter_stop_at = 2; // Stop after 2 calls
    iter_call_count = 0;
    bool result = hashmap_scan(map, iter_callback, NULL);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL_INT(2, iter_call_count);

    teardown_map();
}

void test_hashmap_scan_iter_receives_correct_item_pointers(void) {
    setup_map_with_items(3);

    const int *received_items[3];
    int item_count = 0;

    bool iter_collect(const void *item, void *udata) {
        (void)udata;
        if (item_count < 3) {
            received_items[item_count++] = *(const int **)item;
        }
        return true;
    }

    iter_call_count = 0;
    bool result = hashmap_scan(map, iter_collect, NULL);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(3, iter_call_count);
    TEST_ASSERT_EQUAL_INT(3, item_count);

    // Verify that all items were visited (order may vary due to hash table layout)
    bool found[3] = {false, false, false};
    for (int i = 0; i < item_count; i++) {
        for (int j = 0; j < 3; j++) {
            if (received_items[i] == &test_items[j]) {
                found[j] = true;
                break;
            }
        }
    }
    TEST_ASSERT_TRUE(found[0]);
    TEST_ASSERT_TRUE(found[1]);
    TEST_ASSERT_TRUE(found[2]);

    teardown_map();
}

void test_hashmap_scan_udata_passed_correctly(void) {
    setup_map_with_items(3);

    int udata_value = 12345;
    int *udata_received = NULL;

    bool iter_check_udata(const void *item, void *udata) {
        udata_received = (int *)udata;
        return true;
    }

    bool result = hashmap_scan(map, iter_check_udata, &udata_value);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_PTR(&udata_value, udata_received);

    teardown_map();
}

void test_hashmap_scan_with_custom_udata_modification(void) {
    setup_map_with_items(3);

    int sum = 0;
    bool iter_sum(const void *item, void *udata) {
        int *sum_ptr = (int *)udata;
        *sum_ptr += *(const int *)item;
        return true;
    }

    bool result = hashmap_scan(map, iter_sum, &sum);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(0 + 10 + 20, sum);

    teardown_map();
}

int main(void) {
    unity_install_sighandler();
    UNITY_BEGIN();

    RUN_TEST(test_hashmap_scan_empty_map_returns_true);
    RUN_TEST(test_hashmap_scan_single_item_map_calls_iter_once_and_returns_true);
    RUN_TEST(test_hashmap_scan_multiple_items_calls_iter_for_each_item_and_returns_true);
    RUN_TEST(test_hashmap_scan_stops_early_when_iter_returns_false);
    RUN_TEST(test_hashmap_scan_iter_receives_correct_item_pointers);
    RUN_TEST(test_hashmap_scan_udata_passed_correctly);
    RUN_TEST(test_hashmap_scan_with_custom_udata_modification);

    return UNITY_END();
}