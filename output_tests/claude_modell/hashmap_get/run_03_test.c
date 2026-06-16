#include "hashmap.c"
#include "unity.h"
#include "hashmap.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* -------------------------------------------------------------------------
 * Fixture types and file-scope variables
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

static uint64_t int_item_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
    const int_item_t *it = (const int_item_t *)item;
    return hashmap_murmur(&it->key, sizeof(it->key), seed0, seed1);
}

static int int_item_compare(const void *a, const void *b, void *udata)
{
    (void)udata;
    const int_item_t *ia = (const int_item_t *)a;
    const int_item_t *ib = (const int_item_t *)b;
    return ia->key - ib->key;
}

static uint64_t str_item_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
    const str_item_t *it = (const str_item_t *)item;
    return hashmap_murmur(it->key, strlen(it->key), seed0, seed1);
}

static int str_item_compare(const void *a, const void *b, void *udata)
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
                          int_item_hash, int_item_compare, NULL, NULL);
    str_map = hashmap_new(sizeof(str_item_t), 0, 0, 0,
                          str_item_hash, str_item_compare, NULL, NULL);
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

/* hashmap_get on an empty map must return NULL */
void test_get_empty_map_returns_null(void)
{
    int_item_t key = { .key = 42, .value = 0 };
    const void *result = hashmap_get(int_map, &key);
    TEST_ASSERT_NULL(result);
}

/* hashmap_get for a key that was never inserted returns NULL */
void test_get_missing_key_returns_null(void)
{
    int_item_t item = { .key = 1, .value = 100 };
    hashmap_set(int_map, &item);

    int_item_t key = { .key = 999, .value = 0 };
    const void *result = hashmap_get(int_map, &key);
    TEST_ASSERT_NULL(result);
}

/* hashmap_get returns non-NULL for a key that was inserted */
void test_get_existing_key_returns_non_null(void)
{
    int_item_t item = { .key = 7, .value = 77 };
    hashmap_set(int_map, &item);

    int_item_t key = { .key = 7, .value = 0 };
    const void *result = hashmap_get(int_map, &key);
    TEST_ASSERT_NOT_NULL(result);
}

/* hashmap_get returns the correct value for an inserted item */
void test_get_returns_correct_value(void)
{
    int_item_t item = { .key = 10, .value = 200 };
    hashmap_set(int_map, &item);

    int_item_t key = { .key = 10, .value = 0 };
    const int_item_t *result = (const int_item_t *)hashmap_get(int_map, &key);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(10,  result->key);
    TEST_ASSERT_EQUAL_INT(200, result->value);
}

/* hashmap_get reflects the most recent set (update) */
void test_get_after_update_returns_new_value(void)
{
    int_item_t item1 = { .key = 5, .value = 50 };
    int_item_t item2 = { .key = 5, .value = 99 };
    hashmap_set(int_map, &item1);
    hashmap_set(int_map, &item2);

    int_item_t key = { .key = 5, .value = 0 };
    const int_item_t *result = (const int_item_t *)hashmap_get(int_map, &key);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(99, result->value);
}

/* hashmap_get returns NULL after the key has been deleted */
void test_get_after_delete_returns_null(void)
{
    int_item_t item = { .key = 3, .value = 30 };
    hashmap_set(int_map, &item);

    int_item_t key = { .key = 3, .value = 0 };
    hashmap_delete(int_map, &key);

    const void *result = hashmap_get(int_map, &key);
    TEST_ASSERT_NULL(result);
}

/* hashmap_get works correctly with multiple distinct keys */
void test_get_multiple_keys(void)
{
    int i;
    for (i = 0; i < 20; i++) {
        int_item_t item = { .key = i, .value = i * 10 };
        hashmap_set(int_map, &item);
    }

    for (i = 0; i < 20; i++) {
        int_item_t key = { .key = i, .value = 0 };
        const int_item_t *result = (const int_item_t *)hashmap_get(int_map, &key);
        TEST_ASSERT_NOT_NULL_MESSAGE(result, "Expected non-NULL for inserted key");
        TEST_ASSERT_EQUAL_INT(i,      result->key);
        TEST_ASSERT_EQUAL_INT(i * 10, result->value);
    }
}

