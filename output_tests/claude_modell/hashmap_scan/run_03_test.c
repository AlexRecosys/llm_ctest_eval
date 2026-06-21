#include "hashmap.c"
#include "unity.h"
#include "hashmap.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

/* ---------------------------------------------------------------------------
 * Fixtures / file-scope variables
 * -------------------------------------------------------------------------*/

struct item {
    int   key;
    int   value;
};

static struct hashmap *g_map;

/* ---------------------------------------------------------------------------
 * Helper: hash / compare callbacks
 * -------------------------------------------------------------------------*/

static uint64_t item_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
    const struct item *it = (const struct item *)item;
    return hashmap_murmur(&it->key, sizeof(it->key), seed0, seed1);
}

static int item_compare(const void *a, const void *b, void *udata)
{
    (void)udata;
    const struct item *ia = (const struct item *)a;
    const struct item *ib = (const struct item *)b;
    return ia->key - ib->key;
}

/* ---------------------------------------------------------------------------
 * Iterator callbacks used by tests
 * -------------------------------------------------------------------------*/

/* Counts every item visited */
static bool iter_count(const void *item, void *udata)
{
    (void)item;
    int *cnt = (int *)udata;
    (*cnt)++;
    return true;
}

/* Sums the 'value' field of every item */
static bool iter_sum_values(const void *item, void *udata)
{
    const struct item *it = (const struct item *)item;
    int *sum = (int *)udata;
    *sum += it->value;
    return true;
}

/* Stops iteration after visiting the first item */
static bool iter_stop_immediately(const void *item, void *udata)
{
    (void)item;
    (void)udata;
    return false;
}

/* Stops after a configurable number of items */
typedef struct {
    int limit;
    int visited;
} stop_after_ctx;

static bool iter_stop_after_n(const void *item, void *udata)
{
    (void)item;
    stop_after_ctx *ctx = (stop_after_ctx *)udata;
    ctx->visited++;
    if (ctx->visited >= ctx->limit) {
        return false;
    }
    return true;
}

/* Collects all keys into an array */
typedef struct {
    int  keys[256];
    int  count;
} key_collector;

static bool iter_collect_keys(const void *item, void *udata)
{
    const struct item *it = (const struct item *)item;
    key_collector     *kc = (key_collector *)udata;
    kc->keys[kc->count++] = it->key;
    return true;
}

/* Verifies that every item has value == key * 2 */
static bool iter_verify_value(const void *item, void *udata)
{
    const struct item *it = (const struct item *)item;
    bool *ok = (bool *)udata;
    if (it->value != it->key * 2) {
        *ok = false;
        return false;
    }
    return true;
}

/* ---------------------------------------------------------------------------
 * setUp / tearDown
 * -------------------------------------------------------------------------*/

void setUp(void)
{
    g_map = hashmap_new(sizeof(struct item), 0, 0, 0,
                        item_hash, item_compare, NULL, NULL);
    TEST_ASSERT_NOT_NULL(g_map);
}

void tearDown(void)
{
    hashmap_free(g_map);
    g_map = NULL;
}

/* ---------------------------------------------------------------------------
 * Test cases
 * -------------------------------------------------------------------------*/

/* scan on an empty map must return true and visit zero items */
void test_scan_empty_map_returns_true(void)
{
    int count = 0;
    bool result = hashmap_scan(g_map, iter_count, &count);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(0, count);
}

/* scan visits every item exactly once */
void test_scan_visits_all_items(void)
{
    const int N = 10;
    for (int i = 0; i < N; i++) {
        struct item it = { .key = i, .value = i };
        hashmap_set(g_map, &it);
        TEST_ASSERT_FALSE(hashmap_oom(g_map));
    }

    int count = 0;
    bool result = hashmap_scan(g_map, iter_count, &count);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(N, count);
}

/* scan returns false when the iterator returns false on the first item */
void test_scan_stops_when_iter_returns_false(void)
{
    for (int i = 0; i < 5; i++) {
        struct item it = { .key = i, .value = i };
        hashmap_set(g_map, &it);
    }

    bool result = hashmap_scan(g_map, iter_stop_immediately, NULL);
    TEST_ASSERT_FALSE(result);
}

/* scan returns false when iterator stops partway through */
void test_scan_returns_false_on_early_stop(void)
{
    const int N = 20;
    for (int i = 0; i < N; i++) {
        struct item it = { .key = i, .value = i };
        hashmap_set(g_map, &it);
    }

    stop_after_ctx ctx = { .limit = 5, .visited = 0 };
    bool result = hashmap_scan(g_map, iter_stop_after_n, &ctx);
    TEST_ASSERT_FALSE(result);
    /* visited must be exactly limit because we return false at that point */
    TEST_ASSERT_EQUAL_INT(5, ctx.visited);
}

