#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "copyfd.h"
#include "miscutils.h"

// #define VERBOSE

void set_flags(int fd, int flags)
{
    int oldflags = fcntl(fd, F_GETFL, 0);
    oldflags |= flags;
    ZEROCHECK("fcntl", fcntl(fd, F_SETFL, oldflags));
}

int open_and_listen(int port_num)
{
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

    // Get a client
    NEGCHECK("listen", listen (socketFD, 10));
    int optval = 1;
    NEGCHECK("setsockopt", setsockopt (
	socketFD, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval)));
    struct sockaddr_in addr;
    socklen_t addrlen = (socklen_t)sizeof(addr);
    int connectFD;
    NEGCHECK("accept", (connectFD = accept(
        socketFD, (struct sockaddr*)(&addr), &addrlen)));

    return connectFD;
}

int main (int argc, char* argv[])
{
    if (argc != 3)
    {
        std::cerr << "Usage: tcprelay input_port output_port" << std::endl;
	exit(1);
    }
    int input_port = std::stoi(argv[1]);
    int output_port = std::stoi(argv[2]);

    // Listen on output port
    int output_socket = open_and_listen(output_port);
#ifdef VERBOSE
    std::cerr << "output connected" << std::endl;
#endif

    // Client found for output port, now listen on input port.
    int input_socket = open_and_listen(input_port);
#ifdef VERBOSE
    std::cerr << "input connected" << std::endl;
#endif

    // Modify port properties
    set_flags(input_socket, O_NONBLOCK|O_RDONLY);
    set_flags(output_socket, O_NONBLOCK|O_WRONLY);

    // Copy!
#ifdef VERBOSE
    auto bytes_processed = copyfd(input_socket, output_socket, 131072);
    std::cerr << bytes_processed << " copied" << std::endl;
#else
    copyfd(input_socket, output_socket, 131072);
#endif

    close(output_socket);
    close(input_socket);
    return 0;
}
