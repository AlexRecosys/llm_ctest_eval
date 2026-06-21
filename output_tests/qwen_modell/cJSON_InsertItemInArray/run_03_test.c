#include "cJSON.c"
#include "unity.h"
#include <stdlib.h>
#include <string.h>

/* Helper to create a cJSON array with initial items */
static cJSON *create_array_with_items(int count, ...) {
    cJSON *array = cJSON_CreateArray();
    if (!array) return NULL;

    va_list args;
    va_start(args, count);
    for (int i = 0; i < count; i++) {
        cJSON *item = va_arg(args, cJSON *);
        if (!cJSON_AddItemToArray(array, item)) {
            cJSON_Delete(array);
            va_end(args);
            return NULL;
        }
    }
    va_end(args);
    return array;
}

/* Helper to verify array contents match expected values */
static cJSON_bool verify_array_contents(cJSON *array, const int *expected, size_t expected_count) {
    if (!array || !expected) return false;
    if ((size_t)cJSON_GetArraySize(array) != expected_count) return false;

    for (size_t i = 0; i < expected_count; i++) {
        cJSON *item = cJSON_GetArrayItem(array, (int)i);
        if (!item || item->type != cJSON_Number || item->valuedouble != (double)expected[i]) {
            return false;
        }
    }
    return true;
}

/* Test: Insert at beginning of empty array (should fail because which=0 is invalid for empty array) */
static void test_insert_in_empty_array_at_zero_should_fail(void) {
    cJSON *array = cJSON_CreateArray();
    cJSON *item = cJSON_CreateNumber(42);

    TEST_ASSERT_FALSE(cJSON_InsertItemInArray(array, 0, item));

    /* item should remain untouched */
    TEST_ASSERT_EQUAL_INT(42, item->valuedouble);
    TEST_ASSERT_NULL(array->child);

    cJSON_Delete(array);
    cJSON_Delete(item);
}

/* Test: Insert at negative index should fail */
static void test_insert_negative_index_should_fail(void) {
    cJSON *array = cJSON_CreateArray();
    cJSON *item = cJSON_CreateNumber(42);

    TEST_ASSERT_FALSE(cJSON_InsertItemInArray(array, -1, item));
    TEST_ASSERT_NULL(array->child);

    cJSON_Delete(array);
    cJSON_Delete(item);
}

/* Test: Insert NULL item should fail */
static void test_insert_null_item_should_fail(void) {
    cJSON *array = cJSON_CreateArray();

    TEST_ASSERT_FALSE(cJSON_InsertItemInArray(array, 0, NULL));
    TEST_ASSERT_NULL(array->child);

    cJSON_Delete(array);
}

/* Test: Insert at position beyond array size should append */
static void test_insert_beyond_array_size_should_append(void) {
    cJSON *array = cJSON_CreateArray();
    cJSON *item1 = cJSON_CreateNumber(1);
    cJSON *item2 = cJSON_CreateNumber(2);
    cJSON *item3 = cJSON_CreateNumber(3);

    cJSON_AddItemToArray(array, item1);
    cJSON_AddItemToArray(array, item2);

    /* Insert at index 5 (beyond size=2) → should append */
    TEST_ASSERT_TRUE(cJSON_InsertItemInArray(array, 5, item3));

    int expected[] = {1, 2, 3};
    TEST_ASSERT_TRUE(verify_array_contents(array, expected, 3));

    cJSON_Delete(array);
}

/* Test: Insert at valid index in middle of array */
static void test_insert_in_middle_should_shift_right(void) {
    cJSON *array = cJSON_CreateArray();
    cJSON *item1 = cJSON_CreateNumber(1);
    cJSON *item2 = cJSON_CreateNumber(2);
    cJSON *item3 = cJSON_CreateNumber(3);
    cJSON *new_item = cJSON_CreateNumber(99);

    cJSON_AddItemToArray(array, item1);
    cJSON_AddItemToArray(array, item2);

    TEST_ASSERT_TRUE(cJSON_InsertItemInArray(array, 1, new_item));

    int expected[] = {1, 99, 2};
    TEST_ASSERT_TRUE(verify_array_contents(array, expected, 3));

    /* Verify links */
    TEST_ASSERT_EQUAL_PTR(item1, array->child);
    TEST_ASSERT_EQUAL_PTR(new_item, item1->next);
    TEST_ASSERT_EQUAL_PTR(item2, new_item->next);
    TEST_ASSERT_NULL(item2->next);
    TEST_ASSERT_EQUAL_PTR(new_item, item2->prev);
    TEST_ASSERT_EQUAL_PTR(item1, new_item->prev);
    TEST_ASSERT_NULL(item1->prev);

    cJSON_Delete(array);
}

