#include "hashmap.c"
#include "unity.h"
#include "hashmap.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

/* -------------------------------------------------------------------------
 * Fixtures / file-scope variables
 * ---------------------------------------------------------------------- */

struct hashmap *g_map = NULL;

typedef struct {
    int key;
    int value;
} IntItem;

typedef struct {
    char key[64];
    int  value;
} StrItem;

/* -------------------------------------------------------------------------
 * Helper functions
 * ---------------------------------------------------------------------- */

static uint64_t int_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    const IntItem *it = (const IntItem *)item;
    return hashmap_murmur(&it->key, sizeof(it->key), seed0, seed1);
}

static int int_compare(const void *a, const void *b, void *udata) {
    (void)udata;
    const IntItem *ia = (const IntItem *)a;
    const IntItem *ib = (const IntItem *)b;
    return ia->key - ib->key;
}

static uint64_t str_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    const StrItem *it = (const StrItem *)item;
    return hashmap_murmur(it->key, strlen(it->key), seed0, seed1);
}

static int str_compare(const void *a, const void *b, void *udata) {
    (void)udata;
    const StrItem *ia = (const StrItem *)a;
    const StrItem *ib = (const StrItem *)b;
    return strcmp(ia->key, ib->key);
}

/* Iteration context structures */
typedef struct {
    int  count;
    int  sum;
    bool stop_after_first;
} IntIterCtx;

typedef struct {
    int  count;
    int  stop_at;
} StopCtx;

/* Iterator callbacks */
static bool iter_count_all(const void *item, void *udata) {
    (void)item;
    IntIterCtx *ctx = (IntIterCtx *)udata;
    ctx->count++;
    ctx->sum += ((const IntItem *)item)->value;
    return true;
}

static bool iter_stop_immediately(const void *item, void *udata) {
    (void)item;
    StopCtx *ctx = (StopCtx *)udata;
    ctx->count++;
    return false; /* stop on first call */
}

static bool iter_stop_after_n(const void *item, void *udata) {
    (void)item;
    StopCtx *ctx = (StopCtx *)udata;
    ctx->count++;
    if (ctx->count >= ctx->stop_at) {
        return false;
    }
    return true;
}

static bool iter_collect_keys(const void *item, void *udata) {
    int **arr = (int **)udata;
    const IntItem *it = (const IntItem *)item;
    /* find first free slot (sentinel -1) */
    for (int i = 0; i < 1024; i++) {
        if ((*arr)[i] == -1) {
            (*arr)[i] = it->key;
            break;
        }
    }
    return true;
}

static bool iter_always_true(const void *item, void *udata) {
    (void)item;
    (void)udata;
    return true;
}

/* -------------------------------------------------------------------------
 * setUp / tearDown
 * ---------------------------------------------------------------------- */

void setUp(void) {
    g_map = hashmap_new(sizeof(IntItem), 0, 0, 0,
                        int_hash, int_compare, NULL, NULL);
}

void tearDown(void) {
    if (g_map) {
        hashmap_free(g_map);
        g_map = NULL;
    }
}

/* -------------------------------------------------------------------------
 * Test cases
 * ---------------------------------------------------------------------- */

/* 1. Scan an empty map — should return true, iterator never called */
void test_scan_empty_map_returns_true(void) {
    IntIterCtx ctx = {0, 0, false};
    bool result = hashmap_scan(g_map, iter_count_all, &ctx);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(0, ctx.count);
    TEST_ASSERT_EQUAL_INT(0, ctx.sum);
}

/* 2. Scan a map with one item — iterator called once, returns true */
void test_scan_single_item_visits_once(void) {
    IntItem item = {42, 100};
    hashmap_set(g_map, &item);
    TEST_ASSERT_FALSE(hashmap_oom(g_map));

    IntIterCtx ctx = {0, 0, false};
    bool result = hashmap_scan(g_map, iter_count_all, &ctx);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(1, ctx.count);
    TEST_ASSERT_EQUAL_INT(100, ctx.sum);
}

