

#ifndef CHRPC_CHANNEL_HELPERS_H
#define CHRPC_CHANNEL_HELPERS_H

#include "chrpc/channel.h"
#include <pthread.h>
#include <stdbool.h>

typedef struct _channel_echo_thread_t {
    pthread_mutex_t mut;
    bool should_stop;

    pthread_t thread;
    channel_t *chn;
} channel_echo_thread_t;

// NOTE: Unlike many other functions in this library,
// this will return the generated object directly instead
// of a channel_status_t.
// This is because, this call won't really call any channel functions.
//
// NOTE: The given channel is NOT owned by the echo thread.
// When the echo thread is destroyed, the channel will still be usable.
channel_echo_thread_t *new_channel_echo_thread(channel_t *chn);

// This returns the status that the thread exits with.
// Which IS a channel_status_t.
channel_status_t delete_channel_echo_thread(channel_echo_thread_t *chn_et);

static inline bool chn_et_should_stop(channel_echo_thread_t *chn_et) {
    bool ss;

    pthread_mutex_lock(&(chn_et->mut));
    ss = chn_et->should_stop;
    pthread_mutex_unlock(&(chn_et->mut));

    return ss;
}

static inline void chn_et_stop(channel_echo_thread_t *chn_et) {
    pthread_mutex_lock(&(chn_et->mut));
    chn_et->should_stop = true;
    pthread_mutex_unlock(&(chn_et->mut));
}

#endif
