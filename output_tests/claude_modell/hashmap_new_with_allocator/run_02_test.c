#include "hashmap.c"
#include "unity.h"
#include "hashmap.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/* -------------------------------------------------------------------------
 * File-scope fixtures / state
 * ---------------------------------------------------------------------- */

static int custom_malloc_call_count = 0;
static int custom_free_call_count   = 0;
static int fail_malloc_after        = -1; /* -1 = never fail */

/* -------------------------------------------------------------------------
 * Helper types and callbacks
 * ---------------------------------------------------------------------- */

typedef struct {
    int   key;
    char  value[32];
} test_item_t;

static uint64_t test_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
    const test_item_t *it = (const test_item_t *)item;
    return hashmap_sip(&it->key, sizeof(it->key), seed0, seed1);
}

static int test_compare(const void *a, const void *b, void *udata)
{
    (void)udata;
    const test_item_t *ia = (const test_item_t *)a;
    const test_item_t *ib = (const test_item_t *)b;
    return ia->key - ib->key;
}

static void test_elfree(void *item)
{
    (void)item;
    /* nothing to free in test_item_t */
}

/* Custom allocator helpers */
static void *counting_malloc(size_t sz)
{
    custom_malloc_call_count++;
    if (fail_malloc_after >= 0 && custom_malloc_call_count > fail_malloc_after) {
        return NULL;
    }
    return malloc(sz);
}

static void *counting_realloc(void *ptr, size_t sz)
{
    return realloc(ptr, sz);
}

static void counting_free(void *ptr)
{
    custom_free_call_count++;
    free(ptr);
}

/* -------------------------------------------------------------------------
 * setUp / tearDown
 * ---------------------------------------------------------------------- */

void setUp(void)
{
    custom_malloc_call_count = 0;
    custom_free_call_count   = 0;
    fail_malloc_after        = -1;
}

void tearDown(void)
{
    /* nothing global to clean up */
}

/* -------------------------------------------------------------------------
 * Test cases
 * ---------------------------------------------------------------------- */

/* Basic creation with NULL allocators (should fall back to malloc/realloc/free) */
void test_hashmap_new_with_allocator_null_allocators_returns_non_null(void)
{
    struct hashmap *map = hashmap_new_with_allocator(
        NULL, NULL, NULL,
        sizeof(test_item_t), 0,
        0, 0,
        test_hash, test_compare, NULL, NULL);

    TEST_ASSERT_NOT_NULL(map);
    hashmap_free(map);
}

/* Custom allocators are stored and used */
void test_hashmap_new_with_allocator_custom_allocators_called(void)
{
    custom_malloc_call_count = 0;

    struct hashmap *map = hashmap_new_with_allocator(
        counting_malloc, counting_realloc, counting_free,
        sizeof(test_item_t), 0,
        0, 0,
        test_hash, test_compare, NULL, NULL);

    TEST_ASSERT_NOT_NULL(map);
    /* At least two malloc calls: one for the map struct, one for buckets */
    TEST_ASSERT_GREATER_THAN(1, custom_malloc_call_count);

    hashmap_free(map);
    /* At least two free calls */
    TEST_ASSERT_GREATER_THAN(1, custom_free_call_count);
}

/* Capacity is rounded up to next power of two >= 16 */
void test_hashmap_new_with_allocator_cap_zero_gives_minimum_16(void)
{
    struct hashmap *map = hashmap_new_with_allocator(
        NULL, NULL, NULL,
        sizeof(test_item_t), 0,
        0, 0,
        test_hash, test_compare, NULL, NULL);

    TEST_ASSERT_NOT_NULL(map);
    TEST_ASSERT_EQUAL_UINT(16, (unsigned)map->nbuckets);
    hashmap_free(map);
}

void test_hashmap_new_with_allocator_cap_1_gives_minimum_16(void)
{
    struct hashmap *map = hashmap_new_with_allocator(
        NULL, NULL, NULL,
        sizeof(test_item_t), 1,
        0, 0,
        test_hash, test_compare, NULL, NULL);

    TEST_ASSERT_NOT_NULL(map);
    TEST_ASSERT_EQUAL_UINT(16, (unsigned)map->nbuckets);
    hashmap_free(map);
}

