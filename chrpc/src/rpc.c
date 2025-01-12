
#include "chsys/mem.h"
#include "chutil/list.h"
#include "chrpc/rpc.h"
#include "chutil/map.h"
#include "chutil/string.h"
#include <string.h>
#include <stdarg.h>

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

    if (CHRPC_ENDPOINT_MAX_ARGS < num_args) {
        delete_list(l);
        return NULL;
    }

    // NOTE: delete_and_move should return NULL if length is 0.
    // 0 arguments is ALLOWED!
    chrpc_type_t **args_arr = (chrpc_type_t **)delete_and_move_list(l);

    return new_chrpc_endpoint(n, f, rt, args_arr, num_args); 
}

void delete_chrpc_endpoint(chrpc_endpoint_t *ep) {
    safe_free(ep->name);

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

    if (num_endpoints < 1 || CHRPC_ENDPOINT_SET_MAX_SIZE < num_endpoints) {
        delete_list(l);
        return NULL;
    }

    chrpc_endpoint_t **ep_arr =
        (chrpc_endpoint_t **)delete_and_move_list(l);

    return new_chrpc_endpoint_set(ep_arr, num_endpoints);
}

void delete_chrpc_endpoint_set(chrpc_endpoint_set_t *ep_set) {
    delete_hash_map(ep_set->name_map);

    for (size_t i = 0; i < ep_set->num_endpoints; i++) {
        delete_chrpc_endpoint(ep_set->endpoints[i]);
    }

    safe_free(ep_set->endpoints);
}

const chrpc_endpoint_t *chrpc_endpoint_set_lookup(chrpc_endpoint_set_t *ep_set, const char *name) {
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
