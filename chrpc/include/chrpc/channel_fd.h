
#ifndef CHRPC_CHANNEL_FD_H
#define CHRPC_CHANNEL_FD_H

#include "chrpc/channel.h"

#include <pthread.h>
#include "chutil/queue.h"
#include <stdbool.h>

// A channel will send a formatted message across the given file descriptor.
//
// 1-Byte Start Magic Value
// 4-Byte Unsigned Length
// ... Data ... (Length Bytes)
// 1-Byte End Magic Value
//
// NOTE: I originally had this as 4 byte magic values, but this was extremely
// annoying to implement IMO.

#define CHN_FD_START_MAGIC 0x1E
#define CHN_FD_END_MAGIC   0x2A

#define CHN_FD_MSG_SIZE(len) \
    (sizeof(uint8_t) + sizeof(uint32_t) + (len) + sizeof(uint8_t))


typedef struct _channel_fd_config_t {
    size_t queue_depth;

    // NOTE: the created channel will OWN both file descriptors.
    // Even if the constructor fails both descriptors will be closed.
    //
    // If you'd like to read/write using the same descriptor, set read_fd to -1.
    // Write fd will be dup'd.
    int write_fd;
    int read_fd;

    bool write_over;
    size_t max_msg_size;

    // How much we should read in every go, this CAN be larger than max_msg_size.
    size_t read_chunk_size;
} channel_fd_config_t;

typedef struct _channel_fd_t {
    channel_fd_config_t cfg;

    // Read will be non-blocking.
    int read_fd;
    uint8_t *read_chunk;

    size_t msg_buf_fill;
    uint8_t *msg_buf;

    // Write will be blocking.
    int write_fd;
    uint8_t *write_buf;

    pthread_mutex_t mut;
    queue_t *q;
} channel_fd_t;

extern const channel_impl_t * const CHANNEL_FD_IMPL;

channel_status_t new_channel_fd(channel_fd_t **chn_fd, const channel_fd_config_t *cfg);
channel_status_t delete_channel_fd(channel_fd_t *chn_fd);

channel_status_t chn_fd_max_msg_size(channel_fd_t *chn_fd, size_t *mms);
channel_status_t chn_fd_send(channel_fd_t *chn_fd, const void *msg, size_t len);
channel_status_t chn_fd_refresh(channel_fd_t *chn_fd);
channel_status_t chn_fd_incoming_len(channel_fd_t *chn_fd, size_t *len);
channel_status_t chn_fd_receive(channel_fd_t *chn_fd, void *buf, size_t len, size_t *readden); 

#endif
