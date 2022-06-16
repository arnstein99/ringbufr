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

#endif // __NETUTILS_H_
