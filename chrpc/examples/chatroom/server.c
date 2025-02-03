
#include <stdio.h>

#include "chatroom.h"

#include "chrpc/rpc_server.h"
#include "chrpc/serial_value.h"
#include "chsys/sys.h"
#include "chsys/wrappers.h"
#include "chsys/mem.h"
#include "chsys/log.h"

#include "chutil/map.h"
#include "chutil/string.h"
#include "chutil/list.h"
#include <inttypes.h>

#include <stdbool.h>
#include <pthread.h>
#include <string.h>

static chrpc_value_t *new_chatroom_server_success(void) {
    return new_chrpc_struct_value_va(
        new_chrpc_b8_value(0),
        new_chrpc_str_value("")
    );
}

static chrpc_value_t *new_chatroom_server_error(const char *msg) {
    return new_chrpc_struct_value_va(
        new_chrpc_b8_value(1),  // Non-zero means error!
        new_chrpc_str_value(msg)
    );
}

// Args: String username; Returns: {Byte error_code, String Message} 
static chrpc_server_command_t chatroom_login_ep(channel_id_t id, chatroom_state_t *cs, chrpc_value_t **ret, chrpc_value_t **args, uint32_t num_args) {
    (void)num_args;

    const char *username = args[0]->value->str;
    chatroom_status_t status = chatroom_login(cs, id, username);  

    if (status == CHATROOM_SUCCESS) {
        log_info("Channel %" PRIu64 " logged in as %s", id, username);

        string_t *root_sender = new_string_from_literal("$ROOT");

        string_t *login_msg = new_string();
        s_append_cstr(login_msg, username);
        s_append_cstr(login_msg, " has logged in");

        chatroom_send_global_msg(cs, root_sender, login_msg);

        delete_string(root_sender);
        delete_string(login_msg);

        *ret = new_chatroom_server_success();
    } else {
        *ret = new_chatroom_server_error(chatroom_status_as_literal(status));
    }

    return CHRPC_SC_KEEP_ALIVE;
}

// Args: String message; Returns: {Byte error_code, String Message} 
static chrpc_server_command_t chatroom_send_global_msg_ep(channel_id_t id, chatroom_state_t *cs, chrpc_value_t **ret, chrpc_value_t **args, uint32_t num_args) {
    (void)num_args;

    const char *msg_cstr = args[0]->value->str;

    string_t *msg = new_string_from_cstr(msg_cstr);
    chatroom_status_t status = chatroom_send_global_msg_from_id(cs, id, msg);
    delete_string(msg);

    if (status == CHATROOM_SUCCESS) {
        *ret = new_chatroom_server_success();
    } else {
        *ret = new_chatroom_server_error(chatroom_status_as_literal(status));
    }

    return CHRPC_SC_KEEP_ALIVE;
}

// Args: String receiver, String message; Returns: {Byte error_code, String Message} 
static chrpc_server_command_t chatroom_send_private_msg_ep(channel_id_t id, chatroom_state_t *cs, chrpc_value_t **ret, chrpc_value_t **args, uint32_t num_args) {
    (void)num_args;

    const char *receiver_cstr = args[0]->value->str;

    const char *msg_cstr = args[1]->value->str;
    string_t *msg = new_string_from_cstr(msg_cstr);

    chatroom_status_t status = chatroom_send_private_msg_from_id(cs, receiver_cstr, id, msg);

    if (status == CHATROOM_SUCCESS) {
        *ret = new_chatroom_server_success();
    } else {
        *ret = new_chatroom_server_error(chatroom_status_as_literal(status));
    }

    return CHRPC_SC_KEEP_ALIVE;
}