/* Test: Insert at index 0 (beginning) */
static void test_insert_at_beginning_should_update_head(void) {
    cJSON *array = cJSON_CreateArray();
    cJSON *item1 = cJSON_CreateNumber(1);
    cJSON *item2 = cJSON_CreateNumber(2);
    cJSON *new_item = cJSON_CreateNumber(0);

    cJSON_AddItemToArray(array, item1);
    cJSON_AddItemToArray(array, item2);

    TEST_ASSERT_TRUE(cJSON_InsertItemInArray(array, 0, new_item));

    int expected[] = {0, 1, 2};
    TEST_ASSERT_TRUE(verify_array_contents(array, expected, 3));

    /* Verify head updated */
    TEST_ASSERT_EQUAL_PTR(new_item, array->child);
    TEST_ASSERT_NULL(new_item->prev);
    TEST_ASSERT_EQUAL_PTR(item1, new_item->next);
    TEST_ASSERT_EQUAL_PTR(new_item, item1->prev);

    cJSON_Delete(array);
}

/* Test: Insert at end of array (which == size) */
static void test_insert_at_end_should_append(void) {
    cJSON *array = cJSON_CreateArray();
    cJSON *item1 = cJSON_CreateNumber(1);
    cJSON *item2 = cJSON_CreateNumber(2);
    cJSON *new_item = cJSON_CreateNumber(3);

    cJSON_AddItemToArray(array, item1);
    cJSON_AddItemToArray(array, item2);

    TEST_ASSERT_TRUE(cJSON_InsertItemInArray(array, 2, new_item));

    int expected[] = {1, 2, 3};
    TEST_ASSERT_TRUE(verify_array_contents(array, expected, 3));

    /* Verify tail */
    TEST_ASSERT_EQUAL_PTR(item2, new_item->prev);
    TEST_ASSERT_NULL(new_item->next);

    cJSON_Delete(array);
}

/* Test: Insert into single-element array at index 0 */
static void test_insert_into_single_element_array_at_zero(void) {
    cJSON *array = cJSON_CreateArray();
    cJSON *item1 = cJSON_CreateNumber(1);
    cJSON *new_item = cJSON_CreateNumber(0);

    cJSON_AddItemToArray(array, item1);

    TEST_ASSERT_TRUE(cJSON_InsertItemInArray(array, 0, new_item));

    int expected[] = {0, 1};
    TEST_ASSERT_TRUE(verify_array_contents(array, expected, 2));

    TEST_ASSERT_EQUAL_PTR(new_item, array->child);
    TEST_ASSERT_EQUAL_PTR(item1, new_item->next);
    TEST_ASSERT_EQUAL_PTR(new_item, item1->prev);
    TEST_ASSERT_NULL(new_item->prev);

    cJSON_Delete(array);
}

/* Test: Insert into single-element array at index 1 */
static void test_insert_into_single_element_array_at_one(void) {
    cJSON *array = cJSON_CreateArray();
    cJSON *item1 = cJSON_CreateNumber(1);
    cJSON *new_item = cJSON_CreateNumber(2);

    cJSON_AddItemToArray(array, item1);

    TEST_ASSERT_TRUE(cJSON_InsertItemInArray(array, 1, new_item));

    int expected[] = {1, 2};
    TEST_ASSERT_TRUE(verify_array_contents(array, expected, 2));

    TEST_ASSERT_EQUAL_PTR(item1, array->child);
    TEST_ASSERT_EQUAL_PTR(new_item, item1->next);
    TEST_ASSERT_EQUAL_PTR(item1, new_item->prev);
    TEST_ASSERT_NULL(new_item->next);

    cJSON_Delete(array);
}

/* Test: Insert into larger array at various positions */
static void test_insert_in_larger_array(void) {
    cJSON *array = cJSON_CreateArray();
    cJSON *items[5];
    for (int i = 0; i < 5; i++) {
        items[i] = cJSON_CreateNumber(i);
        cJSON_AddItemToArray(array, items[i]);
    }

    /* Insert at index 2 */
    cJSON *new_item = cJSON_CreateNumber(99);
    TEST_ASSERT_TRUE(cJSON_InsertItemInArray(array, 2, new_item));

    int expected[] = {0, 1, 99, 2, 3, 4};
    TEST_ASSERT_TRUE(verify_array_contents(array, expected, 6));

    /* Verify links */
    TEST_ASSERT_EQUAL_PTR(items[0], array->child);
    TEST_ASSERT_EQUAL_PTR(items[1], items[0]->next);
    TEST_ASSERT_EQUAL_PTR(new_item, items[1]->next);
    TEST_ASSERT_EQUAL_PTR(items[2], new_item->next);
    TEST_ASSERT_EQUAL_PTR(items[3], items[2]->next);
    TEST_ASSERT_EQUAL_PTR(items[4], items[3]->next);
    TEST_ASSERT_NULL(items[4]->next);

    TEST_ASSERT_EQUAL_PTR(items[1], new_item->prev);
    TEST_ASSERT_EQUAL_PTR(new_item, items[2]->prev);
    TEST_ASSERT_EQUAL_PTR(items[1], items[0]->prev);
    TEST_ASSERT_NULL(items[0]->prev);

    cJSON_Delete(array);
}

