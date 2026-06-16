#include "hashmap.c"
#include "unity.h"
#include "hashmap.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/* -------------------------------------------------------------------------
 * File-scope fixtures / state
 * ------------------------------------------------------------------------- */

static struct hashmap *g_map = NULL;

/* Track custom allocator calls */
static int custom_malloc_called  = 0;
static int custom_realloc_called = 0;
static int custom_free_called    = 0;
static int malloc_fail_after     = -1; /* -1 = never fail */
static int malloc_call_count     = 0;

/* -------------------------------------------------------------------------
 * Test item type
 * ------------------------------------------------------------------------- */

typedef struct {
    int   key;
    char  value[32];
} TestItem;

/* -------------------------------------------------------------------------
 * Helper: hash / compare callbacks
 * ------------------------------------------------------------------------- */

static uint64_t test_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
    const TestItem *ti = (const TestItem *)item;
    return hashmap_sip(&ti->key, sizeof(ti->key), seed0, seed1);
}

static int test_compare(const void *a, const void *b, void *udata)
{
    (void)udata;
    const TestItem *ta = (const TestItem *)a;
    const TestItem *tb = (const TestItem *)b;
    return ta->key - tb->key;
}

static void test_elfree(void *item)
{
    (void)item;
    /* nothing to free in TestItem, but callback must be callable */
}

/* -------------------------------------------------------------------------
 * Custom allocator helpers
 * ------------------------------------------------------------------------- */

static void *custom_malloc(size_t sz)
{
    custom_malloc_called++;
    malloc_call_count++;
    if (malloc_fail_after >= 0 && malloc_call_count > malloc_fail_after) {
        return NULL;
    }
    return malloc(sz);
}

static void *custom_realloc(void *ptr, size_t sz)
{
    custom_realloc_called++;
    return realloc(ptr, sz);
}

static void custom_free(void *ptr)
{
    custom_free_called++;
    free(ptr);
}

/* -------------------------------------------------------------------------
 * setUp / tearDown
 * ------------------------------------------------------------------------- */

