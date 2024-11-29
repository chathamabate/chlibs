
#ifndef CHRPC_CHANNEL_H
#define CHRPC_CHANNEL_H

#include <stdlib.h>

typedef enum _channel_status_t {
    CHN_SUCCESS = 0,
    CHN_INVALID_ARGS,
    CHN_BUFFER_TOO_SMALL,
    CHN_INVALID_MSG_SIZE,
    CHN_NO_INCOMING_MSG,
    CHN_MEM_ERROR,
    CHN_CHANNEL_FULL,
} channel_status_t;

#define TRY_CHANNEL_CALL(expr) \
    do { \
        channel_status_t __st = expr; \
        if (__st != CHN_SUCCESS) { \
            return __st; \
        } \
    } while (0)

// Will create some new channel object and write its address
// to the void * pointed to by the first argument.
//
// The argument can be whatever the user wants.
typedef channel_status_t (*channel_constructor_ft)(void **chn_addr, void *arg);
typedef channel_status_t (*channel_destructor_ft)(void *chn);

// A channel has a notion of a maximal sized message.
// No message of a greater size can be sent or received.
typedef channel_status_t (*channel_max_msg_size_ft)(void *chn, size_t *mms);

// To send a message successfully, it is expected that the entire message is sent.
// Usually write calls return how many bytes are written. It should be assumed
// that if an error of any kind occurs, none of the message was transmitted
// successfully.
//
// len is in bytes.
typedef channel_status_t (*channel_send_ft)(void *chn, const void *msg, size_t len);

// This call is somewhat up to the implenter's interpretation.
// The idea is that this call should do the work of preparing received data
// for the user. If you'd like to implement your channel in a single threaded manner,
// you'd expect to call this function before calling chn_incoming_len or chn_receive.
// (A program using this interface arbitrarily should follow this convention)
//
// If the implementer has some sort of background worker preparing data, this function
// may not need to do any work as someone else is in parallel.
//
// NOTE: This may be implemented as a no-op.
//
typedef channel_status_t (*channel_refresh_ft)(void *chn);

// Get the length of the next message which is to be received.
// 
// NOTE: Should return CHN_NO_INCOMING_MSG if there is no message to receive.
typedef channel_status_t (*channel_incoming_len_ft)(void *chn, size_t *len);

// Receives a message and writes it into the given buffer with length len.
// Writes the amout of bytes copied into the buffer to readden.
// 
// NOTE: Should return CHN_BUFFER_TOO_SMALL if the given buffer is not large 
// enough for the incoming message. In this case, the incoming message should NOT
// be lost! The user should be able to call receive again with an adequately sized
// buffer to receive the initial buffer.
//
// NOTE: This can be implemented to block until a message is ready to be received.
// Or, it can be implemented to return CHN_NO_INCOMING_MSG.
typedef channel_status_t (*channel_receive_ft)(void *chn, void *buf, size_t len, size_t *readden); 

// NOTE: None of the below functions given need to check arguments.
// This will be checked by generic functions below.
typedef struct _channel_impl_t {
    channel_constructor_ft  constructor;
    channel_destructor_ft   destructor;
    channel_max_msg_size_ft max_msg_size;
    channel_send_ft         send;
    channel_refresh_ft      refresh;
    channel_incoming_len_ft incoming_len;
    channel_receive_ft      receive;
} channel_impl_t;

typedef struct _channel_t {
    void *channel;
    const channel_impl_t *impl;    
} channel_t;

channel_status_t new_channel(const channel_impl_t *impl, 
        channel_t **chn, void *arg);
channel_status_t delete_channel(channel_t *chn);

static inline channel_status_t chn_max_msg_size(channel_t *chn, size_t *mms) {
    if (!chn || !mms) {
        return CHN_INVALID_ARGS;
    }

    return chn->impl->max_msg_size(chn->channel, mms);
}

static inline channel_status_t chn_send(channel_t *chn, const void *msg, size_t len) {
    if (!chn || !msg) {
        return CHN_INVALID_ARGS;
    }

    return chn->impl->send(chn->channel, msg, len);
}

static inline channel_status_t chn_refresh(channel_t *chn) {
    if (!chn) {
        return CHN_INVALID_ARGS;
    }

    return chn->impl->refresh(chn->channel);
}

static inline channel_status_t chn_incoming_len(channel_t *chn, size_t *len) {
    if (!chn || !len) {
        return CHN_INVALID_ARGS;
    }

    return chn->impl->incoming_len(chn->channel, len);
}

static inline channel_status_t chn_receive(channel_t *chn, void *buf, size_t len, size_t *readden) {
    if (!chn || !buf || !readden) {
        return CHN_INVALID_ARGS;
    }

    return chn->impl->receive(chn->channel, buf, len, readden);
}

// Helper message type. 
//
// NOTE: This does NOT need to be used by your implementation!

typedef struct _channel_msg_t {
    size_t msg_size;
    void *msg_buf;
} channel_msg_t;

#endif