/* Test: Corrupted item (prev != NULL but not array->child) should fail */
static void test_insert_into_corrupted_item_should_fail(void) {
    cJSON *array = cJSON_CreateArray();
    cJSON *item1 = cJSON_CreateNumber(1);
    cJSON *item2 = cJSON_CreateNumber(2);
    cJSON *new_item = cJSON_CreateNumber(99);

    /* Manually corrupt item2's prev pointer */
    cJSON_AddItemToArray(array, item1);
    cJSON_AddItemToArray(array, item2);
    item2->prev = (cJSON *)0x12345678; /* invalid prev */

    /* Should fail because item2->prev is non-NULL but item2 is not array->child */
    TEST_ASSERT_FALSE(cJSON_InsertItemInArray(array, 1, new_item));

    /* Array should be unchanged */
    int expected[] = {1, 2};
    TEST_ASSERT_TRUE(verify_array_contents(array, expected, 2));

    cJSON_Delete(array);
}

/* Test: Insert at index 0 into array with one item */
static void test_insert_at_zero_single_item_array(void) {
    cJSON *array = cJSON_CreateArray();
    cJSON *item1 = cJSON_CreateNumber(1);
    cJSON *new_item = cJSON_CreateNumber(0);

    cJSON_AddItemToArray(array, item1);

    TEST_ASSERT_TRUE(cJSON_InsertItemInArray(array, 0, new_item));

    int expected[] = {0, 1};
    TEST_ASSERT_TRUE(verify_array_contents(array, expected, 2));

    TEST_ASSERT_EQUAL_PTR(new_item, array->child);
    TEST_ASSERT_EQUAL_PTR(item1, new_item->next);
    TEST_ASSERT_EQUAL_PTR(new_item, item1->prev);
    TEST_ASSERT_NULL(new_item->prev);

    cJSON_Delete(array);
}

/* Test: Insert at index 1 into array with one item */
static void test_insert_at_one_single_item_array(void) {
    cJSON *array = cJSON_CreateArray();
    cJSON *item1 = cJSON_CreateNumber(1);
    cJSON *new_item = cJSON_CreateNumber(2);

    cJSON_AddItemToArray(array, item1);

    TEST_ASSERT_TRUE(cJSON_InsertItemInArray(array, 1, new_item));

    int expected[] = {1, 2};
    TEST_ASSERT_TRUE(verify_array_contents(array, expected, 2));

    TEST_ASSERT_EQUAL_PTR(item1, array->child);
    TEST_ASSERT_EQUAL_PTR(new_item, item1->next);
    TEST_ASSERT_EQUAL_PTR(item1, new_item->prev);
    TEST_ASSERT_NULL(new_item->next);

    cJSON_Delete(array);
}

/* Test: Insert at index 2 into array with two items */
static void test_insert_at_two_two_item_array(void) {
    cJSON *array = cJSON_CreateArray();
    cJSON *item1 = cJSON_CreateNumber(1);
    cJSON *item2 = cJSON_CreateNumber(2);
    cJSON *new_item = cJSON_CreateNumber(3);

    cJSON_AddItemToArray(array, item1);
    cJSON_AddItemToArray(array, item2);

    TEST_ASSERT_TRUE(cJSON_InsertItemInArray(array, 2, new_item));

    int expected[] = {1, 2, 3};
    TEST_ASSERT_TRUE(verify_array_contents(array, expected, 3));

    TEST_ASSERT_EQUAL_PTR(item1, array->child);
    TEST_ASSERT_EQUAL_PTR(item2, item1->next);
    TEST_ASSERT_EQUAL_PTR(new_item, item2->next);
    TEST_ASSERT_NULL(new_item->next);

    TEST_ASSERT_EQUAL_PTR(item1, item2->prev);
    TEST_ASSERT_EQUAL_PTR(item2, new_item->prev);
    TEST_ASSERT_NULL(item1->prev);

    cJSON_Delete(array);
}

/* Test: Insert at index 1 into array with two items */
static void test_insert_at_one_two_item_array(void) {
    cJSON *array = cJSON_CreateArray();
    cJSON *item1 = cJSON_CreateNumber(1);
    cJSON *item2 = cJSON_CreateNumber(2);
    cJSON *new_item = cJSON_CreateNumber(99);

    cJSON_AddItemToArray(array, item1);
    cJSON_AddItemToArray(array, item2);

    TEST_ASSERT_TRUE(cJSON_InsertItemInArray(array, 1, new_item));

    int expected[] = {1, 99, 2};
    TEST_ASSERT_TRUE(verify_array_contents(array, expected, 3));

    TEST_ASSERT_EQUAL_PTR(item1, array->child);
    TEST_ASSERT_EQUAL_PTR(new_item, item1->next);
    TEST_ASSERT_EQUAL_PTR(item2, new_item->next);
    TEST_ASSERT_NULL(item2->next);

    TEST_ASSERT_EQUAL_PTR(item1, new_item->prev);
    TEST_ASSERT_EQUAL_PTR(new_item, item2->prev);
    TEST_ASSERT_NULL(item1->prev);

    cJSON_Delete(array);
}

