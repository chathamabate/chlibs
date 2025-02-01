
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
#include "chsys/log.h"
#include <pthread.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

chrpc_endpoint_t *new_chrpc_endpoint(const char *n, chrpc_endpoint_ft f, chrpc_type_t *rt, chrpc_type_t **args, uint32_t num_args) {
    if (CHRPC_ENDPOINT_MAX_ARGS < num_args) {
        return NULL;
    }

    chrpc_endpoint_t *ep = (chrpc_endpoint_t *)safe_malloc(sizeof(chrpc_endpoint_t));

    ep->name = new_string_from_cstr(n);
    ep->func = f;

    ep->ret = rt;
    ep->args = args;
    ep->num_args = num_args;

    return ep;
}

chrpc_endpoint_t *_new_chrpc_endpoint_va(const char *n, chrpc_endpoint_ft f, chrpc_type_t *rt, ...) {
    va_list args;
    va_start(args, rt);

    chrpc_type_t *iter;
    list_t *l = new_list(ARRAY_LIST_IMPL, sizeof(chrpc_type_t *));

    while ((iter = va_arg(args, chrpc_type_t *))) {
        l_push(l, &iter);
    }

    va_end(args);

    size_t num_args = l_len(l);
    chrpc_type_t **args_arr = (chrpc_type_t **)delete_and_move_list(l);

    chrpc_endpoint_t *ep = new_chrpc_endpoint(n, f, rt, args_arr, num_args);

    // If the created endpoint is invalid, we will only clean up what we created
    // within this function. Varargs may cause memory leaks... So call this function
    // correctly!
    if (!ep) {
        safe_free(args_arr);
        return NULL;
    }

    return ep;
}

void delete_chrpc_endpoint(chrpc_endpoint_t *ep) {
    delete_string(ep->name);
    
    if (ep->ret) {
        delete_chrpc_type(ep->ret);
    }

    for (uint32_t i = 0; i < ep->num_args; i++) {
        delete_chrpc_type(ep->args[i]);
    }

    if (ep->args) {
        safe_free(ep->args);
    }

    safe_free(ep);
}

chrpc_endpoint_set_t *new_chrpc_endpoint_set(chrpc_endpoint_t **eps, size_t num_eps) {
    if (num_eps < 1 || CHRPC_ENDPOINT_SET_MAX_SIZE < num_eps) {
        return NULL;
    }

    hash_map_t *hm = new_hash_map(
            sizeof(const string_t *), 
            sizeof(chrpc_endpoint_t *), 
            (hash_map_hash_ft)s_indirect_hash,
            (hash_map_key_eq_ft)s_indirect_equals 
    );

    for (size_t i = 0; i < num_eps; i++) {
        chrpc_endpoint_t *ep = eps[i];

        // Two endpoints with same name NOT ALLOWED.
        if (hm_contains(hm, &(ep->name))) {
            delete_hash_map(hm);
            return NULL;
        }

        hm_put(hm, &(ep->name), &ep);
    }

    chrpc_endpoint_set_t *ep_set = 
        (chrpc_endpoint_set_t *)safe_malloc(sizeof(chrpc_endpoint_set_t));

    ep_set->name_map = hm;
    ep_set->endpoints = eps;
    ep_set->num_endpoints = num_eps;

    return ep_set;
}

chrpc_endpoint_set_t *_new_chrpc_endpoint_set_va(int dummy, ...) {
    va_list args;
    va_start(args, dummy);

    chrpc_endpoint_t *iter;
    list_t *l = new_list(ARRAY_LIST_IMPL, sizeof(chrpc_endpoint_t *));

    while ((iter = va_arg(args, chrpc_endpoint_t *))) {
        l_push(l, &iter);
    }

    va_end(args);

    size_t num_endpoints = l_len(l);
    chrpc_endpoint_t **ep_arr =
        (chrpc_endpoint_t **)delete_and_move_list(l);

    chrpc_endpoint_set_t *ep_set = new_chrpc_endpoint_set(ep_arr, num_endpoints);

    // Same as new endpoint notes above.
    if (!ep_set) {
        safe_free(ep_arr); 
        return NULL;
    }

    return ep_set;
}

