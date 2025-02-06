
#include "chatroom.h"
#include "chrpc/serial_value.h"
#include "chsys/mem.h"
#include "chutil/list.h"
#include "chutil/map.h"
#include "chutil/string.h"
#include "chsys/wrappers.h"
#include "chrpc/rpc_server.h"

const char *chatroom_status_as_literal(chatroom_status_t status) {
    switch (status) {
        case CHATROOM_SUCCESS: 
            return "Success";
        case CHATROOM_CHANNEL_ALREADY_REGISTERED: 
            return "Channel is already registered";
        case CHATROOM_USERNAME_TAKEN: 
            return "Username is taken";
        case CHATROOM_INVALID_USERNAME: 
            return "Given username is invalid";
        case CHATROOM_CHANNEL_NOT_REGISTERED: 
            return "Channel is not registered";
        case CHATROOM_INVALID_MESSAGE_LEN: 
            return "Invalid message length";
        case CHATROOM_UNKNOWN_RECEIVER: 
            return "Unknown receiver";
        default:
            return "Unknown status code";
    }
}

chatroom_message_t *new_chatroom_message(bool general_message, string_t *user, string_t *msg) {
    chatroom_message_t *cm = (chatroom_message_t *)safe_malloc(sizeof(chatroom_message_t));

    cm->general_msg = general_message;
    cm->user = user;
    cm->msg = msg;
    
    return cm;
}

void delete_chatroom_message(chatroom_message_t *cm) {
    delete_string(cm->user);
    delete_string(cm->msg);
    safe_free(cm);
}

chatroom_mailbox_t *new_chatroom_mailbox(void) {
    chatroom_mailbox_t *mb = (chatroom_mailbox_t *)safe_malloc(sizeof(chatroom_mailbox_t));

    safe_pthread_mutex_init(&(mb->mb_mut), NULL);
    mb->mb = new_list(LINKED_LIST_IMPL, sizeof(chatroom_message_t *));

    return mb;
}

void delete_chatroom_mailbox(chatroom_mailbox_t *mb) {
    chatroom_message_t **iter;
    l_reset_iterator(mb->mb);
    while ((iter = l_next(mb->mb))) {
        delete_chatroom_message(*iter);
    }

    delete_list(mb->mb);
    safe_pthread_mutex_destroy(&(mb->mb_mut));
    safe_free(mb);
}

void chatroom_mailbox_push(chatroom_mailbox_t *mb, chatroom_message_t *msg) {
    safe_pthread_mutex_lock(&(mb->mb_mut));
    l_push(mb->mb, &msg);
    safe_pthread_mutex_unlock(&(mb->mb_mut));
}

uint32_t chatroom_mailbox_poll(chatroom_mailbox_t *mb, chatroom_message_t **out_buf, uint32_t out_buf_len) {
    safe_pthread_mutex_lock(&(mb->mb_mut));

    uint32_t mb_len = (uint32_t)l_len(mb->mb);
    uint32_t pop_amt = mb_len < out_buf_len ? mb_len : out_buf_len;

    chatroom_message_t *msg;
    for (uint32_t i = 0; i < pop_amt; i++) {
        l_pop(mb->mb, &msg);
        out_buf[i] = msg;
    }

    safe_pthread_mutex_unlock(&(mb->mb_mut));
    return pop_amt;
}

chatroom_state_t *new_chatroom_state(void) {
    chatroom_state_t *cs = (chatroom_state_t *)safe_malloc(sizeof(chatroom_state_t));

    safe_pthread_rwlock_init(&(cs->global_lock), NULL);
    cs->id_map = new_hash_map(sizeof(channel_id_t), sizeof(string_t *), 
            chrpc_channel_id_hash_func, chrpc_channel_id_equals_func);
    cs->mailboxes = new_hash_map(sizeof(string_t *), sizeof(chatroom_mailbox_t *),
            (hash_map_hash_ft)s_indirect_hash, (hash_map_key_eq_ft)s_indirect_equals);

    return cs;
}