/* Test: Insert at index 0 into array with two items */
static void test_insert_at_zero_two_item_array(void) {
    cJSON *array = cJSON_CreateArray();
    cJSON *item1 = cJSON_CreateNumber(1);
    cJSON *item2 = cJSON_CreateNumber(2);
    cJSON *new_item = cJSON_CreateNumber(0);

    cJSON_AddItemToArray(array, item1);
    cJSON_AddItemToArray(array, item2);

    TEST_ASSERT_TRUE(cJSON_InsertItemInArray(array, 0, new_item));

    int expected[] = {0, 1, 2};
    TEST_ASSERT_TRUE(verify_array_contents(array, expected, 3));

    TEST_ASSERT_EQUAL_PTR(new_item, array->child);
    TEST_ASSERT_EQUAL_PTR(item1, new_item->next);
    TEST_ASSERT_EQUAL_PTR(item2, item1->next);
    TEST_ASSERT_NULL(item2->next);

    TEST_ASSERT_EQUAL_PTR(new_item, item1->prev);
    TEST_ASSERT_EQUAL_PTR(item1, item2->prev);
    TEST_ASSERT_NULL(new_item->prev);

    cJSON_Delete(array);
}

/* Test: Insert at index 3 into array with three items */
static void test_insert_at_three_three_item_array(void) {
    cJSON *array = cJSON_CreateArray();
    cJSON *item1 = cJSON_CreateNumber(1);
    cJSON *item2 = cJSON_CreateNumber(2);
    cJSON *item3 = cJSON_CreateNumber(3);
    cJSON *new_item = cJSON_CreateNumber(4);

    cJSON_AddItemToArray(array, item1);
    cJSON_AddItemToArray(array, item2);
    cJSON_AddItemToArray(array, item3);

    TEST_ASSERT_TRUE(cJSON_InsertItemInArray(array, 3, new_item));

    int expected[] = {1, 2, 3, 4};
    TEST_ASSERT_TRUE(verify_array_contents(array, expected, 4));

    TEST_ASSERT_EQUAL_PTR(item1, array->child);
    TEST_ASSERT_EQUAL_PTR(item2, item1->next);
    TEST_ASSERT_EQUAL_PTR(item3, item2->next);
    TEST_ASSERT_EQUAL_PTR(new_item, item3->next);
    TEST_ASSERT_NULL(new_item->next);

    TEST_ASSERT_EQUAL_PTR(item1, item2->prev);
    TEST_ASSERT_EQUAL_PTR(item2, item3->prev);
    TEST_ASSERT_EQUAL_PTR(item3, new_item->prev);
    TEST_ASSERT_NULL(item1->prev);

    cJSON_Delete(array);
}

/* Test: Insert at index 2 into array with three items */
static void test_insert_at_two_three_item_array(void) {
    cJSON *array = cJSON_CreateArray();
    cJSON *item1 = cJSON_CreateNumber(1);
    cJSON *item2 = cJSON_CreateNumber(2);
    cJSON *item3 = cJSON_CreateNumber(3);
    cJSON *new_item = cJSON_CreateNumber(99);

    cJSON_AddItemToArray(array, item1);
    cJSON_AddItemToArray(array, item2);
    cJSON_AddItemToArray(array, item3);

    TEST_ASSERT_TRUE(cJSON_InsertItemInArray(array, 2, new_item));

    int expected[] = {1, 2, 99, 3};
    TEST_ASSERT_TRUE(verify_array_contents(array, expected, 4));

    TEST_ASSERT_EQUAL_PTR(item1, array->child);
    TEST_ASSERT_EQUAL_PTR(item2, item1->next);
    TEST_ASSERT_EQUAL_PTR(new_item, item2->next);
    TEST_ASSERT_EQUAL_PTR(item3, new_item->next);
    TEST_ASSERT_NULL(item3->next);

    TEST_ASSERT_EQUAL_PTR(item1, item2->prev);
    TEST_ASSERT_EQUAL_PTR(item2, new_item->prev);
    TEST_ASSERT_EQUAL_PTR(new_item, item3->prev);
    TEST_ASSERT_NULL(item1->prev);

    cJSON_Delete(array);
}

