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

static size_t copy(int sock1, int sock2, const std::atomic<bool>& cflag);
static void usage_error();

int main (int argc, char* argv[])
{
    // Process inputs
    int argc_copy = argc - 1;
    char** argv_copy = argv;
    ++argv_copy;
    auto first_uri  = process_args(argc_copy, argv_copy);
    auto second_uri = process_args(argc_copy, argv_copy);
    if (argc_copy != 0) usage_error();

    // Get the listening sockets ready (if any)
    int first_listener=-1, second_listener=-1;
    if (first_uri.listening)
    {
        first_listener = socket_from_address("", first_uri.port);
        no_linger(first_listener);
    }
    if (second_uri.listening)
    {
        second_listener = socket_from_address("", second_uri.port);
        no_linger(second_listener);
    }


    int first_socket=0, second_socket=0;
    bool repeat = first_uri.listening || second_uri.listening;
    do
    {
        // Special processing for double listen
        if (first_uri.listening && second_uri.listening)
        {
            // wait for both URIs to accept
            get_two_clients(
                first_listener, second_listener,  first_socket, second_socket);
        }
        else if (first_uri.listening)
        {
            first_socket = get_client(first_listener);
            if (second_uri.port == -1)
            {
                second_socket = 1;
            }
            else
            {
                second_socket =
                    socket_from_address(second_uri.hostname, second_uri.port);
            }
        }
        else if (second_uri.listening)
        {
            second_socket = get_client(second_listener);
            if (first_uri.port == -1)
            {
                first_socket = 0;
            }
            else
            {
                first_socket =
                    socket_from_address(first_uri.hostname, first_uri.port);
            }
        }
        else // no listening
        {
            if (first_uri.port == -1)
            {
                first_socket = 0;
            }
            else
            {
                first_socket =
                    socket_from_address(first_uri.hostname, first_uri.port);
            }
            if (second_uri.port == -1)
            {
                second_socket = 1;
            }
            else
            {
                second_socket =
                    socket_from_address(second_uri.hostname, second_uri.port);
            }
        }

        // Modify port properties
        set_flags(first_socket , O_NONBLOCK);
        set_flags(second_socket, O_NONBLOCK);

        // Both sockets are complete, so copy now.
        std::cerr << "Begin copy loop" << std::endl;
        std::atomic<bool> continue_flag = true;
        std::thread one([first_socket, second_socket, &continue_flag]()
        {
            copy(first_socket, second_socket, continue_flag);
            continue_flag = false;
        });
        std::thread two([first_socket, second_socket, &continue_flag]()
        {
            copy(second_socket, first_socket, continue_flag);
            continue_flag = false;
        });

        one.join();
        two.join();
        if (second_uri.port != -1) close(second_socket);
        if (first_uri.port != -1)  close(first_socket);
        std::cerr << "End copy loop" << std::endl;

    } while (repeat);

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
    std::cerr << "Usage: tcppipe <first_spec> <second_spec>" << std::endl;
    std::cerr << "Each of <first_spec> and <second_spec> can be one of" <<
        std::endl;
    std::cerr << "    -pipe" << std::endl;
    std::cerr << "    -listen <port_number>" << std::endl;
    std::cerr << "    -connect <hostname> <port_number>" << std::endl;
    exit (1);
}

size_t copy(int sock1, int sock2, const std::atomic<bool>& cflag)
{
    size_t bytes_processed;
    try
    {
#ifdef VERBOSE
        std::cerr << "starting copy, socket " << sock1 <<
            " to socket " << sock2 << std::endl;
        bytes_processed =
            copyfd_while(sock1, sock2, cflag, 500000, 128*1024, 2*1024, 2*1024);
        std::cerr << bytes_processed << " copied" << std::endl;
#else
        bytes_processed =
            copyfd_while(sock1, sock2, cflag, 500000, 128*1024, 2*1024, 2*1024);
#endif
    }
    catch (const ReadException& r)
    {
        bytes_processed = r.byte_count;
    }
    catch (const WriteException& w)
    {
        bytes_processed = w.byte_count;
    }
    return bytes_processed;
}