void delete_chatroom_state(chatroom_state_t *cs) {
    // This assumes no one else is using the chatroom state.
    // All worker threads are done working.
    safe_pthread_rwlock_destroy(&(cs->global_lock));

    key_val_pair_t kvp;

    // Remeber, the values inside the ID map are the Keys of the mailbox map.
    // Only delete these strings once.

    delete_hash_map(cs->id_map);

    hm_reset_iterator(cs->mailboxes);
    while ((kvp = hm_next_kvp(cs->mailboxes)) != HASH_MAP_EXHAUSTED) {
        delete_string(*(string_t **)kvp_key(cs->mailboxes, kvp));
        delete_chatroom_mailbox(*(chatroom_mailbox_t **)kvp_val(cs->mailboxes, kvp));
    }
    delete_hash_map(cs->mailboxes);

    safe_free(cs);
}

chatroom_status_t chatroom_login(chatroom_state_t *cs, channel_id_t id, const char *username) {
    size_t username_len = strlen(username);

    if (username_len == 0 || username_len > 20) {
        return CHATROOM_INVALID_USERNAME;
    }

    if (username[0] == '$') {
        return CHATROOM_INVALID_USERNAME;
    }

    chatroom_status_t status;

    safe_pthread_rwlock_wrlock(&(cs->global_lock));

    if (hm_contains(cs->id_map, &id)) {
        status = CHATROOM_CHANNEL_ALREADY_REGISTERED;

        goto end;     
    }

    string_t *new_username = new_string_from_cstr(username);

    if (hm_contains(cs->mailboxes, &new_username)) {
        delete_string(new_username);
        status = CHATROOM_USERNAME_TAKEN;

        goto end;
    } 

    // Our channel is yet to login AND the given username is available, Success!
        
    hm_put(cs->id_map, &id, &new_username);

    chatroom_mailbox_t *mb = new_chatroom_mailbox();

    // Slightly dangerous, we are using the same string pointer in both maps.
    // This is to help with logout logic.
    hm_put(cs->mailboxes, &new_username, &mb);
    
    status = CHATROOM_SUCCESS;

end:
    safe_pthread_rwlock_unlock(&(cs->global_lock));
    return status;
}

chatroom_status_t chatroom_logout(chatroom_state_t *cs, channel_id_t id) {
    chatroom_status_t status;

    safe_pthread_rwlock_wrlock(&(cs->global_lock));

    string_t **_username = (string_t **)hm_get(cs->id_map, &id);
    if (_username) {
        string_t *username = *_username;
        hm_remove(cs->id_map, &id);
        
        chatroom_mailbox_t *mb = *(chatroom_mailbox_t **)hm_get(cs->mailboxes, &username);
        hm_remove(cs->mailboxes, &username);

        delete_string(username);
        delete_chatroom_mailbox(mb);

        status = CHATROOM_SUCCESS;
    } else {
        status = CHATROOM_CHANNEL_NOT_REGISTERED;
    }

    safe_pthread_rwlock_unlock(&(cs->global_lock));
    return status;
}

string_t *chatroom_get_username(chatroom_state_t *cs, channel_id_t id) {
    string_t *username = NULL;

    safe_pthread_rwlock_rdlock(&(cs->global_lock));

    string_t **_username = (string_t **)hm_get(cs->id_map, &id);
    if (_username) {
        username = s_copy(*_username);
    }

    safe_pthread_rwlock_unlock(&(cs->global_lock));

    return username;
}

// Assumes global write lock is held.
static void _chatroom_send_global_msg(chatroom_state_t *cs, string_t *sender, string_t *msg) {
    key_val_pair_t kvp;
    hm_reset_iterator(cs->mailboxes);
    while ((kvp = hm_next_kvp(cs->mailboxes)) != HASH_MAP_EXHAUSTED) {
        chatroom_mailbox_t *mb = *(chatroom_mailbox_t **)kvp_val(cs->mailboxes, kvp);
        chatroom_mailbox_push(mb, new_chatroom_message(
            true,
            s_copy(sender),
            s_copy(msg)
        ));
    }
}

