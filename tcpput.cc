#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <cstring>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "copyfd.h"

// #define VERBOSE

void set_flags(int fd, int flags)
{
    int oldflags = fcntl(fd, F_GETFL, 0);
    oldflags |= flags;
    ZEROCHECK("fcntl", fcntl(fd, F_SETFL, oldflags));
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
    int socketFD;
    NEGCHECK("socket", (socketFD = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP)));

    // Process host name
    struct hostent* server = gethostbyname(hostname);
    if (server == NULL) errorexit("gethostbyname");
    struct sockaddr_in serveraddr;
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
          (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(port_num);

    // Connect to server
    NEGCHECK ("connect", connect(
        socketFD, (struct sockaddr*)(&serveraddr), sizeof(serveraddr)));
    int optval = 1;
    NEGCHECK("setsockopt", setsockopt(
	socketFD, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval)));
    set_flags(socketFD, O_WRONLY|O_NONBLOCK);

    // Prepare input file descriptor
    set_flags(0, O_NONBLOCK);

    // Copy!
#ifdef VERBOSE
    std::cerr << bytes_processed << " copied" << std::endl;
#else
    copyfd(0, socketFD, 64*1024);
#endif

    close(socketFD);
    return 0;
}
