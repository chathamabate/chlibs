
#include "simple_rpc.h"
#include "chrpc/channel_local.h"
#include "chrpc/channel_local2.h"
#include "chsys/wrappers.h"
#include "chutil/list.h"
#include "chutil/string.h"
#include "unity/unity.h"

#include <_types/_uint32_t.h>
#include <pthread.h>

#include "chrpc/channel.h"
#include "chrpc/rpc.h"
#include "chrpc/serial_type.h"
#include "chrpc/serial_value.h"

#include "chsys/mem.h"

typedef struct _basic_message_t {
    uint32_t sender_id;
    string_t *msg;
} basic_message_t;

basic_message_t *new_basic_message(uint32_t id, const char *msg) {
    basic_message_t *bm = (basic_message_t *)safe_malloc(sizeof(basic_message_t));

    bm->sender_id = id;
    bm->msg = new_string_from_cstr(msg);

    return bm;
}

void delete_basic_message(basic_message_t *bm) {
    delete_string(bm->msg);
    safe_free(bm);
}

typedef struct _basic_server_state_t {
    pthread_mutex_t mut;

    // This is going to be a capacity-less queue of <basic_message_t *>
    list_t *q;
}  basic_server_state_t;

// Takes a user ID and a message, returns the size of the queue.
// (u32, str) => u32
static chrpc_server_command_t basic_server_push_msg(basic_server_state_t *bss, chrpc_value_t **ret, chrpc_value_t **args, uint32_t num_args) {
    (void)num_args;

    pthread_mutex_lock(&(bss->mut));

    basic_message_t *msg = new_basic_message(args[0]->value->u32, 
            args[1]->value->str);

    l_push(bss->q, &msg);

    uint32_t q_size = l_len(bss->q);
    *ret = new_chrpc_u32_value(q_size);

    pthread_mutex_unlock(&(bss->mut));

    return CHRPC_SC_KEEP_ALIVE;
}