/* Test: Insert at index 1 into array with three items */
static void test_insert_at_one_three_item_array(void) {
    cJSON *array = cJSON_CreateArray();
    cJSON *item1 = cJSON_CreateNumber(1);
    cJSON *item2 = cJSON_CreateNumber(2);
    cJSON *item3 = cJSON_CreateNumber(3);
    cJSON *new_item = cJSON_CreateNumber(99);

    cJSON_AddItemToArray(array, item1);
    cJSON_AddItemToArray(array, item2);
    cJSON_AddItemToArray(array, item3);

    TEST_ASSERT_TRUE(cJSON_InsertItemInArray(array, 1, new_item));

    int expected[] = {1, 99, 2, 3};
    TEST_ASSERT_TRUE(verify_array_contents(array, expected, 4));

    TEST_ASSERT_EQUAL_PTR(item1, array->child);
    TEST_ASSERT_EQUAL_PTR(new_item, item1->next);
    TEST_ASSERT_EQUAL_PTR(item2, new_item->next);
    TEST_ASSERT_EQUAL_PTR(item3, item2->next);
    TEST_ASSERT_NULL(item3->next);

    TEST_ASSERT_EQUAL_PTR(item1, new_item->prev);
    TEST_ASSERT_EQUAL_PTR(new_item, item2->prev);
    TEST_ASSERT_EQUAL_PTR(item2, item3->prev);
    TEST_ASSERT_NULL(item1->prev);

    cJSON_Delete(array);
}

/* Test: Insert at index 0 into array with three items */
static void test_insert_at_zero_three_item_array(void) {
    cJSON *array = cJSON_CreateArray();
    cJSON *item1 = cJSON_CreateNumber(1);
    cJSON *item2 = cJSON_CreateNumber(2);
    cJSON *item3 = cJSON_CreateNumber(3);
    cJSON *new_item = cJSON_CreateNumber(0);

    cJSON_AddItemToArray(array, item1);
    cJSON_AddItemToArray(array, item2);
    cJSON_AddItemToArray(array, item3);

    TEST_ASSERT_TRUE(cJSON_InsertItemInArray(array, 0, new_item));

    int expected[] = {0, 1, 2, 3};
    TEST_ASSERT_TRUE(verify_array_contents(array, expected, 4));

    TEST_ASSERT_EQUAL_PTR(new_item, array->child);
    TEST_ASSERT_EQUAL_PTR(item1, new_item->next);
    TEST_ASSERT_EQUAL_PTR(item2, item1->next);
    TEST_ASSERT_EQUAL_PTR(item3, item2->next);
    TEST_ASSERT_NULL(item3->next);

    TEST_ASSERT_EQUAL_PTR(new_item, item1->prev);
    TEST_ASSERT_EQUAL_PTR(item1, item2->prev);
    TEST_ASSERT_EQUAL_PTR(item2, item3->prev);
    TEST_ASSERT_NULL(new_item->prev);

    cJSON_Delete(array);
}

/* Test: Insert at index 4 into array with four items */
static void test_insert_at_four_four_item_array(void) {
    cJSON *array = cJSON_CreateArray();
    cJSON *items[4];
    for (int i = 0; i < 4; i++) {
        items[i] = cJSON_CreateNumber(i);
        cJSON_AddItemToArray(array, items[i]);
    }

    cJSON *new_item = cJSON_CreateNumber(4);
    TEST_ASSERT_TRUE(cJSON_InsertItemInArray(array, 4, new_item));

    int expected[] = {0, 1, 2, 3, 4};
    TEST_ASSERT_TRUE(verify_array_contents(array, expected, 5));

    TEST_ASSERT_EQUAL_PTR(items[0], array->child);
    TEST_ASSERT_EQUAL_PTR(items[1], items[0]->next);
    TEST_ASSERT_EQUAL_PTR(items[2], items[1]->next);
    TEST_ASSERT_EQUAL_PTR(items[3], items[2]->next);
    TEST_ASSERT_EQUAL_PTR(new_item, items[3]->next);
    TEST_ASSERT_NULL(new_item->next);

    TEST_ASSERT_EQUAL_PTR(items[0], items[1]->prev);
    TEST_ASSERT_EQUAL_PTR(items[1], items[2]->prev);
    TEST_ASSERT_EQUAL_PTR(items[2], items[3]->prev);
    TEST_ASSERT_EQUAL_PTR(items[3], new_item->prev);
    TEST_ASSERT_NULL(items[0]->prev);

    cJSON_Delete(array);
}

