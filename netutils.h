#ifndef __NETUTILS_H_
#define __NETUTILS_H_

#include <string>
#include <unistd.h>
#include <sys/socket.h>

// Operates on an active file descriptor.
void set_flags(int fd, int flags);

// Returns connected or bound socket.
int socket_from_address(const std::string& hostname, int port_number);

// Waits for a client
int get_client(int listening_socket);

// Waits for two clients.
void get_two_clients(
    int first_listening_socket, int second_listening_socket,
    int& first_client_socket, int& second_client_socket);

// Disables SO_LINGER on a socket
void no_linger(int socket);

// SO_REUSEADDR and SO_REUSEPORT
void set_reuse(int socket);

#endif // __NETUTILS_H_
