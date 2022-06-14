#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <cstring>
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
    if (argc != 2)
    {
        std::cerr << "Usage: tcpget portnum" << std::endl;
	exit(1);
    }
    int port_num = std::stoi(argv[1]);

    // Create listening socket
    int socketFD;
    NEGCHECK("socket",
        (socketFD = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP)));
    struct sockaddr_in sa;
    memset (&sa, 0, sizeof (sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons ((uint16_t)port_num);
    sa.sin_addr.s_addr = htonl (INADDR_ANY);
    NEGCHECK("bind",
        (bind (socketFD, (struct sockaddr *)(&sa), (socklen_t)sizeof (sa))));

    // Get a server
    NEGCHECK("listen", listen (socketFD, 10));
    int optval = 1;
    NEGCHECK("setsockopt", setsockopt (
	socketFD, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval)));
    struct sockaddr_in addr;
    socklen_t addrlen = (socklen_t)sizeof(addr);
    int connectFD;
    NEGCHECK("accept", (connectFD = accept(
        socketFD, (struct sockaddr*)(&addr), &addrlen)));
#ifdef VERBOSE
    std::cerr << "connected" << std::endl;
#endif
    set_flags(connectFD, O_NONBLOCK|O_RDONLY);

    // Prepare output file descriptor
    set_flags(1, O_NONBLOCK);

    // Copy!
#ifdef VERBOSE
    auto bytes_processed = copyfd(connectFD, 1, 131072);
    std::cerr << bytes_processed << " copied" << std::endl;
#else
    copyfd(connectFD, 1, 131072);
#endif

    close(connectFD);
    return 0;
}
