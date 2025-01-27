
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

static void test_channel_fd_bytewise(channel_t *chn, int write_fd) {
    size_t len;

    uint8_t recv_buf[0x100];

    uint8_t send_buf0[] = {
        CHN_FD_START_MAGIC, 
        1, 0, 0, 0,
        4
    };
    int send_buf0_size = sizeof(send_buf0) / sizeof(uint8_t);
    TEST_ASSERT_EQUAL_INT(send_buf0_size, write(write_fd, send_buf0, send_buf0_size));

    TEST_ASSERT_TRUE(CHN_SUCCESS == chn_refresh(chn));
    TEST_ASSERT_TRUE(CHN_NO_INCOMING_MSG == chn_incoming_len(chn, &len));

    uint8_t send_buf1[] = {
        CHN_FD_END_MAGIC,
        2, 3, 1, 4, // Junk Bytes...
        CHN_FD_START_MAGIC, 
        2, 0, 0, 0, 
        1, 2, 
        4, // Bad Ending magic.
        CHN_FD_START_MAGIC,
        2, 0
    };
    int send_buf1_size = sizeof(send_buf1) / sizeof(uint8_t);
    TEST_ASSERT_EQUAL_INT(send_buf1_size, write(write_fd, send_buf1, send_buf1_size));

    TEST_ASSERT_TRUE(CHN_SUCCESS == chn_refresh(chn));

    TEST_ASSERT_TRUE(CHN_SUCCESS == chn_incoming_len(chn, &len));
    TEST_ASSERT_EQUAL_size_t(1, len);
    TEST_ASSERT_TRUE(CHN_SUCCESS == chn_receive(chn, recv_buf, 0x100, &len));
    TEST_ASSERT_EQUAL_UINT8(4, recv_buf[0]);

    TEST_ASSERT_TRUE(CHN_NO_INCOMING_MSG == chn_incoming_len(chn, &len));

    uint8_t send_buf2[] = {
        0, 0, 
        2, 3,
        CHN_FD_END_MAGIC,
        0, 0, 0, 0, // Junk bytes.
        CHN_FD_START_MAGIC,  // Empty message should be ignored.
        0, 0, 0, 0, 
        CHN_FD_END_MAGIC,

        CHN_FD_START_MAGIC, // Finished length, but nothing else.
        4, 0, 0, 0
    };
    int send_buf2_size = sizeof(send_buf2) / sizeof(uint8_t);
    TEST_ASSERT_EQUAL_INT(send_buf2_size, write(write_fd, send_buf2, send_buf2_size));

    TEST_ASSERT_TRUE(CHN_SUCCESS == chn_refresh(chn));
    TEST_ASSERT_TRUE(CHN_SUCCESS == chn_receive(chn, recv_buf, 0x100, &len));

    TEST_ASSERT_EQUAL_size_t(2, len);
    TEST_ASSERT_EQUAL_UINT8(3, recv_buf[1]);
    TEST_ASSERT_TRUE(CHN_NO_INCOMING_MSG == chn_incoming_len(chn, &len));

    uint8_t send_buf3[] = {
        5, 6, 7, 8,
        CHN_FD_END_MAGIC,
        CHN_FD_START_MAGIC,
        0, 0, 0, 8, // Should be too large for given channel.
        CHN_FD_START_MAGIC
    };
    int send_buf3_size = sizeof(send_buf3) / sizeof(uint8_t);
    TEST_ASSERT_EQUAL_INT(send_buf3_size, write(write_fd, send_buf3, send_buf3_size));

    TEST_ASSERT_TRUE(CHN_SUCCESS == chn_refresh(chn));
    TEST_ASSERT_TRUE(CHN_SUCCESS == chn_receive(chn, recv_buf, 0x100, &len));
    TEST_ASSERT_EQUAL_size_t(4, len);

    uint8_t send_buf4[] = {
        1, 0, 0, 0,
        0xFF, 
        CHN_FD_END_MAGIC,
        1, 1, 1
    };
    int send_buf4_size = sizeof(send_buf4) / sizeof(uint8_t);
    TEST_ASSERT_EQUAL_INT(send_buf4_size, write(write_fd, send_buf4, send_buf4_size));

    TEST_ASSERT_TRUE(CHN_SUCCESS == chn_refresh(chn));
    TEST_ASSERT_TRUE(CHN_SUCCESS == chn_receive(chn, recv_buf, 0x100, &len));
    TEST_ASSERT_EQUAL_size_t(1, len);
}

// Byte wise tests are going to send individual bytes to the channel
// and see if can handle awkward combinations of bytes.

static void test_channel_fd_nbyte_chunks(size_t chunk_size) {
    int pipe_fds[2]; // Remeber, read then write.
    TEST_ASSERT_EQUAL_INT(0, pipe(pipe_fds));

    int read_fd = pipe_fds[0];
    int write_fd = pipe_fds[1];

    // NOTE:  for this test we give the write end of the pipe.
    // HOWEVER, we will never call send.
    // We will personally write to the write end.
    // Then call received on the channel... that's it.

    channel_fd_config_t chn_cfg = {
        .max_msg_size = 0x20,   // 32-byte max
        .read_chunk_size = chunk_size,

        // Aren't really used for bytewise testing.
        .queue_depth = 2,
        .write_over = true,
        
        .write_fd = write_fd, 
        .read_fd = read_fd,
    };

    channel_t *chn;
    TEST_ASSERT_TRUE(CHN_SUCCESS == new_channel(CHANNEL_FD_IMPL, &chn, &chn_cfg));

    test_channel_fd_bytewise(chn, write_fd);

    delete_channel(chn);
}

