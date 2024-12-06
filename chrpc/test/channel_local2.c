
#include "chrpc/channel.h"
#include "chrpc/channel_helpers.h"

#include "channel.h"
#include "channel_local2.h"
#include "chrpc/channel_local.h"
#include "chrpc/channel_local2.h"

#include "unity/unity.h"

#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>


static void new_channel_local2_pipe(channel_local2_core_t **core, 
        channel_t **a2b, channel_t **b2a) {
    channel_local2_core_t *c;
    const channel_local_config_t cfg = {
        .max_msg_size = 0x100,
        .queue_depth = 1,
        .write_over = false
    };

    TEST_ASSERT_EQUAL_INT(CHN_SUCCESS, 
        new_channel_local2_core(&c, &cfg));

    *core = c;

    channel_local2_config_t cfg_a2b = {
        .a2b_direction = true, 
        .core = c,
    };
    TEST_ASSERT_EQUAL_INT(CHN_SUCCESS,
            new_channel(CHANNEL_LOCAL2_IMPL, a2b, (void *)&cfg_a2b));

    channel_local2_config_t cfg_b2a = {
        .a2b_direction = false, 
        .core = c,
    };
    TEST_ASSERT_EQUAL_INT(CHN_SUCCESS,
            new_channel(CHANNEL_LOCAL2_IMPL, b2a, (void *)&cfg_b2a));
}

static void delete_channel_local2_pipe(channel_local2_core_t *core, 
        channel_t *a2b, channel_t *b2a) {
    TEST_ASSERT_EQUAL_INT(CHN_SUCCESS,
            delete_channel(a2b));
    TEST_ASSERT_EQUAL_INT(CHN_SUCCESS,
            delete_channel(b2a));
    TEST_ASSERT_EQUAL_INT(CHN_SUCCESS,
            delete_channel_local2_core(core));
}

static void test_channel_local2_echo(void) {
    channel_local2_core_t *core;
    channel_t *a2b;
    channel_t *b2a;
    channel_echo_thread_t *et;

    new_channel_local2_pipe(&core, &a2b, &b2a);
    
    et = new_channel_echo_thread(b2a);
    TEST_ASSERT_NOT_NULL(et);

    test_chn_echo(a2b, 5);

    TEST_ASSERT_EQUAL_INT(CHN_SUCCESS, 
            delete_channel_echo_thread(et));

    delete_channel_local2_pipe(core, a2b, b2a);
}

static void test_channel_local2_stressful_echo(void) {
    channel_local2_core_t *core;
    channel_t *a2b;
    channel_t *b2a;
    channel_echo_thread_t *et;

    new_channel_local2_pipe(&core, &a2b, &b2a);
    
    et = new_channel_echo_thread(b2a);
    TEST_ASSERT_NOT_NULL(et);

    test_chn_stressful_echo(a2b, 5);

    TEST_ASSERT_EQUAL_INT(CHN_SUCCESS, 
            delete_channel_echo_thread(et));

    delete_channel_local2_pipe(core, a2b, b2a);
}

void channel_local2_tests(void) {
    RUN_TEST(test_channel_local2_echo);
    RUN_TEST(test_channel_local2_stressful_echo);
}
