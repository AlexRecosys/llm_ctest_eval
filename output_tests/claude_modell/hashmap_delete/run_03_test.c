#include "hashmap.c"
#include "unity.h"
#include "hashmap.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* -------------------------------------------------------------------------
 * Fixture types and file-scope variables
 * ---------------------------------------------------------------------- */

struct int_item {
    int key;
    int value;
};

static struct hashmap *g_map;

/* -------------------------------------------------------------------------
 * Helper functions
 * ---------------------------------------------------------------------- */

static uint64_t int_item_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
    const struct int_item *it = (const struct int_item *)item;
    return hashmap_murmur(&it->key, sizeof(it->key), seed0, seed1);
}

static int int_item_compare(const void *a, const void *b, void *udata)
{
    const struct int_item *ia = (const struct int_item *)a;
    const struct int_item *ib = (const struct int_item *)b;
    (void)udata;
    return ia->key - ib->key;
}

static struct hashmap *create_int_map(void)
{
    return hashmap_new(sizeof(struct int_item), 0, 0, 0,
                       int_item_hash, int_item_compare, NULL, NULL);
}

/* -------------------------------------------------------------------------
 * setUp / tearDown
 * ---------------------------------------------------------------------- */

void setUp(void)
{
    g_map = create_int_map();
    TEST_ASSERT_NOT_NULL(g_map);
}

void tearDown(void)
{
    if (g_map) {
        hashmap_free(g_map);
        g_map = NULL;
    }
}

/* -------------------------------------------------------------------------
 * Test cases
 * ---------------------------------------------------------------------- */

void test_delete_existing_key_returns_non_null(void)
{
    struct int_item item = {.key = 42, .value = 100};
    hashmap_set(g_map, &item);

    struct int_item lookup = {.key = 42};
    const void *result = hashmap_delete(g_map, &lookup);

    TEST_ASSERT_NOT_NULL(result);
}

void test_delete_existing_key_returns_correct_value(void)
{
    struct int_item item = {.key = 7, .value = 999};
    hashmap_set(g_map, &item);

    struct int_item lookup = {.key = 7};
    const struct int_item *result =
        (const struct int_item *)hashmap_delete(g_map, &lookup);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(7, result->key);
    TEST_ASSERT_EQUAL_INT(999, result->value);
}

void test_delete_nonexistent_key_returns_null(void)
{
    struct int_item lookup = {.key = 123};
    const void *result = hashmap_delete(g_map, &lookup);

    TEST_ASSERT_NULL(result);
}

void test_delete_reduces_count(void)
{
    struct int_item item = {.key = 1, .value = 10};
    hashmap_set(g_map, &item);

    TEST_ASSERT_EQUAL_INT(1, (int)hashmap_count(g_map));

    struct int_item lookup = {.key = 1};
    hashmap_delete(g_map, &lookup);

    TEST_ASSERT_EQUAL_INT(0, (int)hashmap_count(g_map));
}

void test_delete_then_get_returns_null(void)
{
    struct int_item item = {.key = 55, .value = 77};
    hashmap_set(g_map, &item);

    struct int_item lookup = {.key = 55};
    hashmap_delete(g_map, &lookup);

    const void *found = hashmap_get(g_map, &lookup);
    TEST_ASSERT_NULL(found);
}

void test_delete_from_empty_map_returns_null(void)
{
    struct int_item lookup = {.key = 0};
    const void *result = hashmap_delete(g_map, &lookup);

    TEST_ASSERT_NULL(result);
}

void test_delete_only_removes_target_key(void)
{
    struct int_item a = {.key = 10, .value = 1};
    struct int_item b = {.key = 20, .value = 2};
    struct int_item c = {.key = 30, .value = 3};

    hashmap_set(g_map, &a);
    hashmap_set(g_map, &b);
    hashmap_set(g_map, &c);

    struct int_item lookup = {.key = 20};
    hashmap_delete(g_map, &lookup);

    TEST_ASSERT_EQUAL_INT(2, (int)hashmap_count(g_map));

    struct int_item la = {.key = 10};
    struct int_item lc = {.key = 30};

    TEST_ASSERT_NOT_NULL(hashmap_get(g_map, &la));
    TEST_ASSERT_NOT_NULL(hashmap_get(g_map, &lc));
    TEST_ASSERT_NULL(hashmap_get(g_map, &lookup));
}

void test_delete_same_key_twice_second_returns_null(void)
{
    struct int_item item = {.key = 5, .value = 50};
    hashmap_set(g_map, &item);

    struct int_item lookup = {.key = 5};
    const void *first  = hashmap_delete(g_map, &lookup);
    const void *second = hashmap_delete(g_map, &lookup);

    TEST_ASSERT_NOT_NULL(first);
    TEST_ASSERT_NULL(second);
}

void test_delete_all_items_leaves_empty_map(void)
{
    int n = 10;
    for (int i = 0; i < n; i++) {
        struct int_item item = {.key = i, .value = i * 2};
        hashmap_set(g_map, &item);
    }

    for (int i = 0; i < n; i++) {
        struct int_item lookup = {.key = i};
        hashmap_delete(g_map, &lookup);
    }

    TEST_ASSERT_EQUAL_INT(0, (int)hashmap_count(g_map));
}

void test_delete_and_reinsert_key(void)
{
    struct int_item item = {.key = 99, .value = 111};
    hashmap_set(g_map, &item);

    struct int_item lookup = {.key = 99};
    hashmap_delete(g_map, &lookup);

    struct int_item item2 = {.key = 99, .value = 222};
    hashmap_set(g_map, &item2);

    const struct int_item *found =
        (const struct int_item *)hashmap_get(g_map, &lookup);

    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_INT(222, found->value);
}

void test_delete_many_items_count_consistent(void)
{
    int n = 50;
    for (int i = 0; i < n; i++) {
        struct int_item item = {.key = i, .value = i};
        hashmap_set(g_map, &item);
    }

    int deleted = 0;
    for (int i = 0; i < n; i += 2) {
        struct int_item lookup = {.key = i};
        const void *r = hashmap_delete(g_map, &lookup);
        if (r) deleted++;
    }

    TEST_ASSERT_EQUAL_INT(n / 2, deleted);
    TEST_ASSERT_EQUAL_INT(n - deleted, (int)hashmap_count(g_map));
}

void test_delete_returns_copy_of_stored_item(void)
{
    struct int_item item = {.key = 3, .value = 333};
    hashmap_set(g_map, &item);

    struct int_item lookup = {.key = 3};
    const struct int_item *result =
        (const struct int_item *)hashmap_delete(g_map, &lookup);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(3,   result->key);
    TEST_ASSERT_EQUAL_INT(333, result->value);
}

/* -------------------------------------------------------------------------
 * main
 * ---------------------------------------------------------------------- */

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_delete_existing_key_returns_non_null);
    RUN_TEST(test_delete_existing_key_returns_correct_value);
    RUN_TEST(test_delete_nonexistent_key_returns_null);
    RUN_TEST(test_delete_reduces_count);
    RUN_TEST(test_delete_then_get_returns_null);
    RUN_TEST(test_delete_from_empty_map_returns_null);
    RUN_TEST(test_delete_only_removes_target_key);
    RUN_TEST(test_delete_same_key_twice_second_returns_null);
    RUN_TEST(test_delete_all_items_leaves_empty_map);
    RUN_TEST(test_delete_and_reinsert_key);
    RUN_TEST(test_delete_many_items_count_consistent);
    RUN_TEST(test_delete_returns_copy_of_stored_item);
    return UNITY_END();
}