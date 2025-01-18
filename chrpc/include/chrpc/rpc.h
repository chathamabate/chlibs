
#ifndef CHRPC_RPC_H
#define CHRPC_RPC_H

#include "chrpc/channel.h"
#include "chrpc/serial_type.h"
#include "chrpc/serial_value.h"
#include "chutil/map.h"
#include "chutil/queue.h"
#include "chutil/string.h"

#include <pthread.h>

#define CHRPC_ENDPOINT_MAX_ARGS 10
#define CHRPC_ENDPOINT_SET_MAX_SIZE 300
#define CHRPC_SERVER_BUF_MIN_SIZE 200

typedef enum _chrpc_server_command_t {

    // Destory the channel after sending across a return value.
    CHRPC_SC_DISCONNECT = 0,

    // Return a channel to the channel queue after sending across a return value.
    CHRPC_SC_KEEP_ALIVE,

    // Maybe add more...
} chrpc_server_command_t;

// An RPC server will have somesort of global state that will be passed into every 
// endpoint function. The user can choose what the type of this global state is.
// NOTE: With multiple workers, it is possible/likely endpoints will execute in parallel.
// It will be your responsibility to synchronize the server state!
//
// The arguments passed to this endpoint will match the type signature of the endpoint.
// NOTE: IMPORTANT!! The given values will be deleted by the server AFTER returning form the endpoint.
// Additionally, it is expected that the endpoint returns a NEW value (Not one from the args array).
// This value will also be delete by the server once it is sent across the corresponding channel.
//
// The "returned" CHRPC value should be written to ret.
// If this is a "void" function, NULL should be written to ret.
//
// The command returned determines server behavoir after sending back the return value.
// For example, maybe this is a "disconnect" endpoint, in which case CHRPC_SC_DISCONNECT should be returned.
// This tells the server to destroy the channel instead of keeping it around.
typedef chrpc_server_command_t (*chrpc_endpoint_ft)(void *server_state, chrpc_value_t **ret, chrpc_value_t **args, uint32_t num_args);

