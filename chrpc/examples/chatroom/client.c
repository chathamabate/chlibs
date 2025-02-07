
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
#include <fcntl.h>

static void set_stdin_non_blocking(void) {
    int stdin_flags = fcntl(STDIN_FILENO, F_GETFL);
    if (stdin_flags == -1) {
        log_fatal("Failed to get STDIN Flags");
    }

    if (fcntl(STDIN_FILENO, F_SETFL, stdin_flags | O_NONBLOCK) == -1) {
        log_fatal("Failed to set STDIN as non-blocking");
    }
}

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

// NOTE: This destroys the given buffer!
static void client_process_cmd_buffer(chatroom_mailbox_t *send_q, char *buf) {
    if (buf[0] == '\0') {
        return;
    }

    // A $ sign signifies a private message.
    // Try $<receiver> <msg> to send a private message.
    //
    // As an oversight, this message will only show up on the receiver's end.
    // A better chatroom would fix this!
    if (buf[0] == '$') {
        char *iter = buf + 1;

        // Go until a space or end of string.
        while (*iter != ' ' && *iter != '\0') {
            iter++;
        }

        // No private message to send, or no receiver was given
        if (iter == buf + 1 || *iter == '\0') {
            return;
        }

        *iter = '\0';

        iter++;

        // No message to send after the space!
        if (*iter == '\0') {
            return;
        }
        
        string_t *receiver = new_string_from_cstr(buf + 1);
        string_t *msg = new_string_from_cstr(iter);

        chatroom_mailbox_push(send_q, new_chatroom_message(
            false,
            receiver, 
            msg
        )); 

        // Clear buffer before returning.
        buf[0] = '\0';

        return;
    } 

    // Otherwise we have a general message!

    chatroom_mailbox_push(send_q, new_chatroom_message(
        true, 
        new_string_from_literal(""),
        new_string_from_cstr(buf)
    ));

    buf[0] = '\0';
}

#define ANSI_RESET_LINE "\033[2K\r"
#define CLIENT_PROMPT ANSI_BOLD ">" ANSI_RESET
#define CLIENT_PRINT_PROMPT() printf(ANSI_RESET_LINE CLIENT_PROMPT)

static void client_cmd_prompt_routine(chatroom_mailbox_t *send_q, chatroom_mailbox_t *recv_q) {
    enable_raw_mode();
    set_stdin_non_blocking();
    
    char cmd_buf[0x100] = "";
    const size_t cmd_buf_len = sizeof(cmd_buf);
    size_t cmd_buf_i = 0;

    printf("\n>");
    fflush(stdout);

    while (!sys_sig_exit_requested()) {
        chatroom_message_t *recv_msg;
        if (chatroom_mailbox_poll(recv_q, &recv_msg, 1) != 0) {
            printf(ANSI_RESET_LINE);
            printf(ANSI_RESET_LINE "%s\n", s_get_cstr(recv_msg->msg));
            delete_chatroom_message(recv_msg);

            CLIENT_PRINT_PROMPT();
            fflush(stdout);
        }

        char c;
        ssize_t r = read(STDIN_FILENO, &c, sizeof(char));
        if (r == 1) {
            if (c == '\n') {
                client_process_cmd_buffer(send_q, cmd_buf);
                cmd_buf_i = 0;
                CLIENT_PRINT_PROMPT();
            } else if (c == 127) { // A deletion.
                cmd_buf[cmd_buf_i] = '\0';

                if (cmd_buf_i > 0) {
                    cmd_buf_i--;
                }

                // Consider switching this to a more efficient terminal command.
                CLIENT_PRINT_PROMPT();
                printf("%s", cmd_buf);
            }else if (cmd_buf_i < cmd_buf_len - 1) {
                cmd_buf[cmd_buf_i++] = c;
                cmd_buf[cmd_buf_i] = '\0';
                putc(c, stdout); 
            }
            fflush(stdout);
        }

        usleep(50000);
    }

    disable_raw_mode();
}

int main(int argc, char **argv) {
    sys_init();
    sys_set_sig_exit(false);

    char *username;
    char *ip;

    if (argc < 2) {
        log_fatal("Expected Usage `$ client <username> [<ip>]`"); 
    }

    username = argv[1];
    ip = argc < 3 ? "127.0.0.1" : argv[2];

    int client_fd = client_connect(ip, CHATROOM_PORT);

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
    
    chrpc_value_t *login_ret;
    chrpc_value_t *username_val = new_chrpc_str_value(username);
    rpc_status = chrpc_client_send_request_va(client, "login", &login_ret, username_val);
    delete_chrpc_value(username_val);

    if (rpc_status != CHRPC_SUCCESS) {
        delete_chrpc_client(client);
        log_fatal("Failed to complete login request");
    }

    if (login_ret->value->struct_entries[0]->b8) {
        log_warn("%s", 
                login_ret->value->struct_entries[1]->str);

        delete_chrpc_client(client);
        delete_chrpc_value(login_ret);

        log_fatal("Failed to complete login request");
    }

    // Successful login, let's just delete this return value.
    delete_chrpc_value(login_ret);

    client_worker_state_t worker_state;
    safe_pthread_mutex_init(&(worker_state.should_exit_mut), NULL);
    worker_state.should_exit = false;
    worker_state.recv_q = new_chatroom_mailbox();
    worker_state.send_q = new_chatroom_mailbox();
    worker_state.client = client;

    pthread_t worker_id;
    safe_pthread_create(&worker_id, NULL, client_worker_routine, &worker_state);

    client_cmd_prompt_routine(worker_state.send_q, worker_state.recv_q);

    safe_pthread_mutex_lock(&(worker_state.should_exit_mut));
    worker_state.should_exit = true;
    safe_pthread_mutex_unlock(&(worker_state.should_exit_mut));
    safe_pthread_join(worker_id, NULL);

    delete_chatroom_mailbox(worker_state.recv_q);
    delete_chatroom_mailbox(worker_state.send_q);
    delete_chrpc_client(worker_state.client);

    safe_exit(0);
}
