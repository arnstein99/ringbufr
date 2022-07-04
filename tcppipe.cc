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

static size_t copy(int firstFD, int secondFD, const std::atomic<bool>& cflag);
static void usage_error();

int main (int argc, char* argv[])
{
    // Process user inputs
    int argc_copy = argc - 1;
    char** argv_copy = argv;
    ++argv_copy;
    Uri uri[2];
    uri[0] = process_args(argc_copy, argv_copy);
    uri[1] = process_args(argc_copy, argv_copy);
    if (argc_copy != 0) usage_error();

    // Get the listening sockets ready (if any)
    int listener[2] = {-1, -1};
    auto socket_if = [&listener, &uri] (int index)
    {
        if (uri[index].listening)
        {
            listener[index] = socket_from_address("", uri[index].port);
        }
    };
    socket_if(0);
    socket_if(1);

    int sock[2] = {0, 0};
    bool repeat = uri[0].listening || uri[1].listening;

    // These will be used in the following loop
    auto listen_if = [&uri, &sock, &listener] (int index)
    {
        if (uri[index].listening)
        {
            sock[index] = get_client(listener[index]);
        }
    };
    auto connect_if = [&uri, &sock] (int index)
    {
        if (!uri[index].listening)
        {
            if (uri[index].port == -1)
            {
                sock[index] = index;
            }
            else
            {
                sock[index] =
                    socket_from_address(uri[index].hostname, uri[index].port);
            }
        }
    };

    do
    {
        // Special processing for double listen
        if (uri[0].listening && uri[1].listening)
        {
            // wait for both URIs to accept
            get_two_clients(
                listener[0], listener[1],  sock[0], sock[1]);
        }
        else
        {
            // Wait for client to listening socket, if any.
            listen_if(0);
            listen_if(1);

            // Connect client socket and/or pipe socket, if any.
            connect_if(0);
            connect_if(1);
        }

        // Modify port properties
        set_flags(sock[0], O_NONBLOCK);
        set_flags(sock[1], O_NONBLOCK);

        // Both sockets are complete, so copy now.
        std::cerr << "Begin copy loop" << std::endl;
        std::atomic<bool> continue_flag(true);
        std::thread one([sock, &continue_flag] ()
        {
            copy(sock[0], sock[1], continue_flag);
            continue_flag = false;
        });
        std::thread two([sock, &continue_flag] ()
        {
            copy(sock[1], sock[0], continue_flag);
            continue_flag = false;
        });

        one.join();
        two.join();
        if (uri[1].port != -1) close(sock[1]);
        if (uri[0].port != -1) close(sock[0]);
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

size_t copy(int firstFD, int secondFD, const std::atomic<bool>& cflag)
{
    size_t bytes_processed;
    try
    {
#ifdef VERBOSE
        std::cerr << "starting copy, FD " << firstFD <<
            " to FD " << secondFD << std::endl;
        bytes_processed =
            copyfd_while(
                firstFD, secondFD, cflag, 500000, 128*1024, 2*1024, 2*1024);
        std::cerr << bytes_processed << " copied" << std::endl;
#else
        bytes_processed =
            copyfd_while
                (firstFD, secondFD, cflag, 500000, 128*1024, 2*1024, 2*1024);
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
