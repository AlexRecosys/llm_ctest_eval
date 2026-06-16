#include "hashmap.c"
#include "unity.h"
#include "hashmap.h"

static struct hashmap *map = NULL;
static uint64_t test_seed0 = 0x123456789ABCDEF0ULL;
static uint64_t test_seed1 = 0xFEDCBA9876543210ULL;

static uint64_t dummy_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    (void)item;
    (void)seed0;
    (void)seed1;
    return 0;
}

static int dummy_compare(const void *a, const void *b, void *udata) {
    (void)a;
    (void)b;
    (void)udata;
    return 0;
}

void setUp(void) {
    map = hashmap_new(sizeof(int), 16, test_seed0, test_seed1,
                      dummy_hash, dummy_compare, NULL, NULL);
    TEST_ASSERT_NOT_NULL(map);
}

void tearDown(void) {
    if (map) {
        hashmap_free(map);
        map = NULL;
    }
}

static double clamp_load_factor(double factor, double default_factor) {
    if (factor < 0.1) factor = 0.1;
    if (factor > 0.95) factor = 0.95;
    return factor;
}

void test_hashmap_set_load_factor_sets_loadfactor_and_growat_correctly(void) {
    double factor = 0.75;
    hashmap_set_load_factor(map, factor);

    TEST_ASSERT_EQUAL_DOUBLE_MESSAGE(75.0, map->loadfactor,
                                     "loadfactor should be factor * 100");
    TEST_ASSERT_EQUAL_DOUBLE_MESSAGE(12.0, map->growat,
                                     "growat should be nbuckets * (loadfactor / 100.0)");
}

void test_hashmap_set_load_factor_clamps_low_values(void) {
    double factor = 0.05;
    hashmap_set_load_factor(map, factor);

    // clamp_load_factor should clamp to 0.1
    TEST_ASSERT_EQUAL_DOUBLE_MESSAGE(10.0, map->loadfactor,
                                     "loadfactor should be clamped to 10.0");
    TEST_ASSERT_EQUAL_DOUBLE_MESSAGE(1.6, map->growat,
                                     "growat should be 16 * 0.1 = 1.6");
}

void test_hashmap_set_load_factor_clamps_high_values(void) {
    double factor = 0.99;
    hashmap_set_load_factor(map, factor);

    // clamp_load_factor should clamp to 0.95
    TEST_ASSERT_EQUAL_DOUBLE_MESSAGE(95.0, map->loadfactor,
                                     "loadfactor should be clamped to 95.0");
    TEST_ASSERT_EQUAL_DOUBLE_MESSAGE(15.2, map->growat,
                                     "growat should be 16 * 0.95 = 15.2");
}

void test_hashmap_set_load_factor_with_default_factor(void) {
    // Initial loadfactor is 0.75 (75.0) by default
    double factor = 0.75;
    hashmap_set_load_factor(map, factor);

    TEST_ASSERT_EQUAL_DOUBLE_MESSAGE(75.0, map->loadfactor,
                                     "loadfactor should be 75.0");
    TEST_ASSERT_EQUAL_DOUBLE_MESSAGE(12.0, map->growat,
                                     "growat should be 16 * 0.75 = 12.0");
}

int main(void) {
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_hashmap_set_load_factor_sets_loadfactor_and_growat_correctly);
    RUN_TEST(test_hashmap_set_load_factor_clamps_low_values);
    RUN_TEST(test_hashmap_set_load_factor_clamps_high_values);
    RUN_TEST(test_hashmap_set_load_factor_with_default_factor);
    return UNITY_END();
}