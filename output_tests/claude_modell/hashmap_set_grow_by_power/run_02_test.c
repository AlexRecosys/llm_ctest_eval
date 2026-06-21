#include "hashmap.c"
#include "unity.h"
#include "hashmap.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* File-scope fixtures */
static struct hashmap *map;

/* Helper: simple hash for int keys */
static uint64_t int_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    int key;
    memcpy(&key, item, sizeof(int));
    return hashmap_murmur(&key, sizeof(int), seed0, seed1);
}

/* Helper: simple compare for int keys */
static int int_compare(const void *a, const void *b, void *udata) {
    (void)udata;
    int ia, ib;
    memcpy(&ia, a, sizeof(int));
    memcpy(&ib, b, sizeof(int));
    return ia - ib;
}

void setUp(void) {
    map = hashmap_new(sizeof(int), 0, 0, 0, int_hash, int_compare, NULL, NULL);
}

void tearDown(void) {
    if (map) {
        hashmap_free(map);
        map = NULL;
    }
}

/* Test: power = 0 should clamp to 1 */
static void test_growpower_zero_clamps_to_one(void) {
    hashmap_set_grow_by_power(map, 0);
    TEST_ASSERT_EQUAL_UINT(1, map->growpower);
}

/* Test: power = 1 should be stored as 1 */
static void test_growpower_one_stored_as_one(void) {
    hashmap_set_grow_by_power(map, 1);
    TEST_ASSERT_EQUAL_UINT(1, map->growpower);
}

/* Test: power = 8 (mid-range) should be stored as-is */
static void test_growpower_mid_range_stored_as_is(void) {
    hashmap_set_grow_by_power(map, 8);
    TEST_ASSERT_EQUAL_UINT(8, map->growpower);
}

/* Test: power = 16 (upper boundary) should be stored as 16 */
static void test_growpower_sixteen_stored_as_sixteen(void) {
    hashmap_set_grow_by_power(map, 16);
    TEST_ASSERT_EQUAL_UINT(16, map->growpower);
}

/* Test: power = 17 should clamp to 16 */
static void test_growpower_seventeen_clamps_to_sixteen(void) {
    hashmap_set_grow_by_power(map, 17);
    TEST_ASSERT_EQUAL_UINT(16, map->growpower);
}

/* Test: power = very large value should clamp to 16 */
static void test_growpower_large_value_clamps_to_sixteen(void) {
    hashmap_set_grow_by_power(map, 9999);
    TEST_ASSERT_EQUAL_UINT(16, map->growpower);
}

/* Test: power = SIZE_MAX should clamp to 16 */
static void test_growpower_size_max_clamps_to_sixteen(void) {
    hashmap_set_grow_by_power(map, (size_t)-1);
    TEST_ASSERT_EQUAL_UINT(16, map->growpower);
}

/* Test: power = 2 should be stored as 2 */
static void test_growpower_two_stored_as_two(void) {
    hashmap_set_grow_by_power(map, 2);
    TEST_ASSERT_EQUAL_UINT(2, map->growpower);
}

/* Test: power = 15 should be stored as 15 */
static void test_growpower_fifteen_stored_as_fifteen(void) {
    hashmap_set_grow_by_power(map, 15);
    TEST_ASSERT_EQUAL_UINT(15, map->growpower);
}

/* Test: calling multiple times overwrites previous value */
static void test_growpower_overwrite_previous_value(void) {
    hashmap_set_grow_by_power(map, 4);
    TEST_ASSERT_EQUAL_UINT(4, map->growpower);
    hashmap_set_grow_by_power(map, 10);
    TEST_ASSERT_EQUAL_UINT(10, map->growpower);
    hashmap_set_grow_by_power(map, 0);
    TEST_ASSERT_EQUAL_UINT(1, map->growpower);
}

/* Test: power = 5 stored correctly */
static void test_growpower_five_stored_as_five(void) {
    hashmap_set_grow_by_power(map, 5);
    TEST_ASSERT_EQUAL_UINT(5, map->growpower);
}

/* Test: power = 16 boundary then 17 clamps */
static void test_growpower_boundary_transition(void) {
    hashmap_set_grow_by_power(map, 16);
    TEST_ASSERT_EQUAL_UINT(16, map->growpower);
    hashmap_set_grow_by_power(map, 17);
    TEST_ASSERT_EQUAL_UINT(16, map->growpower);
}

/* Test: power = 1 boundary then 0 clamps */
static void test_growpower_lower_boundary_transition(void) {
    hashmap_set_grow_by_power(map, 1);
    TEST_ASSERT_EQUAL_UINT(1, map->growpower);
    hashmap_set_grow_by_power(map, 0);
    TEST_ASSERT_EQUAL_UINT(1, map->growpower);
}

int main(void) {
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_growpower_zero_clamps_to_one);
    RUN_TEST(test_growpower_one_stored_as_one);
    RUN_TEST(test_growpower_mid_range_stored_as_is);
    RUN_TEST(test_growpower_sixteen_stored_as_sixteen);
    RUN_TEST(test_growpower_seventeen_clamps_to_sixteen);
    RUN_TEST(test_growpower_large_value_clamps_to_sixteen);
    RUN_TEST(test_growpower_size_max_clamps_to_sixteen);
    RUN_TEST(test_growpower_two_stored_as_two);
    RUN_TEST(test_growpower_fifteen_stored_as_fifteen);
    RUN_TEST(test_growpower_overwrite_previous_value);
    RUN_TEST(test_growpower_five_stored_as_five);
    RUN_TEST(test_growpower_boundary_transition);
    RUN_TEST(test_growpower_lower_boundary_transition);
    return UNITY_END();
}