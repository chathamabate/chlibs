
#include "chsys/sock.h"

#include <string.h>
#include <netinet/in.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>

int client_connect(const char *ipaddr, int port) {
    // Basically took this from chatgpt tbh...

    int sockfd;
    struct sockaddr_in server_addr;

    // 1. Create a socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        return -1;
    }

     // 2. Set up the server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    // Convert IP address from text to binary
    if (inet_pton(AF_INET, ipaddr, &server_addr.sin_addr) <= 0) {
        close(sockfd);
        return -1;
    }

    // 3. Connect to the server
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        close(sockfd);
        return -1;
    }
    
    return sockfd;
}

int create_server(int port, int pending_conns) {
    if (pending_conns <= 0) {
        return -1;
    }

    int status;
    int sockfd;
    struct sockaddr_in server_addr;

    // 1. Create a socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        return -1;
    }

    int flags;

    flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) {
        close(sockfd);
        return -1;
    }

    status = fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
    if (status == -1) {
        close(sockfd);
        return -1;
    }

    // 2. Bind the socket to an address and port
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; // Listen on all network interfaces
    server_addr.sin_port = htons(port);

    status = bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));

    if (status) {
        close(sockfd);
        return -1;
    }

    // 3. Listen for incoming connections
    status = listen(sockfd, pending_conns);
    if (status) { 
        close(sockfd);
        return -1;
    } 

    return sockfd;
}
