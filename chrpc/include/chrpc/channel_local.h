
#ifndef CHRPC_CHANNEL_LOCAL_H
#define CHRPC_CHANNEL_LOCAL_H

#include <stdbool.h>

#include <pthread.h>

#include "chrpc/channel.h"
#include "chutil/list.h"

// A local channel will be two endpoints within the same 
// process. The endpoints will be threadsafe.

typedef struct _channel_local_config_t {
    // The number of msgs the channel can hold at once.
    //
    // If 0, the channel can hold an arbitrary number of messages.
    size_t queue_depth;

    // If queue_depth is 0, this settig is ignored.
    //
    // Otherwise, if true, when a message is received, if the queue is full,
    // the oldest message will be written over.
    //
    // If false, when a message is received, if the queue is full,
    // the received message is discarded.
    bool write_over;

    size_t max_msg_size;
} channel_local_config_t;

typedef struct _channel_local_t {
    channel_local_config_t cfg;

    pthread_mutex_t mut;

    // List<channel_msg_t>
    list_t *queue;
} channel_local_t;

extern const channel_impl_t * const CHANNEL_LOCAL_IMPL;

// NOTE: The value pointed to by cfg can be destructed after this
// call. Only the values inside the cfg object will be copied into
// the created channel.
channel_status_t new_channel_local(channel_local_t **chn_l, 
        const channel_local_config_t *cfg);
channel_status_t delete_channel_local(channel_local_t *chn_l);

channel_status_t chn_l_max_msg_size(channel_local_t *chn_l, size_t *mms);
channel_status_t chn_l_send(channel_local_t *chn_l, const void *msg, size_t len);
channel_status_t chn_l_refresh(channel_local_t *chn_l);
channel_status_t chn_l_incoming_len(channel_local_t *chn_l, size_t *len);
channel_status_t chn_l_receive(channel_local_t *chn_l, void *buf, size_t len, size_t *readden); 


#endif

