#include <unistd.h>
#include <fcntl.h>
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
        int temp1 = socket_from_address("", input_uri.port);
        int temp2 = socket_from_address("", output_uri.port);
        no_linger(temp1);
        no_linger(temp2);
        get_two_clients(temp1, temp2, input_socket, output_socket);
        close(temp2);
        close(temp1);
    }
    else if (input_uri.listening)
    {
        int temp1 = socket_from_address("", input_uri.port);
        no_linger(temp1);
        input_socket = get_client(temp1);
        close(temp1);
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
        int temp2 = socket_from_address("", output_uri.port);
        no_linger(temp2);
        output_socket = get_client(temp2);
        close(temp2);
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

    // Modify port properties
    set_flags(input_socket, O_NONBLOCK|O_RDONLY);
    set_flags(output_socket, O_NONBLOCK|O_WRONLY);

    // Copy!
#ifdef VERBOSE
    std::cerr << "starting copy, socket " << input_socket <<
        " to socket " << output_socket << std::endl;
    auto bytes_processed =
        copyfd(input_socket, output_socket, 128*1024, 1024, 1024);
    std::cerr << bytes_processed << " copied" << std::endl;
#else
    copyfd(input_socket, output_socket, 128*1024, 1024, 1024);
#endif

    if (output_uri.port != -1) close(output_socket);
    if (input_uri.port != -1)  close(input_socket);
    return 0;
}

static Uri process_args(int& argc, char**& argv)
// Group can be one of
//     -pipe
//     -listen <port
//     -connect <hostname> <port>
{
    Uri uri;

    if (argc < 1) usage_error();
    const char* option = argv[0];
    ++argv;
    --argc;

    if (strcmp(option, "-pipe") == 0)
    {
        uri.listening = false;
        uri.port = -1;
    }
    else if (strcmp(option, "-listen") == 0)
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
    std::cerr << "Usage: tcpcat <input_spec> <output_spec>" << std::endl;
    std::cerr << "Each of <input_spec> and <output_spec> can be one of" <<
        std::endl;
    std::cerr << "    -pipe" << std::endl;
    std::cerr << "    -listen <port_number>" << std::endl;
    std::cerr << "    -connect <hostname> <port_number>" << std::endl;
    exit (1);
}
