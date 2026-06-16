#include "hashmap.c"
#include "unity.h"
#include "hashmap.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/* -------------------------------------------------------------------------
 * File-scope fixtures / state
 * ---------------------------------------------------------------------- */

typedef struct {
    int   key;
    int   value;
} int_item_t;

static int malloc_fail_countdown = -1;
static int malloc_call_count     = 0;
static int free_call_count       = 0;
static int realloc_call_count    = 0;

/* -------------------------------------------------------------------------
 * Helper allocators
 * ---------------------------------------------------------------------- */

static void *counting_malloc(size_t sz)
{
    malloc_call_count++;
    if (malloc_fail_countdown == 0) {
        return NULL;
    }
    if (malloc_fail_countdown > 0) {
        malloc_fail_countdown--;
    }
    return malloc(sz);
}

static void *counting_realloc(void *ptr, size_t sz)
{
    realloc_call_count++;
    return realloc(ptr, sz);
}

static void counting_free(void *ptr)
{
    free_call_count++;
    free(ptr);
}

/* First malloc succeeds, second fails */
static int first_malloc_done = 0;
static void *second_malloc_fails(size_t sz)
{
    if (first_malloc_done) {
        return NULL;
    }
    first_malloc_done = 1;
    return malloc(sz);
}

static void second_malloc_fails_free(void *ptr)
{
    free(ptr);
}

/* -------------------------------------------------------------------------
 * Hash / compare callbacks
 * ---------------------------------------------------------------------- */

static uint64_t int_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
    const int_item_t *it = (const int_item_t *)item;
    return hashmap_sip(&it->key, sizeof(it->key), seed0, seed1);
}

static int int_compare(const void *a, const void *b, void *udata)
{
    (void)udata;
    const int_item_t *ia = (const int_item_t *)a;
    const int_item_t *ib = (const int_item_t *)b;
    return ia->key - ib->key;
}

static int elfree_call_count = 0;
static void int_elfree(void *item)
{
    (void)item;
    elfree_call_count++;
}

/* -------------------------------------------------------------------------
 * setUp / tearDown
 * ---------------------------------------------------------------------- */

void setUp(void)
{
    malloc_fail_countdown = -1;
    malloc_call_count     = 0;
    free_call_count       = 0;
    realloc_call_count    = 0;
    elfree_call_count     = 0;
    first_malloc_done     = 0;
}

void tearDown(void)
{
    /* nothing */
}

/* =========================================================================
 * Test cases
 * ====================================================================== */

/* Basic creation with default (NULL) allocators */
void test_hashmap_new_with_allocator_null_allocators_uses_defaults(void)
{
    struct hashmap *map = hashmap_new_with_allocator(
        NULL, NULL, NULL,
        sizeof(int_item_t), 0,
        0, 0,
        int_hash, int_compare,
        NULL, NULL);

    TEST_ASSERT_NOT_NULL(map);
    hashmap_free(map);
}

/* Custom allocators are stored and used */
void test_hashmap_new_with_allocator_custom_allocators_called(void)
{
    malloc_call_count = 0;

    struct hashmap *map = hashmap_new_with_allocator(
        counting_malloc, counting_realloc, counting_free,
        sizeof(int_item_t), 0,
        0, 0,
        int_hash, int_compare,
        NULL, NULL);

    TEST_ASSERT_NOT_NULL(map);
    TEST_ASSERT_GREATER_THAN(0, malloc_call_count);

    int prev_free = free_call_count;
    hashmap_free(map);
    TEST_ASSERT_GREATER_THAN(prev_free, free_call_count);
}

/* cap == 0 → minimum capacity 16 */
void test_hashmap_new_with_allocator_zero_cap_defaults_to_16(void)
{
    struct hashmap *map = hashmap_new_with_allocator(
        NULL, NULL, NULL,
        sizeof(int_item_t), 0,
        0, 0,
        int_hash, int_compare,
        NULL, NULL);

    TEST_ASSERT_NOT_NULL(map);
    TEST_ASSERT_EQUAL_UINT((size_t)16, map->nbuckets);
    hashmap_free(map);
}

