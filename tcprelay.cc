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
    if (argc != 3)
    {
        std::cerr << "Usage: tcprelay input_port output_port" << std::endl;
	exit(1);
    }
    int input_port = std::stoi(argv[1]);
    int output_port = std::stoi(argv[2]);

    // Listen on output port
    int output_socket = listening_socket(output_port);

    // Client found for output port, now listen on input port.
    int input_socket = listening_socket(input_port);

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
