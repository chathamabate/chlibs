
#include "chrpc/channel.h"
#include "chrpc/channel_fd.h"
#include "chrpc/channel_local.h"
#include "chrpc/channel_helpers.h"
#include "channel.h"
#include "unity/unity.h"
#include <stdio.h>
#include <unistd.h>

static void new_channel_fd_pipe(channel_t **a2b, channel_t **b2a) {
    int a2b_pipe[2]; // Remeber, read then write.
    TEST_ASSERT_EQUAL_INT(0, pipe(a2b_pipe));

    int b2a_pipe[2];
    TEST_ASSERT_EQUAL_INT(0, pipe(b2a_pipe));
    // Wait, this expects to be able to read/write to the same fd??

    channel_fd_config_t a2b_cfg = {
        .max_msg_size = 0x100,
        .read_chunk_size = 0x80,
        .queue_depth = 1,
        .write_over = false,
        
        .read_fd = b2a_pipe[0],
        .write_fd = a2b_pipe[1]
    };

    TEST_ASSERT_TRUE(CHN_SUCCESS == new_channel(CHANNEL_FD_IMPL, a2b, &a2b_cfg));

    channel_fd_config_t b2a_cfg = {
        .max_msg_size = 0x100,
        .read_chunk_size = 0x80,
        .queue_depth = 1,
        .write_over = false,
        
        .read_fd = a2b_pipe[0],
        .write_fd = b2a_pipe[1]
    };

    TEST_ASSERT_TRUE(CHN_SUCCESS == new_channel(CHANNEL_FD_IMPL, b2a, &b2a_cfg));
}

static void test_channel_fd_echo(void) {
    channel_echo_thread_t *et;

    channel_t *a2b;
    channel_t *b2a;

    new_channel_fd_pipe(&a2b, &b2a);

    et = new_channel_echo_thread(b2a);
    TEST_ASSERT_NOT_NULL(et);

    test_chn_echo(a2b, 5);

    TEST_ASSERT_EQUAL_INT(CHN_SUCCESS, 
            delete_channel_echo_thread(et));

    delete_channel(a2b);
    delete_channel(b2a);
}

static void test_channel_fd_stressful_echo(void) {
    channel_echo_thread_t *et;

    channel_t *a2b;
    channel_t *b2a;

    new_channel_fd_pipe(&a2b, &b2a);

    et = new_channel_echo_thread(b2a);
    TEST_ASSERT_NOT_NULL(et);

    test_chn_stressful_echo(a2b, 5);

    TEST_ASSERT_EQUAL_INT(CHN_SUCCESS, 
            delete_channel_echo_thread(et));

    delete_channel(a2b);
    delete_channel(b2a);
}

void channel_fd_tests(void) {
    RUN_TEST(test_channel_fd_echo);
    RUN_TEST(test_channel_fd_stressful_echo);
}