/* cap < 16 → still rounds up to 16 */
void test_hashmap_new_with_allocator_small_cap_rounds_to_16(void)
{
    struct hashmap *map = hashmap_new_with_allocator(
        NULL, NULL, NULL,
        sizeof(int_item_t), 5,
        0, 0,
        int_hash, int_compare,
        NULL, NULL);

    TEST_ASSERT_NOT_NULL(map);
    TEST_ASSERT_EQUAL_UINT((size_t)16, map->nbuckets);
    hashmap_free(map);
}

/* cap == 16 → stays 16 */
void test_hashmap_new_with_allocator_cap_16_stays_16(void)
{
    struct hashmap *map = hashmap_new_with_allocator(
        NULL, NULL, NULL,
        sizeof(int_item_t), 16,
        0, 0,
        int_hash, int_compare,
        NULL, NULL);

    TEST_ASSERT_NOT_NULL(map);
    TEST_ASSERT_EQUAL_UINT((size_t)16, map->nbuckets);
    hashmap_free(map);
}

/* cap == 17 → rounds up to 32 */
void test_hashmap_new_with_allocator_cap_17_rounds_to_32(void)
{
    struct hashmap *map = hashmap_new_with_allocator(
        NULL, NULL, NULL,
        sizeof(int_item_t), 17,
        0, 0,
        int_hash, int_compare,
        NULL, NULL);

    TEST_ASSERT_NOT_NULL(map);
    TEST_ASSERT_EQUAL_UINT((size_t)32, map->nbuckets);
    hashmap_free(map);
}

/* cap == 1024 → stays 1024 (power of two) */
void test_hashmap_new_with_allocator_cap_1024_stays_1024(void)
{
    struct hashmap *map = hashmap_new_with_allocator(
        NULL, NULL, NULL,
        sizeof(int_item_t), 1024,
        0, 0,
        int_hash, int_compare,
        NULL, NULL);

    TEST_ASSERT_NOT_NULL(map);
    TEST_ASSERT_EQUAL_UINT((size_t)1024, map->nbuckets);
    hashmap_free(map);
}

/* mask == nbuckets - 1 */
void test_hashmap_new_with_allocator_mask_is_nbuckets_minus_1(void)
{
    struct hashmap *map = hashmap_new_with_allocator(
        NULL, NULL, NULL,
        sizeof(int_item_t), 0,
        0, 0,
        int_hash, int_compare,
        NULL, NULL);

    TEST_ASSERT_NOT_NULL(map);
    TEST_ASSERT_EQUAL_UINT(map->nbuckets - 1, map->mask);
    hashmap_free(map);
}

/* elsize is stored correctly */
void test_hashmap_new_with_allocator_elsize_stored(void)
{
    struct hashmap *map = hashmap_new_with_allocator(
        NULL, NULL, NULL,
        sizeof(int_item_t), 0,
        0, 0,
        int_hash, int_compare,
        NULL, NULL);

    TEST_ASSERT_NOT_NULL(map);
    TEST_ASSERT_EQUAL_UINT(sizeof(int_item_t), map->elsize);
    hashmap_free(map);
}

/* seed0 and seed1 are stored */
void test_hashmap_new_with_allocator_seeds_stored(void)
{
    uint64_t s0 = 0xDEADBEEFCAFEBABEULL;
    uint64_t s1 = 0x0102030405060708ULL;

    struct hashmap *map = hashmap_new_with_allocator(
        NULL, NULL, NULL,
        sizeof(int_item_t), 0,
        s0, s1,
        int_hash, int_compare,
        NULL, NULL);

    TEST_ASSERT_NOT_NULL(map);
    TEST_ASSERT_EQUAL_UINT(s0, map->seed0);
    TEST_ASSERT_EQUAL_UINT(s1, map->seed1);
    hashmap_free(map);
}