// Give the number you'd like to poll, this returns at most that many messages as structs.
// (u32) => {u32; str;}[]
static chrpc_server_command_t basic_server_poll_msgs(basic_server_state_t *bss, chrpc_value_t **ret, chrpc_value_t **args, uint32_t num_args) {
    (void)num_args;

    uint32_t requested = args[0]->value->u32;

    pthread_mutex_lock(&(bss->mut));

    uint32_t given = l_len(bss->q) < requested ? l_len(bss->q) : requested;
    chrpc_value_t **eles = (chrpc_value_t **)safe_malloc(given * sizeof(chrpc_value_t *));

    for (uint32_t i = 0; i < given; i++) {
        basic_message_t *msg;
        l_poll(bss->q, &msg);

        eles[i] = new_chrpc_struct_value_va(
            new_chrpc_u32_value(msg->sender_id),
            new_chrpc_str_value(s_get_cstr(msg->msg))
        );

        delete_basic_message(msg);
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
            "poll_msgs",
            (chrpc_endpoint_ft)basic_server_poll_msgs,

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
    safe_pthread_mutex_init(&(bss->mut), NULL);
    bss->q = new_list(LINKED_LIST_IMPL, sizeof(basic_message_t *));

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
    basic_message_t **iter;
    while ((iter = l_next(bss->q))) {
        delete_string((*iter)->msg);
        safe_free(*iter);
    }

    delete_list(bss->q);
    safe_pthread_mutex_destroy(&(bss->mut));
    safe_free(bss);
}

static void test_basic_server_creation(void) {
    chrpc_server_t *server = new_basic_server();
    delete_basic_server(server);
}

static chrpc_status_t basic_client_push_msg(chrpc_client_t *client, uint32_t *len, uint32_t id, const char *msg) {
    chrpc_status_t status;

    chrpc_value_t *ret_val;

    chrpc_value_t *arg0 = new_chrpc_u32_value(id);
    chrpc_value_t *arg1 = new_chrpc_str_value(msg);

    status = chrpc_client_send_request_va(client, "push_msg", &ret_val, arg0, arg1);
    
    delete_chrpc_value(arg0);
    delete_chrpc_value(arg1);

    *len = ret_val->value->u32;

    delete_chrpc_value(ret_val);

    return status;
}

static chrpc_status_t basic_client_poll_msgs(chrpc_client_t *client, list_t **l, uint32_t num) {
    chrpc_status_t status;

    chrpc_value_t *ret_val;

    chrpc_value_t *arg0 = new_chrpc_u32_value(num);

    status = chrpc_client_send_request_va(client, "poll_msgs", &ret_val, arg0);

    delete_chrpc_value(arg0);

    list_t *ret_l = new_list(ARRAY_LIST_IMPL, sizeof(basic_message_t *));
    for (uint32_t i = 0; i < ret_val->value->array_len; i++) {
        basic_message_t *cell = new_basic_message(
            ret_val->value->array_entries[i]->struct_entries[0]->u32,
            ret_val->value->array_entries[i]->struct_entries[1]->str
        );

        l_push(ret_l, &cell);
    }

    *l = ret_l;

    delete_chrpc_value(ret_val);

    return status;
}

static const channel_local_config_t BASIC_CHN_CFG = {
    .max_msg_size =  0x1000,
    .queue_depth = 1,
    .write_over = false
};

static const chrpc_client_attrs_t BASIC_CLIENT_ATTRS = {
    .cadence = 50000,
    .timeout = 3000000
};

static void test_basic_server_correct_usage(void) {
    chrpc_status_t status;

    channel_local2_core_t *core;
    chrpc_client_t *client;

    chrpc_server_t *server = new_basic_server();

    status = new_chrpc_local_client(server, &BASIC_CHN_CFG, &BASIC_CLIENT_ATTRS, &core, &client);
    TEST_ASSERT_TRUE(status == CHRPC_SUCCESS);

    // Simple Message Push.
    uint32_t len;
    list_t *l;
    basic_message_t *msg;

    status = basic_client_push_msg(client, &len, 0, "Hello");
    TEST_ASSERT_TRUE(status == CHRPC_SUCCESS);
    TEST_ASSERT_EQUAL_UINT32(1, len);

    status = basic_client_push_msg(client, &len, 5, "GoodBye");
    TEST_ASSERT_TRUE(status == CHRPC_SUCCESS);
    TEST_ASSERT_EQUAL_UINT32(2, len);

    status = basic_client_poll_msgs(client, &l, 1);
    TEST_ASSERT_TRUE(status == CHRPC_SUCCESS);
    TEST_ASSERT_EQUAL_size_t(1, l_len(l));

    l_poll(l, &msg);
    delete_list(l);

    TEST_ASSERT_EQUAL_UINT32(0, msg->sender_id);
    TEST_ASSERT_EQUAL_STRING("Hello", s_get_cstr(msg->msg));
    delete_basic_message(msg);

    status = basic_client_push_msg(client, &len, 3, "AyeYo");
    TEST_ASSERT_TRUE(status == CHRPC_SUCCESS);
    TEST_ASSERT_EQUAL_UINT32(2, len);

    status = basic_client_poll_msgs(client, &l, 2);
    TEST_ASSERT_TRUE(status == CHRPC_SUCCESS);
    TEST_ASSERT_EQUAL_size_t(2, l_len(l));

    l_poll(l, &msg);
    TEST_ASSERT_EQUAL_UINT32(5, msg->sender_id);
    TEST_ASSERT_EQUAL_STRING("GoodBye", s_get_cstr(msg->msg));
    delete_basic_message(msg);

    l_poll(l, &msg);
    TEST_ASSERT_EQUAL_UINT32(3, msg->sender_id);
    TEST_ASSERT_EQUAL_STRING("AyeYo", s_get_cstr(msg->msg));
    delete_basic_message(msg);

    delete_list(l);

    delete_basic_server(server);
    delete_chrpc_client(client);
    delete_channel_local2_core(core);
}

static void test_basic_server_incorrect_usage(void) {
    chrpc_status_t status;

    channel_local2_core_t *core;
    chrpc_client_t *client;

    chrpc_server_t *server = new_basic_server();

    status = new_chrpc_local_client(server, &BASIC_CHN_CFG, &BASIC_CLIENT_ATTRS, &core, &client);
    TEST_ASSERT_TRUE(status == CHRPC_SUCCESS);

    status = chrpc_client_send_argless_request(client, "Not a function", NULL);
    TEST_ASSERT_TRUE(status != CHRPC_SUCCESS);

    // Incorrect number of args.
    status = chrpc_client_send_argless_request(client, "push_msg", NULL);
    TEST_ASSERT_TRUE(status != CHRPC_SUCCESS);

    chrpc_value_t *ret_val;

    // Incorrect arg types.
    chrpc_value_t *arg0 = new_chrpc_u32_value(123);
    chrpc_value_t *arg1 = new_chrpc_u32_value(123);

    status = chrpc_client_send_request_va(client, "push_msg", &ret_val, arg0, arg1);
    TEST_ASSERT_TRUE(status != CHRPC_SUCCESS);

    delete_chrpc_value(arg0);
    delete_chrpc_value(arg1);

    delete_basic_server(server);
    delete_chrpc_client(client);
    delete_channel_local2_core(core);
}

// Could we maybe try some parrallel usage case?

void chrpc_simple_rpc_tests(void) {
    RUN_TEST(test_basic_server_creation);
    RUN_TEST(test_basic_server_correct_usage);
    RUN_TEST(test_basic_server_incorrect_usage);
}