void test_hashmap_new_with_allocator_cap_16_gives_16(void)
{
    struct hashmap *map = hashmap_new_with_allocator(
        NULL, NULL, NULL,
        sizeof(test_item_t), 16,
        0, 0,
        test_hash, test_compare, NULL, NULL);

    TEST_ASSERT_NOT_NULL(map);
    TEST_ASSERT_EQUAL_UINT(16, (unsigned)map->nbuckets);
    hashmap_free(map);
}

void test_hashmap_new_with_allocator_cap_17_rounds_to_32(void)
{
    struct hashmap *map = hashmap_new_with_allocator(
        NULL, NULL, NULL,
        sizeof(test_item_t), 17,
        0, 0,
        test_hash, test_compare, NULL, NULL);

    TEST_ASSERT_NOT_NULL(map);
    TEST_ASSERT_EQUAL_UINT(32, (unsigned)map->nbuckets);
    hashmap_free(map);
}

void test_hashmap_new_with_allocator_cap_32_gives_32(void)
{
    struct hashmap *map = hashmap_new_with_allocator(
        NULL, NULL, NULL,
        sizeof(test_item_t), 32,
        0, 0,
        test_hash, test_compare, NULL, NULL);

    TEST_ASSERT_NOT_NULL(map);
    TEST_ASSERT_EQUAL_UINT(32, (unsigned)map->nbuckets);
    hashmap_free(map);
}

void test_hashmap_new_with_allocator_cap_100_rounds_to_128(void)
{
    struct hashmap *map = hashmap_new_with_allocator(
        NULL, NULL, NULL,
        sizeof(test_item_t), 100,
        0, 0,
        test_hash, test_compare, NULL, NULL);

    TEST_ASSERT_NOT_NULL(map);
    TEST_ASSERT_EQUAL_UINT(128, (unsigned)map->nbuckets);
    hashmap_free(map);
}

/* mask == nbuckets - 1 */
void test_hashmap_new_with_allocator_mask_is_nbuckets_minus_1(void)
{
    struct hashmap *map = hashmap_new_with_allocator(
        NULL, NULL, NULL,
        sizeof(test_item_t), 64,
        0, 0,
        test_hash, test_compare, NULL, NULL);

    TEST_ASSERT_NOT_NULL(map);
    TEST_ASSERT_EQUAL_UINT(map->nbuckets - 1, map->mask);
    hashmap_free(map);
}

/* elsize is stored correctly */
void test_hashmap_new_with_allocator_elsize_stored(void)
{
    struct hashmap *map = hashmap_new_with_allocator(
        NULL, NULL, NULL,
        sizeof(test_item_t), 0,
        0, 0,
        test_hash, test_compare, NULL, NULL);

    TEST_ASSERT_NOT_NULL(map);
    TEST_ASSERT_EQUAL_UINT(sizeof(test_item_t), map->elsize);
    hashmap_free(map);
}

/* seeds are stored correctly */
void test_hashmap_new_with_allocator_seeds_stored(void)
{
    uint64_t s0 = 0xDEADBEEFCAFEBABEULL;
    uint64_t s1 = 0x0102030405060708ULL;

    struct hashmap *map = hashmap_new_with_allocator(
        NULL, NULL, NULL,
        sizeof(test_item_t), 0,
        s0, s1,
        test_hash, test_compare, NULL, NULL);

    TEST_ASSERT_NOT_NULL(map);
    TEST_ASSERT_EQUAL_HEX(s0, map->seed0);
    TEST_ASSERT_EQUAL_HEX(s1, map->seed1);
    hashmap_free(map);
}

/* hash callback pointer is stored */
void test_hashmap_new_with_allocator_hash_callback_stored(void)
{
    struct hashmap *map = hashmap_new_with_allocator(
        NULL, NULL, NULL,
        sizeof(test_item_t), 0,
        0, 0,
        test_hash, test_compare, NULL, NULL);

    TEST_ASSERT_NOT_NULL(map);
    TEST_ASSERT_EQUAL_PTR(test_hash, map->hash);
    hashmap_free(map);
}