/* hash callback pointer is stored */
void test_hashmap_new_with_allocator_hash_callback_stored(void)
{
    struct hashmap *map = hashmap_new_with_allocator(
        NULL, NULL, NULL,
        sizeof(int_item_t), 0,
        0, 0,
        int_hash, int_compare,
        NULL, NULL);

    TEST_ASSERT_NOT_NULL(map);
    TEST_ASSERT_EQUAL_PTR(int_hash, map->hash);
    hashmap_free(map);
}

/* compare callback pointer is stored */
void test_hashmap_new_with_allocator_compare_callback_stored(void)
{
    struct hashmap *map = hashmap_new_with_allocator(
        NULL, NULL, NULL,
        sizeof(int_item_t), 0,
        0, 0,
        int_hash, int_compare,
        NULL, NULL);

    TEST_ASSERT_NOT_NULL(map);
    TEST_ASSERT_EQUAL_PTR(int_compare, map->compare);
    hashmap_free(map);
}

/* elfree callback pointer is stored */
void test_hashmap_new_with_allocator_elfree_callback_stored(void)
{
    struct hashmap *map = hashmap_new_with_allocator(
        NULL, NULL, NULL,
        sizeof(int_item_t), 0,
        0, 0,
        int_hash, int_compare,
        int_elfree, NULL);

    TEST_ASSERT_NOT_NULL(map);
    TEST_ASSERT_EQUAL_PTR(int_elfree, map->elfree);
    hashmap_free(map);
}

/* udata pointer is stored */
void test_hashmap_new_with_allocator_udata_stored(void)
{
    int sentinel = 42;
    struct hashmap *map = hashmap_new_with_allocator(
        NULL, NULL, NULL,
        sizeof(int_item_t), 0,
        0, 0,
        int_hash, int_compare,
        NULL, &sentinel);

    TEST_ASSERT_NOT_NULL(map);
    TEST_ASSERT_EQUAL_PTR(&sentinel, map->udata);
    hashmap_free(map);
}

/* spare pointer is just after the hashmap struct */
void test_hashmap_new_with_allocator_spare_pointer_correct(void)
{
    struct hashmap *map = hashmap_new_with_allocator(
        NULL, NULL, NULL,
        sizeof(int_item_t), 0,
        0, 0,
        int_hash, int_compare,
        NULL, NULL);

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
        sizeof(int_item_t), 0,
        0, 0,
        int_hash, int_compare,
        NULL, NULL);

    TEST_ASSERT_NOT_NULL(map);
    void *expected_edata = (char *)map->spare + map->bucketsz;
    TEST_ASSERT_EQUAL_PTR(expected_edata, map->edata);
    hashmap_free(map);
}

/* count starts at zero */
void test_hashmap_new_with_allocator_initial_count_zero(void)
{
    struct hashmap *map = hashmap_new_with_allocator(
        NULL, NULL, NULL,
        sizeof(int_item_t), 0,
        0, 0,
        int_hash, int_compare,
        NULL, NULL);

    TEST_ASSERT_NOT_NULL(map);
    TEST_ASSERT_EQUAL_UINT((size_t)0, hashmap_count(map));
    hashmap_free(map);
}

/* growpower is 1 */
void test_hashmap_new_with_allocator_growpower_is_1(void)
{
    struct hashmap *map = hashmap_new_with_allocator(
        NULL, NULL, NULL,
        sizeof(int_item_t), 0,
        0, 0,
        int_hash, int_compare,
        NULL, NULL);

    TEST_ASSERT_NOT_NULL(map);
    TEST_ASSERT_EQUAL_INT(1, map->growpower);
    hashmap_free(map);
}

