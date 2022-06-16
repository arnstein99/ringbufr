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

    return connectFD;
}
