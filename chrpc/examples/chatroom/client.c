
#include <stdio.h>

#include "chatroom.h"
#include "chrpc/channel.h"
#include "chrpc/channel_fd.h"
#include "chrpc/rpc_client.h"
#include "chrpc/rpc_server.h"
#include "chrpc/serial_value.h"
#include "chsys/log.h"
#include "chsys/sock.h"
#include "chsys/sys.h"
#include "chsys/wrappers.h"
#include "chutil/string.h"

#include <termios.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>

// Real time terminal stuff I took from ChatGPT TBH.
struct termios orig_termios;
static void enable_raw_mode(void) {
    tcgetattr(STDIN_FILENO, &orig_termios);

    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ICANON | ECHO);  // Disable canonical mode and echo
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}
static void disable_raw_mode(void) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

typedef struct _client_worker_state_t {
    pthread_mutex_t should_exit_mut;
    bool should_exit;

    // Messages to be sent to the server.
    chatroom_mailbox_t *send_q;

    // Messages received form the server.
    chatroom_mailbox_t *recv_q;

    // The client to be used!
    chrpc_client_t *client;
} client_worker_state_t;

static void *client_worker_routine(void *arg) {
    chrpc_status_t status;
    chrpc_value_t *ret;

    client_worker_state_t *cs = (client_worker_state_t *)arg;

    while (true) {
        bool se;

        safe_pthread_mutex_lock(&(cs->should_exit_mut));
        se = cs->should_exit;
        safe_pthread_mutex_unlock(&(cs->should_exit_mut));

        // If flagged to exit, exit.
        if (se) {
            return NULL;
        }

        chatroom_message_t *send_msg; 
        uint32_t should_send = chatroom_mailbox_poll(cs->send_q, &send_msg, 1);

        // If there is a message in the send queue, attempt to send it.
        if (should_send == 1) {
            bool global = send_msg->general_msg;

            chrpc_value_t *receiver = new_chrpc_str_value(s_get_cstr(send_msg->user));
            chrpc_value_t *msg_text = new_chrpc_str_value(s_get_cstr(send_msg->msg));

            delete_chatroom_message(send_msg);

            if (global) {
                status = chrpc_client_send_request_va(
                    cs->client,
                    "send_global_message",
                    &ret,
                    msg_text
                );
            } else {
                status = chrpc_client_send_request_va(
                    cs->client,
                    "send_private_message",
                    &ret,
                    receiver,
                    msg_text
                );
            }

            delete_chrpc_value(receiver);
            delete_chrpc_value(msg_text);

            if (status != CHRPC_SUCCESS) {
                log_fatal("Unexpected fatal error %u", status);
            } 

            if (ret->value->struct_entries[0]->b8) {
                chatroom_mailbox_push(
                    cs->recv_q,
                    new_chatroom_message(
                        false, 
                        new_string_from_literal("$ERROR"),
                        new_string_from_cstr(ret->value->struct_entries[1]->str)
                    )
                ); 
            }

            delete_chrpc_value(ret);
        }

        // Always try to poll!

        status = chrpc_client_send_argless_request(cs->client, "poll", &ret);

        if (status != CHRPC_SUCCESS) {
            log_fatal("Unexepcted fatal error %u", status);
        }

        for (uint32_t i = 0; i < ret->value->array_len; i++) {
            chrpc_inner_value_t *iv = ret->value->array_entries[i];

            chatroom_mailbox_push(
                cs->recv_q,
                new_chatroom_message(
                    iv->struct_entries[0]->b8 != 0,
                    new_string_from_cstr(iv->struct_entries[1]->str),
                    new_string_from_cstr(iv->struct_entries[2]->str)
                )
            );
        }

        delete_chrpc_value(ret);

        // Should be half a second.
        usleep(500000);
    }
}


int main(void) {
    /*
    enable_raw_mode();
    while (1) {
        char c;
        ssize_t r = read(STDIN_FILENO, &c, sizeof(char));

        if (r == 1) {
            printf("%c\n", c);
            if (c == '\n') {
                disable_raw_mode();
                return 0;
            }
        }

        sleep(1);
    }
    */

    sys_init();
    sys_set_sig_exit(false);

    int client_fd = client_connect("127.0.0.1", CHATROOM_PORT);

    if (client_fd == -1) {
        log_fatal("Failed to connect to server");
    }

    channel_fd_config_t cfg = {
        .max_msg_size = 0x1000,
        .read_chunk_size = 0x1000,
        .read_fd = client_fd,
        .write_fd = client_fd,
        .write_over = true,
        .queue_depth = 5
    };

    channel_t *chn;
    channel_status_t chn_status;

    chn_status = new_channel(CHANNEL_FD_IMPL, &chn, &cfg);

    if (chn_status != CHN_SUCCESS) {
        log_fatal("Failed to create channel from fd"); 
    }

    chrpc_client_attrs_t attrs = {
        .cadence = 20000,    // Poll every 20 ms.
        .timeout = 8000000   // 8 Second timeout.
    };

    chrpc_client_t *client;
    chrpc_status_t rpc_status = new_chrpc_client(&client, chn, &attrs);

    if (rpc_status != CHRPC_SUCCESS) {
        log_fatal("Failed to set up rpc client");
    }

    enable_raw_mode();


    // Hmmmm, I don't really know tbh...

    // Should I login or something???

    chatroom_mailbox_t *mb = new_chatroom_mailbox();


    while (!sys_sig_exit_requested()) {

        // So here, we need to both poll for messages and read input...

        
        sleep(1);
    }

    disable_raw_mode();

    safe_exit(0);
}
