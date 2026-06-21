#include "hashmap.c"
#include "unity.h"
#include <stdlib.h>
#include <string.h>

// Helper to create a minimal valid hashmap for testing
static struct hashmap *create_test_map(void) {
    return hashmap_new(sizeof(int), 16, 0, 0, NULL, NULL, NULL, NULL);
}

void setUp(void) {}
void tearDown(void) {}

static void test_hashmap_set_grow_by_power_clamps_low(void) {
    struct hashmap *map = create_test_map();
    TEST_ASSERT_NOT_NULL(map);

    hashmap_set_grow_by_power(map, 0);
    TEST_ASSERT_EQUAL_INT(1, map->growpower);

    hashmap_set_grow_by_power(map, 1);
    TEST_ASSERT_EQUAL_INT(1, map->growpower);

    hashmap_free(map);
}

static void test_hashmap_set_grow_by_power_preserves_valid_values(void) {
    struct hashmap *map = create_test_map();
    TEST_ASSERT_NOT_NULL(map);

    hashmap_set_grow_by_power(map, 2);
    TEST_ASSERT_EQUAL_INT(2, map->growpower);

    hashmap_set_grow_by_power(map, 8);
    TEST_ASSERT_EQUAL_INT(8, map->growpower);

    hashmap_set_grow_by_power(map, 15);
    TEST_ASSERT_EQUAL_INT(15, map->growpower);

    hashmap_free(map);
}

static void test_hashmap_set_grow_by_power_clamps_high(void) {
    struct hashmap *map = create_test_map();
    TEST_ASSERT_NOT_NULL(map);

    hashmap_set_grow_by_power(map, 16);
    TEST_ASSERT_EQUAL_INT(16, map->growpower);

    hashmap_set_grow_by_power(map, 17);
    TEST_ASSERT_EQUAL_INT(16, map->growpower);

    hashmap_set_grow_by_power(map, 100);
    TEST_ASSERT_EQUAL_INT(16, map->growpower);

    hashmap_free(map);
}

int main(void) {
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_hashmap_set_grow_by_power_clamps_low);
    RUN_TEST(test_hashmap_set_grow_by_power_preserves_valid_values);
    RUN_TEST(test_hashmap_set_grow_by_power_clamps_high);
    return UNITY_END();
}