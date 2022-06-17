#ifndef __NETUTILS_H_
#define __NETUTILS_H_

#include <string>
#include <unistd.h>

// Operates on an active socket.
void set_flags(int fd, int flags);

// Returns connected socket.
int socket_from_address(const std::string& hostname, int port_number);

// Blocks, returns accepted socket.
int listening_socket(int port_number);

// Blocks, finds clients for both ports.
void double_listen(
    int input_port, int output_port, int& input_socket, int& output_socket);

#endif // __NETUTILS_H_
