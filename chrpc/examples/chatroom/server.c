
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

// Args: Nothing; Returns: {Byte general_message, String sender, String message_text}[]
// (If ID is not logged in, no messages will be returned as id has no mailbox)
static chrpc_server_command_t chatroom_poll(channel_id_t id, chatroom_state_t *cs, chrpc_value_t **ret, chrpc_value_t **args, uint32_t num_args) {
    (void)args;
    (void)num_args;
    
    return CHRPC_SC_KEEP_ALIVE;
}

static void chatroom_on_disconnect(channel_id_t id, chatroom_state_t *cs) {
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
            (chrpc_endpoint_ft)chatroom_login,

            new_chrpc_struct_type(
                CHRPC_BYTE_T,       // true on success, false on error.
                CHRPC_STRING_T      // if error, this contains error message.
            ),   

            CHRPC_STRING_T  // The username to login with.
        ),
        new_chrpc_endpoint_va(
            "send_message",   // name.
            (chrpc_endpoint_ft)chatroom_send_message,

            new_chrpc_struct_type(
                CHRPC_BYTE_T,       // true on success, false on error.
                CHRPC_STRING_T      // if error, this contains error message.
            ),   

            CHRPC_STRING_T, // If empty, send to everyone,
                            // otherwise, this should be the username of who to send to.

            CHRPC_STRING_T  // The message it self.
        ),
        new_chrpc_endpoint_argless(
            "poll",
            (chrpc_endpoint_ft)chatroom_poll,

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
