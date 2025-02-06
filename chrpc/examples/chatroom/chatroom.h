
#ifndef CHATROOM_H
#define CHATROOM_H

#include <stdbool.h>
#include "chutil/string.h"
#include "chutil/list.h"
#include "chrpc/serial_value.h"
#include "chrpc/serial_type.h"
#include "chrpc/rpc_server.h"
#include "chutil/map.h"
#include <pthread.h>

#define CHATROOM_PORT 5656

#define CHATROOM_MAX_MESSAGE_LEN 0x100

typedef uint8_t chatroom_status_t;

#define CHATROOM_SUCCESS 0
#define CHATROOM_CHANNEL_ALREADY_REGISTERED 1
#define CHATROOM_USERNAME_TAKEN 2
#define CHATROOM_INVALID_USERNAME 3
#define CHATROOM_CHANNEL_NOT_REGISTERED 4
#define CHATROOM_INVALID_MESSAGE_LEN 5
#define CHATROOM_UNKNOWN_RECEIVER 6

const char *chatroom_status_as_literal(chatroom_status_t status);

typedef struct _chatroom_message_t {
    bool general_msg;    // false = private message.
    string_t *user;      // Could be sender or receiver...
    string_t *msg;
} chatroom_message_t;

// This assumes ownership of both given strings.
chatroom_message_t *new_chatroom_message(bool general_message, string_t *user, string_t *msg);
void delete_chatroom_message(chatroom_message_t *cm);

typedef struct _chatroom_mailbox_t {
    pthread_mutex_t mb_mut;
    // List<chatroom_message_t *>
    list_t *mb;
} chatroom_mailbox_t;

chatroom_mailbox_t *new_chatroom_mailbox(void);
void delete_chatroom_mailbox(chatroom_mailbox_t *mb);

// Assumes ownership of given message.
void chatroom_mailbox_push(chatroom_mailbox_t *mb, chatroom_message_t *msg);
uint32_t chatroom_mailbox_poll(chatroom_mailbox_t *mb, chatroom_message_t **out_buf, uint32_t out_buf_len);

typedef struct _chatroom_state_t {
    // Basically, to allow for workers to actually do things in parallel,
    // We promise that the structure of the id map and the structure of the mailboxes map
    // will only ever change when the global write lock is held.
    //
    // If you just want to use a singular mailbox, or lookup a username, you just need the 
    // global read lock. (Followed by the individual mailbox lock if applicable)
    pthread_rwlock_t global_lock;

    // NOTE: The string values in id_map are the same as the string keys in the mailbox map.

    // Map<channel_id_t, string_t *>
    hash_map_t *id_map; 

    // Map<string_t *, chatroom_mailbox_t *>
    hash_map_t *mailboxes;
} chatroom_state_t;

chatroom_state_t *new_chatroom_state(void);
void delete_chatroom_state(chatroom_state_t *cs);

chatroom_status_t chatroom_login(chatroom_state_t *cs, channel_id_t id, const char *username);
chatroom_status_t chatroom_logout(chatroom_state_t *cs, channel_id_t id);

// Returns NULL if no username, otherwise returns a NEW string containing the username.
//
// NOTE: Be careful with this function, it is always possible the user logs out sometime
// shortly after making this call. 
string_t *chatroom_get_username(chatroom_state_t *cs, channel_id_t id);

// string_t *'s are used where the string values will be efficiently copied.
// Given string_t *'s are not consumed in any way.

chatroom_status_t chatroom_send_global_msg_from_id(chatroom_state_t *cs, channel_id_t id, string_t *msg);
chatroom_status_t chatroom_send_global_msg(chatroom_state_t *cs, string_t *sender, string_t *msg);

chatroom_status_t chatroom_send_private_msg_from_id(chatroom_state_t *cs, const char *receiver, channel_id_t id, string_t *msg);
chatroom_status_t chatroom_send_private_msg(chatroom_state_t *cs, const char *receiver, string_t *sender, string_t *msg);

chatroom_status_t chatroom_poll(chatroom_state_t *cs, channel_id_t id, chatroom_message_t **out_buf, uint32_t out_buf_len, uint32_t *written);

#endif
