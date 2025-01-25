
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
    int fd;

    bool write_over;
    size_t max_msg_size;
} channel_fd_config_t;

typedef struct _channel_fd_t {
    channel_fd_config_t cfg;

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
