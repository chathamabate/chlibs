
#include "chrpc/channel_fd.h"
#include "chrpc/channel.h"
#include "chrpc/serial_type.h"
#include "chsys/mem.h"
#include <pthread.h>
#include <unistd.h>
#include <poll.h>

channel_status_t new_channel_fd(channel_fd_t **chn_fd, const channel_fd_config_t *cfg) {
    if (!cfg) {
        return CHN_INVALID_ARGS;
    }

    if (cfg->max_msg_size == 0 || cfg->queue_depth == 0 || cfg->fd < 0) {
        return CHN_INVALID_ARGS;
    }

    channel_fd_t *chn = (channel_fd_t *)safe_malloc(sizeof(channel_fd_t));
    chn->cfg = *cfg;  

    if (pthread_mutex_init(&(chn->mut), NULL)) {
        safe_free(chn);
        return CHN_UNKNOWN_ERROR;
    }

    chn->q = new_queue(cfg->queue_depth, sizeof(channel_msg_t));
    *chn_fd = chn;

    return CHN_SUCCESS;
}

channel_status_t delete_channel_fd(channel_fd_t *chn_fd) {
    channel_msg_t msg;
    while (q_poll(chn_fd->q, &msg) == 0) {
        safe_free(msg.msg_buf);
    }

    safe_free(chn_fd->q);
    pthread_mutex_destroy(&(chn_fd->mut));
    close(chn_fd->cfg.fd);

    return CHN_SUCCESS;
}

channel_status_t chn_fd_max_msg_size(channel_fd_t *chn_fd, size_t *mms) {
    *mms = chn_fd->cfg.max_msg_size;
    return CHN_SUCCESS;
}

channel_status_t chn_fd_send(channel_fd_t *chn_fd, const void *msg, size_t len) {
    channel_status_t status = CHN_SUCCESS;

    pthread_mutex_lock(&(chn_fd->mut));

    struct pollfd pfd;

    pfd.fd = chn_fd->cfg.fd;
    pfd.events = POLLOUT;
    int result = poll(&pfd, 1, 0);

    if (result == 0 || (result > 0 && !(pfd.revents & POLLOUT))) {
        status = CHN_CANNOT_WRITE;
        goto end;
    }

    if (result < 0) {
        status = CHN_UNKNOWN_ERROR;
        goto end;
    }

    // result > 0 AND POLLOUT is true!
    // We can write!

    // How should we do this??
    // Send one big chunk??
    // Receive one big chunk??
    // Honestly not as simple as I would like here...

end:
    pthread_mutex_unlock(&(chn_fd->mut));
    return status;
}

channel_status_t chn_fd_refresh(channel_fd_t *chn_fd) {
    return CHN_UNKNOWN_ERROR;
}

channel_status_t chn_fd_incoming_len(channel_fd_t *chn_fd, size_t *len) {
    channel_status_t return_status = CHN_SUCCESS;

    pthread_mutex_lock(&(chn_fd->mut));

    if (q_len(chn_fd->q) == 0) {
        return_status = CHN_NO_INCOMING_MSG;
    } else {
        const channel_msg_t *msg = (const channel_msg_t *)q_peek(chn_fd->q);
        *len = msg->msg_size;
    }

    pthread_mutex_unlock(&(chn_fd->mut));

    return return_status;
}

channel_status_t chn_fd_receive(channel_fd_t *chn_fd, void *buf, size_t len, size_t *readden) {
    return CHN_UNKNOWN_ERROR;
}