/* growat > 0 */
void test_hashmap_new_with_allocator_growat_positive(void)
{
    struct hashmap *map = hashmap_new_with_allocator(
        NULL, NULL, NULL,
        sizeof(int_item_t), 0,
        0, 0,
        int_hash, int_compare,
        NULL, NULL);

    TEST_ASSERT_NOT_NULL(map);
    TEST_ASSERT_GREATER_THAN((size_t)0, map->growat);
    hashmap_free(map);
}

/* shrinkat < nbuckets */
void test_hashmap_new_with_allocator_shrinkat_less_than_nbuckets(void)
{
    struct hashmap *map = hashmap_new_with_allocator(
        NULL, NULL, NULL,
        sizeof(int_item_t), 0,
        0, 0,
        int_hash, int_compare,
        NULL, NULL);

    TEST_ASSERT_NOT_NULL(map);
    TEST_ASSERT_LESS_THAN(map->nbuckets, map->shrinkat);
    hashmap_free(map);
}

/* First malloc fails → returns NULL */
void test_hashmap_new_with_allocator_first_malloc_fails_returns_null(void)
{
    malloc_fail_countdown = 0; /* fail immediately */

    struct hashmap *map = hashmap_new_with_allocator(
        counting_malloc, counting_realloc, counting_free,
        sizeof(int_item_t), 0,
        0, 0,
        int_hash, int_compare,
        NULL, NULL);

    TEST_ASSERT_NULL(map);
}

/* Second malloc (buckets) fails → returns NULL and frees map */
void test_hashmap_new_with_allocator_second_malloc_fails_returns_null(void)
{
    struct hashmap *map = hashmap_new_with_allocator(
        second_malloc_fails, NULL, second_malloc_fails_free,
        sizeof(int_item_t), 0,
        0, 0,
        int_hash, int_compare,
        NULL, NULL);

    TEST_ASSERT_NULL(map);
}

/* The created map is functional: insert and retrieve */
void test_hashmap_new_with_allocator_map_is_functional(void)
{
    struct hashmap *map = hashmap_new_with_allocator(
        NULL, NULL, NULL,
        sizeof(int_item_t), 0,
        0, 0,
        int_hash, int_compare,
        NULL, NULL);

    TEST_ASSERT_NOT_NULL(map);

    int_item_t item = {.key = 7, .value = 99};
    hashmap_set(map, &item);

    int_item_t key = {.key = 7};
    const int_item_t *found = (const int_item_t *)hashmap_get(map, &key);
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_INT(99, found->value);

    hashmap_free(map);
}

/* bucketsz is aligned to sizeof(uintptr_t) */
void test_hashmap_new_with_allocator_bucketsz_aligned(void)
{
    struct hashmap *map = hashmap_new_with_allocator(
        NULL, NULL, NULL,
        sizeof(int_item_t), 0,
        0, 0,
        int_hash, int_compare,
        NULL, NULL);

    TEST_ASSERT_NOT_NULL(map);
    TEST_ASSERT_EQUAL_UINT(0, map->bucketsz & (sizeof(uintptr_t) - 1));
    hashmap_free(map);
}

/* bucketsz >= sizeof(struct bucket) + elsize */
void test_hashmap_new_with_allocator_bucketsz_at_least_bucket_plus_elsize(void)
{
    struct hashmap *map = hashmap_new_with_allocator(
        NULL, NULL, NULL,
        sizeof(int_item_t), 0,
        0, 0,
        int_hash, int_compare,
        NULL, NULL);

    TEST_ASSERT_NOT_NULL(map);
    size_t min_bucketsz = sizeof(struct bucket) + sizeof(int_item_t);
    TEST_ASSERT_TRUE(map->bucketsz >= min_bucketsz);
    hashmap_free(map);
}

/* cap field equals nbuckets after construction */
void test_hashmap_new_with_allocator_cap_equals_nbuckets(void)
{
    struct hashmap *map = hashmap_new_with_allocator(
        NULL, NULL, NULL,
        sizeof(int_item_t), 64,
        0, 0,
        int_hash, int_compare,
        NULL, NULL);

    TEST_ASSERT_NOT_NULL(map);
    TEST_ASSERT_EQUAL_UINT(map->cap, map->nbuckets);
    hashmap_free(map);
}

