
#include "chrpc/channel.h"
#include "chrpc/serial_type.h"
#include "chsys/mem.h"
#include "chsys/wrappers.h"
#include "chutil/list.h"
#include "chrpc/rpc.h"
#include "chutil/map.h"
#include "chutil/queue.h"
#include "chutil/string.h"
#include <pthread.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
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

// In buf will have size server->attrs.max_msg_size.
static chrpc_status_t chrpc_poll_channel(channel_t *chn, chrpc_server_t *server, uint8_t *in_buf) {
    chrpc_status_t status;

    TRY_CHANNEL_CALL(chn_refresh(chn));

    size_t incoming_len;
    TRY_CHANNEL_CALL(chn_incoming_len(chn, &incoming_len));

    // We make it here, incoming_len is non-zero.

    size_t readden;
    TRY_CHANNEL_CALL(chn_receive(chn, in_buf, server->attrs.max_msg_size, &readden));

    // Now what, we parse a value???
    // If there is an error here, I believe we send stuff back???
    return CHRPC_SUCCESS; 
}

static void *chrpc_server_worker_routine(void *arg) {
    chrpc_server_t *server = (chrpc_server_t *)arg;
    uint8_t *in_buf = (uint8_t *)safe_malloc(server->attrs.max_msg_size);

    while (true) {

        bool should_exit;
        safe_pthread_mutex_lock(&(server->should_exit_mut));
        should_exit = server->should_exit;
        safe_pthread_mutex_unlock(&(server->should_exit_mut));

        if (should_exit) {
            safe_free(in_buf);
            return NULL;
        }

        // Otherwise, let's do actual work.
        
        channel_t *chn;
        int e;

        safe_pthread_mutex_lock(&(server->q_mut));
        e = q_pop(server->channels_q, &chn);
        safe_pthread_mutex_unlock(&(server->q_mut));

        if (e) {
            usleep(server->attrs.worker_usleep_amt);
            continue;
        } 

        // If we make it here, we have a channel, to work with.
        // Let's call our poll helper function.
        // This function will do the work of attempting to read an incoming message.
        chrpc_status_t status = chrpc_poll_channel(chn, server, in_buf);

        if (status != CHN_NO_INCOMING_MSG && status != CHN_SUCCESS) {
            delete_channel(chn);

            safe_pthread_mutex_lock(&(server->q_mut));
            server->num_channels--;
            safe_pthread_mutex_unlock(&(server->q_mut));

            continue;
        } 

        // SUCCESS or NO INCOMING MESSAGE.
            
        if (status == CHN_NO_INCOMING_MSG) {
            // If no work was done, sleep a lil'
            usleep(server->attrs.worker_usleep_amt);
        }

        // Give channel back to the channel queue.
        safe_pthread_mutex_lock(&(server->q_mut));
        q_push(server->channels_q, &chn);
        safe_pthread_mutex_unlock(&(server->q_mut));
    }

    // Should never make it here.
    return NULL;
}

chrpc_status_t new_chrpc_server(chrpc_server_t **server, chrpc_server_attrs_t attrs, chrpc_endpoint_set_t *eps) {
    // Just as a not, my argument checking is usually kinda arbitrary.
    if (!server || attrs.max_connections == 0 || attrs.num_workers == 0 || attrs.max_msg_size == 0) {
        return CHRPC_SERVER_CREATION_ERROR;
    }

    if (attrs.max_connections < attrs.num_workers) {
        return CHRPC_SERVER_CREATION_ERROR;
    }

    chrpc_server_t *s = (chrpc_server_t *)safe_malloc(sizeof(chrpc_server_t));
    s->attrs = attrs;

    safe_pthread_mutex_init(&(s->q_mut), NULL);
    s->num_channels = 0;
    s->channels_q = new_queue(s->attrs.max_connections, sizeof(channel_t *));

    s->worker_ids = (pthread_t *)safe_malloc(sizeof(pthread_t) * s->attrs.num_workers);
    safe_pthread_mutex_init(&(s->should_exit_mut), NULL);
    s->should_exit = false;

    s->ep_set = eps;

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

    channel_t *chan; 
    while (q_pop(server->channels_q, &chan) == 0) {
        delete_channel(chan);
    }

    delete_queue(server->channels_q);
    safe_pthread_mutex_destroy(&(server->q_mut));

    // Finally, delete endpoint set.
    delete_chrpc_endpoint_set((chrpc_endpoint_set_t *)(server->ep_set));
    safe_free(server);
}

int chrpc_server_give_channel(chrpc_server_t *server, channel_t *chn) {
    if (!chn) {
        return 1;
    }

    size_t mms;
    chrpc_status_t status = chn_max_msg_size(chn, &mms);

    if (status != CHRPC_SUCCESS || server->attrs.max_msg_size < mms) {
        return 1;
    }

    int ret_val;

    safe_pthread_mutex_lock(&(server->q_mut));

    if (server->num_channels == q_cap(server->channels_q)) {
        ret_val = 1;
    } else {
        q_push(server->channels_q, &chn);
        server->num_channels++; 
        ret_val = 0;
    }

    safe_pthread_mutex_unlock(&(server->q_mut));

    return ret_val;
}
