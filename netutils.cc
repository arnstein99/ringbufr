#include "netutils.h"
#include "miscutils.h"

#include <thread>
#include <chrono>
using namespace std::chrono_literals;
#include <string.h>
#include <fcntl.h>
#include <netdb.h>
#include <arpa/inet.h>

#define VERBOSE
#ifdef VERBOSE
#include <iostream>
#endif

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

    // Process host name
    struct sockaddr_in serveraddr;
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(port_number);

    if (hostname == "")
    {
        serveraddr.sin_addr.s_addr = htonl (INADDR_ANY);

        // Bind to address but do not connect to anything
        NEGCHECK("bind",
            bind(
                socketFD,
                (struct sockaddr *)(&serveraddr),
                (socklen_t)sizeof (serveraddr)));
    }
    else
    {
        struct hostent* server = gethostbyname(hostname.c_str());
        if (server == NULL) errorexit("gethostbyname");
        bcopy((char *)server->h_addr,
        (char *)&serveraddr.sin_addr.s_addr, server->h_length);

        // Connect to server
        NEGCHECK ("connect", connect(
            socketFD, (struct sockaddr*)(&serveraddr), sizeof(serveraddr)));
        #ifdef VERBOSE
            std::cerr << "connected socket " << socketFD << std::endl;
        #endif
    }

    return socketFD;
}

int get_client(int listening_socket)
{
    // Get a client
    NEGCHECK("listen", listen (listening_socket, 1));
    struct sockaddr_in addr;
    socklen_t addrlen = (socklen_t)sizeof(addr);
    int client_socket;
    NEGCHECK("accept", (client_socket = accept(
        listening_socket, (struct sockaddr*)(&addr), &addrlen)));
#ifdef VERBOSE
    std::cerr << "connected" << std::endl;
#endif

    return client_socket;
}

void get_two_clients(
    int first_listening_socket, int second_listening_socket,
    int& first_client_socket, int& second_client_socket)
{
    set_flags(first_listening_socket , O_NONBLOCK);
    set_flags(second_listening_socket, O_NONBLOCK);

    // Mark both sockets for listening
    NEGCHECK("listen", listen (first_listening_socket , 1));
    NEGCHECK("listen", listen (second_listening_socket, 1));

    // Get both sockets accepted
    int first_cl_socket = -1, second_cl_socket = -1;
    do
    {
        struct sockaddr_in addr;
        socklen_t addrlen = (socklen_t)sizeof(addr);
        fd_set* p_read_set = nullptr;
        fd_set read_set;
        FD_ZERO(&read_set);
        int maxfd =
            std::max(first_listening_socket, second_listening_socket) + 1;

        if (first_cl_socket < 0)
        {
            first_cl_socket =
                accept(
                    first_listening_socket,
                    (struct sockaddr*)(&addr), &addrlen);
            if (first_cl_socket < 0)
            {
                if ((errno == EWOULDBLOCK) || (errno == EAGAIN))
                {
                    // This is when to select()
                    FD_SET(first_listening_socket, &read_set);
                    p_read_set = &read_set;
                }
                else
                {
                    // Some other error on input
                    errorexit("accept");
                }
            }
#ifdef VERBOSE
            else
            {
                std::cerr << "accepted on input socket " <<
                    first_listening_socket << std::endl;
            }
#endif
        }

        if (second_cl_socket < 0)
        {
            second_cl_socket =
                accept(
                    second_listening_socket,
                    (struct sockaddr*)(&addr), &addrlen);
            if (second_cl_socket < 0)
            {
                if ((errno == EWOULDBLOCK) || (errno == EAGAIN))
                {
                    // This is when to select()
                    FD_SET(second_listening_socket, &read_set);
                    p_read_set = &read_set;
                }
                else
                {
                    // Some other error on input
                    errorexit("accept");
                }
            }
#ifdef VERBOSE
            else
            {
                std::cerr << "accepted on output socket " <<
                    second_listening_socket << std::endl;
            }
#endif
        }

        if (p_read_set)
        {
            int select_return;
            NEGCHECK("select",
                (select_return = select(
                    maxfd, p_read_set, nullptr, nullptr, nullptr)));
        }

    } while ((first_cl_socket < 0) || (second_cl_socket < 0));
#ifdef VERBOSE
    std::cerr << "finished accepts" << std::endl;
#endif
    first_client_socket  = first_cl_socket;
    second_client_socket = second_cl_socket;
}

void no_linger(int socket)
{
    struct linger nope;
    nope.l_onoff = 1;
    nope.l_linger = 0;
    NEGCHECK("setsockopt",
        setsockopt(socket, SOL_SOCKET, SO_LINGER, &nope, sizeof(nope)));
}
