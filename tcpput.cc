#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <cstring>
#include <netdb.h>
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
    if (argc != 3)
    {
        std::cerr << "Usage: tcpput hostname portnum" << std::endl;
	exit(1);
    }
    const char* hostname = argv[1];
    const int port_num = std::stoi(argv[2]);

    // Create socket
    int socketFD = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socketFD == -1)
    {
	std::cerr << "socket: " << strerror(errno) << std::endl;
	exit (1);
    }

    // Process host name
    struct hostent* server = gethostbyname(hostname);
    if (server == NULL) {
	std::cerr << "gethostbyname: " << strerror(errno) << std::endl;
	exit (1);
    }
    struct sockaddr_in serveraddr;
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
          (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(port_num);

    // Connect to server
    if (connect(
        socketFD, (struct sockaddr*)(&serveraddr), sizeof(serveraddr)) < 0)
    {
	std::cerr << "connect: " << strerror(errno) << std::endl;
	exit (1);
    }
    int optval = 1;
    if (setsockopt (
	socketFD, SOL_SOCKET, SO_KEEPALIVE|O_WRONLY, &optval, sizeof(optval))
	< 0)
    {
	std::cerr << "setsockopt: " << strerror(errno) << std::endl;
	exit (1);
    }
    set_flags(socketFD, O_WRONLY|O_NONBLOCK);

    // Prepare input file descriptor
    set_flags(0, O_NONBLOCK);

    // Copy!
    auto bytes_processed = copyfd(0, socketFD, 64*1024);
    std::cerr << bytes_processed << " copied" << std::endl;

    return 0;
}
