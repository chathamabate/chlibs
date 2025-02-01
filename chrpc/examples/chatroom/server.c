
#include <stdio.h>

#include "chatroom.h"

#include "chrpc/rpc_server.h"
#include "chrpc/serial_value.h"
#include "chsys/wrappers.h"
#include "chsys/mem.h"

#include "chutil/map.h"
#include "chutil/string.h"
#include "chutil/list.h"

#include <stdbool.h>
#include <pthread.h>
#include <string.h>

typedef struct _chatroom_message_t {
    channel_id_t sender; // ID of sender.
    bool general_msg;    // false = private message.
    string_t *msg;
} chatroom_message_t;

static chatroom_message_t *new_chatroom_message(channel_id_t sender, bool general_message, const char *msg) {
    chatroom_message_t *cm = (chatroom_message_t *)safe_malloc(sizeof(chatroom_message_t));

    cm->sender = sender;
    cm->general_msg = general_message;
    cm->msg = new_string_from_cstr(msg);    // message will be copied entirely into heap memory.
    
    return cm;
}

static void delete_chatroom_message(chatroom_message_t *cm) {
    delete_string(cm->msg);
    safe_free(cm);
}

typedef struct _chatroom_mailbox_t {
    pthread_mutex_t mb_mut;
    // List<chatroom_message_t *>
    list_t *mb;
} chatroom_mailbox_t;

static chatroom_mailbox_t *new_chatroom_mailbox(void) {
    chatroom_mailbox_t *mb = (chatroom_mailbox_t *)safe_malloc(sizeof(chatroom_mailbox_t));

    safe_pthread_mutex_init(&(mb->mb_mut), NULL);
    mb->mb = new_list(LINKED_LIST_IMPL, sizeof(chatroom_message_t *));

    return mb;
}

static void delete_chatroom_mailbox(chatroom_mailbox_t *mb) {
    chatroom_message_t **iter;
    l_reset_iterator(mb->mb);
    while ((iter = l_next(mb->mb))) {
        delete_chatroom_message(*iter);
    }

    delete_list(mb->mb);
    safe_pthread_mutex_destroy(&(mb->mb_mut));
    safe_free(mb);
}

typedef struct _chatroom_state_t {
    // Basically, to allow for workers to actually do things in parallel,
    // We promise that the structure of the id map and the structure of the mailboxes map
    // will only ever change when the global write lock is held.
    //
    // If you just want to use a singular mailbox, or lookup a username, you just need the 
    // global read lock. (Followed by the individual mailbox lock if applicable)
    pthread_rwlock_t global_lock;

    // ID's to Usernames. (THIS MAP OWNS THE USERNAMES!)
    // Map<channel_id_t, string_t *>
    hash_map_t *id_map; 

    // Usernames to Mailboxes.  (THIS MAP OWNS THE MAILBOXES)
    // Map<string_t *, chatroom_mailbox_t *>
    hash_map_t *mailboxes;
} chatroom_state_t;

static chrpc_value_t *new_chatroom_error(const char *msg) {
    return new_chrpc_struct_value_va(
        new_chrpc_b8_value(1),
        new_chrpc_str_value(msg)
    );
}

static chrpc_value_t *new_chatroom_success(void) {
    return new_chrpc_struct_value_va(new_chrpc_b8_value(0), new_chrpc_str_value(""));
}

// Args: String username; Returns: {Byte error_code, String Message} 
static chrpc_server_command_t chatroom_login(channel_id_t id, chatroom_state_t *cs, chrpc_value_t **ret, chrpc_value_t **args, uint32_t num_args) {
    const char *username_cstr = args[0]->value->str;

    size_t username_len = strlen(username_cstr);
    if (username_len == 0) {
        *ret = new_chatroom_error("Empty usernames are not allowed");
        return CHRPC_SC_KEEP_ALIVE;     
    } 

    if (username_len > 20) {
        *ret = new_chatroom_error("Usernames cannot be more than 20 characters");
        return CHRPC_SC_KEEP_ALIVE;     
    } 

    if (username_cstr[0] == '$') {
        *ret = new_chatroom_error("Usernames cannot start with $");
        return CHRPC_SC_KEEP_ALIVE;     
    }

    string_t *new_username = new_string_from_cstr(username_cstr);

    safe_pthread_rwlock_wrlock(&(cs->global_lock));

    bool taken = hm_contains(cs->mailboxes, &new_username);

    if (taken) {
        delete_string(new_username);
        *ret = new_chatroom_error("Given username is already taken");
    } else {
        // Give ID Map ownership of new username.
        hm_put(cs->id_map, &id, &new_username);

        // Add new mailbox under given username.
        chatroom_mailbox_t *mb = new_chatroom_mailbox();
        hm_put(cs->mailboxes, &new_username, &mb);

        *ret = new_chatroom_success();
    }

    safe_pthread_rwlock_unlock(&(cs->global_lock));

    return CHRPC_SC_KEEP_ALIVE;
}

