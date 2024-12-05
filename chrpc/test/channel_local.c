
#include "chrpc/channel.h"
#include "chrpc/channel_local.h"

#include "channel_local.h"
#include "channel.h"
#include "unity/unity.h"
#include "unity/unity_internals.h"
#include <string.h>

// Generic Channel Tests.

static void test_channel_local_echo(void) {
    const channel_local_config_t cfg = {
        .queue_depth = 1,
        .max_msg_size = 0x100,
        .write_over = false,
    };

    channel_t *chn;

    TEST_ASSERT_EQUAL_INT(CHN_SUCCESS, 
            new_channel(CHANNEL_LOCAL_IMPL, &chn, (void *)&cfg));

    test_chn_echo(chn, 100, 5); 

    TEST_ASSERT_EQUAL_INT(CHN_SUCCESS, 
            delete_channel(chn));
}

static void test_channel_local_stressful_echo(void) {
    const channel_local_config_t cfg = {
        .queue_depth = 1,
        .max_msg_size = 0x100,
        .write_over = false,
    };

    channel_t *chn;

    TEST_ASSERT_EQUAL_INT(CHN_SUCCESS, 
            new_channel(CHANNEL_LOCAL_IMPL, &chn, (void *)&cfg));

    test_chn_stressful_echo(chn, 100, 5);

    TEST_ASSERT_EQUAL_INT(CHN_SUCCESS, 
            delete_channel(chn));
}

// Local Specific Channel Tests.

static void test_channel_local_write_over(void) {
    // A few test messages and their lengths...
    const char *msgs[] = {
        "HELLO",
        "GoodBye",
        " ",
        "Aye Yo",
        "Hmmmmmmmmmm"
    };
    size_t msg_lens[sizeof(msgs) / sizeof(const char *)];
    const size_t num_msgs = sizeof(msgs) / sizeof(const char *);
    for (size_t i = 0; i < num_msgs; i++) {
        msg_lens[i] = strlen(msgs[i]) + 1;
    }

    // Now for the actual testing.

    const channel_local_config_t cfg = {
        .queue_depth = 3,
        .max_msg_size = 0x100,
        .write_over = true,
    };

    channel_t *chn;

    TEST_ASSERT_EQUAL_INT(CHN_SUCCESS, 
            new_channel(CHANNEL_LOCAL_IMPL, &chn, (void *)&cfg));

    TEST_ASSERT_EQUAL_INT(CHN_SUCCESS, 
            chn_send(chn, msgs[0], msg_lens[0]));
    TEST_ASSERT_EQUAL_INT(CHN_SUCCESS, 
            chn_send(chn, msgs[1], msg_lens[1]));

    expect_chn_receive(chn, msgs[0], msg_lens[0], 5);
    expect_chn_receive(chn, msgs[1], msg_lens[1], 5);

    // Now write all the messages.
    for (size_t i = 0; i < num_msgs; i++)  {
        TEST_ASSERT_EQUAL_INT(CHN_SUCCESS, 
                chn_send(chn, msgs[i], msg_lens[i]));
    }

    for (size_t i = 0; i < cfg.queue_depth; i++) {
        size_t exp_msg_index = num_msgs - cfg.queue_depth + i;
        expect_chn_receive(chn, msgs[exp_msg_index], msg_lens[exp_msg_index], 5);
    }

    size_t dummy_len;
    TEST_ASSERT_EQUAL_INT(CHN_NO_INCOMING_MSG, chn_incoming_len(chn, &dummy_len));

    TEST_ASSERT_EQUAL_INT(CHN_SUCCESS, 
            delete_channel(chn));
}

static void test_channel_local_no_write_over(void) {
    const channel_local_config_t cfg = {
        .queue_depth = 3,
        .max_msg_size = 0x100,
        .write_over = false,
    };

    channel_t *chn;

    TEST_ASSERT_EQUAL_INT(CHN_SUCCESS, 
            new_channel(CHANNEL_LOCAL_IMPL, &chn, (void *)&cfg));

    const char *msg = "AAA";
    const size_t msg_len = strlen(msg) + 1;
    
    for (size_t i = 0; i < cfg.queue_depth; i++) {
        TEST_ASSERT_EQUAL_INT(CHN_SUCCESS,
                chn_send(chn, msg, msg_len));
    }

    TEST_ASSERT_EQUAL_INT(CHN_CHANNEL_FULL, 
            chn_send(chn, "BBB", 4));

    for (size_t i = 0; i < cfg.queue_depth; i++) {
        expect_chn_receive(chn, msg, msg_len, 5);
    }

    TEST_ASSERT_EQUAL_INT(CHN_SUCCESS, 
            delete_channel(chn));
}

static void test_channel_local_destructor(void) {
    const channel_local_config_t cfg = {
        .queue_depth = 3,
        .max_msg_size = 0x100,
        .write_over = true,
    };

    channel_t *chn;

    TEST_ASSERT_EQUAL_INT(CHN_SUCCESS, 
            new_channel(CHANNEL_LOCAL_IMPL, &chn, (void *)&cfg));

    TEST_ASSERT_EQUAL_INT(CHN_SUCCESS,
            chn_send(chn, "ABC", 4));
    TEST_ASSERT_EQUAL_INT(CHN_SUCCESS,
            chn_send(chn, "ABC", 4));

    TEST_ASSERT_EQUAL_INT(CHN_SUCCESS, 
            delete_channel(chn));
}

void channel_local_tests(void) {
    RUN_TEST(test_channel_local_echo);
    RUN_TEST(test_channel_local_stressful_echo);
    RUN_TEST(test_channel_local_write_over);
    RUN_TEST(test_channel_local_no_write_over);
    RUN_TEST(test_channel_local_destructor);
}


