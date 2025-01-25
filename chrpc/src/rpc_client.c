
#include "chrpc/rpc_client.h"
#include "chrpc/rpc_server.h"

#include "chrpc/channel.h"
#include "chrpc/channel_local2.h"
#include "chrpc/serial_helpers.h"
#include "chrpc/serial_type.h"
#include "chrpc/serial_value.h"
#include "chsys/mem.h"
#include "chsys/wrappers.h"
#include "chutil/list.h"
#include "chutil/map.h"
#include "chutil/queue.h"
#include "chutil/string.h"
#include <pthread.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>

static const chrpc_client_attrs_t _CHRPC_DEFAULT_CLIENT_ATTRS = {
    .cadence = 50000,
    .timeout = 5000000
};
const chrpc_client_attrs_t * const CHRPC_DEFAULT_CLIENT_ATTRS = &_CHRPC_DEFAULT_CLIENT_ATTRS;

chrpc_status_t new_chrpc_client(chrpc_client_t **client, channel_t *chn, const chrpc_client_attrs_t *attrs) {
    if (!chn || attrs->cadence == 0 || attrs->cadence == 0) {
        return CHRPC_CLIENT_CREATION_ERROR;
    }

    channel_status_t chn_status;

    size_t mms;
    chn_status = chn_max_msg_size(chn, &mms);

    if (chn_status != CHN_SUCCESS) {
        return CHRPC_CLIENT_CHANNEL_ERROR;
    }

    chrpc_client_t *c = (chrpc_client_t *)safe_malloc(sizeof(chrpc_client_t));

    c->attrs = *attrs;
    c->chn = chn;

    c->buf_len = mms;
    c->buf = (uint8_t *)safe_malloc(mms);

    c->req_type = new_chrpc_struct_type(
        CHRPC_STRING_T,
        new_chrpc_array_type(
            new_chrpc_array_type(CHRPC_BYTE_T)
        )
    );
    c->resp_type = new_chrpc_struct_type(
        CHRPC_BYTE_T,
        new_chrpc_array_type(CHRPC_BYTE_T)
    );

    *client = c;
    return CHRPC_SUCCESS;
}

void delete_chrpc_client(chrpc_client_t *client) {
    // It's possible the underlying channel was already destroyed earlier.
    if (client->chn) {
        delete_channel(client->chn);
    }

    safe_free(client->buf);

    delete_chrpc_type((chrpc_type_t *)(client->req_type));
    delete_chrpc_type((chrpc_type_t *)(client->resp_type));

    safe_free(client);
}

