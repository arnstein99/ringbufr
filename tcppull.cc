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
        std::cerr << "Usage: pull hostname portnum" << std::endl;
	exit(1);
    }
    const char* hostname = argv[1];
    const int port_num = std::stoi(argv[2]);

    // Create socket
    int socketFD = socket_from_address(hostname, port_num);
    set_flags(socketFD, O_RDONLY|O_NONBLOCK);

    // Prepare output file descriptor
    set_flags(1, O_NONBLOCK);

    // Copy!
#ifdef VERBOSE
    auto bytes_processed = copyfd(socketFD, 1, 64*1024);
    std::cerr << bytes_processed << " copied" << std::endl;
#else
    copyfd(socketFD, 1, 64*1024);
#endif

    close(socketFD);
    return 0;
}
