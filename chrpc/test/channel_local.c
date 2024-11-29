
#include "chrpc/channel.h"
#include "chrpc/channel_local.h"

#include "channel_local.h"
#include "channel.h"
#include "unity/unity.h"

void channel_local_tests(void) {
    const channel_local_config_t cfg = {
        .queue_depth = 1,
        .max_msg_size = 0x100,
        .write_over = false,
    };

    channel_t *chn;

    TEST_ASSERT_EQUAL_INT(CHN_SUCCESS, 
            new_channel(CHANNEL_LOCAL_IMPL, &chn, (void *)&cfg));

    test_chn_echo(chn); 

    TEST_ASSERT_EQUAL_INT(CHN_SUCCESS, 
            delete_channel(chn));
}
