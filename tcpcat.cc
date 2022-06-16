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

// socket will be -1 if user wants to use stdin or stdout.
void
process_args(int& argc, char** argv, bool& listening, int& socket);

static void usage_error();

int main (int argc, char* argv[])
{
    // Process inputs
    int argc_copy = argc - 1;
    char** argv_copy = argv;
    ++argv_copy;
    bool input_listening, output_listening;
    int input_socket, output_socket;
    process_args(argc_copy, argv_copy, input_listening , input_socket );
    process_args(argc_copy, argv_copy, output_listening, output_socket);
    if (argc_copy != 0) usage_error();

    // Special case for local I/O
    if (input_socket  == -1) input_socket  = 0;
    if (output_socket == -1) output_socket = 1;

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

void
process_args(int& argc, char** argv, bool& listening, int& socket)
{
    if (argc < 1) usage_error();
    const char* option = argv[0];
    ++argv;
    --argc;
    if (argc < 1) usage_error();
    const char* value = argv[0];
    --argc;
    ++argv;
    if (strcmp(option, "-listen") == 0)
    {
        listening = true;
        if (strcmp(value, "pipe") == 0)
        {
            socket = -1;
        }
        else
        {
            int port = std::stoi(value);
            socket = listening_socket(port);
        }
    }
    else if (strcmp(option, "-connect") == 0)
    {
        listening = false;
        if (strcmp(value, "pipe") == 0)
        {
            socket = -1;
        }
        else
        {
            const char* hostname = value;
            if (argc < 1) usage_error();
            --argc;
            ++argv;
            int port = std::stoi(argv[0]);
            socket = socket_from_address(hostname, port);
        }
    }
    else
    {
        usage_error();
    }
}

void usage_error()
{
    // Later
    std::cerr << "Usage error" << std::endl;
    exit (1);
}