void delete_chrpc_endpoint_set(chrpc_endpoint_set_t *ep_set) {
    delete_hash_map(ep_set->name_map);

    for (size_t i = 0; i < ep_set->num_endpoints; i++) {
        delete_chrpc_endpoint(ep_set->endpoints[i]);
    }

    safe_free(ep_set->endpoints);
    safe_free(ep_set);
}

const chrpc_endpoint_t *chrpc_endpoint_set_lookup(const chrpc_endpoint_set_t *ep_set, const char *name) {
    // NOTE: this is slightly hacky, we know that our temp key
    // will not outlive this function, so we can pretend the
    // given name is a literal to avoid copying.
    string_t *temp_key = new_string_from_literal(name);

    chrpc_endpoint_t **ep = (chrpc_endpoint_t **)hm_get(ep_set->name_map, &temp_key);

    delete_string(temp_key);

    if (!ep) {
        return NULL;
    }

    return *ep;
}

// This sends an error to the client.
static chrpc_status_t chrpc_send_error_response(channel_t *chn, chrpc_server_t *server, uint8_t *buf, chrpc_status_t err) {
    chrpc_status_t rpc_status;
    channel_status_t chn_status;

    chrpc_value_t *err_val = new_chrpc_struct_value_va(
        new_chrpc_b8_value(err),
        new_chrpc_b8_empty_array_value() // empty return value.
    );

    size_t written;
    rpc_status = chrpc_value_to_buffer(err_val, buf, server->attrs.max_msg_size, &written);
    delete_chrpc_value(err_val);

    // Really, this should never happen IMO. 
    if (rpc_status != CHRPC_SUCCESS) {
        return rpc_status;
    }

    chn_status = chn_send(chn, buf, written);
    if (chn_status != CHN_SUCCESS) {
        return CHRPC_CLIENT_CHANNEL_ERROR;
    }

    return CHRPC_SUCCESS;
}

// Accounts for NULL being passed in, this is only really used for comparing return value
// types from endpoint inner fucntions.
static inline bool chrpc_type_equals_wrapper(const chrpc_type_t *ct, const chrpc_value_t *cv) {
    if (ct == NULL && cv == NULL) {
        return true;
    }

    if (ct == NULL || cv == NULL) {
        return false;
    }

    return chrpc_type_equals(ct, cv->type);
}

// This will craft and send a response to the given endpoint using the given buffer.
// This assumes the args array is already populated with arguments of the expected types.
static chrpc_status_t chrpc_send_response(const chrpc_endpoint_t *ep, chrpc_value_t **args, chrpc_queue_ele_t *ele, chrpc_server_t *server, uint8_t *buf) {
    chrpc_server_command_t cmd;
    channel_t *chn = ele->chn;

    chrpc_value_t *ret_val = NULL;
    cmd = ep->func(ele->id, server->server_state, &ret_val, args, ep->num_args);

    if (!chrpc_type_equals_wrapper(ep->ret, ret_val)) {
        if (ret_val) {
            delete_chrpc_value(ret_val);
        }
        return CHRPC_SERVER_INTERNAL_ERROR;
    }

    // FINALLY A SUCCESS!

    // OK, this is when things get slightly hacky.
    // Because the response structure is very simple, we are going to serialize on the fly here...
    // This way we don't need to copy between a bunch of different buffers.
    
    const size_t mms = server->attrs.max_msg_size;

    size_t total_written = 0;

    size_t written;
    chrpc_status_t status;

    status = chrpc_type_to_buffer(server->resp_type, buf + total_written, mms - total_written, &written);
    total_written += written;

    // Write status flag.
    if (status == CHRPC_SUCCESS) {
        // We don't have room for the status flag :(
        if (mms - total_written < sizeof(uint8_t)) {
            status = CHRPC_BUFFER_TOO_SMALL;
        } else {
            buf[total_written] = CHRPC_SUCCESS;
            total_written += sizeof(uint8_t);
        }
    }

    // Now serialize response value.
    if (status == CHRPC_SUCCESS) {
        // Remeber, for this call specifically, ret_val can be NULL.
        status = chrpc_value_to_buffer_with_length(ret_val, buf + total_written, mms - total_written, &written); 
        total_written += written;
    }

    // Finally, done with return value.
    if (ret_val) {
        delete_chrpc_value(ret_val);
    }

    // Was there an error serializing?
    if (status != CHRPC_SUCCESS) {
        return chrpc_send_error_response(chn, server, buf, status);
    }

    // Otherwise, we can attempt to send our message!

    channel_status_t chn_status = chn_send(chn, buf, total_written);

    // If our message is too big, we can still send a valid response to the client.
    if (chn_status == CHN_INVALID_MSG_SIZE) {
        return chrpc_send_error_response(chn, server, buf, CHRPC_BUFFER_TOO_SMALL);
    }

    // If there is some other channel error, this is seen as non-recoverable,
    // don't even try to send an error code to the client.
    if (chn_status != CHN_SUCCESS) {
        return CHRPC_CLIENT_CHANNEL_ERROR;        
    }

    // Otherwise, things were actually sent successfully!
    if (cmd == CHRPC_SC_DISCONNECT) {
        return CHRPC_DISCONNECT;
    }

    return CHRPC_SUCCESS;
}

