#include "chrpc/channel.h"
#include "chsys/mem.h"


channel_status_t new_channel(const channel_impl_t *impl, 
        channel_t **chn, void *arg) {
    if (!impl || !chn) {
        return CHN_INVALID_ARGS;
    }

    void *inner_chn = NULL;
    TRY_CHANNEL_CALL(impl->constructor(&inner_chn, arg));

    channel_t *chn_val = safe_malloc(sizeof(channel_t));
    chn_val->channel = inner_chn;
    chn_val->impl = impl;

    *chn = chn_val;

    return CHN_SUCCESS;
}

channel_status_t delete_channel(channel_t *chn) {
    if (!chn) {
        return CHN_INVALID_ARGS;
    }

    channel_destructor_ft destructor = chn->impl->destructor;
    void *inner_chn = chn->channel;

    safe_free(chn);

    return destructor(inner_chn);
}
