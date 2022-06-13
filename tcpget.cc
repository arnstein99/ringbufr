#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "copyfd.h"

void set_flags(int fd, int flags)
{
    int oldflags = fcntl(fd, F_GETFL, 0);
    oldflags |= flags;
    int retval = fcntl(fd, F_SETFL, oldflags);
    if (retval != 0)
    {
        std::cerr << "fcntl " << strerror(errno) << std::endl;
	exit(retval);
    }
}

int main (int argc, char* argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: tcpget portnum" << std::endl;
	exit(1);
    }
    int port_num = std::stoi(argv[1]);

    // Create listening socket
    int socketFD = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socketFD == -1)
    {
	std::cerr << "socket: " << strerror(errno) << std::endl;
	exit (1);
    }
    struct sockaddr_in sa;
    memset (&sa, 0, sizeof (sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons ((uint16_t)port_num);
    sa.sin_addr.s_addr = htonl (INADDR_ANY);
    if (bind (socketFD, (struct sockaddr *)(&sa), (socklen_t)sizeof (sa))
	== -1)
    {
	std::cerr << "bind: " << strerror(errno) << std::endl;
	exit (1);
    }

    // Get a server
    if (listen (socketFD, 10) == -1)
    {
	std::cerr << "listen: " << strerror(errno) << std::endl;
	exit (1);
    }
    int optval = 1;
    if (setsockopt (
	socketFD, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval))
	< 0)
    {
	std::cerr << "setsockopt: " << strerror(errno) << std::endl;
	exit (1);
    }
    struct sockaddr_in addr;
    socklen_t addrlen = (socklen_t)sizeof(addr);
    int connectFD = accept (socketFD, (struct sockaddr*)(&addr), &addrlen);
    if (0 > connectFD)
    {
	std::cerr << "accept: " << strerror(errno) << std::endl;
	exit (1);
    }
    std::cerr << "connected" << std::endl;
    set_flags(connectFD, O_NONBLOCK|O_RDONLY);

    // Prepare output file descriptor
    set_flags(1, O_NONBLOCK);

    // Copy!
    auto bytes_processed = copyfd(connectFD, 1, 64*1024);
    std::cerr << bytes_processed << " copied" << std::endl;

    close(connectFD);
    return 0;
}
