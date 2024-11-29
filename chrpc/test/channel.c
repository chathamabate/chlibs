
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

void test_chn_echo(channel_t *chn) {
    test_chn_echo_single(chn, (const void *)"Hello", 5, 100);
}
