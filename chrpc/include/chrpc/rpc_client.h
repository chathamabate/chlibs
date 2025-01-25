
#ifndef CHRPC_RPC_CLIENT_H
#define CHRPC_RPC_CLIENT_H

#include "chrpc/channel.h"
#include "chrpc/channel_local2.h"
#include "chrpc/serial_type.h"
#include "chrpc/serial_value.h"
#include "chutil/map.h"
#include "chutil/queue.h"
#include "chutil/string.h"

#include <pthread.h>
#include <unistd.h>

#include "chrpc/rpc_server.h"

typedef struct _chrpc_client_attrs_t {
    // Frequency by which to check.
    useconds_t cadence;

    // At least how long to wait until giving up.
    useconds_t timeout;
} chrpc_client_attrs_t;

typedef struct _chrpc_client_t {
    // The client will wait at least timeout microseconds for a response from the server.
    // If no response is received, the given channel will be destroyed and set to NULL.
    //
    // Similarly, If there is some fatal error sending a message over the channel or
    // receiving a message over the channel, the channel will also be destoryed and set
    // to NULL. From that point on, a CHRPC_CLIENT_ERROR will be returned everytime the user
    // tries to send a request. 
    //
    // If there is a message in the channel, it should always be receivable into buf, since
    // buf's size will be the max message size of the channel.
    //
    // If the receivied response is malformed, the channel will persist.
    chrpc_client_attrs_t attrs;
    channel_t *chn;

    // Buf will be used for preparing serialized requests.
    size_t buf_len;
    uint8_t *buf;

    // Would be nice not to need to have these allocated for every client/server
    // instance, although, tbh, it's not very significant.
    const chrpc_type_t *req_type;
    const chrpc_type_t *resp_type;
} chrpc_client_t;

extern const chrpc_client_attrs_t * const CHRPC_DEFAULT_CLIENT_ATTRS;

// The given channel will be OWNED by the created client.
//
// NOTE: Attrs are copied into resulting struct.
chrpc_status_t new_chrpc_client(chrpc_client_t **client, channel_t *chn, const chrpc_client_attrs_t *attrs);

static inline chrpc_status_t new_chrpc_default_client(chrpc_client_t **client, channel_t *chn) {
    return new_chrpc_client(client, chn, CHRPC_DEFAULT_CLIENT_ATTRS);
}

void delete_chrpc_client(chrpc_client_t *client);

// If the function being called has no return value, ret can be NULL.
// Similarly, if the function being called takes no arguments, args can be NULL.
//
// NOTE: The given args are NOT CLEANED UP by this function. It is the user's responsibility to free
// each arg after performing an rpc call.
chrpc_status_t chrpc_client_send_request(chrpc_client_t *client, const char *name, chrpc_value_t **ret, chrpc_value_t **args, uint8_t num_args);

// Expects a NULL terminated sequnce of pointer to chrpc_value_t's
chrpc_status_t _chrpc_client_send_request_va(chrpc_client_t *client, const char *name, chrpc_value_t **ret, ...);

#define chrpc_client_send_request_va(client, name, ret, ...) \
    _chrpc_client_send_request_va(client, name, ret, __VA_ARGS__, NULL)

static inline chrpc_status_t chrpc_client_send_argless_request(chrpc_client_t *client, const char *name, chrpc_value_t **ret) {
    return chrpc_client_send_request(client, name, ret, NULL, 0);
}

// This is a helper function that makes life easy.
// It creates a local bi direction channel, adds one end to the server, and creates a client with the other end.
//
// NOTE: Just be careful... i.e., don't delete the core while the server still holds one end of the connection.
// Remember that the local2 channel is kinda awkward to work with.
//
// NOTE: VERY IMPORTANT: If you are going to use a local channel, understand that it won't be removed from the 
// server's connections pull unless told explicitly to do so. (I.e. a disconnect endpoint)
// This is because, closing one end of the channel, does nothing to the other.
chrpc_status_t new_chrpc_local_client(chrpc_server_t *server, 
        const channel_local_config_t *cfg,
        const chrpc_client_attrs_t *attrs,
        channel_local2_core_t **core, chrpc_client_t **client);

#endif