static void test_channel_fd_1byte_chunks(void) {
    test_channel_fd_nbyte_chunks(1);
}

static void test_channel_fd_3byte_chunks(void) {
    test_channel_fd_nbyte_chunks(3);
}

static void test_channel_fd_16byte_chunks(void) {
    test_channel_fd_nbyte_chunks(0x10);
}

static void test_channel_fd_32byte_chunks(void) {
    test_channel_fd_nbyte_chunks(0x20);
}

static void test_channel_fd_writeover(void) {
    int pipe_fds[2]; // Remeber, read then write.
    TEST_ASSERT_EQUAL_INT(0, pipe(pipe_fds));

    int read_fd = pipe_fds[0];
    int write_fd = pipe_fds[1];

    channel_fd_config_t chn_cfg = {
        .max_msg_size = 0x10,   // 16-byte max
        .read_chunk_size = 0x20, // 32-byte read chunks.

        .queue_depth = 2,
        .write_over = true,
        
        .write_fd = write_fd, 
        .read_fd = read_fd,
    };

    channel_t *chn;
    TEST_ASSERT_TRUE(CHN_SUCCESS == new_channel(CHANNEL_FD_IMPL, &chn, &chn_cfg));

    uint8_t recv_buf[0x100];
    uint8_t send_buf[] = {
        CHN_FD_START_MAGIC,
        1, 0, 0, 0,
        1,
        CHN_FD_END_MAGIC,
        CHN_FD_START_MAGIC,
        2, 0, 0, 0,
        2, 3,
        CHN_FD_END_MAGIC,
        CHN_FD_START_MAGIC,
        3, 0, 0, 0,
        4, 5, 6,
        CHN_FD_END_MAGIC,
    };
    int send_buf_size = sizeof(send_buf) / sizeof(uint8_t);
    
    TEST_ASSERT_EQUAL_INT(send_buf_size, write(write_fd, send_buf, send_buf_size));
    TEST_ASSERT_TRUE(CHN_SUCCESS == chn_refresh(chn));

    size_t len;
    TEST_ASSERT_TRUE(CHN_SUCCESS == chn_receive(chn, recv_buf, 0x100, &len));
    TEST_ASSERT_EQUAL_size_t(2, len);
    TEST_ASSERT_EQUAL_UINT8(3, recv_buf[1]);

    TEST_ASSERT_TRUE(CHN_SUCCESS == chn_receive(chn, recv_buf, 0x100, &len));
    TEST_ASSERT_EQUAL_size_t(3, len);
    TEST_ASSERT_EQUAL_UINT8(6, recv_buf[2]);

    TEST_ASSERT_TRUE(CHN_NO_INCOMING_MSG == chn_receive(chn, recv_buf, 0x100, &len));
    delete_channel(chn);
}

static void test_channel_fd_no_writeover(void) {
    int pipe_fds[2]; // Remeber, read then write.
    TEST_ASSERT_EQUAL_INT(0, pipe(pipe_fds));

    int read_fd = pipe_fds[0];
    int write_fd = pipe_fds[1];

    channel_fd_config_t chn_cfg = {
        .max_msg_size = 0x10,   // 16-byte max
        .read_chunk_size = 0x20, // 32-byte read chunks.

        .queue_depth = 1,
        .write_over = false,
        
        .write_fd = write_fd, 
        .read_fd = read_fd,
    };

    channel_t *chn;
    TEST_ASSERT_TRUE(CHN_SUCCESS == new_channel(CHANNEL_FD_IMPL, &chn, &chn_cfg));

    uint8_t recv_buf[0x100];
    uint8_t send_buf[] = {
        CHN_FD_START_MAGIC,
        1, 0, 0, 0,
        1,
        CHN_FD_END_MAGIC,
        CHN_FD_START_MAGIC,
        2, 0, 0, 0,
        2, 3,
        CHN_FD_END_MAGIC
    };
    int send_buf_size = sizeof(send_buf) / sizeof(uint8_t);
    
    TEST_ASSERT_EQUAL_INT(send_buf_size, write(write_fd, send_buf, send_buf_size));
    TEST_ASSERT_TRUE(CHN_SUCCESS == chn_refresh(chn));

    size_t len;
    TEST_ASSERT_TRUE(CHN_SUCCESS == chn_receive(chn, recv_buf, 0x100, &len));
    TEST_ASSERT_EQUAL_size_t(1, len);
    TEST_ASSERT_EQUAL_UINT8(1, recv_buf[0]);

    TEST_ASSERT_TRUE(CHN_NO_INCOMING_MSG == chn_receive(chn, recv_buf, 0x100, &len));
    delete_channel(chn);
}

void channel_fd_tests(void) {
    RUN_TEST(test_channel_fd_echo);
    RUN_TEST(test_channel_fd_stressful_echo);

    RUN_TEST(test_channel_fd_1byte_chunks);
    RUN_TEST(test_channel_fd_3byte_chunks);
    RUN_TEST(test_channel_fd_16byte_chunks);
    RUN_TEST(test_channel_fd_32byte_chunks);

    RUN_TEST(test_channel_fd_writeover);
    RUN_TEST(test_channel_fd_no_writeover);
}