/* Test: Insert at index 2 into array with four items */
static void test_insert_at_two_four_item_array(void) {
    cJSON *array = cJSON_CreateArray();
    cJSON *items[4];
    for (int i = 0; i < 4; i++) {
        items[i] = cJSON_CreateNumber(i);
        cJSON_AddItemToArray(array, items[i]);
    }

    cJSON *new_item = cJSON_CreateNumber(99);
    TEST_ASSERT_TRUE(cJSON_InsertItemInArray(array, 2, new_item));

    int expected[] = {0, 1, 99, 2, 3};
    TEST_ASSERT_TRUE(verify_array_contents(array, expected, 5));

    TEST_ASSERT_EQUAL_PTR(items[0], array->child);
    TEST_ASSERT_EQUAL_PTR(items[1], items[0]->next);
    TEST_ASSERT_EQUAL_PTR(new_item, items[1]->next);
    TEST_ASSERT_EQUAL_PTR(items[2], new_item->next);
    TEST_ASSERT_EQUAL_PTR(items[3], items[2]->next);
    TEST_ASSERT_NULL(items[3]->next);

    TEST_ASSERT_EQUAL_PTR(items[0], items[1]->prev);
    TEST_ASSERT_EQUAL_PTR(items[1], new_item->prev);
    TEST_ASSERT_EQUAL_PTR(new_item, items[2]->prev);
    TEST_ASSERT_EQUAL_PTR(items[2], items[3]->prev);
    TEST_ASSERT_NULL(items[0]->prev);

    cJSON_Delete(array);
}

/* Test: Insert at index 1 into array with four items */
static void test_insert_at_one_four_item_array(void) {
    cJSON *array = cJSON_CreateArray();
    cJSON *items[4];
    for (int i = 0; i < 4; i++) {
        items[i] = cJSON_CreateNumber(i);
        cJSON_AddItemToArray(array, items[i]);
    }

    cJSON *new_item = cJSON_CreateNumber(99);
    TEST_ASSERT_TRUE(cJSON_InsertItemInArray(array, 1, new_item));

    int expected[] = {0, 99, 1, 2, 3};
    TEST_ASSERT_TRUE(verify_array_contents(array, expected, 5));

    TEST_ASSERT_EQUAL_PTR(items[0], array->child);
    TEST_ASSERT_EQUAL_PTR(new_item, items[0]->next);
    TEST_ASSERT_EQUAL_PTR(items[1], new_item->next);
    TEST_ASSERT_EQUAL_PTR(items[2], items[1]->next);
    TEST_ASSERT_EQUAL_PTR(items[3], items[2]->next);
    TEST_ASSERT_NULL(items[3]->next);

    TEST_ASSERT_EQUAL_PTR(items[0], new_item->prev);
    TEST_ASSERT_EQUAL_PTR(new_item, items[1]->prev);
    TEST_ASSERT_EQUAL_PTR(items[1], items[2]->prev);
    TEST_ASSERT_EQUAL_PTR(items[2], items[3]->prev);
    TEST_ASSERT_NULL(items[0]->prev);

    cJSON_Delete(array);
}

/* Test: Insert at index 0 into array with four items */
static void test_insert_at_zero_four_item_array(void) {
    cJSON *array = cJSON_CreateArray();
    cJSON *items[4];
    for (int i = 0; i < 4; i++) {
        items[i] = cJSON_CreateNumber(i);
        cJSON_AddItemToArray(array, items[i]);
    }

    cJSON *new_item = cJSON_CreateNumber(-1);
    TEST_ASSERT_TRUE(cJSON_InsertItemInArray(array, 0, new_item));

    int expected[] = {-1, 0, 1, 2, 3};
    TEST_ASSERT_TRUE(verify_array_contents(array, expected, 5));

    TEST_ASSERT_EQUAL_PTR(new_item, array->child);
    TEST_ASSERT_EQUAL_PTR(items[0], new_item->next);
    TEST_ASSERT_EQUAL_PTR(items[1], items[0]->next);
    TEST_ASSERT_EQUAL_PTR(items[2], items[1]->next);
    TEST_ASSERT_EQUAL_PTR(items[3], items[2]->next);
    TEST_ASSERT_NULL(items[3]->next);

    TEST_ASSERT_EQUAL_PTR(new_item, items[0]->prev);
    TEST_ASSERT_EQUAL_PTR(items[0], items[1]->prev);
    TEST_ASSERT_EQUAL_PTR(items[1], items[2]->prev);
    TEST_ASSERT_EQUAL_PTR(items[2], items[3]->prev);
    TEST_ASSERT_NULL(new_item->prev);

    cJSON_Delete(array);
}

