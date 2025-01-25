

#ifndef CHSYS_SOCK_H
#define CHSYS_SOCK_H

// Returns sockfd on success, and -1 on failure.
int client_connect(const char *ipaddr, int port);

// Returns sockfd on success, and -1 on failure.
int create_server(int port, int pending_conns);

#endif