typedef struct _chrpc_endpoint_t {
    // Going to use chutil string here because it helps with hashing later.
    string_t *name;

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

static inline chrpc_endpoint_t *new_chrpc_endpoint_argless(const char *n, chrpc_endpoint_ft f, chrpc_type_t *rt) {
    return _new_chrpc_endpoint_va(n, f, rt, NULL);
}

void delete_chrpc_endpoint(chrpc_endpoint_t *ep);

// An endpoint set MUST be non-empty.
typedef struct _chrpc_endpoint_set_t {
    // (const string_t *) => (chrpc_endpoint_t *)
    // (DOES NOT OWN ANY OF THE KEYS OR VALUES)
    hash_map_t *name_map;

    chrpc_endpoint_t **endpoints;
    size_t num_endpoints;
} chrpc_endpoint_set_t;

// Given array should live entirely in dynamic memory and will be OWNED
// by returned endpoint set.
chrpc_endpoint_set_t *new_chrpc_endpoint_set(chrpc_endpoint_t **eps, size_t num_eps);

// Expects a NULL terminated sequence of chrpc_endpoint_t *'s
chrpc_endpoint_set_t *_new_chrpc_endpoint_set_va(int dummy, ...);
#define new_chrpc_endpoint_set_va(...) \
    _new_chrpc_endpoint_set_va(0, __VA_ARGS__, NULL)

void delete_chrpc_endpoint_set(chrpc_endpoint_set_t *ep_set);

// Returns NULL if name is non-existent in the endpoint set.
const chrpc_endpoint_t *chrpc_endpoint_set_lookup(const chrpc_endpoint_set_t *ep_set, const char *name);


typedef struct _chrpc_server_attrs_t {

    // NOTE: When we create our response, it must not be larger than the max message size of the 
    // client channel. If it is, we will need to send back some sort of error code.
    size_t max_msg_size;

    // max_connections cannot be less than the number of workers.
    // If this were allowed, there would always be idle workers.
    size_t max_connections;
    size_t num_workers;

    size_t worker_usleep_amt;

} chrpc_server_attrs_t;

typedef struct _chrpc_server_t {
    // User defined, can be NULL.
    // (Remember, if actually stateful, it is the user's responsibility to synchronize!)
    void *server_state;

    chrpc_server_attrs_t attrs;

    // NOTE: q_mut encloses num_channels and channels_q.
    // Worker threads will pop channels off of the channels queue.
    // When a channel is popped off the queue, it will be checked for incoming messages.
    // If there is an incoming message, it will be parsed and executed.
    // Then the corresponding return value will be sent back over the channel.
    //
    // If there is an error using the channel, it will be assumed that said channel has
    // disconnected. If so, it will be destroyed.
    // Otherwise, it will be pushed back onto the queue.
    pthread_mutex_t q_mut;
    size_t num_channels;
    queue_t *channels_q;

    // Workers will be spawned at server creation time.
    // Workers kinda connect channels and endpoints.. doing the work required
    // for processing individual requests.
    pthread_t *worker_ids;

    // Workers poll should_exit to determine when the should return.
    pthread_mutex_t should_exit_mut;
    bool should_exit;

    // While the "const" qualifier does not prevent the mutation of underlying structures
    // within the endpoint set, it's purpose is to declare the set as threadsafe for reading.
    // We should NEVER change the internals of this set once the server has been created.
    // Threads will be looking up endpoints in parallel, it is important that this always succeeds.
    //
    // TODO: Consider adding a mutex to be safe.
    //
    // NOTE: While this is marked const, it is OWNED by the server.
    // Thus, it will be deleted by the server when the server itself is deleted.
    const chrpc_endpoint_set_t *ep_set;

    // Same as ep_set, NEVER CHANGE THIS.
    //
    // This will be allocated for every server state.
    // It will point to the type that all requests should have.
    const chrpc_type_t *req_type;
} chrpc_server_t;

// NOTE: Here are some notes on server internal workflow...
//
// The server will enforce that all owned channel's have a max message size less
// than some known value. This gaurantees that incoming messages can always be 
// received by the server!
//
// Expected RPC message format type:
//
// {
//      str;    // function name
//      b8[][]; // Array of arguments (Must be parsed server side)
// }
//
// Expected RPC return message format:
//
// {
//      b8;     // Status
//      b8[];   // Return Value (Not populated if void function, or error)
// }
// 
// When receiving or sending messages, if there is a recoverable error,
// the error will be returned over the channel. The channel will remain in the server's
// channel queue.
//
// Recoverable Errors:
//  * The given function name doesn't match any endpoints.
//  * The given arguments, don't match the expected types.
//  * The given arguments are malformed and fail on parse.
//  * The return value is to large for the receiving channel.
//
// If there is a non-recoverable error, the channel will be destroyed and deleted from the
// server's queue.
//
// Non-Recoverable Errors:
//  * There is a fatal channel error.
//  * The channel's max-message-size is too small to even be sent an error message.

// As the chrpc server will spawn threads, it is possible this call fails.
// For example, if this process has already spawned the maximum number of threads.
//
// NOTE: max_cons must be >= workers, otherwise an error will be thrown.
//
// NOTE: The created server assumes ownership of the given endpoint set.
// When the server is deleted, so is the endpoint set.
chrpc_status_t new_chrpc_server(chrpc_server_t **server, void *ss, chrpc_server_attrs_t attrs, chrpc_endpoint_set_t *eps);
void delete_chrpc_server(chrpc_server_t *server);

// Returns CHRPC_SUCCESS, if ownership of the channel is successfully given to the server.
// Returns CHRPC_SERVER_FULL, if the server cannot accept anymroe channels at this time.
// In this case, it is the user's responsibility to cleanup the channel.
chrpc_status_t chrpc_server_give_channel(chrpc_server_t *server, channel_t *chn);


#endif