/* scan with a single item visits it and returns true */
void test_scan_single_item(void)
{
    struct item it = { .key = 42, .value = 99 };
    hashmap_set(g_map, &it);

    int count = 0;
    bool result = hashmap_scan(g_map, iter_count, &count);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(1, count);
}

/* udata pointer is correctly forwarded to the iterator */
void test_scan_udata_forwarded(void)
{
    for (int i = 1; i <= 5; i++) {
        struct item it = { .key = i, .value = i * 10 };
        hashmap_set(g_map, &it);
    }

    int sum = 0;
    bool result = hashmap_scan(g_map, iter_sum_values, &sum);
    TEST_ASSERT_TRUE(result);
    /* 10+20+30+40+50 = 150 */
    TEST_ASSERT_EQUAL_INT(150, sum);
}

/* scan visits every key exactly once (no duplicates, no missing) */
void test_scan_no_duplicate_visits(void)
{
    const int N = 50;
    for (int i = 0; i < N; i++) {
        struct item it = { .key = i, .value = i };
        hashmap_set(g_map, &it);
    }

    key_collector kc;
    memset(&kc, 0, sizeof(kc));
    bool result = hashmap_scan(g_map, iter_collect_keys, &kc);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(N, kc.count);

    /* verify each key 0..N-1 appears exactly once */
    int seen[50];
    memset(seen, 0, sizeof(seen));
    for (int i = 0; i < kc.count; i++) {
        int k = kc.keys[i];
        TEST_ASSERT_TRUE_MESSAGE(k >= 0 && k < N, "key out of range");
        seen[k]++;
    }
    for (int i = 0; i < N; i++) {
        TEST_ASSERT_EQUAL_INT_MESSAGE(1, seen[i], "key not visited exactly once");
    }
}

/* scan after deleting some items visits only the remaining items */
void test_scan_after_delete(void)
{
    const int N = 10;
    for (int i = 0; i < N; i++) {
        struct item it = { .key = i, .value = i };
        hashmap_set(g_map, &it);
    }
    /* delete even keys */
    for (int i = 0; i < N; i += 2) {
        struct item key = { .key = i };
        hashmap_delete(g_map, &key);
    }

    int count = 0;
    bool result = hashmap_scan(g_map, iter_count, &count);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(N / 2, count);
}

/* scan after clear visits zero items */
void test_scan_after_clear(void)
{
    for (int i = 0; i < 8; i++) {
        struct item it = { .key = i, .value = i };
        hashmap_set(g_map, &it);
    }
    hashmap_clear(g_map, false);

    int count = 0;
    bool result = hashmap_scan(g_map, iter_count, &count);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(0, count);
}

/* scan item data is correct (value == key * 2) */
void test_scan_item_data_correct(void)
{
    const int N = 30;
    for (int i = 0; i < N; i++) {
        struct item it = { .key = i, .value = i * 2 };
        hashmap_set(g_map, &it);
    }

    bool ok = true;
    bool result = hashmap_scan(g_map, iter_verify_value, &ok);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(ok);
}

/* scan on a large map returns true and visits all items */
void test_scan_large_map(void)
{
    const int N = 1000;
    for (int i = 0; i < N; i++) {
        struct item it = { .key = i, .value = i };
        hashmap_set(g_map, &it);
        TEST_ASSERT_FALSE(hashmap_oom(g_map));
    }

    int count = 0;
    bool result = hashmap_scan(g_map, iter_count, &count);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(N, count);
}

/* scan count matches hashmap_count */
void test_scan_count_matches_hashmap_count(void)
{
    const int N = 37;
    for (int i = 0; i < N; i++) {
        struct item it = { .key = i * 3, .value = i };
        hashmap_set(g_map, &it);
    }

    int scan_count = 0;
    hashmap_scan(g_map, iter_count, &scan_count);
    TEST_ASSERT_EQUAL_INT((int)hashmap_count(g_map), scan_count);
}

/* ---------------------------------------------------------------------------
 * main
 * -------------------------------------------------------------------------*/

int main(void)
{
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_scan_empty_map_returns_true);
    RUN_TEST(test_scan_visits_all_items);
    RUN_TEST(test_scan_stops_when_iter_returns_false);
    RUN_TEST(test_scan_returns_false_on_early_stop);
    RUN_TEST(test_scan_single_item);
    RUN_TEST(test_scan_udata_forwarded);
    RUN_TEST(test_scan_no_duplicate_visits);
    RUN_TEST(test_scan_after_delete);
    RUN_TEST(test_scan_after_clear);
    RUN_TEST(test_scan_item_data_correct);
    RUN_TEST(test_scan_large_map);
    RUN_TEST(test_scan_count_matches_hashmap_count);
    return UNITY_END();
}