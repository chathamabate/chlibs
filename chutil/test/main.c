#include "unity/unity_internals.h"
#include "unity/unity.h"


#include "map.h"
#include "queue.h"
#include "list.h"
#include "heap.h"
#include "_string.h"
#include "list_helpers.h"
#include "stream.h"
#include "utf8.h"

#include "chsys/sys.h"


void setUp(void) {
    sys_reset_malloc_count();
}

void tearDown(void) {
    TEST_ASSERT_EQUAL_size_t(0, sys_get_malloc_count());
}


int main(void) {
    sys_init();

    UNITY_BEGIN();
    list_tests();
    list_helpers_tests();
    map_tests();
    queue_tests();
    heap_tests();
    string_tests(); 
    stream_tests();
    utf8_tests();
    safe_exit(UNITY_END());
}