static chrpc_status_t chrpc_handle_request(chrpc_value_t *req, chrpc_queue_ele_t *ele, chrpc_server_t *server, uint8_t *buf) {
    channel_t *chn = ele->chn;

    // Valid Chrpc Value found, is it the correct type tho?
    if (!chrpc_type_equals(server->req_type, req->type)) {
        return chrpc_send_error_response(chn, server, buf, CHRPC_BAD_REQUEST);
    }

    const char *endpoint_name = req->value->struct_entries[0]->str;
    const chrpc_inner_value_t *arg_arrays = req->value->struct_entries[1];

    // Now, let's lookup our endpoint.
    const chrpc_endpoint_t *ep = chrpc_endpoint_set_lookup(server->ep_set, endpoint_name);

    // Couldn't find an endpoint.
    if (!ep) {
        return chrpc_send_error_response(chn, server, buf, CHRPC_UKNOWN_ENDPOINT);
    }

    if (ep->num_args != arg_arrays->array_len) {
        return chrpc_send_error_response(chn, server, buf, CHRPC_ARGUMENT_MISMATCH);
    }

    chrpc_status_t status = CHRPC_SUCCESS;
    chrpc_value_t **given_args = (chrpc_value_t **)safe_malloc(sizeof(chrpc_value_t *) * ep->num_args);

    uint8_t parsed_values = 0;

    for (uint8_t i = 0; i < ep->num_args; i++) {
        uint8_t *field_buf = arg_arrays->array_entries[i]->b8_arr;
        uint32_t field_buf_len = arg_arrays->array_entries[i]->array_len;

        chrpc_value_t *field;

        size_t readden; // somewhat unused
        status = chrpc_value_from_buffer(&field, field_buf, field_buf_len, &readden);

        if (status != CHRPC_SUCCESS) {
            break;
        }

        // We successfully parsed a value.
        parsed_values++;

        // Store our parsed field.
        given_args[i] = field;

        if (!chrpc_type_equals(ep->args[i], field->type)) {
            status = CHRPC_ARGUMENT_MISMATCH;
            break;
        }
    }

    if (status != CHRPC_SUCCESS) {
        // There's been some kind of argument parsing error, let's send it 
        // back to the client instead of actually calling the endpoint.

        status = chrpc_send_error_response(chn, server, buf, status);
    } else {
        // Otherwise, things went well! Let's attempt to actually use our endpoint.

        status = chrpc_send_response(ep, given_args, ele, server, buf);
    }

    // Regardless of what happened, let's clean up our arguments.

    for (uint8_t i = 0; i < parsed_values; i++) {
        delete_chrpc_value(given_args[i]); 
    }
    safe_free(given_args);
    
    return status;
}


