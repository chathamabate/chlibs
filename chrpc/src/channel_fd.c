
#include "chrpc/channel_fd.h"
#include "chrpc/channel.h"
#include "chrpc/serial_type.h"
#include "chsys/mem.h"
#include "chutil/queue.h"
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <poll.h>
#include <string.h>
#include <fcntl.h>

// A channel will send a formatted message across the given file descriptor.
//
// 4-Byte Start Magic Value
// 4-Byte Unsigned Length
// ... Data ... (Length Bytes)
// 4-Byte End Magic Value

#define CHN_FD_START_MAGIC 0x1234C0DE
#define CHN_FD_END_MAGIC   0xEF01C0DE

#define CHN_FD_MSG_SIZE(len) \
    ((2 * sizeof(uint32_t)) + (len) + sizeof(uint32_t))


static int setup_fds(int write_fd, int read_fd) {
    int flags;

    flags = fcntl(write_fd, F_GETFL, 0);
    if (flags == -1) {
        return -1;
    }

    // Set write fd in blocking mode.
    if (fcntl(write_fd, F_SETFL, flags & ~O_NONBLOCK) == -1) {
        return -1;
    }

    flags = fcntl(read_fd, F_GETFL, 0);
    if (flags == -1) {
        return -1;
    }
    
    // Set read fd in non-blocking mode.
    if (fcntl(read_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        return -1;
    }

    return 0;
}

channel_status_t new_channel_fd(channel_fd_t **chn_fd, const channel_fd_config_t *cfg) {
    if (!cfg) {
        return CHN_INVALID_ARGS;
    }

    if (cfg->max_msg_size == 0 || cfg->queue_depth == 0 || 
            cfg->fd < 0 || cfg->read_chunk_size == 0) {
        return CHN_INVALID_ARGS;
    }

    channel_fd_t *chn = (channel_fd_t *)safe_malloc(sizeof(channel_fd_t));
    if (pthread_mutex_init(&(chn->mut), NULL)) {
        safe_free(chn);
        return CHN_UNKNOWN_ERROR;
    }
    
    chn->cfg = *cfg;  
    chn->write_fd = cfg->fd;
    chn->read_fd = -1;
    chn->read_chunk = (uint8_t *)safe_malloc(cfg->read_chunk_size);
    chn->write_buf = (uint8_t *)safe_malloc(CHN_FD_MSG_SIZE(cfg->max_msg_size));
    chn->q = new_queue(cfg->queue_depth, sizeof(channel_msg_t));

    int flags;

    int read_fd = dup(cfg->fd);
    if (read_fd < 0) {
        delete_channel_fd(chn);
        return CHN_UNKNOWN_ERROR;
    }
    chn->read_fd = read_fd;

    if (setup_fds(chn->write_fd, chn->read_fd) < 0) {
        delete_channel_fd(chn);
        return CHN_UNKNOWN_ERROR;
    }

    *chn_fd = chn;

    return CHN_SUCCESS;
}

channel_status_t delete_channel_fd(channel_fd_t *chn_fd) {
    if (chn_fd->read_fd >= 0) {
        close(chn_fd->read_fd);
    }

    if (chn_fd->write_fd >= 0) {
        close(chn_fd->write_fd);
    }

    channel_msg_t msg;
    while (q_poll(chn_fd->q, &msg) == 0) {
        safe_free(msg.msg_buf);
    }

    delete_queue(chn_fd->q);
    safe_free(chn_fd->read_chunk);
    safe_free(chn_fd->write_buf);

    pthread_mutex_destroy(&(chn_fd->mut));

    return CHN_SUCCESS;
}

channel_status_t chn_fd_max_msg_size(channel_fd_t *chn_fd, size_t *mms) {
    *mms = chn_fd->cfg.max_msg_size;
    return CHN_SUCCESS;
}


channel_status_t chn_fd_send(channel_fd_t *chn_fd, const void *msg, size_t len) {
    if (len == 0 || chn_fd->cfg.max_msg_size < len) {
        return CHN_INVALID_MSG_SIZE;
    }

    channel_status_t status = CHN_SUCCESS;

    pthread_mutex_lock(&(chn_fd->mut));

    if (chn_fd->write_fd < 0) {
        status = CHN_CANNOT_WRITE;
        goto end;
    }

    size_t write_size = CHN_FD_MSG_SIZE(len);

    *(uint32_t *)(chn_fd->write_buf) = CHN_FD_START_MAGIC;
    *((uint32_t *)(chn_fd->write_buf) + 1) = len;
    memcpy(chn_fd->write_buf + (2 * sizeof(uint32_t)), msg, len);
    *(uint32_t *)(chn_fd->write_buf + (2 * sizeof(uint32_t)) + len) = CHN_FD_END_MAGIC;

    // NOTE: A more rigorous implementation would write again if a partial write occurred.
    // In this case, any write which was not complete will close the write file descriptor.
    //
    // Remeber that the write file descriptor should be configured as blocking.

    int written = write(chn_fd->write_fd, chn_fd->write_buf, write_size);

    if (written != write_size) {
        close(chn_fd->write_fd);
        chn_fd->write_fd = -1;
        status = CHN_CANNOT_WRITE;

        goto end;
    }

    // Don't think I need to do anything more if write was successful.

end:
    pthread_mutex_unlock(&(chn_fd->mut));
    return status;
}

channel_status_t chn_fd_refresh(channel_fd_t *chn_fd) {
    channel_status_t status = CHN_SUCCESS;

    pthread_mutex_lock(&(chn_fd->mut));

    // NOTE: we will say that we are allowed to call refresh on a channel who's endpoint has
    // already closed. Just return Success and call it a day.
    //
    // In fact, any error here we will ultimately let slide.

    if (chn_fd->read_fd < 0) {
        goto end;
    }

    int readden;
    while ((readden = read(chn_fd->read_fd, chn_fd->read_chunk, chn_fd->cfg.read_chunk_size)) > 0) {
        // TODO: FILL THIS IN!
    }

    // EOF is still a success!
    if (readden == 0) {
        close(chn_fd->read_fd);
        chn_fd->read_fd = -1;    

        goto end;
    }

    if (readden < 0) {
        // NOTE: I recently learned all threads have their own instance of errno.
        // This is huge!
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            // Error, just close up our boy here..
            close(chn_fd->read_fd);
            chn_fd->read_fd = -1;    
        }

        // We don't do anything with a wouldblock/again error.
        goto end;
    }

    // Successful read logic should all be handled in above while loop.

end:
    pthread_mutex_unlock(&(chn_fd->mut));
    return status;
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

