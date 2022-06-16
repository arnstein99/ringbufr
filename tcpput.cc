#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <cstring>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "copyfd.h"
#include "miscutils.h"
#include "netutils.h"

// #define VERBOSE

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
    int socketFD = socket_from_address(hostname, port_num);
    set_flags(socketFD, O_WRONLY|O_NONBLOCK);

    // Prepare input file descriptor
    set_flags(0, O_NONBLOCK);

    // Copy!
#ifdef VERBOSE
    auto bytes_processed = copyfd(0, socketFD, 131072);
    std::cerr << bytes_processed << " copied" << std::endl;
#else
    copyfd(0, socketFD, 131072);
#endif

    close(socketFD);
    return 0;
}
