#include "hashmap.c"
#include "unity.h"
#include "hashmap.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

/* -------------------------------------------------------------------------
 * Fixtures / file-scope variables
 * ---------------------------------------------------------------------- */

typedef struct {
    int   key;
    int   value;
} int_item;

typedef struct {
    char  key[64];
    int   value;
} str_item;

static struct hashmap *g_map = NULL;

/* -------------------------------------------------------------------------
 * Helper callbacks
 * ---------------------------------------------------------------------- */

static uint64_t int_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
    const int_item *it = (const int_item *)item;
    return hashmap_murmur(&it->key, sizeof(it->key), seed0, seed1);
}

static int int_compare(const void *a, const void *b, void *udata)
{
    (void)udata;
    return ((const int_item *)a)->key - ((const int_item *)b)->key;
}

static uint64_t str_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
    const str_item *it = (const str_item *)item;
    return hashmap_murmur(it->key, strlen(it->key), seed0, seed1);
}

static int str_compare(const void *a, const void *b, void *udata)
{
    (void)udata;
    return strcmp(((const str_item *)a)->key, ((const str_item *)b)->key);
}

/* -------------------------------------------------------------------------
 * setUp / tearDown
 * ---------------------------------------------------------------------- */

void setUp(void)
{
    g_map = hashmap_new(sizeof(int_item), 0, 0, 0,
                        int_hash, int_compare, NULL, NULL);
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
 * Helper: compute the hash the same way the map would
 * ---------------------------------------------------------------------- */

static uint64_t compute_int_hash(const struct hashmap *map, int key)
{
    int_item tmp;
    memset(&tmp, 0, sizeof(tmp));
    tmp.key = key;
    /* hashmap_get_hash is not public; use the hash callback directly.
     * We replicate what hashmap_set/get does: call the user hash fn. */
    return int_hash(&tmp, 0, 0);   /* seeds 0,0 match hashmap_new defaults */
}

static uint64_t compute_str_hash(const char *key)
{
    str_item tmp;
    memset(&tmp, 0, sizeof(tmp));
    strncpy(tmp.key, key, sizeof(tmp.key) - 1);
    return str_hash(&tmp, 0, 0);
}

/* -------------------------------------------------------------------------
 * Test cases
 * ---------------------------------------------------------------------- */

/* 1. get on empty map returns NULL */
void test_get_with_hash_empty_map_returns_null(void)
{
    int_item key_item;
    memset(&key_item, 0, sizeof(key_item));
    key_item.key = 42;

    uint64_t h = compute_int_hash(g_map, 42);
    const void *result = hashmap_get_with_hash(g_map, &key_item, h);
    TEST_ASSERT_NULL(result);
}

/* 2. get existing item returns correct pointer */
void test_get_with_hash_existing_item_found(void)
{
    int_item item;
    memset(&item, 0, sizeof(item));
    item.key   = 7;
    item.value = 100;
    hashmap_set(g_map, &item);

    int_item key_item;
    memset(&key_item, 0, sizeof(key_item));
    key_item.key = 7;

    uint64_t h = compute_int_hash(g_map, 7);
    const int_item *result = (const int_item *)hashmap_get_with_hash(g_map, &key_item, h);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(7,   result->key);
    TEST_ASSERT_EQUAL_INT(100, result->value);
}

/* 3. get non-existing key returns NULL */
void test_get_with_hash_missing_key_returns_null(void)
{
    int_item item;
    memset(&item, 0, sizeof(item));
    item.key   = 1;
    item.value = 99;
    hashmap_set(g_map, &item);

    int_item key_item;
    memset(&key_item, 0, sizeof(key_item));
    key_item.key = 2;   /* different key */

    uint64_t h = compute_int_hash(g_map, 2);
    const void *result = hashmap_get_with_hash(g_map, &key_item, h);
    TEST_ASSERT_NULL(result);
}

/* 4. correct item returned when multiple items present */
void test_get_with_hash_multiple_items_correct_item(void)
{
    for (int i = 0; i < 20; i++) {
        int_item item;
        memset(&item, 0, sizeof(item));
        item.key   = i;
        item.value = i * 10;
        hashmap_set(g_map, &item);
    }

    for (int i = 0; i < 20; i++) {
        int_item key_item;
        memset(&key_item, 0, sizeof(key_item));
        key_item.key = i;

        uint64_t h = compute_int_hash(g_map, i);
        const int_item *result = (const int_item *)hashmap_get_with_hash(g_map, &key_item, h);
        TEST_ASSERT_NOT_NULL_MESSAGE(result, "Expected item not found");
        TEST_ASSERT_EQUAL_INT(i,      result->key);
        TEST_ASSERT_EQUAL_INT(i * 10, result->value);
    }
}

/* 5. wrong hash (hash mismatch) causes miss even if key exists */
void test_get_with_hash_wrong_hash_returns_null(void)
{
    int_item item;
    memset(&item, 0, sizeof(item));
    item.key   = 5;
    item.value = 55;
    hashmap_set(g_map, &item);

    int_item key_item;
    memset(&key_item, 0, sizeof(key_item));
    key_item.key = 5;

    /* Deliberately use a hash that is very unlikely to match */
    uint64_t wrong_hash = compute_int_hash(g_map, 5) ^ 0xDEADBEEFCAFEBABEULL;
    const void *result = hashmap_get_with_hash(g_map, &key_item, wrong_hash);
    /* With a wrong hash the bucket chain will not contain the item */
    TEST_ASSERT_NULL(result);
}

/* 6. get after delete returns NULL */
void test_get_with_hash_after_delete_returns_null(void)
{
    int_item item;
    memset(&item, 0, sizeof(item));
    item.key   = 3;
    item.value = 33;
    hashmap_set(g_map, &item);

    int_item key_item;
    memset(&key_item, 0, sizeof(key_item));
    key_item.key = 3;

    hashmap_delete(g_map, &key_item);

    uint64_t h = compute_int_hash(g_map, 3);
    const void *result = hashmap_get_with_hash(g_map, &key_item, h);
    TEST_ASSERT_NULL(result);
}

/* 7. get after update returns updated value */
void test_get_with_hash_after_update_returns_new_value(void)
{
    int_item item;
    memset(&item, 0, sizeof(item));
    item.key   = 9;
    item.value = 1;
    hashmap_set(g_map, &item);

    item.value = 999;
    hashmap_set(g_map, &item);   /* overwrite */

    int_item key_item;
    memset(&key_item, 0, sizeof(key_item));
    key_item.key = 9;

    uint64_t h = compute_int_hash(g_map, 9);
    const int_item *result = (const int_item *)hashmap_get_with_hash(g_map, &key_item, h);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(999, result->value);
}

/* 8. string-keyed map: found */
void test_get_with_hash_string_key_found(void)
{
    struct hashmap *smap = hashmap_new(sizeof(str_item), 0, 0, 0,
                                       str_hash, str_compare, NULL, NULL);
    TEST_ASSERT_NOT_NULL(smap);

    str_item item;
    memset(&item, 0, sizeof(item));
    strncpy(item.key, "hello", sizeof(item.key) - 1);
    item.value = 42;
    hashmap_set(smap, &item);

    str_item key_item;
    memset(&key_item, 0, sizeof(key_item));
    strncpy(key_item.key, "hello", sizeof(key_item.key) - 1);

    uint64_t h = compute_str_hash("hello");
    const str_item *result = (const str_item *)hashmap_get_with_hash(smap, &key_item, h);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("hello", result->key);
    TEST_ASSERT_EQUAL_INT(42, result->value);

    hashmap_free(smap);
}

/* 9. string-keyed map: not found */
void test_get_with_hash_string_key_not_found(void)
{
    struct hashmap *smap = hashmap_new(sizeof(str_item), 0, 0, 0,
                                       str_hash, str_compare, NULL, NULL);
    TEST_ASSERT_NOT_NULL(smap);

    str_item item;
    memset(&item, 0, sizeof(item));
    strncpy(item.key, "world", sizeof(item.key) - 1);
    item.value = 7;
    hashmap_set(smap, &item);

    str_item key_item;
    memset(&key_item, 0, sizeof(key_item));
    strncpy(key_item.key, "missing", sizeof(key_item.key) - 1);

    uint64_t h = compute_str_hash("missing");
    const void *result = hashmap_get_with_hash(smap, &key_item, h);
    TEST_ASSERT_NULL(result);

    hashmap_free(smap);
}

/* 10. large number of insertions — every item retrievable by hash */
void test_get_with_hash_large_insertion_all_found(void)
{
    const int N = 500;
    for (int i = 0; i < N; i++) {
        int_item item;
        memset(&item, 0, sizeof(item));
        item.key   = i;
        item.value = i + 1000;
        hashmap_set(g_map, &item);
    }

    for (int i = 0; i < N; i++) {
        int_item key_item;
        memset(&key_item, 0, sizeof(key_item));
        key_item.key = i;

        uint64_t h = compute_int_hash(g_map, i);
        const int_item *result = (const int_item *)hashmap_get_with_hash(g_map, &key_item, h);
        TEST_ASSERT_NOT_NULL_MESSAGE(result, "Large map: item not found");
        TEST_ASSERT_EQUAL_INT(i,          result->key);
        TEST_ASSERT_EQUAL_INT(i + 1000,   result->value);
    }
}

/* 11. key 0 (boundary) is found correctly */
void test_get_with_hash_key_zero_found(void)
{
    int_item item;
    memset(&item, 0, sizeof(item));
    item.key   = 0;
    item.value = 77;
    hashmap_set(g_map, &item);

    int_item key_item;
    memset(&key_item, 0, sizeof(key_item));
    key_item.key = 0;

    uint64_t h = compute_int_hash(g_map, 0);
    const int_item *result = (const int_item *)hashmap_get_with_hash(g_map, &key_item, h);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(0,  result->key);
    TEST_ASSERT_EQUAL_INT(77, result->value);
}

/* 12. negative key is found correctly */
void test_get_with_hash_negative_key_found(void)
{
    int_item item;
    memset(&item, 0, sizeof(item));
    item.key   = -1;
    item.value = 88;
    hashmap_set(g_map, &item);

    int_item key_item;
    memset(&key_item, 0, sizeof(key_item));
    key_item.key = -1;

    uint64_t h = compute_int_hash(g_map, -1);
    const int_item *result = (const int_item *)hashmap_get_with_hash(g_map, &key_item, h);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(-1, result->key);
    TEST_ASSERT_EQUAL_INT(88, result->value);
}

/* -------------------------------------------------------------------------
 * main
 * ---------------------------------------------------------------------- */

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_get_with_hash_empty_map_returns_null);
    RUN_TEST(test_get_with_hash_existing_item_found);
    RUN_TEST(test_get_with_hash_missing_key_returns_null);
    RUN_TEST(test_get_with_hash_multiple_items_correct_item);
    RUN_TEST(test_get_with_hash_wrong_hash_returns_null);
    RUN_TEST(test_get_with_hash_after_delete_returns_null);
    RUN_TEST(test_get_with_hash_after_update_returns_new_value);
    RUN_TEST(test_get_with_hash_string_key_found);
    RUN_TEST(test_get_with_hash_string_key_not_found);
    RUN_TEST(test_get_with_hash_large_insertion_all_found);
    RUN_TEST(test_get_with_hash_key_zero_found);
    RUN_TEST(test_get_with_hash_negative_key_found);
    return UNITY_END();
}