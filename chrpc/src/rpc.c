
#include "chrpc/channel.h"
#include "chrpc/serial_type.h"
#include "chsys/mem.h"
#include "chutil/list.h"
#include "chrpc/rpc.h"
#include "chutil/map.h"
#include "chutil/queue.h"
#include "chutil/string.h"
#include <pthread.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

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

static void *chrpc_server_worker_routine(void *arg) {
    // TODO: Fill in the real work here...
    return NULL;
}

chrpc_status_t new_chrpc_server(chrpc_server_t **server, size_t max_cons, size_t workers, chrpc_endpoint_set_t *eps) {
    int e;

    if (!server || max_cons == 0 || workers == 0) {
        return CHRPC_SERVER_CREATION_ERROR;
    }

    chrpc_server_t *s = (chrpc_server_t *)safe_malloc(sizeof(chrpc_server_t));

    e = pthread_mutex_init(&(s->q_mut), NULL);
    if (e) {
        goto err_q_mut;
    }

    s->num_channels = 0;
    s->channels_q = new_queue(max_cons, sizeof(channel_t *));

    s->num_workers = workers;
    s->worker_ids = (pthread_t *)safe_malloc(sizeof(pthread_t) * workers);

    e = pthread_mutex_init(&(s->should_exit_mut), NULL); 
    if (e) {
        goto err_should_exit_mut;
    }
    s->should_exit = false;

    s->ep_set = eps;

    // Finally, spawn workers....
    size_t i;
    for (i = 0; i < s->num_workers; i++) {
        e = pthread_create(&(s->worker_ids[i]), NULL, 
                chrpc_server_worker_routine, NULL);
        if (e) {
            break;
        }
    }

    if (i < s->num_workers) {
        goto err_pthread_create;
    }

    *server = s;
    return CHRPC_SUCCESS;

err_pthread_create:
    // Destory all threads which were correctly spawned.
    pthread_mutex_lock(&(s->should_exit_mut));
    s->should_exit = true;
    pthread_mutex_unlock(&(s->should_exit_mut));

    for (size_t j = 0; j < i; j++) {
        pthread_join(s->worker_ids[j], NULL);
    }

    pthread_mutex_destroy(&(s->should_exit_mut));

err_should_exit_mut:
    safe_free(s->worker_ids);
    delete_queue(s->channels_q);
    pthread_mutex_destroy(&(s->q_mut));

err_q_mut:
err:
    safe_free(s);

    *server = NULL;
    return CHRPC_SERVER_CREATION_ERROR;
}

void delete_chrpc_server(chrpc_server_t *server) {
    // Join all workers.
    pthread_mutex_lock(&(server->should_exit_mut));
    server->should_exit = true;
    pthread_mutex_unlock(&(server->should_exit_mut));

    for (size_t i = 0; i < server->num_workers; i++) {
        pthread_join(server->worker_ids[i], NULL);
    }

    // Cannot free any of the server's state until we know
    // all worker threads are done working!

    pthread_mutex_destroy(&(server->should_exit_mut));
    safe_free(server->worker_ids);

    // Cleanup channel queue.

    channel_t *chan; 
    while (q_pop(server->channels_q, &chan) == 0) {
        delete_channel(chan);
    }

    delete_queue(server->channels_q);
    pthread_mutex_destroy(&(server->q_mut));

    // Finally, delete endpoint set.
    delete_chrpc_endpoint_set((chrpc_endpoint_set_t *)(server->ep_set));
    safe_free(server);
}

int chrpc_server_give_channel(chrpc_server_t *server, channel_t *chn) {
    if (!chn) {
        return 1;
    }

    int ret_val;

    pthread_mutex_lock(&(server->q_mut));

    if (server->num_channels == q_cap(server->channels_q)) {
        ret_val = 1;
    } else {
        q_push(server->channels_q, &chn);
        server->num_channels++; 
        ret_val = 0;
    }

    pthread_mutex_unlock(&(server->q_mut));

    return ret_val;
}
