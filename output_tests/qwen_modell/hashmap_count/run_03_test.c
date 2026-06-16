#include "hashmap.c"
#include "unity.h"
#include "hashmap.h"

static struct hashmap *map = NULL;
static uint64_t test_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    (void)item;
    (void)seed0;
    (void)seed1;
    return 0;
}
static int test_compare(const void *a, const void *b, void *udata) {
    (void)a;
    (void)b;
    (void)udata;
    return 0;
}

void setUp(void) {
    map = hashmap_new(sizeof(int), 0, 0, 0, test_hash, test_compare, NULL, NULL);
    TEST_ASSERT_NOT_NULL(map);
}

void tearDown(void) {
    hashmap_free(map);
    map = NULL;
}

void test_hashmap_count_returns_zero_for_new_map(void) {
    TEST_ASSERT_EQUAL_INT(0, hashmap_count(map));
}

void test_hashmap_count_increases_after_insertion(void) {
    int val = 42;
    const void *ret = hashmap_set(map, &val);
    TEST_ASSERT_NULL(ret);
    TEST_ASSERT_EQUAL_INT(1, hashmap_count(map));
}

void test_hashmap_count_remains_unchanged_after_replacement(void) {
    int val1 = 42;
    int val2 = 99;
    const void *ret1 = hashmap_set(map, &val1);
    TEST_ASSERT_NULL(ret1);
    TEST_ASSERT_EQUAL_INT(1, hashmap_count(map));
    const void *ret2 = hashmap_set(map, &val1);
    TEST_ASSERT_EQUAL_PTR(&val1, ret2);
    TEST_ASSERT_EQUAL_INT(1, hashmap_count(map));
}

void test_hashmap_count_decreases_after_deletion(void) {
    int val = 42;
    hashmap_set(map, &val);
    TEST_ASSERT_EQUAL_INT(1, hashmap_count(map));
    const void *ret = hashmap_delete(map, &val);
    TEST_ASSERT_NOT_NULL(ret);
    TEST_ASSERT_EQUAL_INT(0, hashmap_count(map));
}

void test_hashmap_count_remains_unchanged_after_clear(void) {
    int val = 42;
    hashmap_set(map, &val);
    TEST_ASSERT_EQUAL_INT(1, hashmap_count(map));
    hashmap_clear(map, false);
    TEST_ASSERT_EQUAL_INT(0, hashmap_count(map));
}

int main(void) {
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_hashmap_count_returns_zero_for_new_map);
    RUN_TEST(test_hashmap_count_increases_after_insertion);
    RUN_TEST(test_hashmap_count_remains_unchanged_after_replacement);
    RUN_TEST(test_hashmap_count_decreases_after_deletion);
    RUN_TEST(test_hashmap_count_remains_unchanged_after_clear);
    return UNITY_END();
}