/* 3. Scan visits every item exactly once */
void test_scan_visits_all_items(void) {
    const int N = 50;
    int expected_sum = 0;
    for (int i = 0; i < N; i++) {
        IntItem item = {i, i * 2};
        hashmap_set(g_map, &item);
        expected_sum += i * 2;
    }
    TEST_ASSERT_EQUAL_UINT((size_t)N, hashmap_count(g_map));

    IntIterCtx ctx = {0, 0, false};
    bool result = hashmap_scan(g_map, iter_count_all, &ctx);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(N, ctx.count);
    TEST_ASSERT_EQUAL_INT(expected_sum, ctx.sum);
}

/* 4. Iterator returning false on first call stops scan, returns false */
void test_scan_stops_when_iter_returns_false_immediately(void) {
    for (int i = 0; i < 10; i++) {
        IntItem item = {i, i};
        hashmap_set(g_map, &item);
    }

    StopCtx ctx = {0, 1};
    bool result = hashmap_scan(g_map, iter_stop_immediately, &ctx);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL_INT(1, ctx.count);
}

/* 5. Iterator returning false after N calls stops early */
void test_scan_stops_early_after_n_items(void) {
    const int N = 20;
    for (int i = 0; i < N; i++) {
        IntItem item = {i, i};
        hashmap_set(g_map, &item);
    }

    StopCtx ctx = {0, 5};
    bool result = hashmap_scan(g_map, iter_stop_after_n, &ctx);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL_INT(5, ctx.count);
}

/* 6. Iterator that always returns true causes scan to return true */
void test_scan_returns_true_when_iter_always_true(void) {
    for (int i = 0; i < 30; i++) {
        IntItem item = {i, i};
        hashmap_set(g_map, &item);
    }

    bool result = hashmap_scan(g_map, iter_always_true, NULL);
    TEST_ASSERT_TRUE(result);
}

/* 7. Scan after clearing the map — empty again, returns true */
void test_scan_after_clear_returns_true(void) {
    for (int i = 0; i < 10; i++) {
        IntItem item = {i, i};
        hashmap_set(g_map, &item);
    }
    hashmap_clear(g_map, false);

    IntIterCtx ctx = {0, 0, false};
    bool result = hashmap_scan(g_map, iter_count_all, &ctx);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(0, ctx.count);
}

/* 8. Scan count matches hashmap_count */
void test_scan_count_matches_hashmap_count(void) {
    const int N = 77;
    for (int i = 0; i < N; i++) {
        IntItem item = {i * 3, i};
        hashmap_set(g_map, &item);
    }

    IntIterCtx ctx = {0, 0, false};
    hashmap_scan(g_map, iter_count_all, &ctx);
    TEST_ASSERT_EQUAL_UINT(hashmap_count(g_map), (size_t)ctx.count);
}

/* 9. Scan with NULL udata — iterator receives NULL udata */
static bool iter_check_null_udata(const void *item, void *udata) {
    (void)item;
    TEST_ASSERT_NULL(udata);
    return true;
}

void test_scan_null_udata_passed_to_iter(void) {
    IntItem item = {7, 7};
    hashmap_set(g_map, &item);

    bool result = hashmap_scan(g_map, iter_check_null_udata, NULL);
    TEST_ASSERT_TRUE(result);
}

/* 10. Scan collects all keys — no duplicates, all present */
void test_scan_collects_all_keys_no_duplicates(void) {
    const int N = 15;
    int *collected = (int *)malloc(sizeof(int) * 1024);
    for (int i = 0; i < 1024; i++) collected[i] = -1;

    for (int i = 0; i < N; i++) {
        IntItem item = {i + 1, i + 1};
        hashmap_set(g_map, &item);
    }

    bool result = hashmap_scan(g_map, iter_collect_keys, &collected);
    TEST_ASSERT_TRUE(result);

    /* Verify each key 1..N appears exactly once */
    for (int k = 1; k <= N; k++) {
        int found = 0;
        for (int j = 0; j < 1024; j++) {
            if (collected[j] == k) found++;
        }
        TEST_ASSERT_EQUAL_INT_MESSAGE(1, found, "Each key should appear exactly once");
    }

    free(collected);
}

/* 11. Scan with large map — all items visited */
void test_scan_large_map_all_items_visited(void) {
    const int N = 500;
    int expected_sum = 0;
    for (int i = 0; i < N; i++) {
        IntItem item = {i, i};
        hashmap_set(g_map, &item);
        expected_sum += i;
    }

    IntIterCtx ctx = {0, 0, false};
    bool result = hashmap_scan(g_map, iter_count_all, &ctx);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(N, ctx.count);
    TEST_ASSERT_EQUAL_INT(expected_sum, ctx.sum);
}