// Args: String recipient, String message; Returns: {Byte error_code, String Message} 
static chrpc_server_command_t chatroom_send_message(channel_id_t id, chatroom_state_t *cs, chrpc_value_t **ret, chrpc_value_t **args, uint32_t num_args) {
    const char *recip_cstr = args[0]->value->str;
    const char *msg_cstr = args[1]->value->str;

    size_t msg_len = strlen(msg_cstr);
    if (msg_len == 0) {
        *ret = new_chatroom_error("Empty messages not allowed");
        return CHRPC_SC_KEEP_ALIVE;
    }

    if (msg_len > 256) {
        *ret = new_chatroom_error("Messages cannot be more than 256 characters");
        return CHRPC_SC_KEEP_ALIVE;
    }

    // Now, try to send our message.

    safe_pthread_rwlock_rdlock(&(cs->global_lock));
    bool logged_in = hm_contains(cs->id_map, &id);
    safe_pthread_rwlock_unlock(&(cs->global_lock));

    if (!logged_in) {
        *ret = new_chatroom_error("Please login before sending messages");
        return CHRPC_SC_KEEP_ALIVE;
    }

    size_t recip_len = strlen(recip_cstr);

    if (recip_len == 0) {
        // This is slightly frustrating... When sending a message to every current user.
        // We must loop over the mailbox map. Doing so, uses the hashmap internal state.
        // Thus, only one thread can do this at a time. We need the global write lock.
        safe_pthread_rwlock_wrlock(&(cs->global_lock));
        key_val_pair_t kvp;
        hm_reset_iterator(cs->mailboxes);
        while ((kvp = hm_next_kvp(cs->mailboxes))) {
            chatroom_message_t *msg = new_chatroom_message(id, true, msg_cstr);
            chatroom_mailbox_t *mb = *(chatroom_mailbox_t **)kvp_val(cs->mailboxes, kvp);

            // Technically this lock is unecessary since we hold the global write lock,
            // but whatevs.
            safe_pthread_mutex_lock(&(mb->mb_mut));
            l_push(mb->mb, &msg);
            safe_pthread_mutex_unlock(&(mb->mb_mut));
        }
        safe_pthread_rwlock_unlock(&(cs->global_lock));

        *ret = new_chatroom_success();
        return CHRPC_SC_KEEP_ALIVE;
    }

    // Otherwise, we have an individual recipient.
    // We can use the global read lock.

    safe_pthread_rwlock_rdlock(&(cs->global_lock));
    string_t *recip = new_string_from_cstr(recip_cstr);

    chatroom_mailbox_t **_mb;
    _mb = hm_get(cs->mailboxes, &recip);
    delete_string(recip);

    if (!_mb)  {
        *ret = new_chatroom_error("Recipient could not be found");
    } else {
        chatroom_mailbox_t *mb = *_mb;
        chatroom_message_t *msg = new_chatroom_message(id, false, msg_cstr);

        safe_pthread_mutex_lock(&(mb->mb_mut));
        l_push(mb->mb, &msg);
        safe_pthread_mutex_unlock(&(mb->mb_mut));

        *ret = new_chatroom_success();
    }

    safe_pthread_rwlock_unlock(&(cs->global_lock));

    return CHRPC_SC_KEEP_ALIVE;
}

// Args: Nothing; Returns: {Byte general_message, String sender, String message_text}[]
// (If ID is not logged in, no messages will be returned as id has no mailbox)
static chrpc_server_command_t chatroom_poll(channel_id_t id, chatroom_state_t *cs, chrpc_value_t **ret, chrpc_value_t **args, uint32_t num_args) {
    safe_pthread_rwlock_rdlock(&(cs->global_lock));

    string_t **_username = (string_t **)hm_get(cs->id_map, &id);

    uint32_t num_msgs = 0;
    chrpc_value_t **msgs = NULL;

    if (_username) {
        string_t *username = *_username;
        chatroom_mailbox_t *mb = *(chatroom_mailbox_t **)hm_get(cs->mailboxes, &username);

        safe_pthread_mutex_lock(&(mb->mb_mut));

        num_msgs = l_len(mb->mb);
        if (num_msgs > 20) {
            num_msgs = 20;
        }

        msgs = (chrpc_value_t **)safe_malloc(sizeof(chrpc_value_t *) * num_msgs);

        for (uint32_t i = 0; i < num_msgs; i++) {
            chatroom_message_t *msg;
            l_poll(mb->mb, &msg);

            string_t **_sender_username = (string_t **)hm_get(cs->id_map, &(msg->sender));
            const char *sender_username = _sender_username ? s_get_cstr(*_sender_username) : "$UNKNOWN";

            msgs[i] = new_chrpc_struct_value_va(
                new_chrpc_b8_value(msg->general_msg),
                new_chrpc_str_value(sender_username),
                new_chrpc_str_value(s_get_cstr(msg->msg))
            );

            delete_chatroom_message(msg);
        }

        safe_pthread_mutex_unlock(&(mb->mb_mut));
    }

    safe_pthread_rwlock_unlock(&(cs->global_lock));


    *ret = new_chrpc_composite_nempty_array_value(msgs, num_msgs);
    
    return CHRPC_SC_KEEP_ALIVE;
}

static void chatroom_on_disconnect(channel_id_t id, chatroom_state_t *cs) {
    safe_pthread_rwlock_wrlock(&(cs->global_lock));

    string_t **_username = (string_t **)hm_get(cs->id_map)

    if () {

    }

    safe_pthread_rwlock_unlock(&(cs->global_lock));
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
                                   
                    CHRPC_STRING_T, // Sender
                    CHRPC_STRING_T  // Message
                )
            )
        )
    );
}


int main(void) {
    printf("Hello From Server\n");
}