/* Test: Insert at index 5 into array with five items */
static void test_insert_at_five_five_item_array(void) {
    cJSON *array = cJSON_CreateArray();
    cJSON *items[5];
    for (int i = 0; i < 5; i++) {
        items[i] = cJSON_CreateNumber(i);
        cJSON_AddItemToArray(array, items[i]);
    }

    cJSON *new_item = cJSON_CreateNumber(5);
    TEST_ASSERT_TRUE(cJSON_InsertItemInArray(array, 5, new_item));

    int expected[] = {0, 1, 2, 3, 4, 5};
    TEST_ASSERT_TRUE(verify_array_contents(array, expected, 6));

    TEST_ASSERT_EQUAL_PTR(items[0], array->child);
    TEST_ASSERT_EQUAL_PTR(items[1], items[0]->next);
    TEST_ASSERT_EQUAL_PTR(items[2], items[1]->next);
    TEST_ASSERT_EQUAL_PTR(items[3], items[2]->next);
    TEST_ASSERT_EQUAL_PTR(items[4], items[3]->next);
    TEST_ASSERT_EQUAL_PTR(new_item, items[4]->next);
    TEST_ASSERT_NULL(new_item->next);

    TEST_ASSERT_EQUAL_PTR(items[0], items[1]->prev);
    TEST_ASSERT_EQUAL_PTR(items[1], items[2]->prev);
    TEST_ASSERT_EQUAL_PTR(items[2], items[3]->prev);
    TEST_ASSERT_EQUAL_PTR(items[3], items[4]->prev);
    TEST_ASSERT_EQUAL_PTR(items[4], new_item->prev);
    TEST_ASSERT_NULL(items[0]->prev);

    cJSON_Delete(array);
}

/* Test: Insert at index 3 into array with five items */
static void test_insert_at_three_five_item_array(void) {
    cJSON *array = cJSON_CreateArray();
    cJSON *items[5];
    for (int i = 0; i < 5; i++) {
        items[i] = cJSON_CreateNumber(i);
        cJSON_AddItemToArray(array, items[i]);
    }

    cJSON *new_item = cJSON_CreateNumber(99);
    TEST_ASSERT_TRUE(cJSON_InsertItemInArray(array, 3, new_item));

    int expected[] = {0, 1, 2, 99, 3, 4};
    TEST_ASSERT_TRUE(verify_array_contents(array, expected, 6));

    TEST_ASSERT_EQUAL_PTR(items[0], array->child);
    TEST_ASSERT_EQUAL_PTR(items[1], items[0]->next);
    TEST_ASSERT_EQUAL_PTR(items[2], items[1]->next);
    TEST_ASSERT_EQUAL_PTR(new_item, items[2]->next);
    TEST_ASSERT_EQUAL_PTR(items[3], new_item->next);
    TEST_ASSERT_EQUAL_PTR(items[4], items[3]->next);
    TEST_ASSERT_NULL(items[4]->next);

    TEST_ASSERT_EQUAL_PTR(items[0], items[1]->prev);
    TEST_ASSERT_EQUAL_PTR(items[1], items[2]->prev);
    TEST_ASSERT_EQUAL_PTR(items[2], new_item->prev);
    TEST_ASSERT_EQUAL_PTR(new_item, items[3]->prev);
    TEST_ASSERT_EQUAL_PTR(items[3], items[4]->prev);
    TEST_ASSERT_NULL(items[0]->prev);

    cJSON_Delete(array);
}

/* Test: Insert at index 2 into array with five items */
static void test_insert_at_two_five_item_array(void) {
    cJSON *array = cJSON_CreateArray();
    cJSON *items[5];
    for (int i = 0; i < 5; i++) {
        items[i] = cJSON_CreateNumber(i);
        cJSON_AddItemToArray(array, items[i]);
    }

    cJSON *new_item = cJSON_CreateNumber(99);
    TEST_ASSERT_TRUE(cJSON_InsertItemInArray(array, 2, new_item));

    int expected[] = {0, 1, 99, 2, 3, 4};
    TEST_ASSERT_TRUE(verify_array_contents(array, expected, 6));

    TEST_ASSERT_EQUAL_PTR(items[0], array->child);
    TEST_ASSERT_EQUAL_PTR(items[1], items[0]->next);
    TEST_ASSERT_EQUAL_PTR(new_item, items[1]->next);
    TEST_ASSERT_EQUAL_PTR(items[2], new_item->next);
    TEST_ASSERT_EQUAL_PTR(items[3], items[2]->next);
    TEST_ASSERT_EQUAL_PTR(items[4], items[3]->next);
    TEST_ASSERT_NULL(items[4]->next);

    TEST_ASSERT_EQUAL_PTR(items[0], items[1]->prev);
    TEST_ASSERT_EQUAL_PTR(items[1], new_item->prev);
    TEST_ASSERT_EQUAL_PTR(new_item, items[2]->prev);
    TEST_ASSERT_EQUAL_PTR(items[2], items[3]->prev);
    TEST_ASSERT_EQUAL_PTR(items[3], items[4]->prev);
    TEST_ASSERT_NULL(items[0]->prev);

    cJSON_Delete(array);
}