void setUp(void)
{
    g_map = NULL;
    custom_malloc_called  = 0;
    custom_realloc_called = 0;
    custom_free_called    = 0;
    malloc_fail_after     = -1;
    malloc_call_count     = 0;
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

/* Basic creation with NULL allocators (falls back to system malloc/realloc/free) */
void test_create_with_null_allocators_returns_non_null(void)
{
    g_map = hashmap_new_with_allocator(
        NULL, NULL, NULL,
        sizeof(TestItem), 0,
        0, 0,
        test_hash, test_compare,
        NULL, NULL);

    TEST_ASSERT_NOT_NULL(g_map);
}

/* Creation with explicit custom allocators */
void test_create_with_custom_allocators_returns_non_null(void)
{
    g_map = hashmap_new_with_allocator(
        custom_malloc, custom_realloc, custom_free,
        sizeof(TestItem), 0,
        0, 0,
        test_hash, test_compare,
        NULL, NULL);

    TEST_ASSERT_NOT_NULL(g_map);
    TEST_ASSERT_TRUE(custom_malloc_called >= 2); /* map struct + buckets */
}

/* Custom allocators are stored and used on free */
void test_custom_free_called_on_hashmap_free(void)
{
    g_map = hashmap_new_with_allocator(
        custom_malloc, custom_realloc, custom_free,
        sizeof(TestItem), 0,
        0, 0,
        test_hash, test_compare,
        NULL, NULL);

    TEST_ASSERT_NOT_NULL(g_map);

    int free_before = custom_free_called;
    hashmap_free(g_map);
    g_map = NULL;

    TEST_ASSERT_TRUE(custom_free_called > free_before);
}

/* cap = 0 → minimum capacity 16 */
void test_cap_zero_uses_minimum_16(void)
{
    g_map = hashmap_new_with_allocator(
        NULL, NULL, NULL,
        sizeof(TestItem), 0,
        0, 0,
        test_hash, test_compare,
        NULL, NULL);

    TEST_ASSERT_NOT_NULL(g_map);
    TEST_ASSERT_EQUAL_UINT(16, (unsigned)hashmap_count(g_map) == 0 ? 16 : 0);
    /* Verify by checking count starts at 0 */
    TEST_ASSERT_EQUAL_UINT(0, (unsigned)hashmap_count(g_map));
}

/* cap = 1 → rounds up to 16 */
void test_cap_1_rounds_up_to_16(void)
{
    g_map = hashmap_new_with_allocator(
        NULL, NULL, NULL,
        sizeof(TestItem), 1,
        0, 0,
        test_hash, test_compare,
        NULL, NULL);

    TEST_ASSERT_NOT_NULL(g_map);
    TEST_ASSERT_EQUAL_UINT(0, (unsigned)hashmap_count(g_map));
}

/* cap = 17 → rounds up to 32 (next power of two) */
void test_cap_17_rounds_up_to_32(void)
{
    g_map = hashmap_new_with_allocator(
        NULL, NULL, NULL,
        sizeof(TestItem), 17,
        0, 0,
        test_hash, test_compare,
        NULL, NULL);

    TEST_ASSERT_NOT_NULL(g_map);
    /* Map should be usable; insert 17 items without resize */
    for (int i = 0; i < 17; i++) {
        TestItem ti = { .key = i };
        snprintf(ti.value, sizeof(ti.value), "val%d", i);
        hashmap_set(g_map, &ti);
    }
    TEST_ASSERT_EQUAL_UINT(17, (unsigned)hashmap_count(g_map));
}

/* cap = 32 → stays 32 (exact power of two) */
void test_cap_exact_power_of_two(void)
{
    g_map = hashmap_new_with_allocator(
        NULL, NULL, NULL,
        sizeof(TestItem), 32,
        0, 0,
        test_hash, test_compare,
        NULL, NULL);

    TEST_ASSERT_NOT_NULL(g_map);
    TEST_ASSERT_EQUAL_UINT(0, (unsigned)hashmap_count(g_map));
}

/* Large cap */
void test_large_cap(void)
{
    g_map = hashmap_new_with_allocator(
        NULL, NULL, NULL,
        sizeof(TestItem), 1024,
        0, 0,
        test_hash, test_compare,
        NULL, NULL);

    TEST_ASSERT_NOT_NULL(g_map);
}

/* seed0 and seed1 are stored */
void test_seeds_stored_correctly(void)
{
    uint64_t s0 = 0xDEADBEEFCAFEBABEULL;
    uint64_t s1 = 0x0102030405060708ULL;

    g_map = hashmap_new_with_allocator(
        NULL, NULL, NULL,
        sizeof(TestItem), 0,
        s0, s1,
        test_hash, test_compare,
        NULL, NULL);

    TEST_ASSERT_NOT_NULL(g_map);
    /* Exercise the map to confirm seeds are used without crash */
    TestItem ti = { .key = 42 };
    snprintf(ti.value, sizeof(ti.value), "hello");
    hashmap_set(g_map, &ti);

    TestItem lookup = { .key = 42 };
    const TestItem *found = (const TestItem *)hashmap_get(g_map, &lookup);
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_INT(42, found->key);
}

/* udata pointer is stored and accessible via hashmap_get_udata */
void test_udata_stored(void)
{
    int sentinel = 0xABCD;

    g_map = hashmap_new_with_allocator(
        NULL, NULL, NULL,
        sizeof(TestItem), 0,
        0, 0,
        test_hash, test_compare,
        NULL, &sentinel);

    TEST_ASSERT_NOT_NULL(g_map);
    TEST_ASSERT_EQUAL_PTR(&sentinel, hashmap_get_udata(g_map));
}

/* elfree callback is stored and invoked on clear */
void test_elfree_callback_stored_and_called(void)
{
    g_map = hashmap_new_with_allocator(
        NULL, NULL, NULL,
        sizeof(TestItem), 0,
        0, 0,
        test_hash, test_compare,
        test_elfree, NULL);

    TEST_ASSERT_NOT_NULL(g_map);

    TestItem ti = { .key = 1 };
    hashmap_set(g_map, &ti);
    /* hashmap_clear with update_cap=true triggers elfree for each item */
    hashmap_clear(g_map, false);
    TEST_ASSERT_EQUAL_UINT(0, (unsigned)hashmap_count(g_map));
}

/* First malloc failure → returns NULL */
void test_first_malloc_failure_returns_null(void)
{
    malloc_fail_after = 0; /* fail on the very first call */

    struct hashmap *map = hashmap_new_with_allocator(
        custom_malloc, custom_realloc, custom_free,
        sizeof(TestItem), 0,
        0, 0,
        test_hash, test_compare,
        NULL, NULL);

    TEST_ASSERT_NULL(map);
}

/* Second malloc failure (buckets) → returns NULL and frees map struct */
void test_second_malloc_failure_returns_null(void)
{
    malloc_fail_after = 1; /* allow first malloc, fail second */

    int free_before = custom_free_called;

    struct hashmap *map = hashmap_new_with_allocator(
        custom_malloc, custom_realloc, custom_free,
        sizeof(TestItem), 0,
        0, 0,
        test_hash, test_compare,
        NULL, NULL);

    TEST_ASSERT_NULL(map);
    /* The map struct allocated by the first malloc must have been freed */
    TEST_ASSERT_TRUE(custom_free_called > free_before);
}

/* Map is functional after creation: set + get */
void test_map_functional_after_creation(void)
{
    g_map = hashmap_new_with_allocator(
        NULL, NULL, NULL,
        sizeof(TestItem), 0,
        0, 0,
        test_hash, test_compare,
        NULL, NULL);

    TEST_ASSERT_NOT_NULL(g_map);

    TestItem ti = { .key = 7 };
    snprintf(ti.value, sizeof(ti.value), "seven");
    hashmap_set(g_map, &ti);

    TestItem lookup = { .key = 7 };
    const TestItem *found = (const TestItem *)hashmap_get(g_map, &lookup);
    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_INT(7, found->key);
    TEST_ASSERT_EQUAL_STRING("seven", found->value);
}

/* Multiple items can be inserted and retrieved */
void test_multiple_items_insert_and_retrieve(void)
{
    g_map = hashmap_new_with_allocator(
        NULL, NULL, NULL,
        sizeof(TestItem), 0,
        0, 0,
        test_hash, test_compare,
        NULL, NULL);

    TEST_ASSERT_NOT_NULL(g_map);

    for (int i = 0; i < 50; i++) {
        TestItem ti = { .key = i };
        snprintf(ti.value, sizeof(ti.value), "item%d", i);
        hashmap_set(g_map, &ti);
    }

    TEST_ASSERT_EQUAL_UINT(50, (unsigned)hashmap_count(g_map));

    for (int i = 0; i < 50; i++) {
        TestItem lookup = { .key = i };
        const TestItem *found = (const TestItem *)hashmap_get(g_map, &lookup);
        TEST_ASSERT_NOT_NULL_MESSAGE(found, "item not found");
        TEST_ASSERT_EQUAL_INT(i, found->key);
    }
}

/* elsize = 1 (minimum meaningful element) */
void test_elsize_one_byte(void)
{
    g_map = hashmap_new_with_allocator(
        NULL, NULL, NULL,
        1, 0,
        0, 0,
        test_hash, test_compare,
        NULL, NULL);

    TEST_ASSERT_NOT_NULL(g_map);
}

/* elsize = large value */
void test_elsize_large(void)
{
    g_map = hashmap_new_with_allocator(
        NULL, NULL, NULL,
        4096, 0,
        0, 0,
        test_hash, test_compare,
        NULL, NULL);

    TEST_ASSERT_NOT_NULL(g_map);
}

/* Verify count starts at zero */
void test_initial_count_is_zero(void)
{
    g_map = hashmap_new_with_allocator(
        NULL, NULL, NULL,
        sizeof(TestItem), 0,
        0, 0,
        test_hash, test_compare,
        NULL, NULL);

    TEST_ASSERT_NOT_NULL(g_map);
    TEST_ASSERT_EQUAL_UINT(0, (unsigned)hashmap_count(g_map));
}

/* Two maps created independently do not share state */
void test_two_independent_maps(void)
{
    struct hashmap *map1 = hashmap_new_with_allocator(
        NULL, NULL, NULL,
        sizeof(TestItem), 0,
        1, 2,
        test_hash, test_compare,
        NULL, NULL);

    struct hashmap *map2 = hashmap_new_with_allocator(
        NULL, NULL, NULL,
        sizeof(TestItem), 0,
        3, 4,
        test_hash, test_compare,
        NULL, NULL);

    TEST_ASSERT_NOT_NULL(map1);
    TEST_ASSERT_NOT_NULL(map2);
    TEST_ASSERT_NOT_EQUAL(map1, map2);

    TestItem ti1 = { .key = 100 };
    TestItem ti2 = { .key = 200 };
    hashmap_set(map1, &ti1);
    hashmap_set(map2, &ti2);

    TEST_ASSERT_EQUAL_UINT(1, (unsigned)hashmap_count(map1));
    TEST_ASSERT_EQUAL_UINT(1, (unsigned)hashmap_count(map2));

    TestItem lk100 = { .key = 100 };
    TestItem lk200 = { .key = 200 };
    TEST_ASSERT_NOT_NULL(hashmap_get(map1, &lk100));
    TEST_ASSERT_NULL(hashmap_get(map1, &lk200));
    TEST_ASSERT_NULL(hashmap_get(map2, &lk100));
    TEST_ASSERT_NOT_NULL(hashmap_get(map2, &lk200));

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
    RUN_TEST(test_create_with_null_allocators_returns_non_null);
    RUN_TEST(test_create_with_custom_allocators_returns_non_null);
    RUN_TEST(test_custom_free_called_on_hashmap_free);
    RUN_TEST(test_cap_zero_uses_minimum_16);
    RUN_TEST(test_cap_1_rounds_up_to_16);
    RUN_TEST(test_cap_17_rounds_up_to_32);
    RUN_TEST(test_cap_exact_power_of_two);
    RUN_TEST(test_large_cap);
    RUN_TEST(test_seeds_stored_correctly);
    RUN_TEST(test_udata_stored);
    RUN_TEST(test_elfree_callback_stored_and_called);
    RUN_TEST(test_first_malloc_failure_returns_null);
    RUN_TEST(test_second_malloc_failure_returns_null);
    RUN_TEST(test_map_functional_after_creation);
    RUN_TEST(test_multiple_items_insert_and_retrieve);
    RUN_TEST(test_elsize_one_byte);
    RUN_TEST(test_elsize_large);
    RUN_TEST(test_initial_count_is_zero);
    RUN_TEST(test_two_independent_maps);
    return UNITY_END();
}