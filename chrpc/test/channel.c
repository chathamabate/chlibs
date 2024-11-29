
#include <unistd.h>
#include <stdbool.h>

#include "chrpc/channel.h"
#include "channel.h"

#include "chsys/mem.h"
#include "unity/unity.h"
#include "unity/unity_internals.h"

static void test_chn_echo_single(channel_t *chn, const void *buf, size_t buf_len, 
        uint32_t wait_amt) {
    channel_status_t status;

    TEST_ASSERT_EQUAL_INT(CHN_SUCCESS, chn_send(chn, buf, buf_len));

    size_t act_len;
    while (true) {
        status = chn_incoming_len(chn, &act_len);        

        if (status == CHN_SUCCESS) {
            break;
        }
        
        TEST_ASSERT_EQUAL_INT(CHN_NO_INCOMING_MSG, status);

        if (wait_amt > 0) {
            usleep(wait_amt);
        }

        TEST_ASSERT_EQUAL_INT(CHN_SUCCESS, chn_refresh(chn));
    }

    TEST_ASSERT_EQUAL_size_t(buf_len, act_len);

    void *recv_buf = safe_malloc(act_len * sizeof(uint8_t));

    size_t readden;
    TEST_ASSERT_EQUAL_INT(CHN_SUCCESS, chn_receive(chn, recv_buf, act_len, &readden));
    TEST_ASSERT_EQUAL_size_t(act_len, readden);

    TEST_ASSERT_EQUAL_UINT8_ARRAY(buf, recv_buf, buf_len);

    safe_free(recv_buf);
}

static void test_chn_echo_gen_single(channel_t *chn, size_t len, uint32_t wait_amt) {
    size_t mms; 

    TEST_ASSERT_EQUAL_INT(CHN_SUCCESS, 
            chn_max_msg_size(chn, &mms));

    // Skip test if the given length is too large.
    if (len > mms || len == 0) {
        return;
    }

    uint8_t *buf = (uint8_t *)safe_malloc(len * sizeof(uint8_t));
    for (size_t i = 0; i < len; i++) {
        buf[i] = (uint8_t)i;
    }

    test_chn_echo_single(chn, buf, len, wait_amt);

    safe_free(buf);
}


// This should do a series of back and forths.
void test_chn_echo(channel_t *chn, uint32_t wait_amt) {
    test_chn_echo_gen_single(chn, 1, wait_amt);
    test_chn_echo_gen_single(chn, 25, wait_amt);
    test_chn_echo_gen_single(chn, 16, wait_amt);
    test_chn_echo_gen_single(chn, 200, wait_amt);
    test_chn_echo_gen_single(chn, 256, wait_amt);

    size_t mms;
    TEST_ASSERT_EQUAL_INT(CHN_SUCCESS, 
            chn_max_msg_size(chn, &mms));

    test_chn_echo_gen_single(chn, mms - 16, wait_amt);
    test_chn_echo_gen_single(chn, mms - 1, wait_amt);
    test_chn_echo_gen_single(chn, mms, wait_amt);
}

void test_chn_stressful_echo(channel_t *chn, uint32_t wait_amt) {
    const size_t TRIALS = 20;

    size_t mms;
    TEST_ASSERT_EQUAL_INT(CHN_SUCCESS, 
            chn_max_msg_size(chn, &mms));

    // We will generate our own message here just to add
    // another difference between this test and the normal echo test.
    uint8_t *buf = (uint8_t *)safe_malloc(mms * sizeof(uint8_t));    
    for (size_t i = 0; i < mms; i++) {
        buf[i] = (i % 3) * 27; 
    }

    for (size_t i = 0; i < TRIALS; i++) {
        test_chn_echo_single(chn, buf, mms, wait_amt);
    }

    safe_free(buf);
}