/* Test: Insert at index 1 into array with five items */
static void test_insert_at_one_five_item_array(void) {
    cJSON *array = cJSON_CreateArray();
    cJSON *items[5];
    for (int i = 0; i < 5; i++) {
        items[i] = cJSON_CreateNumber(i);
        cJSON_AddItemToArray(array, items[i]);
    }

    cJSON *new_item = cJSON_CreateNumber(99);
    TEST_ASSERT_TRUE(cJSON_InsertItemInArray(array, 1, new_item));

    int expected[] = {0, 99, 1, 2, 3, 4};
    TEST_ASSERT_TRUE(verify_array_contents(array, expected, 6));

    TEST_ASSERT_EQUAL_PTR(items[0], array->child);
    TEST_ASSERT_EQUAL_PTR(new_item, items[0]->next);
    TEST_ASSERT_EQUAL_PTR(items[1], new_item->next);
    TEST_ASSERT_EQUAL_PTR(items[2], items[1]->next);
    TEST_ASSERT_EQUAL_PTR(items[3], items[2]->next);
    TEST_ASSERT_EQUAL_PTR(items[4], items[3]->next);
    TEST_ASSERT_NULL(items[4]->next);

    TEST_ASSERT_EQUAL_PTR(items[0], new_item->prev);
    TEST_ASSERT_EQUAL_PTR(new_item, items[1]->prev);
    TEST_ASSERT_EQUAL_PTR(items[1], items[2]->prev);
    TEST_ASSERT_EQUAL_PTR(items[2], items[3]->prev);
    TEST_ASSERT_EQUAL_PTR(items[3], items[4]->prev);
    TEST_ASSERT_NULL(items[0]->prev);

    cJSON_Delete(array);
}

/* Test: Insert at index 0 into array with five items */
static void test_insert_at_zero_five_item_array(void) {
    cJSON *array = cJSON_CreateArray();
    cJSON *items[5];
    for (int i = 0; i < 5; i++) {
        items[i] = cJSON_CreateNumber(i);
        cJSON_AddItemToArray(array, items[i]);
    }

    cJSON *new_item = cJSON_CreateNumber(-1);
    TEST_ASSERT_TRUE(cJSON_InsertItemInArray(array, 0, new_item));

    int expected[] = {-1, 0, 1, 2, 3, 4};
    TEST_ASSERT_TRUE(verify_array_contents(array, expected, 6));

    TEST_ASSERT_EQUAL_PTR(new_item, array->child);
    TEST_ASSERT_EQUAL_PTR(items[0], new_item->next);
    TEST_ASSERT_EQUAL_PTR(items[1], items[0]->next);
    TEST_ASSERT_EQUAL_PTR(items[2], items[1]->next);
    TEST_ASSERT_EQUAL_PTR(items[3], items[2]->next);
    TEST_ASSERT_EQUAL_PTR(items[4], items[3]->next);
    TEST_ASSERT_NULL(items[4]->next);

    TEST_ASSERT_EQUAL_PTR(new_item, items[0]->prev);
    TEST_ASSERT_EQUAL_PTR(items[0], items[1]->prev);
    TEST_ASSERT_EQUAL_PTR(items[1], items[2]->prev);
    TEST_ASSERT_EQUAL_PTR(items[2], items[3]->prev);
    TEST_ASSERT_EQUAL_PTR(items[3], items[4]->prev);
    TEST_ASSERT_NULL(new_item->prev);

    cJSON_Delete(array);
}

int main(void) {
    unity_install_sighandler();
    UNITY_BEGIN();

    RUN_TEST(test_insert_in_empty_array_at_zero_should_fail);
    RUN_TEST(test_insert_negative_index_should_fail);
    RUN_TEST(test_insert_null_item_should_fail);
    RUN_TEST(test_insert_beyond_array_size_should_append);
    RUN_TEST(test_insert_in_middle_should_shift_right);
    RUN_TEST(test_insert_at_beginning_should_update_head);
    RUN_TEST(test_insert_at_end_should_append);
    RUN_TEST(test_insert_into_single_element_array_at_zero);
    RUN_TEST(test_insert_into_single_element_array_at_one);
    RUN_TEST(test_insert_in_larger_array);
    RUN_TEST(test_insert_into_corrupted_item_should_fail);
    RUN_TEST(test_insert_at_zero_single_item_array);
    RUN_TEST(test_insert_at_one_single_item_array);
    RUN_TEST(test_insert_at_two_two_item_array);
    RUN_TEST(test_insert_at_one_two_item_array);
    RUN_TEST(test_insert_at_zero_two_item_array);
    RUN_TEST(test_insert_at_three_three_item_array);
    RUN_TEST(test_insert_at_two_three_item_array);
    RUN_TEST(test_insert_at_one_three_item_array);
    RUN_TEST(test_insert_at_zero_three_item_array);
    RUN_TEST(test_insert_at_four_four_item_array);
    RUN_TEST(test_insert_at_two_four_item_array);
    RUN_TEST(test_insert_at_one_four_item_array);
    RUN_TEST(test_insert_at_zero_four_item_array);
    RUN_TEST(test_insert_at_five_five_item_array);
    RUN_TEST(test_insert_at_three_five_item_array);
    RUN_TEST(test_insert_at_two_five_item_array);
    RUN_TEST(test_insert_at_one_five_item_array);
    RUN_TEST(test_insert_at_zero_five_item_array);

    return UNITY_END();
}