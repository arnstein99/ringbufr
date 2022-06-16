#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "copyfd.h"
#include "miscutils.h"
#include "netutils.h"

// #define VERBOSE

int main (int argc, char* argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: tcpget portnum" << std::endl;
	exit(1);
    }
    int port_num = std::stoi(argv[1]);

    // Create listening socket
    int socketFD = listening_socket(port_num);
    set_flags(socketFD, O_NONBLOCK|O_RDONLY);

    // Prepare output file descriptor
    set_flags(1, O_NONBLOCK);

    // Copy!
#ifdef VERBOSE
    auto bytes_processed = copyfd(socketFD, 1, 131072);
    std::cerr << bytes_processed << " copied" << std::endl;
#else
    copyfd(socketFD, 1, 131072);
#endif

    close(socketFD);
    return 0;
}
