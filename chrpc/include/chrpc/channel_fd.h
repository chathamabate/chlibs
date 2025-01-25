
#ifndef CHRPC_CHANNEL_FD_H
#define CHRPC_CHANNEL_FD_H

#include "chrpc/channel.h"

#include <pthread.h>
#include "chutil/queue.h"
#include <stdbool.h>

typedef struct _channel_fd_config_t {
    size_t queue_depth;

    // NOTE: The given file descriptor will be OWNED by the created channel.
    // That is, it will be CLOSED when the channel is destroyed.
    //
    // NOTE: Very Important: Unlike some other code of I have in this project,
    // if the constructor fails, this file descriptor will be closed.
    int fd;

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

    // TODO:
    // size_t curr_msg_len;
    // uint8_t *curr_msg;

    //uint8_t *read_buf;

    // Write will be blocking.
    int write_fd;
    uint8_t *write_buf;

    pthread_mutex_t mut;
    queue_t *q;
} channel_fd_t;

channel_status_t new_channel_fd(channel_fd_t **chn_fd, const channel_fd_config_t *cfg);
channel_status_t delete_channel_fd(channel_fd_t *chn_fd);

channel_status_t chn_fd_max_msg_size(channel_fd_t *chn_fd, size_t *mms);
channel_status_t chn_fd_send(channel_fd_t *chn_fd, const void *msg, size_t len);
channel_status_t chn_fd_refresh(channel_fd_t *chn_fd);
channel_status_t chn_fd_incoming_len(channel_fd_t *chn_fd, size_t *len);
channel_status_t chn_fd_receive(channel_fd_t *chn_fd, void *buf, size_t len, size_t *readden); 

#endif