// In buf will have size server->attrs.max_msg_size.
static chrpc_status_t chrpc_poll_channel(chrpc_queue_ele_t *ele, chrpc_server_t *server, uint8_t *buf) {
    chrpc_status_t rpc_status;
    channel_status_t chn_status;

    channel_t *chn = ele->chn;

    if (chn_refresh(chn) != CHN_SUCCESS) {
        return CHRPC_CLIENT_CHANNEL_ERROR;
    }

    // NOTE: we know that the channel has a max message size less than the size of buf,
    // we should be able to call receive without worrying about buf being overflowed.

    size_t msg_size;
    chn_status = chn_receive(chn, buf, server->attrs.max_msg_size, &msg_size);
    if (chn_status == CHN_NO_INCOMING_MSG) {
        return CHRPC_CLIENT_CHANNEL_EMTPY;
    }

    if (chn_status != CHN_SUCCESS) {
        return CHRPC_CLIENT_CHANNEL_ERROR;
    }

    chrpc_value_t *req;

    size_t readden; // somewhat unused here.
    rpc_status = chrpc_value_from_buffer(&req, buf, msg_size, &readden);

    // Probably some sort of syntax error/end of buffer.
    if (rpc_status != CHRPC_SUCCESS) {
        return chrpc_send_error_response(chn, server, buf, rpc_status);
    }

    rpc_status = chrpc_handle_request(req, ele, server, buf);

    delete_chrpc_value(req);

    return rpc_status;
}

static void *chrpc_server_worker_routine(void *arg) {
    chrpc_server_t *server = (chrpc_server_t *)arg;

    // This buffer will be populated when receiving messages,
    // then written over when serializing the response.
    // (I don't think I really need 2 buffers for this)
    uint8_t *buf = (uint8_t *)safe_malloc(server->attrs.max_msg_size);

    while (true) {

        bool should_exit;
        safe_pthread_mutex_lock(&(server->should_exit_mut));
        should_exit = server->should_exit;
        safe_pthread_mutex_unlock(&(server->should_exit_mut));

        if (should_exit) {
            safe_free(buf);
            return NULL;
        }

        // Otherwise, let's do actual work.
        
        chrpc_queue_ele_t ele;
        int e;

        safe_pthread_mutex_lock(&(server->q_mut));
        e = q_poll(server->q, &ele);
        safe_pthread_mutex_unlock(&(server->q_mut));

        if (e) {
            usleep(server->attrs.worker_usleep_amt);
            continue;
        } 

        // If we make it here, we have a channel, to work with.
        // Let's call our poll helper function.
        // This function will do the work of attempting to read an incoming message.
        //
        // It will also do the work of sending error messages back to the client when
        // necessary.
        //
        // If an error occurs while working with the channel, but an error code is successfully
        // sent back to the client, this field will be SUCCESS.
        chrpc_status_t status = chrpc_poll_channel(&ele, server, buf);

        // Whether or not we should disconnect the channel at the end.
        bool disconnect = false;

        // Whether or not the worker should wait a bit before looking for more work.
        bool pause = false;

        time_t now = time(NULL);
        
        if (status == CHRPC_CLIENT_CHANNEL_EMTPY) {
            if (server->attrs.idle_timeout > 0 && 
                    (now - ele.since_last_req) > server->attrs.idle_timeout) {
                disconnect = true;
            } 

            pause = true;
        } else if (status == CHRPC_SUCCESS) {
            ele.since_last_req = now;
        } else {
            // An error or requested disconnect. (No pause in this case
            disconnect = true; 
        }

        if (disconnect) {
            delete_channel(ele.chn);

            safe_pthread_mutex_lock(&(server->q_mut));
            server->num_channels--;
            safe_pthread_mutex_unlock(&(server->q_mut));
        } else {
            safe_pthread_mutex_lock(&(server->q_mut));
            q_push(server->q, &ele);
            safe_pthread_mutex_unlock(&(server->q_mut));
        }

        if (pause) {
            usleep(server->attrs.worker_usleep_amt);
        }
    }

    // Should never make it here.
    return NULL;
}

