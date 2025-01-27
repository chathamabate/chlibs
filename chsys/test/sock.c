
#include "./sock.h"
#include "chsys/log.h"
#include "chsys/sock.h"
#include "chsys/sys.h"
#include "chsys/wrappers.h"
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>


#define ECHO_ROUTINE_PORT 8080
#define ECHO_ROUTINE_BUF_SIZE 0x100
static void *server_echo_routine(void *arg) {
    (void)arg;

    int serverfd = create_server(ECHO_ROUTINE_PORT, 5);
    if (serverfd < 0) {
        log_fatal("Failed to create server\n");
    }

    int clientfd;
    struct sockaddr_in client_addr;
    socklen_t client_addr_len;

    client_addr_len = sizeof(client_addr);
    clientfd = accept(serverfd, (struct sockaddr *)&client_addr, &client_addr_len);
    if (clientfd < 0) {
        log_fatal("Failed to accept client\n");
    }

    char buf[ECHO_ROUTINE_BUF_SIZE];

    ssize_t readden = read(clientfd, buf, ECHO_ROUTINE_BUF_SIZE);
    if (readden < 0) {
        log_fatal("Failed to read from client\n");
    }

    ssize_t written = write(clientfd, buf, readden);
    if (written < 0) {
        log_fatal("Failed to write to client\n");
    }

    close(serverfd);

    return NULL;
}

static void test_echo_socket(void) {
    pthread_t server;
    safe_pthread_create(&server, NULL, server_echo_routine, NULL);
    sleep(1);

    int clientfd = client_connect("127.0.0.1", ECHO_ROUTINE_PORT);

    if (clientfd < 0) {
        log_fatal("Failed to connect\n");
    }

    write(clientfd, "Hello", 6);

    char buf[6];
    read(clientfd, buf, 6);
    buf[5] = '\0';

    printf("%s\n", buf);

    close(clientfd);
    safe_exit(0);
}

void run_sock_tests(void) {
    (void)test_echo_socket;
    /*
    sys_init();
    test_echo_socket();
    safe_exit(0);
    */
}
