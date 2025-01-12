
#include "chsys/mem.h"
#include "chutil/list.h"
#include "chrpc/rpc.h"
#include <string.h>
#include <stdarg.h>

chrpc_endpoint_t *new_chrpc_endpoint(const char *n, chrpc_endpoint_ft f, chrpc_type_t *rt, chrpc_type_t **args, uint32_t num_args) {
    // Copy name into a new dynamic buffer.
    size_t slen = strlen(n);
    char *dyna_name = (char *)safe_malloc(sizeof(char) * (slen + 1));
    strcpy(dyna_name, n);

    chrpc_endpoint_t *ep = (chrpc_endpoint_t *)safe_malloc(sizeof(chrpc_endpoint_t));

    ep->name = dyna_name;
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

    if (num_args < 1 || CHRPC_ENDPOINT_MAX_ARGS < num_args) {
        delete_list(l);
        return NULL;
    }

    chrpc_type_t **args_arr = (chrpc_type_t **)delete_and_move_list(l);

    return new_chrpc_endpoint(n, f, rt, args_arr, num_args); 
}

void delete_chrpc_endpoint(chrpc_endpoint_t *ep) {
    safe_free(ep->name);
    
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