chrpc_status_t chrpc_client_send_request(chrpc_client_t *client, const char *name, chrpc_value_t **ret, chrpc_value_t **args, uint8_t num_args) {
    // NOTE: again, we are going to do this in a slightly hacky manner once again.
    // Going to serialize on the fly.

    chrpc_status_t rpc_status;
    channel_status_t chn_status;

    // A fatal error has happened once before.
    if (!(client->chn)) {
        return CHRPC_CLIENT_CHANNEL_ERROR; 
    }

    size_t buf_len = client->buf_len;
    uint8_t *buf = client->buf;

    size_t total_written = 0;
    size_t written;

    rpc_status = chrpc_type_to_buffer(client->req_type, buf + total_written, buf_len - total_written, &written);
    total_written += written;

    if (rpc_status != CHRPC_SUCCESS) {
        return rpc_status;
    }

    // Now string name.
    size_t name_len = strlen(name) + 1;
    if (buf_len - total_written < sizeof(uint32_t) + name_len) {
        return CHRPC_BUFFER_TOO_SMALL;
    }

    *((uint32_t *)(buf + total_written)) = name_len;
    total_written += sizeof(uint32_t);

    memcpy(buf + total_written, name, name_len);
    total_written += name_len;

    // Now for arguments.
    if (buf_len - total_written < sizeof(uint32_t)) {
        return CHRPC_BUFFER_TOO_SMALL;
    }

    *(uint32_t *)(buf + total_written) = num_args;
    total_written += sizeof(uint32_t);

    for (uint8_t i = 0; i < num_args; i++) {
        rpc_status = chrpc_value_to_buffer_with_length(args[i], buf + total_written, buf_len - total_written, &written);

        if (rpc_status != CHRPC_SUCCESS) {
            return rpc_status;
        }

        total_written += written;
    }

    // Now attempt to send our request.

    chn_status = chn_send(client->chn, buf, total_written);

    // NOTE: It is gauranteed that the channel can handle the buffer's size.
    // So, we should never get some invalid message size error.
    // Any other error is seen as fatal for the channel.
    if (chn_status != CHN_SUCCESS) {
        delete_channel(client->chn);
        client->chn = NULL;

        return CHRPC_CLIENT_CHANNEL_ERROR;
    }

    // Now we poll.

    size_t readden;
    bool message_received = false;
    useconds_t time_waited = 0;

    while (time_waited < client->attrs.timeout) {
        // Before every sleep we refresh and poll the channel.
        chn_status = chn_refresh(client->chn);
        if (chn_status != CHN_SUCCESS) {
            break;
        }

        chn_status = chn_receive(client->chn, buf, buf_len, &readden);

        if (chn_status == CHN_SUCCESS) {
            message_received = true;
            break;
        }

        // fatal error.
        if (chn_status != CHN_NO_INCOMING_MSG) {
            break;
        }

        usleep(client->attrs.cadence);
        time_waited += client->attrs.cadence;
    }

    // No message was received :,(
    if (!message_received) {
        delete_channel(client->chn);
        client->chn = NULL;

        // Timeout.
        if (chn_status == CHN_NO_INCOMING_MSG) {
            return CHRPC_DISCONNECT;
        }

        return CHRPC_CLIENT_CHANNEL_ERROR;
    }

    // Now we parse the response.
    
    chrpc_value_t *resp;

    rpc_status = chrpc_value_from_buffer(&resp, buf, buf_len, &readden);

    if (rpc_status != CHRPC_SUCCESS) {
        return CHRPC_BAD_RESPONSE;
    }

    if (!chrpc_type_equals(client->resp_type, resp->type)) {
        delete_chrpc_value(resp);
        return CHRPC_BAD_RESPONSE;
    }

    // Status code of response.
    rpc_status = resp->value->struct_entries[0]->b8;
    
    if (rpc_status != CHRPC_SUCCESS) {
        delete_chrpc_value(resp);
        return rpc_status;
    }

    // Now, we must parse the return value.

    chrpc_value_t *ret_val = NULL;

    uint32_t serial_len = resp->value->struct_entries[1]->array_len;
    uint8_t *serial_ret_val = resp->value->struct_entries[1]->b8_arr;

    if (serial_len > 0) {
        rpc_status = chrpc_value_from_buffer(&ret_val, serial_ret_val, serial_len, &readden);
    }

    // This is entered when there is no return value, or when the return value is succesfully parsed!
    if (rpc_status == CHRPC_SUCCESS) {
        if (ret) {
            *ret = ret_val;
        } else if (ret_val) {
            // Kinda hacky here, this is the case we are not given a pointer to store the return value,
            // but a value is returned. Instead of doing some error situation, I'd rather just delete the
            // return value and call it a day.
            delete_chrpc_value(ret_val);
        }
    }

    // Always delete the response.
    delete_chrpc_value(resp);

    return rpc_status;
}

chrpc_status_t _chrpc_client_send_request_va(chrpc_client_t *client, const char *name, chrpc_value_t **ret, ...) {
    va_list args;
    va_start(args, ret);

    chrpc_value_t *iter;
    list_t *l = new_list(ARRAY_LIST_IMPL, sizeof(chrpc_value_t *));

    while ((iter = va_arg(args, chrpc_value_t *))) {
        l_push(l, &iter);
    }

    va_end(args);

    size_t num_args = l_len(l);

    if (CHRPC_ENDPOINT_MAX_ARGS < num_args) {
        delete_list(l);
        return CHRPC_TOO_MANY_ARGUMENTS;
    }

    chrpc_value_t **args_arr = delete_and_move_list(l);

    chrpc_status_t s;
    s = chrpc_client_send_request(client, name, ret, args_arr, num_args);
    safe_free(args_arr);

    return s;
}

chrpc_status_t new_chrpc_local_client(chrpc_server_t *server, 
        const channel_local_config_t *cfg,
        const chrpc_client_attrs_t *attrs,
        channel_local2_core_t **core, chrpc_client_t **client) {
    channel_local2_core_t *co;
    channel_t *a2b;
    channel_t *b2a;

    channel_status_t chn_status;

    chn_status = new_channel_local2_pipe(cfg, &co, &a2b, &b2a);

    if (chn_status != CHN_SUCCESS) {
        return CHRPC_CLIENT_CHANNEL_ERROR;
    }

    chrpc_status_t rpc_status;
    chrpc_client_t *cl;

    rpc_status = new_chrpc_client(&cl, a2b, attrs);

    if (rpc_status != CHRPC_SUCCESS) {
        delete_channel(a2b); 
        delete_channel(b2a); 
        delete_channel_local2_core(co);

        return rpc_status;
    }

    // From this point on, a2b is owned by the client.

    rpc_status = chrpc_server_give_channel(server, b2a);

    if (rpc_status != CHRPC_SUCCESS) {
        delete_chrpc_client(cl);

        delete_channel(b2a); 
        delete_channel_local2_core(co);

        return rpc_status;
    }

    // Otherwise, Success!

    *core = co;
    *client = cl;

    return CHRPC_SUCCESS;
}

