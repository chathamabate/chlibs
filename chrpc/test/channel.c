
#include <unistd.h>
#include <time.h>
#include <stdbool.h>

#include "chrpc/channel.h"
#include "channel.h"

#include "chsys/mem.h"
#include "unity/unity.h"

void expect_chn_receive(channel_t *chn, const void *exp_buf, size_t exp_len, uint32_t tries) {
    TEST_ASSERT(tries > 0);

    channel_status_t status;

    size_t act_len;

    uint32_t trial = 0;

    const struct timespec slp_amt = {
        .tv_sec = 0,
        .tv_nsec = 10000000 // Should be 10 ms
    };

    // Somewhat unused.
    struct timespec rem;

    while (true) {
        status = chn_incoming_len(chn, &act_len);        

        if (status == CHN_SUCCESS) {
            break;
        }
        
        TEST_ASSERT_EQUAL_INT(CHN_NO_INCOMING_MSG, status);

        nanosleep(&slp_amt, &rem);

        TEST_ASSERT_EQUAL_INT(CHN_SUCCESS, chn_refresh(chn));

        trial++;
        if (trial >= tries) {
            TEST_FAIL_MESSAGE("No message was received.");
        }
    }

    TEST_ASSERT_EQUAL_size_t(exp_len, act_len);

    void *recv_buf = safe_malloc(act_len * sizeof(uint8_t));

    size_t readden;
    TEST_ASSERT_EQUAL_INT(CHN_SUCCESS, chn_receive(chn, recv_buf, act_len, &readden));
    TEST_ASSERT_EQUAL_size_t(act_len, readden);

    TEST_ASSERT_EQUAL_UINT8_ARRAY(exp_buf, recv_buf, exp_len);

    safe_free(recv_buf);
}

static void test_chn_echo_single(channel_t *chn, const void *buf, size_t buf_len, 
        uint32_t wait_amt, uint32_t tries) {
    TEST_ASSERT_EQUAL_INT(CHN_SUCCESS, chn_send(chn, buf, buf_len));
    expect_chn_receive(chn, buf, buf_len, tries);
}

static void test_chn_echo_gen_single(channel_t *chn, size_t len, 
        uint32_t wait_amt, uint32_t tries) {
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

    test_chn_echo_single(chn, buf, len, wait_amt, tries);

    safe_free(buf);
}


// This should do a series of back and forths.
void test_chn_echo(channel_t *chn, uint32_t wait_amt, uint32_t tries) {
    test_chn_echo_gen_single(chn, 1, wait_amt, tries);
    test_chn_echo_gen_single(chn, 25, wait_amt, tries);
    test_chn_echo_gen_single(chn, 16, wait_amt, tries);
    test_chn_echo_gen_single(chn, 200, wait_amt, tries);
    test_chn_echo_gen_single(chn, 256, wait_amt, tries);

    size_t mms;
    TEST_ASSERT_EQUAL_INT(CHN_SUCCESS, 
            chn_max_msg_size(chn, &mms));

    test_chn_echo_gen_single(chn, mms - 16, wait_amt, tries);
    test_chn_echo_gen_single(chn, mms - 1, wait_amt, tries);
    test_chn_echo_gen_single(chn, mms, wait_amt, tries);
}

void test_chn_stressful_echo(channel_t *chn, uint32_t wait_amt, uint32_t tries) {
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
        test_chn_echo_single(chn, buf, mms, wait_amt, tries);
    }

    safe_free(buf);
}
