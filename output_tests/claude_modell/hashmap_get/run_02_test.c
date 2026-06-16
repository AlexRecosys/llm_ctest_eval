#include "hashmap.c"
#include "unity.h"
#include "hashmap.h"
#include <string.h>
#include <stdlib.h>

/* -------------------------------------------------------------------------
 * Fixtures / file-scope variables
 * ---------------------------------------------------------------------- */

typedef struct {
    int   key;
    int   value;
} int_item_t;

typedef struct {
    char  key[64];
    int   value;
} str_item_t;

static struct hashmap *int_map  = NULL;
static struct hashmap *str_map  = NULL;

/* -------------------------------------------------------------------------
 * Helper callbacks
 * ---------------------------------------------------------------------- */

static uint64_t int_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
    const int_item_t *e = (const int_item_t *)item;
    return hashmap_murmur(&e->key, sizeof(e->key), seed0, seed1);
}

static int int_compare(const void *a, const void *b, void *udata)
{
    (void)udata;
    const int_item_t *ia = (const int_item_t *)a;
    const int_item_t *ib = (const int_item_t *)b;
    return ia->key - ib->key;
}

static uint64_t str_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
    const str_item_t *e = (const str_item_t *)item;
    return hashmap_murmur(e->key, strlen(e->key), seed0, seed1);
}

static int str_compare(const void *a, const void *b, void *udata)
{
    (void)udata;
    const str_item_t *ia = (const str_item_t *)a;
    const str_item_t *ib = (const str_item_t *)b;
    return strcmp(ia->key, ib->key);
}

/* -------------------------------------------------------------------------
 * setUp / tearDown
 * ---------------------------------------------------------------------- */

void setUp(void)
{
    int_map = hashmap_new(sizeof(int_item_t), 0, 0, 0,
                          int_hash, int_compare, NULL, NULL);
    str_map = hashmap_new(sizeof(str_item_t), 0, 0, 0,
                          str_hash, str_compare, NULL, NULL);
}

void tearDown(void)
{
    if (int_map) {
        hashmap_free(int_map);
        int_map = NULL;
    }
    if (str_map) {
        hashmap_free(str_map);
        str_map = NULL;
    }
}

/* -------------------------------------------------------------------------
 * Test cases
 * ---------------------------------------------------------------------- */

/* Searching an empty map must return NULL */
void test_hashmap_get_empty_map_returns_null(void)
{
    int_item_t key = { .key = 42, .value = 0 };
    const void *result = hashmap_get(int_map, &key);
    TEST_ASSERT_NULL(result);
}

/* After inserting one item, get must find it */
void test_hashmap_get_existing_key_returns_item(void)
{
    int_item_t item = { .key = 7, .value = 100 };
    hashmap_set(int_map, &item);

    int_item_t key = { .key = 7, .value = 0 };
    const void *result = hashmap_get(int_map, &key);

    TEST_ASSERT_NOT_NULL(result);
    const int_item_t *found = (const int_item_t *)result;
    TEST_ASSERT_EQUAL_INT(7,   found->key);
    TEST_ASSERT_EQUAL_INT(100, found->value);
}

/* Searching for a key that was never inserted must return NULL */
void test_hashmap_get_missing_key_returns_null(void)
{
    int_item_t item = { .key = 1, .value = 10 };
    hashmap_set(int_map, &item);

    int_item_t key = { .key = 999, .value = 0 };
    const void *result = hashmap_get(int_map, &key);
    TEST_ASSERT_NULL(result);
}

/* get must return the most recently set value for a key */
void test_hashmap_get_returns_updated_value_after_overwrite(void)
{
    int_item_t item1 = { .key = 5, .value = 11 };
    int_item_t item2 = { .key = 5, .value = 22 };
    hashmap_set(int_map, &item1);
    hashmap_set(int_map, &item2);

    int_item_t key = { .key = 5, .value = 0 };
    const void *result = hashmap_get(int_map, &key);

    TEST_ASSERT_NOT_NULL(result);
    const int_item_t *found = (const int_item_t *)result;
    TEST_ASSERT_EQUAL_INT(22, found->value);
}

/* get must work correctly with string keys */
void test_hashmap_get_string_key_found(void)
{
    str_item_t item;
    memset(&item, 0, sizeof(item));
    strncpy(item.key, "hello", sizeof(item.key) - 1);
    item.value = 42;
    hashmap_set(str_map, &item);

    str_item_t key;
    memset(&key, 0, sizeof(key));
    strncpy(key.key, "hello", sizeof(key.key) - 1);

    const void *result = hashmap_get(str_map, &key);
    TEST_ASSERT_NOT_NULL(result);
    const str_item_t *found = (const str_item_t *)result;
    TEST_ASSERT_EQUAL_STRING("hello", found->key);
    TEST_ASSERT_EQUAL_INT(42, found->value);
}