/* malloc / realloc / free function pointers stored in map */
void test_hashmap_new_with_allocator_allocator_pointers_stored(void)
{
    struct hashmap *map = hashmap_new_with_allocator(
        counting_malloc, counting_realloc, counting_free,
        sizeof(int_item_t), 0,
        0, 0,
        int_hash, int_compare,
        NULL, NULL);

    TEST_ASSERT_NOT_NULL(map);
    TEST_ASSERT_EQUAL_PTR(counting_malloc,  map->malloc);
    TEST_ASSERT_EQUAL_PTR(counting_realloc, map->realloc);
    TEST_ASSERT_EQUAL_PTR(counting_free,    map->free);
    hashmap_free(map);
}

/* loadfactor > 0 */
void test_hashmap_new_with_allocator_loadfactor_positive(void)
{
    struct hashmap *map = hashmap_new_with_allocator(
        NULL, NULL, NULL,
        sizeof(int_item_t), 0,
        0, 0,
        int_hash, int_compare,
        NULL, NULL);

    TEST_ASSERT_NOT_NULL(map);
    TEST_ASSERT_GREATER_THAN(0, map->loadfactor);
    hashmap_free(map);
}

/* =========================================================================
 * main
 * ====================================================================== */

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_hashmap_new_with_allocator_null_allocators_uses_defaults);
    RUN_TEST(test_hashmap_new_with_allocator_custom_allocators_called);
    RUN_TEST(test_hashmap_new_with_allocator_zero_cap_defaults_to_16);
    RUN_TEST(test_hashmap_new_with_allocator_small_cap_rounds_to_16);
    RUN_TEST(test_hashmap_new_with_allocator_cap_16_stays_16);
    RUN_TEST(test_hashmap_new_with_allocator_cap_17_rounds_to_32);
    RUN_TEST(test_hashmap_new_with_allocator_cap_1024_stays_1024);
    RUN_TEST(test_hashmap_new_with_allocator_mask_is_nbuckets_minus_1);
    RUN_TEST(test_hashmap_new_with_allocator_elsize_stored);
    RUN_TEST(test_hashmap_new_with_allocator_seeds_stored);
    RUN_TEST(test_hashmap_new_with_allocator_hash_callback_stored);
    RUN_TEST(test_hashmap_new_with_allocator_compare_callback_stored);
    RUN_TEST(test_hashmap_new_with_allocator_elfree_callback_stored);
    RUN_TEST(test_hashmap_new_with_allocator_udata_stored);
    RUN_TEST(test_hashmap_new_with_allocator_spare_pointer_correct);
    RUN_TEST(test_hashmap_new_with_allocator_edata_pointer_correct);
    RUN_TEST(test_hashmap_new_with_allocator_initial_count_zero);
    RUN_TEST(test_hashmap_new_with_allocator_growpower_is_1);
    RUN_TEST(test_hashmap_new_with_allocator_growat_positive);
    RUN_TEST(test_hashmap_new_with_allocator_shrinkat_less_than_nbuckets);
    RUN_TEST(test_hashmap_new_with_allocator_first_malloc_fails_returns_null);
    RUN_TEST(test_hashmap_new_with_allocator_second_malloc_fails_returns_null);
    RUN_TEST(test_hashmap_new_with_allocator_map_is_functional);
    RUN_TEST(test_hashmap_new_with_allocator_bucketsz_aligned);
    RUN_TEST(test_hashmap_new_with_allocator_bucketsz_at_least_bucket_plus_elsize);
    RUN_TEST(test_hashmap_new_with_allocator_cap_equals_nbuckets);
    RUN_TEST(test_hashmap_new_with_allocator_allocator_pointers_stored);
    RUN_TEST(test_hashmap_new_with_allocator_loadfactor_positive);
    return UNITY_END();
}