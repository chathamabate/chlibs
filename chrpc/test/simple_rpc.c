
#include "simple_rpc.h"
#include "chutil/list.h"
#include "chutil/string.h"
#include "unity/unity.h"

#include <pthread.h>

#include "chrpc/channel.h"
#include "chrpc/rpc.h"
#include "chrpc/serial_type.h"
#include "chrpc/serial_value.h"

#include "chsys/mem.h"

typedef struct _basic_server_message_t {
    uint32_t sender_id;
    string_t *msg;
} basic_server_message_t;

typedef struct _basic_server_state_t {
    pthread_mutex_t mut;

    // This is going to be a capacity-less queue of <basic_server_message_t *>
    list_t *q;
}  basic_server_state_t;

// Takes a user ID and a message, returns the size of the queue.
// (u32, str) => u32
static chrpc_server_command_t basic_server_push_msg(basic_server_state_t *bss, chrpc_value_t **ret, chrpc_value_t **args, uint32_t num_args) {
    (void)num_args;

    pthread_mutex_lock(&(bss->mut));

    basic_server_message_t *msg = (basic_server_message_t *)safe_malloc(sizeof(basic_server_message_t));

    msg->sender_id = args[0]->value->u32;
    msg->msg = new_string_from_cstr(args[1]->value->str);

    l_push(bss->q, &msg);

    uint32_t q_size = l_len(bss->q);
    *ret = new_chrpc_u32_value(q_size);

    pthread_mutex_unlock(&(bss->mut));

    return CHRPC_SC_KEEP_ALIVE;
}

// Give the number you'd like to pop, this returns at most that many messages as structs.
// (u32) => {u32; str;}[]
static chrpc_server_command_t basic_server_pop_msgs(basic_server_state_t *bss, chrpc_value_t **ret, chrpc_value_t **args, uint32_t num_args) {
    (void)num_args;

    uint32_t requested = args[0]->value->u32;

    pthread_mutex_lock(&(bss->mut));

    uint32_t given = l_len(bss->q) < requested ? l_len(bss->q) : requested;
    chrpc_value_t **eles = (chrpc_value_t **)safe_malloc(given * sizeof(chrpc_value_t *));

    for (uint32_t i = 0; i < given; i++) {
        basic_server_message_t *msg;
        l_pop(bss->q, &msg);

        eles[i] = new_chrpc_struct_value_va(
            new_chrpc_u32_value(msg->sender_id),
            new_chrpc_str_value(s_get_cstr(msg->msg))
        );

        delete_string(msg->msg);
        safe_free(msg);
    }

    *ret = new_chrpc_composite_nempty_array_value(eles, given);

    pthread_mutex_unlock(&(bss->mut));

    return CHRPC_SC_KEEP_ALIVE;
}

chrpc_server_t *new_basic_server(void) {
    chrpc_endpoint_set_t *eps = new_chrpc_endpoint_set_va(
        new_chrpc_endpoint_va(
            "push_msg",
            (chrpc_endpoint_ft)basic_server_push_msg,

            // Ret Value.
            CHRPC_UINT32_T,

            // Args.
            CHRPC_UINT32_T,
            CHRPC_STRING_T
        ),
        new_chrpc_endpoint_va(
            "pop_msgs",
            (chrpc_endpoint_ft)basic_server_pop_msgs,

            // Ret Value.
            new_chrpc_array_type(
                new_chrpc_struct_type(
                    CHRPC_UINT32_T,
                    CHRPC_STRING_T
                )
            ),

            // Args.
            CHRPC_UINT32_T
        )
    );

    basic_server_state_t *bss = (basic_server_state_t *)safe_malloc(sizeof(basic_server_state_t));
    pthread_mutex_init(&(bss->mut), NULL);
    bss->q = new_list(LINKED_LIST_IMPL, sizeof(basic_server_message_t *));

    chrpc_server_t *server; 

    chrpc_status_t status = new_chrpc_server(
        &server, bss, 
        (chrpc_server_attrs_t){
            .max_connections = 4,
            .max_msg_size = 0x1000,
            .num_workers = 3,
            .worker_usleep_amt = 100
        },
        eps
    );

    TEST_ASSERT_TRUE(status == CHRPC_SUCCESS);

    return server;
}

static void delete_basic_server(chrpc_server_t *server) {
    basic_server_state_t *bss = (basic_server_state_t *)chrpc_server_state(server); 
    delete_chrpc_server(server); // Always delete server before the server state.
    
    l_reset_iterator(bss->q);
    basic_server_message_t **iter;
    while ((iter = l_next(bss->q))) {
        delete_string((*iter)->msg);
        safe_free(*iter);
    }

    delete_list(bss->q);
    pthread_mutex_destroy(&(bss->mut));
    safe_free(bss);
}

static void test_basic_server_creation(void) {
    chrpc_server_t *server = new_basic_server();
    delete_basic_server(server);
}

void chrpc_simple_rpc_tests(void) {
    RUN_TEST(test_basic_server_creation);
}
