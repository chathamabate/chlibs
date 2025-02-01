
#include <stdio.h>

#include "chatroom.h"

#include "chrpc/rpc_server.h"

#include "chutil/map.h"
#include "chutil/string.h"
#include <stdbool.h>

typedef struct _chatroom_message_t {
    bool general_msg;    // false = private message.
    channel_id_t sender; // ID of sender.
    string_t *msg;
} chatroom_message_t;

typedef struct _chatroom_state_t {
    // ID's to Usernames.
    // Map<channel_id_t, string_t *>
    hash_map_t id_map; 

    // Lot's of questions about how this should work TBH...

    // Usernames to Mailboxes. 
    // Map<string_t *, List<chatroom_message_t>>
    hash_map_t mailboxes;
} chatroom_state_t;

static chrpc_server_command_t chatroom_login(channel_id_t id, chatroom_state_t *cs, chrpc_value_t **ret, chrpc_value_t **args, uint32_t num_args) {
    return CHRPC_SC_KEEP_ALIVE;
}

static chrpc_server_command_t chatroom_send_message(channel_id_t id, chatroom_state_t *cs, chrpc_value_t **ret, chrpc_value_t **args, uint32_t num_args) {
    return CHRPC_SC_KEEP_ALIVE;
}

static chrpc_server_command_t chatroom_poll(channel_id_t id, chatroom_state_t *cs, chrpc_value_t **ret, chrpc_value_t **args, uint32_t num_args) {
    return CHRPC_SC_KEEP_ALIVE;
}


void new_chat_server(void) {
    chrpc_endpoint_set_t *ep = new_chrpc_endpoint_set_va(
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
                                   
                    CHRPC_STRING_T,
                    CHRPC_STRING_T
                )
            )
        )
    );
}


int main(void) {
    printf("Hello From Server\n");
}
