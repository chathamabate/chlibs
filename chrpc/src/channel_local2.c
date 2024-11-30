
#include "chrpc/channel_local2.h"
#include "chrpc/channel.h"
#include "chrpc/channel_local.h"
#include "chsys/mem.h"

static const channel_impl_t _CHANNEL_LOCAL2_IMPL = {
    .constructor = (channel_constructor_ft)new_channel_local,
    .destructor = (channel_destructor_ft)delete_channel_local,
    .max_msg_size = (channel_max_msg_size_ft)chn_l2_max_msg_size,
    .send = (channel_send_ft)chn_l2_send,
    .refresh = (channel_refresh_ft)chn_l2_refresh,
    .incoming_len = (channel_incoming_len_ft)chn_l2_incoming_len,
    .receive = (channel_receive_ft)chn_l2_receive
};

const channel_impl_t * const CHANNEL_LOCAL2_IMPL = &_CHANNEL_LOCAL2_IMPL;

channel_status_t new_channel_local2_core(channel_local2_core_t **chn_l2_c, 
        const channel_local_config_t *cfg) {
    if (!chn_l2_c || !cfg) {
        return CHN_INVALID_ARGS;
    }

    channel_status_t status;

    channel_local_t *a2b;
    channel_local_t *b2a;

    status = new_channel_local(&a2b, cfg);
    if (status != CHN_SUCCESS) {
        return status;
    }

    status = new_channel_local(&b2a, cfg);
    if (status != CHN_SUCCESS) {
        delete_channel_local(a2b);
        return status;
    }

    channel_local2_core_t *core = 
        (channel_local2_core_t *)safe_malloc(sizeof(channel_local2_core_t));
    core->a2b = a2b;
    core->b2a = b2a;
    
    *chn_l2_c = core;

    return CHN_SUCCESS;
}

channel_status_t delete_channel_local2_core(channel_local2_core_t *chn_l2_c) {
    channel_status_t s1, s2;

    s1 = delete_channel_local(chn_l2_c->a2b);
    s2 = delete_channel_local(chn_l2_c->b2a);

    if (s1 != CHN_SUCCESS) {
        return s1;
    }

    if (s2 != CHN_SUCCESS) {
        return s2;
    }

    return CHN_SUCCESS;
}

channel_status_t new_channel_local2(channel_local2_t **chn_l2, 
        const channel_local2_config_t *cfg) {
    if (!cfg) {
        return CHN_INVALID_ARGS;
    }

    channel_local2_t *c = (channel_local2_t *)safe_malloc(sizeof(channel_local2_t));

    c->a2b_direction = cfg->a2b_direction;
    c->core = cfg->core;

    *chn_l2 = c;

    return CHN_SUCCESS;
}
channel_status_t delete_channel_local2(channel_local2_t *chn_l2) {
    safe_free(chn_l2);
    return CHN_SUCCESS;
}

channel_status_t chn_l2_max_msg_size(channel_local2_t *chn_l2, size_t *mms) {
    // It is understood that both the a2b and b2a channels have the same
    // max message size.

    return chn_l_max_msg_size(chn_l2->core->a2b, mms);
}

channel_status_t chn_l2_send(channel_local2_t *chn_l2, const void *msg, size_t len) {
    channel_local_t *chn_l = chn_l2->a2b_direction 
        ? chn_l2->core->a2b : chn_l2->core->b2a;

    return chn_l_send(chn_l, msg, len);
}
channel_status_t chn_l2_refresh(channel_local2_t *chn_l2) {
    channel_local_t *chn_l = chn_l2->a2b_direction 
        ? chn_l2->core->b2a : chn_l2->core->a2b;

    // This is a no-op, 
    return chn_l_refresh(chn_l);
}
channel_status_t chn_l2_incoming_len(channel_local2_t *chn_l2, size_t *len) {
    channel_local_t *chn_l = chn_l2->a2b_direction 
        ? chn_l2->core->b2a : chn_l2->core->a2b;

    return chn_l_incoming_len(chn_l, len);
}

channel_status_t chn_l2_receive(channel_local2_t *chn_l2, void *buf, size_t len, size_t *readden) {
    channel_local_t *chn_l = chn_l2->a2b_direction 
        ? chn_l2->core->b2a : chn_l2->core->a2b;

    return chn_l_receive(chn_l, buf, len, readden);
}