/* compare callback pointer is stored */
void test_hashmap_new_with_allocator_compare_callback_stored(void)
{
    struct hashmap *map = hashmap_new_with_allocator(
        NULL, NULL, NULL,
        sizeof(test_item_t), 0,
        0, 0,
        test_hash, test_compare, NULL, NULL);

    TEST_ASSERT_NOT_NULL(map);
    TEST_ASSERT_EQUAL_PTR(test_compare, map->compare);
    hashmap_free(map);
}

/* elfree callback pointer is stored */
void test_hashmap_new_with_allocator_elfree_callback_stored(void)
{
    struct hashmap *map = hashmap_new_with_allocator(
        NULL, NULL, NULL,
        sizeof(test_item_t), 0,
        0, 0,
        test_hash, test_compare, test_elfree, NULL);

    TEST_ASSERT_NOT_NULL(map);
    TEST_ASSERT_EQUAL_PTR(test_elfree, map->elfree);
    hashmap_free(map);
}

/* udata pointer is stored */
void test_hashmap_new_with_allocator_udata_stored(void)
{
    int sentinel = 42;

    struct hashmap *map = hashmap_new_with_allocator(
        NULL, NULL, NULL,
        sizeof(test_item_t), 0,
        0, 0,
        test_hash, test_compare, NULL, &sentinel);

    TEST_ASSERT_NOT_NULL(map);
    TEST_ASSERT_EQUAL_PTR(&sentinel, map->udata);
    hashmap_free(map);
}

/* spare pointer is just after the hashmap struct */
void test_hashmap_new_with_allocator_spare_pointer_correct(void)
{
    struct hashmap *map = hashmap_new_with_allocator(
        NULL, NULL, NULL,
        sizeof(test_item_t), 0,
        0, 0,
        test_hash, test_compare, NULL, NULL);

    TEST_ASSERT_NOT_NULL(map);
    void *expected_spare = ((char *)map) + sizeof(struct hashmap);
    TEST_ASSERT_EQUAL_PTR(expected_spare, map->spare);
    hashmap_free(map);
}

/* edata pointer is spare + bucketsz */
void test_hashmap_new_with_allocator_edata_pointer_correct(void)
{
    struct hashmap *map = hashmap_new_with_allocator(
        NULL, NULL, NULL,
        sizeof(test_item_t), 0,
        0, 0,
        test_hash, test_compare, NULL, NULL);

    TEST_ASSERT_NOT_NULL(map);
    void *expected_edata = (char *)map->spare + map->bucketsz;
    TEST_ASSERT_EQUAL_PTR(expected_edata, map->edata);
    hashmap_free(map);
}

/* buckets pointer is non-NULL */
void test_hashmap_new_with_allocator_buckets_non_null(void)
{
    struct hashmap *map = hashmap_new_with_allocator(
        NULL, NULL, NULL,
        sizeof(test_item_t), 0,
        0, 0,
        test_hash, test_compare, NULL, NULL);

    TEST_ASSERT_NOT_NULL(map);
    TEST_ASSERT_NOT_NULL(map->buckets);
    hashmap_free(map);
}

/* count starts at zero */
void test_hashmap_new_with_allocator_initial_count_zero(void)
{
    struct hashmap *map = hashmap_new_with_allocator(
        NULL, NULL, NULL,
        sizeof(test_item_t), 0,
        0, 0,
        test_hash, test_compare, NULL, NULL);

    TEST_ASSERT_NOT_NULL(map);
    TEST_ASSERT_EQUAL_UINT(0, (unsigned)hashmap_count(map));
    hashmap_free(map);
}

/* growpower initialised to 1 */
void test_hashmap_new_with_allocator_growpower_is_1(void)
{
    struct hashmap *map = hashmap_new_with_allocator(
        NULL, NULL, NULL,
        sizeof(test_item_t), 0,
        0, 0,
        test_hash, test_compare, NULL, NULL);

    TEST_ASSERT_NOT_NULL(map);
    TEST_ASSERT_EQUAL_INT(1, (int)map->growpower);
    hashmap_free(map);
}

