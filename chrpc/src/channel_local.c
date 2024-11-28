
#include "chrpc/channel_local.h"
#include "chrpc/channel.h"
#include "chsys/mem.h"
#include "chutil/list.h"
#include <pthread.h>

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

    delete_list(chn_l->queue);
    pthread_mutex_destroy(&(chn_l->mut));
    safe_free(chn_l);

    return CHN_SUCCESS;
}

channel_status_t chn_l_max_msg_size(channel_local_t *chn_l, size_t *mms) {
    return CHN_SUCCESS;
}

channel_status_t chn_l_send(channel_local_t *chn_l, const void *msg, size_t len) {
    return CHN_SUCCESS;
}
channel_status_t chn_l_refresh(channel_local_t *chn_l) {
    return CHN_SUCCESS;
}
channel_status_t chn_l_incoming_len(channel_local_t *chn_l, size_t *len) {
    return CHN_SUCCESS;
}
channel_status_t chn_l_receive(channel_local_t *chn_l, void *buf, size_t len, size_t *readden) {
    return CHN_SUCCESS;
}