chatroom_status_t chatroom_send_global_msg_from_id(chatroom_state_t *cs, channel_id_t id, string_t *msg) {
    if (s_len(msg) == 0 || s_len(msg) > CHATROOM_MAX_MESSAGE_LEN) {
        return CHATROOM_INVALID_MESSAGE_LEN;
    }

    chatroom_status_t status;

    safe_pthread_rwlock_wrlock(&(cs->global_lock));

    string_t **_sender = (string_t **)hm_get(cs->id_map, &id);

    if (_sender) {
        string_t *sender = *_sender;

        _chatroom_send_global_msg(cs, sender, msg);

        status = CHATROOM_SUCCESS;
    } else {
        status = CHATROOM_CHANNEL_NOT_REGISTERED;
    }

    safe_pthread_rwlock_unlock(&(cs->global_lock));

    return status;
}

chatroom_status_t chatroom_send_global_msg(chatroom_state_t *cs, string_t *sender, string_t *msg) {
    if (s_len(msg) == 0 || s_len(msg) > CHATROOM_MAX_MESSAGE_LEN) {
        return CHATROOM_INVALID_MESSAGE_LEN;
    }

    safe_pthread_rwlock_wrlock(&(cs->global_lock));
    _chatroom_send_global_msg(cs, sender, msg);
    safe_pthread_rwlock_unlock(&(cs->global_lock));

    return CHATROOM_SUCCESS;
}

// Assumes global read lock is held!
static chatroom_status_t _chatroom_send_private_msg(chatroom_state_t *cs, const char *receiver, string_t *sender, string_t *msg) {
    string_t *receiver_str = new_string_from_literal(receiver);
    chatroom_mailbox_t **_mb = (chatroom_mailbox_t **)hm_get(cs->mailboxes, &receiver_str);
    delete_string(receiver_str);

    if (!_mb) {
        return CHATROOM_UNKNOWN_RECEIVER;
    }

    chatroom_mailbox_t *mb = *_mb;

    chatroom_mailbox_push(mb, new_chatroom_message(
        s_copy(sender),
        false,
        s_copy(msg)
    ));

    return CHATROOM_SUCCESS;
}

chatroom_status_t chatroom_send_private_msg_from_id(chatroom_state_t *cs, const char *receiver, channel_id_t id, string_t *msg) {
    if (s_len(msg) == 0 || s_len(msg) > CHATROOM_MAX_MESSAGE_LEN) {
        return CHATROOM_INVALID_MESSAGE_LEN;
    }

    chatroom_status_t status;
      
    safe_pthread_rwlock_rdlock(&(cs->global_lock));
    string_t **_sender = (string_t **)hm_get(cs->id_map, &id);

    if (_sender) {
        string_t *sender = *_sender;
        status = _chatroom_send_private_msg(cs, receiver, sender, msg);
    } else {
        status = CHATROOM_CHANNEL_NOT_REGISTERED;
    }

    safe_pthread_rwlock_unlock(&(cs->global_lock));

    return status;
}

chatroom_status_t chatroom_send_private_msg(chatroom_state_t *cs, const char *receiver, string_t *sender, string_t *msg) {
    if (s_len(msg) == 0 || s_len(msg) > CHATROOM_MAX_MESSAGE_LEN) {
        return CHATROOM_INVALID_MESSAGE_LEN;
    }

    chatroom_status_t status;
      
    safe_pthread_rwlock_rdlock(&(cs->global_lock));
    status = _chatroom_send_private_msg(cs, receiver, sender, msg);
    safe_pthread_rwlock_unlock(&(cs->global_lock));

    return status;
}

chatroom_status_t chatroom_poll(chatroom_state_t *cs, channel_id_t id, chatroom_message_t **out_buf, uint32_t out_buf_len, uint32_t *written) {
    if (out_buf_len == 0) {
        *written = 0;
        return CHATROOM_SUCCESS;
    }

    chatroom_status_t status;
      
    safe_pthread_rwlock_rdlock(&(cs->global_lock));
    string_t **_username = (string_t **)hm_get(cs->id_map, &id);

    if (_username) {
        string_t *username = *_username;
        chatroom_mailbox_t *mb = *(chatroom_mailbox_t **)hm_get(cs->mailboxes, &username);

        *written = chatroom_mailbox_poll(mb, out_buf, out_buf_len);
        status = CHATROOM_SUCCESS;

    } else {
        status = CHATROOM_CHANNEL_NOT_REGISTERED;
    }

    safe_pthread_rwlock_unlock(&(cs->global_lock));

    return status;
}
