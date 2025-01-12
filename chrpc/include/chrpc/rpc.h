
#ifndef CHRPC_RPC_H
#define CHRPC_RPC_H

#include "chrpc/serial_type.h"
#include "chrpc/serial_value.h"

#define CHRPC_ENDPOINT_MAX_ARGS 10

// An RPC server will have somesort of global state that will be passed into every 
// endpoint function. The user can choose what the type of this global state is.
// NOTE: With multiple workers, it is possible/likely endpoints will execute in parallel.
// It will be your responsibility to synchronize the server state!

// The arguments passed to this endpoint will match the type signature of the endpoint.
// NOTE: IMPORTANT!! The given values will be deleted by the server AFTER returning form the endpoint.
// Additionally, it is expected that the endpoint returns a NEW value (Not one from the args array).
// This value will also be delete by the server once it is sent across the corresponding channel.
//
// If this is a "void" function, i.e. one with no return value, return NULL.
typedef chrpc_value_t *(*chrpc_endpoint_ft)(void *server_state, chrpc_value_t **args, uint32_t num_args);

typedef struct _chrpc_endpoint_t {
    char *name;

    // This will be NULL for functions with no return type.
    chrpc_type_t *ret;

    chrpc_type_t **args;
    uint8_t num_args;

    chrpc_endpoint_ft func;
} chrpc_endpoint_t;

// NOTE: The below function expects rt and args to both completely live in dynamic memory.
// n (i.e. the name of the endpoint) will be copied into a new dynamic buffer. This way literals can be used.
// The returned rpc_endpoint will OWN the given return type and args array.
//
// For a function with no return type, rt should be NULL.
// For a function with no arguments, args should be NULL.
chrpc_endpoint_t *new_chrpc_endpoint(const char *n, chrpc_endpoint_ft f, chrpc_type_t *rt, chrpc_type_t **args, uint32_t num_args);

// This expects that the last argument is followed by NULL.
// This should still work even if rt is NULL.
chrpc_endpoint_t *_new_chrpc_endpoint_va(const char *n, chrpc_endpoint_ft f, chrpc_type_t *rt, ...);
#define new_chrpc_endpoint_va(name, func, rt, ...) \
    _new_chrpc_endpoint_va(name, func, rt, __VA_ARGS__, NULL)

void delete_chrpc_endpoint(chrpc_endpoint_t *ep);



#endif
