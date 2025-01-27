
#include "chrpc/channel_fd.h"
#include "chrpc/channel.h"
#include "chrpc/serial_type.h"
#include "chsys/mem.h"
#include "chutil/queue.h"
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>

const channel_impl_t _CHANNEL_FD_IMPL = {
    .constructor = (channel_constructor_ft)new_channel_fd,
    .destructor = (channel_destructor_ft)delete_channel_fd,
    .incoming_len = (channel_incoming_len_ft)chn_fd_incoming_len,
    .receive = (channel_receive_ft)chn_fd_receive,
    .max_msg_size = (channel_max_msg_size_ft)chn_fd_max_msg_size,
    .refresh = (channel_refresh_ft)chn_fd_refresh,
    .send = (channel_send_ft)chn_fd_send
};

const channel_impl_t * const CHANNEL_FD_IMPL = &_CHANNEL_FD_IMPL;

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
            cfg->write_fd < 0 || cfg->read_chunk_size == 0) {
        return CHN_INVALID_ARGS;
    }

    channel_fd_t *chn = (channel_fd_t *)safe_malloc(sizeof(channel_fd_t));
    if (pthread_mutex_init(&(chn->mut), NULL)) {
        safe_free(chn);
        return CHN_UNKNOWN_ERROR;
    }
    
    chn->cfg = *cfg;  
    chn->write_fd = cfg->write_fd;
    chn->read_fd = cfg->read_fd;
    chn->read_chunk = (uint8_t *)safe_malloc(cfg->read_chunk_size);
    chn->msg_buf_fill = 0;
    chn->msg_buf = (uint8_t *)safe_malloc(CHN_FD_MSG_SIZE(cfg->max_msg_size));
    chn->write_buf = (uint8_t *)safe_malloc(CHN_FD_MSG_SIZE(cfg->max_msg_size));
    chn->q = new_queue(cfg->queue_depth, sizeof(channel_msg_t));

    if (chn->read_fd < 0) {
        int read_fd = dup(cfg->write_fd);
        if (read_fd < 0) {
            delete_channel_fd(chn);
            return CHN_UNKNOWN_ERROR;
        }
        chn->read_fd = read_fd;
    }

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
    safe_free(chn_fd->msg_buf);
    safe_free(chn_fd->write_buf);

    pthread_mutex_destroy(&(chn_fd->mut));
    safe_free(chn_fd);

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

    chn_fd->write_buf[0] = CHN_FD_START_MAGIC;
    *(uint32_t *)(chn_fd->write_buf + 1) = len;
    memcpy(chn_fd->write_buf + (sizeof(uint8_t) + sizeof(uint32_t)), msg, len);
    *(chn_fd->write_buf + sizeof(uint8_t) + sizeof(uint32_t) + len) = CHN_FD_END_MAGIC;

    // NOTE: A more rigorous implementation would write again if a partial write occurred.
    // In this case, any write which was not complete will close the write file descriptor.
    //
    // Remeber that the write file descriptor should be configured as blocking.

    int written = write(chn_fd->write_fd, chn_fd->write_buf, write_size);

    if (written < 0 || (size_t)written != write_size) {
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

// NOTE: Assumes we have the channel lock, returns new value of read_chunk_pos.
static size_t _chn_fd_process_single_msg(channel_fd_t *chn_fd, size_t read_chunk_pos, size_t readden) {
    if (read_chunk_pos >= readden) {
        return read_chunk_pos;
    }

    // If there is nothing, we must search for the magic start value.
    if (chn_fd->msg_buf_fill == 0) {
        while (read_chunk_pos < readden) {
            // We found our magic value!
            if (chn_fd->read_chunk[read_chunk_pos] == CHN_FD_START_MAGIC) {
                chn_fd->msg_buf[0] = CHN_FD_START_MAGIC;

                chn_fd->msg_buf_fill++;
                read_chunk_pos++;

                break;
            }

            read_chunk_pos++;
        }
    }

    // NOTE: that if we never find a starting magic value, this is ok.
    // It'll just return here before doing any of the other logic.
    if (read_chunk_pos >= readden) {
        return read_chunk_pos;
    }

    if (chn_fd->msg_buf_fill < (sizeof(uint8_t) + sizeof(uint32_t))) {
        // If we make it here, we know that we have our starting magic value in the
        // message buffer, but we don't have the complete length... AND there are more bytes to read.
        
        size_t length_bytes_needed = (sizeof(uint8_t) + sizeof(uint32_t)) - chn_fd->msg_buf_fill;
        size_t bytes_to_read = readden - read_chunk_pos;
        if (bytes_to_read > length_bytes_needed) {
            bytes_to_read = length_bytes_needed; 
        }

        memcpy(chn_fd->msg_buf + chn_fd->msg_buf_fill, chn_fd->read_chunk + read_chunk_pos, bytes_to_read);
        
        chn_fd->msg_buf_fill += bytes_to_read;
        read_chunk_pos += bytes_to_read;
    }

    if (read_chunk_pos >= readden) {
        return read_chunk_pos;
    }

    // If we make it here, the buffer holds a starting magic value, a length value (And potentially more)
    size_t msg_length = *(uint32_t *)(chn_fd->msg_buf + sizeof(uint8_t));

    // Throw out any messages which are too large.
    if (msg_length > chn_fd->cfg.max_msg_size) {
        chn_fd->msg_buf_fill = 0;
        return read_chunk_pos;
    }

    if (chn_fd->msg_buf_fill < (sizeof(uint8_t) + sizeof(uint32_t) + msg_length)) {
        size_t msg_bytes_needed =  (sizeof(uint8_t) + sizeof(uint32_t) + msg_length) - chn_fd->msg_buf_fill;
        size_t bytes_to_read = readden - read_chunk_pos;
        if (bytes_to_read > msg_bytes_needed) {
            bytes_to_read = msg_bytes_needed;
        }

        memcpy(chn_fd->msg_buf + chn_fd->msg_buf_fill, chn_fd->read_chunk + read_chunk_pos, bytes_to_read);

        chn_fd->msg_buf_fill += bytes_to_read;
        read_chunk_pos += bytes_to_read;
    }

    if (read_chunk_pos >= readden) {
        return read_chunk_pos;
    }

    // If we make it here, the buffer holds everything except maybe the ending magic value.
    if (chn_fd->msg_buf_fill < (sizeof(uint8_t) + sizeof(uint32_t) + msg_length + sizeof(uint8_t))) {
        // Uh-Oh, we failed to read the end message value :,(
        if (chn_fd->read_chunk[read_chunk_pos] != CHN_FD_END_MAGIC) {
            chn_fd->msg_buf_fill = 0;
            read_chunk_pos++;

            return read_chunk_pos;
        }

        // We have a complete message!
        chn_fd->msg_buf[chn_fd->msg_buf_fill++] = CHN_FD_END_MAGIC;
        read_chunk_pos++;
    }

    // Clearing the fill of our message buf here will make things easier imo.
    chn_fd->msg_buf_fill = 0;

    // Throw out empty messages.
    if (msg_length == 0) {
        return read_chunk_pos;
    }

    channel_msg_t msg;

    if (q_len(chn_fd->q) == q_cap(chn_fd->q)) {
        // No write over?, call it a day.
        if (!(chn_fd->cfg.write_over)) {
            return read_chunk_pos;
        }
        
        // Throw out oldest message in write over case.
        q_poll(chn_fd->q, &msg); 
        safe_free(msg.msg_buf); 
    }

    msg.msg_buf = safe_malloc(msg_length);
    memcpy(msg.msg_buf, chn_fd->msg_buf + sizeof(uint8_t) + sizeof(uint32_t), msg_length);

    msg.msg_size = msg_length;
    
    q_push(chn_fd->q, &msg);

    return read_chunk_pos;
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
        size_t read_chunk_pos = 0;
        while (read_chunk_pos < (size_t)readden) {
            read_chunk_pos = _chn_fd_process_single_msg(chn_fd, read_chunk_pos, readden);
        }
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

    // I don't think I need to do anything else here.

end:
    pthread_mutex_unlock(&(chn_fd->mut));
    return status;
}

channel_status_t chn_fd_incoming_len(channel_fd_t *chn_fd, size_t *len) {
    channel_status_t return_status = CHN_SUCCESS;

    pthread_mutex_lock(&(chn_fd->mut));

    if (q_len(chn_fd->q) == 0) {
        return_status = chn_fd->read_fd < 0 ? CHN_CANNOT_READ : CHN_NO_INCOMING_MSG;
    } else {
        const channel_msg_t *msg = (const channel_msg_t *)q_peek(chn_fd->q);
        *len = msg->msg_size;
    }

    pthread_mutex_unlock(&(chn_fd->mut));

    return return_status;
}

channel_status_t chn_fd_receive(channel_fd_t *chn_fd, void *buf, size_t len, size_t *readden) {
    channel_status_t return_status = CHN_SUCCESS;

    pthread_mutex_lock(&(chn_fd->mut));

    if (q_len(chn_fd->q) == 0) {
        return_status = chn_fd->read_fd < 0 ? CHN_CANNOT_READ : CHN_NO_INCOMING_MSG;
        goto end;
    } 

    const channel_msg_t *peek_msg = (const channel_msg_t *)q_peek(chn_fd->q);
    if (peek_msg->msg_size > len) {
        return_status = CHN_BUFFER_TOO_SMALL;
        goto end;
    }

    // We can successfully pop a message of the queue!
    
    channel_msg_t msg;
    q_poll(chn_fd->q, &msg);
    memcpy(buf, msg.msg_buf, msg.msg_size);
    *readden = msg.msg_size;    
    safe_free(msg.msg_buf);

end:
    pthread_mutex_unlock(&(chn_fd->mut));
    return return_status;
}

