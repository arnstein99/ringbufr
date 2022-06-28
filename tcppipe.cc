#include <unistd.h>
#include <fcntl.h>
#include <thread>
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "copyfd.h"
#include "miscutils.h"
#include "netutils.h"

struct Uri
{
    bool listening;
    int port;                // -1 means stdin or stdout
    std::string hostname;    // Not always defined
};
static Uri process_args(int& argc, char**& argv);

static void usage_error();

int main (int argc, char* argv[])
{
    // Process inputs
    int argc_copy = argc - 1;
    char** argv_copy = argv;
    ++argv_copy;
    auto input_uri  = process_args(argc_copy, argv_copy);
    auto output_uri = process_args(argc_copy, argv_copy);
    if (argc_copy != 0) usage_error();

    // Special processing for double listen
    int input_socket=0, output_socket=0;
    if (input_uri.listening && output_uri.listening)
    {
        // wait for both URIs to accept
        double_listen(
            input_uri.port, output_uri.port, input_socket, output_socket);
    }
    else if (input_uri.listening)
    {
        input_socket = listening_socket(input_uri.port);
        if (output_uri.port == -1)
        {
            output_socket = 1;
        }
        else
        {
            output_socket =
                socket_from_address(output_uri.hostname, output_uri.port);
        }
    }
    else if (output_uri.listening)
    {
        output_socket = listening_socket(output_uri.port);
        if (input_uri.port == -1)
        {
            input_socket = 0;
        }
        else
        {
            input_socket =
                socket_from_address(input_uri.hostname, input_uri.port);
        }
    }
    else // no listening
    {
        if (input_uri.port == -1)
        {
            input_socket = 0;
        }
        else
        {
            input_socket =
                socket_from_address(input_uri.hostname, input_uri.port);
        }
        if (output_uri.port == -1)
        {
            output_socket = 1;
        }
        else
        {
            output_socket =
                socket_from_address(output_uri.hostname, output_uri.port);
        }
    }

    auto copy = [] (int sock1, int sock2)
    {
    // Copy!
#ifdef VERBOSE
    std::cerr << "starting copy, socket " << sock1 <<
        " to socket " << sock2 << std::endl;
    auto bytes_processed =
        copyfd(sock1, sock2, 128*1024, 1024, 1024);
    std::cerr << bytes_processed << " copied" << std::endl;
#else
    copyfd(sock1, sock2, 128*1024, 1024, 1024);
#endif
    };
    std::thread one([&copy, input_socket, output_socket]()
    {
        copy(input_socket, output_socket);
    });
    std::thread two([&copy, input_socket, output_socket]()
    {
        copy(output_socket, input_socket);
    });
    one.join();
    two.join();

    close(output_socket);
    close(input_socket);
    return 0;
}

static Uri process_args(int& argc, char**& argv)
// Group can be one of
//     -listen <port
//     -connect <hostname> <port>
{
    Uri uri;

    if (argc < 1) usage_error();
    const char* option = argv[0];
    ++argv;
    --argc;

    if (strcmp(option, "-listen") == 0)
    {
        uri.listening = true;
        if (argc < 1) usage_error();
        const char* value = argv[0];
        ++argv;
        --argc;
        uri.port = std::stoi(value);
    }
    else if (strcmp(option, "-connect") == 0)
    {
        uri.listening = false;
        if (argc < 1) usage_error();
        const char* value = argv[0];
        --argc;
        ++argv;
        uri.hostname = value;
        if (argc < 1) usage_error();
        uri.port = std::stoi(argv[0]);
        --argc;
        ++argv;
    }
    else
    {
        usage_error();
    }
    return uri;
}

void usage_error()
{
    std::cerr << "Usage: tcppipe <input_spec> <output_spec>" << std::endl;
    std::cerr << "Each of <input_spec> and <output_spec> can be one of" <<
        std::endl;
    std::cerr << "    -listen <port_number>" << std::endl;
    std::cerr << "    -connect <hostname> <port_number>" << std::endl;
    exit (1);
}