/* growat > 0 */
void test_hashmap_new_with_allocator_growat_positive(void)
{
    struct hashmap *map = hashmap_new_with_allocator(
        NULL, NULL, NULL,
        sizeof(test_item_t), 0,
        0, 0,
        test_hash, test_compare, NULL, NULL);

    TEST_ASSERT_NOT_NULL(map);
    TEST_ASSERT_GREATER_THAN(0, (int)map->growat);
    hashmap_free(map);
}

/* shrinkat < nbuckets */
void test_hashmap_new_with_allocator_shrinkat_less_than_nbuckets(void)
{
    struct hashmap *map = hashmap_new_with_allocator(
        NULL, NULL, NULL,
        sizeof(test_item_t), 0,
        0, 0,
        test_hash, test_compare, NULL, NULL);

    TEST_ASSERT_NOT_NULL(map);
    TEST_ASSERT_LESS_THAN((int)map->nbuckets, (int)map->shrinkat);
    hashmap_free(map);
}

/* bucketsz is aligned to sizeof(uintptr_t) */
void test_hashmap_new_with_allocator_bucketsz_aligned(void)
{
    struct hashmap *map = hashmap_new_with_allocator(
        NULL, NULL, NULL,
        sizeof(test_item_t), 0,
        0, 0,
        test_hash, test_compare, NULL, NULL);

    TEST_ASSERT_NOT_NULL(map);
    TEST_ASSERT_EQUAL_UINT(0, map->bucketsz & (sizeof(uintptr_t) - 1));
    hashmap_free(map);
}

/* bucketsz >= sizeof(struct bucket) + elsize */
void test_hashmap_new_with_allocator_bucketsz_at_least_bucket_plus_elsize(void)
{
    struct hashmap *map = hashmap_new_with_allocator(
        NULL, NULL, NULL,
        sizeof(test_item_t), 0,
        0, 0,
        test_hash, test_compare, NULL, NULL);

    TEST_ASSERT_NOT_NULL(map);
    TEST_ASSERT_GREATER_THAN(
        (int)(sizeof(struct bucket) + sizeof(test_item_t) - 1),
        (int)map->bucketsz);
    hashmap_free(map);
}

/* First malloc failure returns NULL */
void test_hashmap_new_with_allocator_first_malloc_fail_returns_null(void)
{
    fail_malloc_after = 0; /* fail on the very first call */

    struct hashmap *map = hashmap_new_with_allocator(
        counting_malloc, counting_realloc, counting_free,
        sizeof(test_item_t), 0,
        0, 0,
        test_hash, test_compare, NULL, NULL);

    TEST_ASSERT_NULL(map);
}

/* Second malloc failure (buckets) returns NULL and frees map struct */
void test_hashmap_new_with_allocator_second_malloc_fail_returns_null(void)
{
    fail_malloc_after = 1; /* allow first malloc, fail second */

    struct hashmap *map = hashmap_new_with_allocator(
        counting_malloc, counting_realloc, counting_free,
        sizeof(test_item_t), 0,
        0, 0,
        test_hash, test_compare, NULL, NULL);

    TEST_ASSERT_NULL(map);
    /* The map struct should have been freed */
    TEST_ASSERT_EQUAL_INT(1, custom_free_call_count);
}

/* Map created with custom allocators can actually store and retrieve items */
void test_hashmap_new_with_allocator_functional_set_get(void)
{
    struct hashmap *map = hashmap_new_with_allocator(
        counting_malloc, counting_realloc, counting_free,
        sizeof(test_item_t), 0,
        0, 0,
        test_hash, test_compare, NULL, NULL);

    TEST_ASSERT_NOT_NULL(map);

    test_item_t item = { .key = 99 };
    strncpy(item.value, "hello", sizeof(item.value) - 1);

    hashmap_set(map, &item);

    test_item_t key = { .key = 99 };
    const test_item_t *found = (const test_item_t *)hashmap_get(map, &key);

    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_INT(99, found->key);
    TEST_ASSERT_EQUAL_STRING("hello", found->value);

    hashmap_free(map);
}

/* Large capacity request */
void test_hashmap_new_with_allocator_large_cap_1024(void)
{
    struct hashmap *map = hashmap_new_with_allocator(
        NULL, NULL, NULL,
        sizeof(test_item_t), 1024,
        0, 0,
        test_hash, test_compare, NULL, NULL);

    TEST_ASSERT_NOT_NULL(map);
    TEST_ASSERT_EQUAL_UINT(1024, (unsigned)map->nbuckets);
    hashmap_free(map);
}