// Args: Nothing; Returns: {Byte general_message, String sender, String message_text}[]
// (If ID is not logged in, no messages will be returned as id has no mailbox)
static chrpc_server_command_t chatroom_poll_ep(channel_id_t id, chatroom_state_t *cs, chrpc_value_t **ret, 
        chrpc_value_t **args, uint32_t num_args) {
    (void)args;
    (void)num_args;
    
    const uint32_t poll_size = 5;
    chatroom_message_t **msg_buf = (chatroom_message_t **)safe_malloc(sizeof(chatroom_message_t *) * poll_size);

    uint32_t written;
    chatroom_status_t status = chatroom_poll(cs, id, msg_buf, poll_size, &written);

    if (status == CHATROOM_SUCCESS) {
        chrpc_value_t **rpc_msg_buf = (chrpc_value_t **)safe_malloc(sizeof(chrpc_value_t *) * written);
        
        for (uint32_t i = 0; i < written; i++) {
            chatroom_message_t *msg = msg_buf[i];

            rpc_msg_buf[i] = new_chrpc_struct_value_va(
                new_chrpc_b8_value(msg->general_msg),
                new_chrpc_str_value(s_get_cstr(msg->sender)),
                new_chrpc_str_value(s_get_cstr(msg->msg))
            );

            delete_chatroom_message(msg);
        }

        *ret = new_chrpc_composite_nempty_array_value(rpc_msg_buf, written);
    } else {
        *ret = new_chrpc_composite_empty_array_value(
            new_chrpc_struct_type(
                CHRPC_BYTE_T,
                CHRPC_STRING_T,
                CHRPC_STRING_T
            )
        );
    }

    safe_free(msg_buf);

    return CHRPC_SC_KEEP_ALIVE;
}

static void chatroom_on_disconnect(channel_id_t id, chatroom_state_t *cs) {
    string_t *username = chatroom_get_username(cs, id);
    chatroom_status_t status = chatroom_logout(cs, id);

    if (username) {
        if (status == CHATROOM_SUCCESS) {
            log_info("Channel %" PRIu64 " logged out from %s", id, username);

            string_t *root_sender = new_string_from_literal("$ROOT");

            string_t *logout_msg = new_string();
            s_append_string(logout_msg, username);
            s_append_cstr(logout_msg, " has logged out");

            chatroom_send_global_msg(cs, root_sender, logout_msg);

            delete_string(root_sender);
            delete_string(logout_msg);
        }
        delete_string(username);
    }
}

void run_chat_server(void) {
    chatroom_state_t *cs = new_chatroom_state();

    chrpc_server_t *server;

    chrpc_server_attrs_t attrs = {
        .max_msg_size = 0x1000,
        .max_connections = 20,
        .num_workers = 4,
        .worker_usleep_amt = 1000, // 1ms
        .idle_timeout = 5,
        .on_disconnect = (chrpc_server_disconnect_ft)chatroom_on_disconnect
    };

    chrpc_endpoint_set_t *eps = new_chrpc_endpoint_set_va(
        new_chrpc_endpoint_va(
            "login",
            (chrpc_endpoint_ft)chatroom_login_ep,

            new_chrpc_struct_type(
                CHRPC_BYTE_T,       // non-zero on error.
                CHRPC_STRING_T      // if error, this contains error message.
            ),   

            CHRPC_STRING_T  // The username to login with.
        ),
        new_chrpc_endpoint_va(
            "send_global_message",   // name.
            (chrpc_endpoint_ft)chatroom_send_global_msg_ep,

            new_chrpc_struct_type(
                CHRPC_BYTE_T,       // non-zero on error.
                CHRPC_STRING_T      // if error, this contains error message.
            ),   

            CHRPC_STRING_T  // The message to send.
        ),
        new_chrpc_endpoint_va(
            "send_private_message",   // name.
            (chrpc_endpoint_ft)chatroom_send_private_msg_ep,

            new_chrpc_struct_type(
                CHRPC_BYTE_T,       // non-zero on error.
                CHRPC_STRING_T      // if error, this contains error message.
            ),   

            CHRPC_STRING_T, // The receiver.
            CHRPC_STRING_T  // The message to send.
        ),
        new_chrpc_endpoint_argless(
            "poll",
            (chrpc_endpoint_ft)chatroom_poll_ep,

            // Array of messages received.
            new_chrpc_array_type(
                new_chrpc_struct_type(
                    CHRPC_BYTE_T,  // true for general message, 
                                   // false for private message.
                                   
                    CHRPC_STRING_T, // Sender
                    CHRPC_STRING_T  // Message
                )
            )
        )
    );

    chrpc_status_t status = new_chrpc_server(&server, cs, &attrs, eps);
    if (status != CHRPC_SUCCESS) {
        log_fatal("Failed to create server");
    }

    delete_chrpc_server(server);
    delete_chatroom_state(cs);
}


int main(void) {
    sys_init();
    run_chat_server();
    safe_exit(0);
}
