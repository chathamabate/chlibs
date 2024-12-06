
#ifndef CHRPC_CHANNEL_LOCAL2_H
#define CHRPC_CHANNEL_LOCAL2_H

// A channel_local2, is also a channel within a single process.
// However, this channel allows for 2 bidirectional endpoints.

#include <stdbool.h>

#include "chrpc/channel.h"
#include "chrpc/channel_local.h"

// The "core" will be shared by the 2 (or more) channels which wish to 
// communicate.
typedef struct _channel_local2_core_t {
    channel_local_t *a2b;
    channel_local_t *b2a;
} channel_local2_core_t;

channel_status_t new_channel_local2_core(channel_local2_core_t **chn_l2_c, 
        const channel_local_config_t *cfg);
channel_status_t delete_channel_local2_core(channel_local2_core_t *chn_l2_c);

typedef struct _channel_local2_t {
    bool a2b_direction;
    channel_local2_core_t *core;
} channel_local2_t;

extern const channel_impl_t * const CHANNEL_LOCAL2_IMPL;

// The information required to make the channel, and the channel's state
// will have the same structure.
// Calling new_channel_local2 will simply copy the core and direction
// into dynamic memory.
typedef channel_local2_t channel_local2_config_t;

channel_status_t new_channel_local2(channel_local2_t **chn_l2, 
        const channel_local2_config_t *cfg);
channel_status_t delete_channel_local2(channel_local2_t *chn_l2);

channel_status_t chn_l2_max_msg_size(channel_local2_t *chn_l2, size_t *mms);
channel_status_t chn_l2_send(channel_local2_t *chn_l2, const void *msg, size_t len);
channel_status_t chn_l2_refresh(channel_local2_t *chn_l2);
channel_status_t chn_l2_incoming_len(channel_local2_t *chn_l2, size_t *len);
channel_status_t chn_l2_receive(channel_local2_t *chn_l2, void *buf, size_t len, size_t *readden); 

#endif
