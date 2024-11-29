
#include "chrpc/channel_local.h"
#include "chrpc/channel.h"
#include "chsys/mem.h"
#include "chutil/list.h"
#include <pthread.h>
#include <string.h>

static const channel_impl_t _CHANNEL_LOCAL_IMPL = {
    .constructor = (channel_constructor_ft)new_channel_local,
    .destructor = (channel_destructor_ft)delete_channel_local,
    .max_msg_size = (channel_max_msg_size_ft)chn_l_max_msg_size,
    .send = (channel_send_ft)chn_l_send,
    .refresh = (channel_refresh_ft)chn_l_refresh,
    .incoming_len = (channel_incoming_len_ft)chn_l_incoming_len,
    .receive = (channel_receive_ft)chn_l_receive
};

const channel_impl_t * const CHANNEL_LOCAL_IMPL = &_CHANNEL_LOCAL_IMPL;

channel_status_t new_channel_local(channel_local_t **chn_l, 
        const channel_local_config_t *cfg) {
    if (!chn_l || !cfg) {
        return CHN_INVALID_ARGS;
    }

    channel_local_t *chn_val = safe_malloc(sizeof(channel_local_t));
    chn_val->cfg.max_msg_size = cfg->max_msg_size;
    chn_val->cfg.queue_depth = cfg->queue_depth;
    chn_val->cfg.write_over = cfg->write_over;

    pthread_mutex_init(&(chn_val->mut), NULL);
    chn_val->queue = new_list(LINKED_LIST_IMPL, sizeof(channel_msg_t));

    *chn_l = chn_val;

    return CHN_SUCCESS;
}

channel_status_t delete_channel_local(channel_local_t *chn_l) {
    if (!chn_l) {
        return CHN_INVALID_ARGS;
    }

    channel_msg_t *iter;
    l_reset_iterator(chn_l->queue);
    while ((iter = l_next(chn_l->queue))) {
        safe_free(iter->msg_buf);
    }

    delete_list(chn_l->queue);
    pthread_mutex_destroy(&(chn_l->mut));
    safe_free(chn_l);

    return CHN_SUCCESS;
}

channel_status_t chn_l_max_msg_size(channel_local_t *chn_l, size_t *mms) {
    *mms = chn_l->cfg.max_msg_size;

    return CHN_SUCCESS;
}

channel_status_t chn_l_send(channel_local_t *chn_l, const void *msg, size_t len) {
    if (len == 0 || len > chn_l->cfg.max_msg_size) {
        return CHN_INVALID_MSG_SIZE;
    }

    channel_status_t return_status = CHN_SUCCESS;

    const channel_local_config_t *cfg = &(chn_l->cfg);

    pthread_mutex_lock(&(chn_l->mut));

    if (cfg->queue_depth > 0 && l_len(chn_l->queue) == cfg->queue_depth) {

        // If we are here, the channel is full!

        if (!(cfg->write_over)) {
            return_status = CHN_CHANNEL_FULL;
            goto end;
        }

        channel_msg_t oldest_msg;

        // When write over is on, we just toss the oldest message.
        l_poll(chn_l->queue, (void *)&oldest_msg);

        safe_free(oldest_msg.msg_buf);
    }

    channel_msg_t local_msg = {
        .msg_size = len,
        .msg_buf = safe_malloc(len),
    };
    memcpy(local_msg.msg_buf, msg, len);
    l_push(chn_l->queue, &local_msg);

end:
    pthread_mutex_unlock(&(chn_l->mut));
    return return_status;
}

channel_status_t chn_l_refresh(channel_local_t *chn_l) {
    (void)chn_l;

    // Not needed for such a trivial channel.

    return CHN_SUCCESS;
}

channel_status_t chn_l_incoming_len(channel_local_t *chn_l, size_t *len) {
    channel_status_t return_status = CHN_SUCCESS;

    pthread_mutex_lock(&(chn_l->mut));

    if (l_len(chn_l->queue) == 0) {
        return_status = CHN_NO_INCOMING_MSG;
    } else {
        const channel_msg_t *msg = (const channel_msg_t *)l_get(chn_l->queue, 0);
        *len = msg->msg_size;
    }

    pthread_mutex_unlock(&(chn_l->mut));

    return return_status;
}

channel_status_t chn_l_receive(channel_local_t *chn_l, void *buf, size_t len, size_t *readden) {
    channel_status_t return_status = CHN_SUCCESS;

    pthread_mutex_lock(&(chn_l->mut));

    // Is there a message to receive?
    
    if (l_len(chn_l->queue) == 0) {
        return_status = CHN_NO_INCOMING_MSG;
        goto end;
    }

    const channel_msg_t *msg = (const channel_msg_t *)l_get(chn_l->queue, 0);
    if (msg->msg_size > len) {
        return_status = CHN_BUFFER_TOO_SMALL;
        goto end;
    }

    // We have confirmed we can poll!
    
    channel_msg_t local_msg;
    l_poll(chn_l->queue, &local_msg);

    // Our buffer is big enough! we can perform the receive!
    memcpy(buf, local_msg.msg_buf, local_msg.msg_size);
    *readden = local_msg.msg_size;
    
    safe_free(local_msg.msg_buf);

end:
    pthread_mutex_unlock(&(chn_l->mut));
    return return_status;
}
