
#include <stdio.h>
#include "chsys/sys.h"

#include "unity/unity.h"
#include "unity/unity_internals.h"

#include "channel_local.h"

void setUp(void) {
    sys_reset_malloc_count();
}

void tearDown(void) {
    TEST_ASSERT_EQUAL_size_t(0, sys_get_malloc_count());
}

int main(void) {
    sys_init();

    UNITY_BEGIN();
    RUN_TEST(channel_local_tests);
    safe_exit(UNITY_END());
}