chrpc_status_t new_chrpc_server(chrpc_server_t **server, void *ss, const chrpc_server_attrs_t *attrs, chrpc_endpoint_set_t *eps) {
    // Just as a not, my argument checking is usually kinda arbitrary.
    if (!server || attrs->max_connections == 0 || attrs->num_workers == 0 || attrs->max_msg_size < CHRPC_SERVER_BUF_MIN_SIZE) {
        return CHRPC_SERVER_CREATION_ERROR;
    }

    if (attrs->max_connections < attrs->num_workers) {
        return CHRPC_SERVER_CREATION_ERROR;
    }

    chrpc_server_t *s = (chrpc_server_t *)safe_malloc(sizeof(chrpc_server_t));

    s->server_state = ss;
    s->attrs = *attrs;

    safe_pthread_mutex_init(&(s->q_mut), NULL);
    s->id_counter = 0;
    s->num_channels = 0;
    s->q = new_queue(s->attrs.max_connections, sizeof(chrpc_queue_ele_t));

    s->worker_ids = (pthread_t *)safe_malloc(sizeof(pthread_t) * s->attrs.num_workers);
    safe_pthread_mutex_init(&(s->should_exit_mut), NULL);
    s->should_exit = false;

    s->ep_set = eps;
    s->req_type = new_chrpc_struct_type(
        CHRPC_STRING_T,     // Endpoint name.

        // b8[][] array of serialized arguments!
        new_chrpc_array_type(
            new_chrpc_array_type(CHRPC_BYTE_T)
        )
    );
    s->resp_type = new_chrpc_struct_type(
        CHRPC_BYTE_T,
        new_chrpc_array_type(CHRPC_BYTE_T)
    );

    // Finally, spawn workers....
    size_t i;
    for (i = 0; i < s->attrs.num_workers; i++) {
        safe_pthread_create(&(s->worker_ids[i]), NULL, chrpc_server_worker_routine, (void *)s);
    }

    *server = s;
    return CHRPC_SUCCESS;
}

void delete_chrpc_server(chrpc_server_t *server) {
    // Join all workers.
    safe_pthread_mutex_lock(&(server->should_exit_mut));
    server->should_exit = true;
    safe_pthread_mutex_unlock(&(server->should_exit_mut));

    for (size_t i = 0; i < server->attrs.num_workers; i++) {
        safe_pthread_join(server->worker_ids[i], NULL);
    }

    // Cannot free any of the server's state until we know
    // all worker threads are done working!

    safe_pthread_mutex_destroy(&(server->should_exit_mut));
    safe_free(server->worker_ids);

    // Cleanup channel queue.

    chrpc_queue_ele_t ele;
    while (q_poll(server->q, &ele) == 0) {
        delete_channel(ele.chn);
    }

    delete_queue(server->q);
    safe_pthread_mutex_destroy(&(server->q_mut));

    // Finally, delete endpoint set.
    delete_chrpc_endpoint_set((chrpc_endpoint_set_t *)(server->ep_set));
    delete_chrpc_type((chrpc_type_t *)(server->req_type));
    delete_chrpc_type((chrpc_type_t *)(server->resp_type));

    safe_free(server);
}

chrpc_status_t chrpc_server_give_channel(chrpc_server_t *server, channel_t *chn) {
    if (!chn) {
        return CHRPC_ARGUMENT_MISMATCH;
    }

    size_t mms;
    chrpc_status_t status = chn_max_msg_size(chn, &mms);

    if (status != CHRPC_SUCCESS || mms < CHRPC_SERVER_BUF_MIN_SIZE || server->attrs.max_msg_size < mms) {
        return CHRPC_BUFFER_TOO_SMALL;
    }

    int ret_val;

    safe_pthread_mutex_lock(&(server->q_mut));

    if (server->num_channels == q_cap(server->q)) {
        ret_val = CHRPC_SERVER_FULL;
        goto end;
    } 

    if (server->id_counter == UINT64_MAX) {
        ret_val = CHRPC_SERVER_FULL;
        goto end;
    }

    chrpc_queue_ele_t ele = {
        .chn = chn,
        .id = server->id_counter++,
        .since_last_req = time(NULL)
    };

    q_push(server->q, &ele);
    server->num_channels++; 
    ret_val = CHRPC_SUCCESS;

end:
    safe_pthread_mutex_unlock(&(server->q_mut));

    return ret_val;
}
