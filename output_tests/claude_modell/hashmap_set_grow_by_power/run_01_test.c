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

/* setUp and tearDown */
void setUp(void) {
    map = hashmap_new(sizeof(int), 0, 0, 0, int_hash, int_compare, NULL, NULL);
}

void tearDown(void) {
    if (map) {
        hashmap_free(map);
        map = NULL;
    }
}

/* Helper to read growpower from the map struct.
   Since struct hashmap is opaque in the header but defined in the included .c,
   we can access it directly here. */
static size_t get_growpower(struct hashmap *m) {
    return m->growpower;
}

/* Test: power = 0 should clamp to 1 */
static void test_growpower_zero_clamps_to_one(void) {
    hashmap_set_grow_by_power(map, 0);
    TEST_ASSERT_EQUAL_UINT(1, get_growpower(map));
}

/* Test: power = 1 should be stored as 1 */
static void test_growpower_one_stored_as_one(void) {
    hashmap_set_grow_by_power(map, 1);
    TEST_ASSERT_EQUAL_UINT(1, get_growpower(map));
}

/* Test: power = 8 (mid-range) should be stored as 8 */
static void test_growpower_mid_range_stored_correctly(void) {
    hashmap_set_grow_by_power(map, 8);
    TEST_ASSERT_EQUAL_UINT(8, get_growpower(map));
}

/* Test: power = 16 (max boundary) should be stored as 16 */
static void test_growpower_sixteen_stored_as_sixteen(void) {
    hashmap_set_grow_by_power(map, 16);
    TEST_ASSERT_EQUAL_UINT(16, get_growpower(map));
}

/* Test: power = 17 should clamp to 16 */
static void test_growpower_seventeen_clamps_to_sixteen(void) {
    hashmap_set_grow_by_power(map, 17);
    TEST_ASSERT_EQUAL_UINT(16, get_growpower(map));
}

/* Test: power = 100 (large value) should clamp to 16 */
static void test_growpower_large_value_clamps_to_sixteen(void) {
    hashmap_set_grow_by_power(map, 100);
    TEST_ASSERT_EQUAL_UINT(16, get_growpower(map));
}

/* Test: power = SIZE_MAX should clamp to 16 */
static void test_growpower_size_max_clamps_to_sixteen(void) {
    hashmap_set_grow_by_power(map, (size_t)-1);
    TEST_ASSERT_EQUAL_UINT(16, get_growpower(map));
}

/* Test: power = 2 should be stored as 2 */
static void test_growpower_two_stored_as_two(void) {
    hashmap_set_grow_by_power(map, 2);
    TEST_ASSERT_EQUAL_UINT(2, get_growpower(map));
}

/* Test: power = 15 (just below max) should be stored as 15 */
static void test_growpower_fifteen_stored_as_fifteen(void) {
    hashmap_set_grow_by_power(map, 15);
    TEST_ASSERT_EQUAL_UINT(15, get_growpower(map));
}

/* Test: calling multiple times overwrites previous value */
static void test_growpower_overwrite_previous_value(void) {
    hashmap_set_grow_by_power(map, 4);
    TEST_ASSERT_EQUAL_UINT(4, get_growpower(map));
    hashmap_set_grow_by_power(map, 10);
    TEST_ASSERT_EQUAL_UINT(10, get_growpower(map));
    hashmap_set_grow_by_power(map, 1);
    TEST_ASSERT_EQUAL_UINT(1, get_growpower(map));
}

/* Test: setting growpower does not corrupt map count */
static void test_growpower_does_not_corrupt_map_count(void) {
    int key = 42;
    hashmap_set(map, &key);
    hashmap_set_grow_by_power(map, 5);
    TEST_ASSERT_EQUAL_UINT(1, hashmap_count(map));
}

/* Test: setting growpower does not corrupt map data */
static void test_growpower_does_not_corrupt_map_data(void) {
    int key = 99;
    hashmap_set(map, &key);
    hashmap_set_grow_by_power(map, 7);
    const int *found = (const int *)hashmap_get(map, &key);
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_INT(99, *found);
}

/* Test: power = 3 boundary check */
static void test_growpower_three_stored_as_three(void) {
    hashmap_set_grow_by_power(map, 3);
    TEST_ASSERT_EQUAL_UINT(3, get_growpower(map));
}

/* Test: power = 16 is not clamped (exact boundary) */
static void test_growpower_exact_upper_boundary_not_clamped(void) {
    hashmap_set_grow_by_power(map, 16);
    TEST_ASSERT_EQUAL_UINT(16, get_growpower(map));
    /* Ensure 17 IS clamped */
    hashmap_set_grow_by_power(map, 17);
    TEST_ASSERT_EQUAL_UINT(16, get_growpower(map));
}

int main(void) {
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_growpower_zero_clamps_to_one);
    RUN_TEST(test_growpower_one_stored_as_one);
    RUN_TEST(test_growpower_mid_range_stored_correctly);
    RUN_TEST(test_growpower_sixteen_stored_as_sixteen);
    RUN_TEST(test_growpower_seventeen_clamps_to_sixteen);
    RUN_TEST(test_growpower_large_value_clamps_to_sixteen);
    RUN_TEST(test_growpower_size_max_clamps_to_sixteen);
    RUN_TEST(test_growpower_two_stored_as_two);
    RUN_TEST(test_growpower_fifteen_stored_as_fifteen);
    RUN_TEST(test_growpower_overwrite_previous_value);
    RUN_TEST(test_growpower_does_not_corrupt_map_count);
    RUN_TEST(test_growpower_does_not_corrupt_map_data);
    RUN_TEST(test_growpower_three_stored_as_three);
    RUN_TEST(test_growpower_exact_upper_boundary_not_clamped);
    return UNITY_END();
}