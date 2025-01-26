
#include "channel_local2.h"
#include "chrpc/serial_type.h"
#include "chrpc/serial_value.h"
#include "chsys/sys.h"

#include "endpoints.h"
#include "serial_type.h"
#include "serial_value.h"
#include "simple_rpc.h"
#include "unity/unity.h"
#include "unity/unity_internals.h"

#include "channel_local.h"
#include "channel_fd.h"
#include "chutil/misc.h"
#include "chrpc/serial_helpers.h"

void setUp(void) {
    sys_reset_malloc_count();
}

void tearDown(void) {
    TEST_ASSERT_EQUAL_size_t(0, sys_get_malloc_count());
}

int main(void) {
    sys_init();

    UNITY_BEGIN();
    channel_local_tests();
    channel_local2_tests();
    channel_fd_tests();
    chrpc_serial_type_tests();
    chrpc_serial_value_tests();
    chrpc_rpc_endpoint_set_tests();
    chrpc_simple_rpc_tests();
    safe_exit(UNITY_END());

}
