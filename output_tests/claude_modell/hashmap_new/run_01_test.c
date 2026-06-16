#include "hashmap.c"
#include "unity.h"
#include "hashmap.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

/* -------------------------------------------------------------------------
 * File-scope fixtures
 * ------------------------------------------------------------------------- */

static struct hashmap *g_map = NULL;

/* -------------------------------------------------------------------------
 * Helper types and callbacks
 * ------------------------------------------------------------------------- */

typedef struct {
    int   key;
    int   value;
} IntItem;

typedef struct {
    char  key[64];
    int   value;
} StrItem;

static uint64_t int_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
    const IntItem *it = (const IntItem *)item;
    return hashmap_sip(&it->key, sizeof(it->key), seed0, seed1);
}

static int int_compare(const void *a, const void *b, void *udata)
{
    (void)udata;
    const IntItem *ia = (const IntItem *)a;
    const IntItem *ib = (const IntItem *)b;
    return ia->key - ib->key;
}

static uint64_t str_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
    const StrItem *it = (const StrItem *)item;
    return hashmap_sip(it->key, strlen(it->key), seed0, seed1);
}

static int str_compare(const void *a, const void *b, void *udata)
{
    (void)udata;
    const StrItem *sa = (const StrItem *)a;
    const StrItem *sb = (const StrItem *)b;
    return strcmp(sa->key, sb->key);
}

static int g_elfree_count = 0;

static void counting_elfree(void *item)
{
    (void)item;
    g_elfree_count++;
}

/* -------------------------------------------------------------------------
 * setUp / tearDown
 * ------------------------------------------------------------------------- */