/* cap == nbuckets after construction */
void test_hashmap_new_with_allocator_cap_equals_nbuckets(void)
{
    struct hashmap *map = hashmap_new_with_allocator(
        NULL, NULL, NULL,
        sizeof(test_item_t), 64,
        0, 0,
        test_hash, test_compare, NULL, NULL);

    TEST_ASSERT_NOT_NULL(map);
    TEST_ASSERT_EQUAL_UINT(map->cap, map->nbuckets);
    hashmap_free(map);
}

/* malloc/realloc/free function pointers stored in map */
void test_hashmap_new_with_allocator_function_pointers_stored(void)
{
    struct hashmap *map = hashmap_new_with_allocator(
        counting_malloc, counting_realloc, counting_free,
        sizeof(test_item_t), 0,
        0, 0,
        test_hash, test_compare, NULL, NULL);

    TEST_ASSERT_NOT_NULL(map);
    TEST_ASSERT_EQUAL_PTR(counting_malloc,  map->malloc);
    TEST_ASSERT_EQUAL_PTR(counting_realloc, map->realloc);
    TEST_ASSERT_EQUAL_PTR(counting_free,    map->free);

    hashmap_free(map);
}

/* -------------------------------------------------------------------------
 * main
 * ---------------------------------------------------------------------- */

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();

    RUN_TEST(test_hashmap_new_with_allocator_null_allocators_returns_non_null);
    RUN_TEST(test_hashmap_new_with_allocator_custom_allocators_called);
    RUN_TEST(test_hashmap_new_with_allocator_cap_zero_gives_minimum_16);
    RUN_TEST(test_hashmap_new_with_allocator_cap_1_gives_minimum_16);
    RUN_TEST(test_hashmap_new_with_allocator_cap_16_gives_16);
    RUN_TEST(test_hashmap_new_with_allocator_cap_17_rounds_to_32);
    RUN_TEST(test_hashmap_new_with_allocator_cap_32_gives_32);
    RUN_TEST(test_hashmap_new_with_allocator_cap_100_rounds_to_128);
    RUN_TEST(test_hashmap_new_with_allocator_mask_is_nbuckets_minus_1);
    RUN_TEST(test_hashmap_new_with_allocator_elsize_stored);
    RUN_TEST(test_hashmap_new_with_allocator_seeds_stored);
    RUN_TEST(test_hashmap_new_with_allocator_hash_callback_stored);
    RUN_TEST(test_hashmap_new_with_allocator_compare_callback_stored);
    RUN_TEST(test_hashmap_new_with_allocator_elfree_callback_stored);
    RUN_TEST(test_hashmap_new_with_allocator_udata_stored);
    RUN_TEST(test_hashmap_new_with_allocator_spare_pointer_correct);
    RUN_TEST(test_hashmap_new_with_allocator_edata_pointer_correct);
    RUN_TEST(test_hashmap_new_with_allocator_buckets_non_null);
    RUN_TEST(test_hashmap_new_with_allocator_initial_count_zero);
    RUN_TEST(test_hashmap_new_with_allocator_growpower_is_1);
    RUN_TEST(test_hashmap_new_with_allocator_growat_positive);
    RUN_TEST(test_hashmap_new_with_allocator_shrinkat_less_than_nbuckets);
    RUN_TEST(test_hashmap_new_with_allocator_bucketsz_aligned);
    RUN_TEST(test_hashmap_new_with_allocator_bucketsz_at_least_bucket_plus_elsize);
    RUN_TEST(test_hashmap_new_with_allocator_first_malloc_fail_returns_null);
    RUN_TEST(test_hashmap_new_with_allocator_second_malloc_fail_returns_null);
    RUN_TEST(test_hashmap_new_with_allocator_functional_set_get);
    RUN_TEST(test_hashmap_new_with_allocator_large_cap_1024);
    RUN_TEST(test_hashmap_new_with_allocator_cap_equals_nbuckets);
    RUN_TEST(test_hashmap_new_with_allocator_function_pointers_stored);

    return UNITY_END();
}