/* hashmap_get does not confuse adjacent integer keys */
void test_get_does_not_confuse_adjacent_keys(void)
{
    int_item_t item_a = { .key = 100, .value = 1 };
    int_item_t item_b = { .key = 101, .value = 2 };
    hashmap_set(int_map, &item_a);
    hashmap_set(int_map, &item_b);

    int_item_t key_a = { .key = 100, .value = 0 };
    int_item_t key_b = { .key = 101, .value = 0 };

    const int_item_t *ra = (const int_item_t *)hashmap_get(int_map, &key_a);
    const int_item_t *rb = (const int_item_t *)hashmap_get(int_map, &key_b);

    TEST_ASSERT_NOT_NULL(ra);
    TEST_ASSERT_NOT_NULL(rb);
    TEST_ASSERT_EQUAL_INT(1, ra->value);
    TEST_ASSERT_EQUAL_INT(2, rb->value);
}

/* hashmap_get works with string keys */
void test_get_string_key_found(void)
{
    str_item_t item;
    memset(&item, 0, sizeof(item));
    strncpy(item.key, "hello", sizeof(item.key) - 1);
    item.value = 42;
    hashmap_set(str_map, &item);

    str_item_t key;
    memset(&key, 0, sizeof(key));
    strncpy(key.key, "hello", sizeof(key.key) - 1);

    const str_item_t *result = (const str_item_t *)hashmap_get(str_map, &key);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("hello", result->key);
    TEST_ASSERT_EQUAL_INT(42, result->value);
}

/* hashmap_get returns NULL for a different string key */
void test_get_string_key_not_found(void)
{
    str_item_t item;
    memset(&item, 0, sizeof(item));
    strncpy(item.key, "world", sizeof(item.key) - 1);
    item.value = 7;
    hashmap_set(str_map, &item);

    str_item_t key;
    memset(&key, 0, sizeof(key));
    strncpy(key.key, "earth", sizeof(key.key) - 1);

    const void *result = hashmap_get(str_map, &key);
    TEST_ASSERT_NULL(result);
}

/* hashmap_get survives a large number of insertions (forces rehash) */
void test_get_after_many_insertions(void)
{
    int i;
    const int N = 500;
    for (i = 0; i < N; i++) {
        int_item_t item = { .key = i, .value = i + 1 };
        hashmap_set(int_map, &item);
    }

    for (i = 0; i < N; i++) {
        int_item_t key = { .key = i, .value = 0 };
        const int_item_t *result = (const int_item_t *)hashmap_get(int_map, &key);
        TEST_ASSERT_NOT_NULL_MESSAGE(result, "Key missing after large insertion");
        TEST_ASSERT_EQUAL_INT(i + 1, result->value);
    }
}

/* hashmap_get returns NULL for a key just beyond the inserted range */
void test_get_out_of_range_key_returns_null(void)
{
    int i;
    for (i = 0; i < 10; i++) {
        int_item_t item = { .key = i, .value = i };
        hashmap_set(int_map, &item);
    }

    int_item_t key = { .key = 10, .value = 0 };
    const void *result = hashmap_get(int_map, &key);
    TEST_ASSERT_NULL(result);
}

/* -------------------------------------------------------------------------
 * main
 * ---------------------------------------------------------------------- */

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_get_empty_map_returns_null);
    RUN_TEST(test_get_missing_key_returns_null);
    RUN_TEST(test_get_existing_key_returns_non_null);
    RUN_TEST(test_get_returns_correct_value);
    RUN_TEST(test_get_after_update_returns_new_value);
    RUN_TEST(test_get_after_delete_returns_null);
    RUN_TEST(test_get_multiple_keys);
    RUN_TEST(test_get_does_not_confuse_adjacent_keys);
    RUN_TEST(test_get_string_key_found);
    RUN_TEST(test_get_string_key_not_found);
    RUN_TEST(test_get_after_many_insertions);
    RUN_TEST(test_get_out_of_range_key_returns_null);
    return UNITY_END();
}