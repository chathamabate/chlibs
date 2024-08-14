#include <stdio.h>

#include "chutil/list.h"
#include "map.h"
#include "unity/unity_internals.h"
#include "unity/unity.h"

#include "chutil/debug.h"

#include "./list.h"

void setUp(void) {
    reset_malloc_count();
}

void tearDown(void) {
    TEST_ASSERT_EQUAL_UINT(0, get_malloc_count());
}


int main(void) {
    UNITY_BEGIN();
    list_tests();
    map_tests();
    return UNITY_END();
}