/* get must return NULL for a string key that was never inserted */
void test_hashmap_get_string_key_not_found(void)
{
    str_item_t item;
    memset(&item, 0, sizeof(item));
    strncpy(item.key, "world", sizeof(item.key) - 1);
    item.value = 99;
    hashmap_set(str_map, &item);

    str_item_t key;
    memset(&key, 0, sizeof(key));
    strncpy(key.key, "missing", sizeof(key.key) - 1);

    const void *result = hashmap_get(str_map, &key);
    TEST_ASSERT_NULL(result);
}

/* Insert many items and verify each one is retrievable (stress / collision) */
void test_hashmap_get_multiple_items_all_found(void)
{
    const int N = 200;
    for (int i = 0; i < N; i++) {
        int_item_t item = { .key = i, .value = i * 3 };
        hashmap_set(int_map, &item);
    }

    for (int i = 0; i < N; i++) {
        int_item_t key = { .key = i, .value = 0 };
        const void *result = hashmap_get(int_map, &key);
        TEST_ASSERT_NOT_NULL_MESSAGE(result, "Expected item not found");
        const int_item_t *found = (const int_item_t *)result;
        TEST_ASSERT_EQUAL_INT(i,     found->key);
        TEST_ASSERT_EQUAL_INT(i * 3, found->value);
    }
}

/* After deleting a key, get must return NULL for that key */
void test_hashmap_get_after_delete_returns_null(void)
{
    int_item_t item = { .key = 3, .value = 30 };
    hashmap_set(int_map, &item);

    int_item_t key = { .key = 3, .value = 0 };
    hashmap_delete(int_map, &key);

    const void *result = hashmap_get(int_map, &key);
    TEST_ASSERT_NULL(result);
}

/* Deleting one key must not affect retrieval of another key */
void test_hashmap_get_unrelated_key_survives_delete(void)
{
    int_item_t item_a = { .key = 10, .value = 100 };
    int_item_t item_b = { .key = 20, .value = 200 };
    hashmap_set(int_map, &item_a);
    hashmap_set(int_map, &item_b);

    int_item_t del_key = { .key = 10, .value = 0 };
    hashmap_delete(int_map, &del_key);

    int_item_t get_key = { .key = 20, .value = 0 };
    const void *result = hashmap_get(int_map, &get_key);
    TEST_ASSERT_NOT_NULL(result);
    const int_item_t *found = (const int_item_t *)result;
    TEST_ASSERT_EQUAL_INT(200, found->value);
}

/* The returned pointer must point into the map's storage (not a copy) */
void test_hashmap_get_returns_pointer_into_map(void)
{
    int_item_t item = { .key = 55, .value = 555 };
    const void *stored = hashmap_set(int_map, &item);

    int_item_t key = { .key = 55, .value = 0 };
    const void *result = hashmap_get(int_map, &key);

    /* Both calls should refer to the same internal slot */
    TEST_ASSERT_EQUAL_PTR(stored, result);
}

/* get with key == 0 (boundary integer key) */
void test_hashmap_get_zero_key(void)
{
    int_item_t item = { .key = 0, .value = 7 };
    hashmap_set(int_map, &item);

    int_item_t key = { .key = 0, .value = 0 };
    const void *result = hashmap_get(int_map, &key);
    TEST_ASSERT_NOT_NULL(result);
    const int_item_t *found = (const int_item_t *)result;
    TEST_ASSERT_EQUAL_INT(0, found->key);
    TEST_ASSERT_EQUAL_INT(7, found->value);
}

/* get with negative integer key */
void test_hashmap_get_negative_key(void)
{
    int_item_t item = { .key = -1, .value = 99 };
    hashmap_set(int_map, &item);

    int_item_t key = { .key = -1, .value = 0 };
    const void *result = hashmap_get(int_map, &key);
    TEST_ASSERT_NOT_NULL(result);
    const int_item_t *found = (const int_item_t *)result;
    TEST_ASSERT_EQUAL_INT(-1, found->key);
    TEST_ASSERT_EQUAL_INT(99, found->value);
}

/* -------------------------------------------------------------------------
 * main
 * ---------------------------------------------------------------------- */

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_hashmap_get_empty_map_returns_null);
    RUN_TEST(test_hashmap_get_existing_key_returns_item);
    RUN_TEST(test_hashmap_get_missing_key_returns_null);
    RUN_TEST(test_hashmap_get_returns_updated_value_after_overwrite);
    RUN_TEST(test_hashmap_get_string_key_found);
    RUN_TEST(test_hashmap_get_string_key_not_found);
    RUN_TEST(test_hashmap_get_multiple_items_all_found);
    RUN_TEST(test_hashmap_get_after_delete_returns_null);
    RUN_TEST(test_hashmap_get_unrelated_key_survives_delete);
    RUN_TEST(test_hashmap_get_returns_pointer_into_map);
    RUN_TEST(test_hashmap_get_zero_key);
    RUN_TEST(test_hashmap_get_negative_key);
    return UNITY_END();
}