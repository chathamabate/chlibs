
#include "chrpc/channel.h"

#include "channel.h"
#include "channel_local2.h"
#include "chrpc/channel_local.h"
#include "chrpc/channel_local2.h"
#include "chsys/mem.h"

#include "unity/unity.h"
#include "unity/unity_internals.h"

#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>

// It'd be nice if there we some generic way we could make an echo thread???
// There probably is a nice way tbh??
// Maybe even a synchronous way??
// Nah.....

typedef struct _test_thread_arg_t {
    pthread_mutex_t mut;
    bool should_stop;
    
    channel_t *chn;
} test_thread_arg_t;

static void echo_thread(void *_arg) {
    test_thread_arg_t *arg = (test_thread_arg_t *)_arg;

    channel_status_t status;

    size_t mms;
    status = chn_max_msg_size(arg->chn, &mms);
    if (status != CHN_SUCCESS) {
        pthread_exit((void *)status);
    }

    void *buf = safe_malloc(mms * sizeof(uint8_t));

    bool should_stop;
    size_t readden;

    while (true) {
        pthread_mutex_lock(&(arg->mut));
        should_stop = arg->should_stop;
        pthread_mutex_unlock(&(arg->mut));

        if (should_stop) {
            status = CHN_SUCCESS;
            break;
        }

        status = chn_receive(arg->chn, buf, mms, &readden);

        if (status == CHN_SUCCESS) {
            // echo time!
            status = chn_send(arg->chn, buf, readden);
            if (status != CHN_SUCCESS) {
                break;
            }
        } else if (status == CHN_NO_INCOMING_MSG) {
            // Time to sleep and refresh!
            usleep(50);
            status = chn_refresh(arg->chn);
            if (status != CHN_SUCCESS) {
                break;
            }
        } else {
            // Return unexpected receive error.
            break;
        }
    }

    safe_free(buf);
    pthread_exit((void *)status);
}

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
    pthread_t thread;

    channel_local2_core_t *core;
    channel_t *a2b;
    channel_t *b2a;

    new_channel_local2_pipe(&core, &a2b, &b2a);

    // UGH!!! This is annoying!!!!!! 


    delete_channel_local2_pipe(core, a2b, b2a);
}

static void test_channel_local2_stressful_echo(void) {

}


void channel_local2_tests(void) {

}