void setUp(void)
{
    g_map = NULL;
    g_elfree_count = 0;
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
 * ------------------------------------------------------------------------- */

void test_hashmap_new_returns_non_null(void)
{
    g_map = hashmap_new(sizeof(IntItem), 0, 0, 0,
                        int_hash, int_compare, NULL, NULL);
    TEST_ASSERT_NOT_NULL(g_map);
}

void test_hashmap_new_initial_count_is_zero(void)
{
    g_map = hashmap_new(sizeof(IntItem), 0, 0, 0,
                        int_hash, int_compare, NULL, NULL);
    TEST_ASSERT_NOT_NULL(g_map);
    TEST_ASSERT_EQUAL_UINT(0, hashmap_count(g_map));
}

void test_hashmap_new_with_nonzero_cap(void)
{
    g_map = hashmap_new(sizeof(IntItem), 64, 0, 0,
                        int_hash, int_compare, NULL, NULL);
    TEST_ASSERT_NOT_NULL(g_map);
    TEST_ASSERT_EQUAL_UINT(0, hashmap_count(g_map));
}

void test_hashmap_new_with_seeds(void)
{
    g_map = hashmap_new(sizeof(IntItem), 0,
                        0xDEADBEEFDEADBEEFULL,
                        0xCAFEBABECAFEBABEULL,
                        int_hash, int_compare, NULL, NULL);
    TEST_ASSERT_NOT_NULL(g_map);
}

void test_hashmap_new_set_and_get(void)
{
    g_map = hashmap_new(sizeof(IntItem), 0, 0, 0,
                        int_hash, int_compare, NULL, NULL);
    TEST_ASSERT_NOT_NULL(g_map);

    IntItem item = {42, 100};
    hashmap_set(g_map, &item);

    IntItem key = {42, 0};
    const IntItem *found = (const IntItem *)hashmap_get(g_map, &key);
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_INT(42,  found->key);
    TEST_ASSERT_EQUAL_INT(100, found->value);
}

void test_hashmap_new_get_missing_returns_null(void)
{
    g_map = hashmap_new(sizeof(IntItem), 0, 0, 0,
                        int_hash, int_compare, NULL, NULL);
    TEST_ASSERT_NOT_NULL(g_map);

    IntItem key = {99, 0};
    const IntItem *found = (const IntItem *)hashmap_get(g_map, &key);
    TEST_ASSERT_NULL(found);
}

void test_hashmap_new_count_increments_on_set(void)
{
    g_map = hashmap_new(sizeof(IntItem), 0, 0, 0,
                        int_hash, int_compare, NULL, NULL);
    TEST_ASSERT_NOT_NULL(g_map);

    IntItem item;
    for (int i = 0; i < 10; i++) {
        item.key   = i;
        item.value = i * 2;
        hashmap_set(g_map, &item);
    }
    TEST_ASSERT_EQUAL_UINT(10, hashmap_count(g_map));
}

void test_hashmap_new_overwrite_existing_key(void)
{
    g_map = hashmap_new(sizeof(IntItem), 0, 0, 0,
                        int_hash, int_compare, NULL, NULL);
    TEST_ASSERT_NOT_NULL(g_map);

    IntItem item1 = {7, 111};
    IntItem item2 = {7, 222};
    hashmap_set(g_map, &item1);
    hashmap_set(g_map, &item2);

    TEST_ASSERT_EQUAL_UINT(1, hashmap_count(g_map));

    IntItem key = {7, 0};
    const IntItem *found = (const IntItem *)hashmap_get(g_map, &key);
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_INT(222, found->value);
}

void test_hashmap_new_delete_item(void)
{
    g_map = hashmap_new(sizeof(IntItem), 0, 0, 0,
                        int_hash, int_compare, NULL, NULL);
    TEST_ASSERT_NOT_NULL(g_map);

    IntItem item = {5, 50};
    hashmap_set(g_map, &item);

    IntItem key = {5, 0};
    const IntItem *deleted = (const IntItem *)hashmap_delete(g_map, &key);
    TEST_ASSERT_NOT_NULL(deleted);
    TEST_ASSERT_EQUAL_INT(5,  deleted->key);
    TEST_ASSERT_EQUAL_INT(50, deleted->value);
    TEST_ASSERT_EQUAL_UINT(0, hashmap_count(g_map));
}

void test_hashmap_new_with_elfree_callback(void)
{
    g_map = hashmap_new(sizeof(IntItem), 0, 0, 0,
                        int_hash, int_compare, counting_elfree, NULL);
    TEST_ASSERT_NOT_NULL(g_map);

    IntItem item = {1, 10};
    hashmap_set(g_map, &item);

    /* freeing the map should invoke elfree for each stored item */
    hashmap_free(g_map);
    g_map = NULL;

    TEST_ASSERT_EQUAL_INT(1, g_elfree_count);
}

void test_hashmap_new_with_udata(void)
{
    int udata_value = 0xABCD;
    g_map = hashmap_new(sizeof(IntItem), 0, 0, 0,
                        int_hash, int_compare, NULL, &udata_value);
    TEST_ASSERT_NOT_NULL(g_map);

    /* Verify the map is functional when udata is provided */
    IntItem item = {3, 30};
    hashmap_set(g_map, &item);

    IntItem key = {3, 0};
    const IntItem *found = (const IntItem *)hashmap_get(g_map, &key);
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_INT(30, found->value);
}

void test_hashmap_new_string_key_type(void)
{
    g_map = hashmap_new(sizeof(StrItem), 0, 0, 0,
                        str_hash, str_compare, NULL, NULL);
    TEST_ASSERT_NOT_NULL(g_map);

    StrItem item;
    strncpy(item.key, "hello", sizeof(item.key) - 1);
    item.key[sizeof(item.key) - 1] = '\0';
    item.value = 42;
    hashmap_set(g_map, &item);

    StrItem key;
    strncpy(key.key, "hello", sizeof(key.key) - 1);
    key.key[sizeof(key.key) - 1] = '\0';
    key.value = 0;

    const StrItem *found = (const StrItem *)hashmap_get(g_map, &key);
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_STRING("hello", found->key);
    TEST_ASSERT_EQUAL_INT(42, found->value);
}

void test_hashmap_new_large_cap_hint(void)
{
    g_map = hashmap_new(sizeof(IntItem), 1024, 0, 0,
                        int_hash, int_compare, NULL, NULL);
    TEST_ASSERT_NOT_NULL(g_map);
    TEST_ASSERT_EQUAL_UINT(0, hashmap_count(g_map));
}

void test_hashmap_new_many_items_no_crash(void)
{
    g_map = hashmap_new(sizeof(IntItem), 0, 0, 0,
                        int_hash, int_compare, NULL, NULL);
    TEST_ASSERT_NOT_NULL(g_map);

    IntItem item;
    for (int i = 0; i < 1000; i++) {
        item.key   = i;
        item.value = i;
        hashmap_set(g_map, &item);
    }
    TEST_ASSERT_EQUAL_UINT(1000, hashmap_count(g_map));

    /* Verify a sample of items */
    for (int i = 0; i < 1000; i += 100) {
        IntItem key = {i, 0};
        const IntItem *found = (const IntItem *)hashmap_get(g_map, &key);
        TEST_ASSERT_NOT_NULL(found);
        TEST_ASSERT_EQUAL_INT(i, found->value);
    }
}

void test_hashmap_new_elfree_called_multiple_items(void)
{
    g_map = hashmap_new(sizeof(IntItem), 0, 0, 0,
                        int_hash, int_compare, counting_elfree, NULL);
    TEST_ASSERT_NOT_NULL(g_map);

    IntItem item;
    for (int i = 0; i < 5; i++) {
        item.key   = i;
        item.value = i;
        hashmap_set(g_map, &item);
    }

    hashmap_free(g_map);
    g_map = NULL;

    TEST_ASSERT_EQUAL_INT(5, g_elfree_count);
}

void test_hashmap_new_clear_resets_count(void)
{
    g_map = hashmap_new(sizeof(IntItem), 0, 0, 0,
                        int_hash, int_compare, NULL, NULL);
    TEST_ASSERT_NOT_NULL(g_map);

    IntItem item;
    for (int i = 0; i < 10; i++) {
        item.key   = i;
        item.value = i;
        hashmap_set(g_map, &item);
    }
    TEST_ASSERT_EQUAL_UINT(10, hashmap_count(g_map));

    hashmap_clear(g_map, false);
    TEST_ASSERT_EQUAL_UINT(0, hashmap_count(g_map));
}

void test_hashmap_new_different_seeds_produce_valid_maps(void)
{
    struct hashmap *map1 = hashmap_new(sizeof(IntItem), 0,
                                       0x0000000000000001ULL,
                                       0x0000000000000002ULL,
                                       int_hash, int_compare, NULL, NULL);
    struct hashmap *map2 = hashmap_new(sizeof(IntItem), 0,
                                       0xFFFFFFFFFFFFFFFFULL,
                                       0xFFFFFFFFFFFFFFFEULL,
                                       int_hash, int_compare, NULL, NULL);
    TEST_ASSERT_NOT_NULL(map1);
    TEST_ASSERT_NOT_NULL(map2);

    IntItem item = {1, 99};
    hashmap_set(map1, &item);
    hashmap_set(map2, &item);

    IntItem key = {1, 0};
    TEST_ASSERT_NOT_NULL(hashmap_get(map1, &key));
    TEST_ASSERT_NOT_NULL(hashmap_get(map2, &key));

    hashmap_free(map1);
    hashmap_free(map2);
}

/* -------------------------------------------------------------------------
 * main
 * ------------------------------------------------------------------------- */

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_hashmap_new_returns_non_null);
    RUN_TEST(test_hashmap_new_initial_count_is_zero);
    RUN_TEST(test_hashmap_new_with_nonzero_cap);
    RUN_TEST(test_hashmap_new_with_seeds);
    RUN_TEST(test_hashmap_new_set_and_get);
    RUN_TEST(test_hashmap_new_get_missing_returns_null);
    RUN_TEST(test_hashmap_new_count_increments_on_set);
    RUN_TEST(test_hashmap_new_overwrite_existing_key);
    RUN_TEST(test_hashmap_new_delete_item);
    RUN_TEST(test_hashmap_new_with_elfree_callback);
    RUN_TEST(test_hashmap_new_with_udata);
    RUN_TEST(test_hashmap_new_string_key_type);
    RUN_TEST(test_hashmap_new_large_cap_hint);
    RUN_TEST(test_hashmap_new_many_items_no_crash);
    RUN_TEST(test_hashmap_new_elfree_called_multiple_items);
    RUN_TEST(test_hashmap_new_clear_resets_count);
    RUN_TEST(test_hashmap_new_different_seeds_produce_valid_maps);
    return UNITY_END();
}