/* 12. Scan on map with string keys */
static bool str_iter_count(const void *item, void *udata) {
    (void)item;
    int *cnt = (int *)udata;
    (*cnt)++;
    return true;
}

void test_scan_string_key_map(void) {
    struct hashmap *smap = hashmap_new(sizeof(StrItem), 0, 0, 0,
                                       str_hash, str_compare, NULL, NULL);
    TEST_ASSERT_NOT_NULL(smap);

    const char *keys[] = {"alpha", "beta", "gamma", "delta", "epsilon"};
    int n = 5;
    for (int i = 0; i < n; i++) {
        StrItem it;
        memset(&it, 0, sizeof(it));
        strncpy(it.key, keys[i], sizeof(it.key) - 1);
        it.value = i;
        hashmap_set(smap, &it);
    }

    int count = 0;
    bool result = hashmap_scan(smap, str_iter_count, &count);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(n, count);

    hashmap_free(smap);
}

/* 13. Scan after deleting some items — count is correct */
void test_scan_after_delete_correct_count(void) {
    const int N = 20;
    for (int i = 0; i < N; i++) {
        IntItem item = {i, i};
        hashmap_set(g_map, &item);
    }
    /* Delete half */
    for (int i = 0; i < N; i += 2) {
        IntItem key = {i, 0};
        hashmap_delete(g_map, &key);
    }

    IntIterCtx ctx = {0, 0, false};
    bool result = hashmap_scan(g_map, iter_count_all, &ctx);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_UINT(hashmap_count(g_map), (size_t)ctx.count);
}

/* 14. Scan with stop_at == 1 on a map with exactly 1 item returns false */
void test_scan_stop_at_one_single_item_returns_false(void) {
    IntItem item = {99, 99};
    hashmap_set(g_map, &item);

    StopCtx ctx = {0, 1};
    bool result = hashmap_scan(g_map, iter_stop_after_n, &ctx);
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL_INT(1, ctx.count);
}

/* 15. Scan with capacity hint — map still scanned correctly */
void test_scan_with_capacity_hint(void) {
    struct hashmap *cmap = hashmap_new(sizeof(IntItem), 128, 12345, 67890,
                                       int_hash, int_compare, NULL, NULL);
    TEST_ASSERT_NOT_NULL(cmap);

    const int N = 10;
    int expected_sum = 0;
    for (int i = 0; i < N; i++) {
        IntItem item = {i, i * 3};
        hashmap_set(cmap, &item);
        expected_sum += i * 3;
    }

    IntIterCtx ctx = {0, 0, false};
    bool result = hashmap_scan(cmap, iter_count_all, &ctx);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_INT(N, ctx.count);
    TEST_ASSERT_EQUAL_INT(expected_sum, ctx.sum);

    hashmap_free(cmap);
}

/* -------------------------------------------------------------------------
 * main
 * ---------------------------------------------------------------------- */

int main(void) {
    unity_install_sighandler();
    UNITY_BEGIN();
    RUN_TEST(test_scan_empty_map_returns_true);
    RUN_TEST(test_scan_single_item_visits_once);
    RUN_TEST(test_scan_visits_all_items);
    RUN_TEST(test_scan_stops_when_iter_returns_false_immediately);
    RUN_TEST(test_scan_stops_early_after_n_items);
    RUN_TEST(test_scan_returns_true_when_iter_always_true);
    RUN_TEST(test_scan_after_clear_returns_true);
    RUN_TEST(test_scan_count_matches_hashmap_count);
    RUN_TEST(test_scan_null_udata_passed_to_iter);
    RUN_TEST(test_scan_collects_all_keys_no_duplicates);
    RUN_TEST(test_scan_large_map_all_items_visited);
    RUN_TEST(test_scan_string_key_map);
    RUN_TEST(test_scan_after_delete_correct_count);
    RUN_TEST(test_scan_stop_at_one_single_item_returns_false);
    RUN_TEST(test_scan_with_capacity_hint);
    return UNITY_END();
}