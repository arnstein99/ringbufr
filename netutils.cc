#include "netutils.h"
#include "miscutils.h"

#include <string.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>

void set_flags(int fd, int flags)
{
    int oldflags = fcntl(fd, F_GETFL, 0);
    oldflags |= flags;
    ZEROCHECK("fcntl", fcntl(fd, F_SETFL, oldflags));
}

int socket_from_address(const std::string& hostname, int port_number)
{
    // Create socket
    int socketFD;
    NEGCHECK("socket", (socketFD = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP)));

    // Not sure if this is required
    int optval = 1;
    NEGCHECK("setsockopt", setsockopt (
        socketFD, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval)));

    // Process host name
    struct hostent* server = gethostbyname(hostname.c_str());
    if (server == NULL) errorexit("gethostbyname");
    struct sockaddr_in serveraddr;
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
    (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(port_number);

    // Connect to server
    NEGCHECK ("connect", connect(
        socketFD, (struct sockaddr*)(&serveraddr), sizeof(serveraddr)));

    return socketFD;
}

int listening_socket(int port_number)
{
    // Create listening socket
    int socketFD;
    NEGCHECK("socket",
        (socketFD = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP)));
    struct sockaddr_in sa;
    memset (&sa, 0, sizeof (sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons ((uint16_t)port_number);
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
#ifdef VERBOSE
    std::cerr << "connected" << std::endl;
#endif

    return connectFD;
}

void double_listen(
    int input_port, int output_port, int input_socket, int output_socket)
{
    // Create listening sockets
    int socketFD_in;
    NEGCHECK("socket",
        (socketFD_in = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP)));
    int socketFD_out;
    NEGCHECK("socket",
        (socketFD_out = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP)));

    // Bind both sockets
    struct sockaddr_in sa;
    memset (&sa, 0, sizeof (sa));
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = htonl (INADDR_ANY);

    sa.sin_port = htons ((uint16_t)input_port);
    NEGCHECK("bind",
        (bind (socketFD_in, (struct sockaddr *)(&sa), (socklen_t)sizeof (sa))));
    set_flags(socketFD_in, O_NONBLOCK);

    sa.sin_port = htons ((uint16_t)output_port);
    NEGCHECK("bind",
        (bind (socketFD_out, (
            struct sockaddr *)(&sa), (socklen_t)sizeof (sa))));
    set_flags(socketFD_out, O_NONBLOCK);

    // Mark both sockets for listening
    int optval = 1;
    NEGCHECK("listen", listen (socketFD_in, 10));
    NEGCHECK("setsockopt", setsockopt (
        socketFD_in, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval)));
    NEGCHECK("listen", listen (socketFD_out, 10));
    NEGCHECK("setsockopt", setsockopt (
        socketFD_out, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval)));

    // Get both sockets accepted
    int connectFD_in = -1, connectFD_out = -1;
    do
    {
        struct sockaddr_in addr;
        socklen_t addrlen = (socklen_t)sizeof(addr);
        fd_set* p_read_set = nullptr;
        fd_set read_set;
        FD_ZERO(&read_set);
        int maxfd = std::max(socketFD_in, socketFD_out) + 1;

        if (connectFD_in < 0)
        {
            connectFD_in =
                accept(socketFD_in, (struct sockaddr*)(&addr), &addrlen);
            if (connectFD_in < 0)
            {
                if ((errno == EWOULDBLOCK) || (errno == EAGAIN))
                {
                    // This is when to select()
                    FD_SET(socketFD_in, &read_set);
                    p_read_set = &read_set;
                }
                else
                {
                    // Some other error on input
                    errorexit("accept");
                }
            }
        }

        if (connectFD_out < 0)
        {
            connectFD_out =
                accept(socketFD_out, (struct sockaddr*)(&addr), &addrlen);
            if (connectFD_out < 0)
            {
                if ((errno == EWOULDBLOCK) || (errno == EAGAIN))
                {
                    // This is when to select()
                    FD_SET(socketFD_out, &read_set);
                    p_read_set = &read_set;
                }
                else
                {
                    // Some other error on input
                    errorexit("accept");
                }
            }
        }

        if (p_read_set)
        {
            int select_return;
            NEGCHECK("select",
                (select_return = select(
                    maxfd, p_read_set, nullptr, nullptr, nullptr)));
        }

    } while ((connectFD_in < 0) || (connectFD_out < 0));
}
