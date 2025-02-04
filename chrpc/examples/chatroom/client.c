
#include <stdio.h>

#include "chatroom.h"
#include "chrpc/channel.h"
#include "chrpc/channel_fd.h"
#include "chrpc/rpc_client.h"
#include "chrpc/rpc_server.h"
#include "chsys/log.h"
#include "chsys/sock.h"
#include "chsys/sys.h"

#include <termios.h>
#include <unistd.h>

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




int main(void) {
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

    chatroom_mailbox_t *mb = new_chatroom_mailbox();

    enable_raw_mode();

    while (!sys_sig_exit_requested()) {

        // So here, we need to both poll for messages and read input...

        
        sleep(1);
    }

    disable_raw_mode();

    safe_exit(0);
}
