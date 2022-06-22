#include <cstdlib>
#include <cstring>
#include <iostream>
#include <errno.h>
#include <unistd.h>
#include <string>
#include <thread>
#include <stdio.h>
#include <mutex>
#include "posix_ringbufr.h"

// Tuning
static const int read_usleep_range = 1000000;
static const int write_usleep_range = 1000000;
static const int buffer_size = 37;
static const int guard_size = 0; // 7
static const int verbose = 1;
// #define USE_POSIX
#define DEFAULT_RUN_SECONDS 300
std::mutex _mutex;

class Dummy
{
public:
    int serialNumber;
    double doubleMember;
    char stringMember[32];
};

#ifdef USE_POSIX
static Posix_RingbufR<Dummy> rbuf(buffer_size, verbose, guard_size);
#else
static RingbufR<Dummy> rbuf (buffer_size, guard_size);
#endif
static bool running = true;

static void Reader ();
static void Writer ();
static void Usage_exit (int exit_val);

int main (int argc, char* argv[])
{
    int run_seconds;
    switch (argc)
    {
    case 1:
        run_seconds = DEFAULT_RUN_SECONDS;
        break;
    case 2:
        if (sscanf (argv[1], "%d", &run_seconds) != 1)
        {
            std::cerr << "Illegal numeric expression \"" << argv[1] <<
                         "\"" << std::endl;
            exit (1);
        }
        if (run_seconds < 0)
        {
            std::cerr << "Please enter a non-negative number or nothing" <<
                         std::endl;
            Usage_exit (1);
        }
        break;
    default:
        run_seconds = 0; // silence compiler warning
        Usage_exit (0);
        break;
    }
    
    std::thread hReader (Reader);
    std::thread hWriter (Writer);
    sleep (run_seconds);
    
    running = false;
    hWriter.join();
    hReader.join();
}

static void Writer ()
{
    static __thread int serial = 0;

    while (running)
    {
        int write_usleep = (rand() % write_usleep_range) + 1;
        usleep (write_usleep);
        size_t available;
        Dummy* start;
        auto lock = std::lock_guard(_mutex);
        rbuf.pushInquire(available, start);
        if (available)
        {
            size_t count = 1;
            if (available > 1)
                count = (rand() % (available-1)) + 1;
            if (verbose >= 1)
                std::cout << "(pushing " << count << ":";
            size_t i = count;
            while (i-- > 0)
            {
                start->serialNumber = ++serial;
                if (verbose >= 1)
                    std::cout << " " << serial;
                ++start;
            }
            if (verbose >= 1)
                std::cout << std::endl;
            try
            {
                rbuf.push(count);
            }
            catch (RingbufRFullException)
            {
                std::cout << "Write failure" << std::endl;
                exit(1);
            }
        }
    }
}


static void Reader ()
{
    static __thread int serial = 0;

    while (true)
    {
        int read_usleep = (rand() % read_usleep_range) + 1;
        usleep (read_usleep);
        size_t available;
        Dummy* start;
        auto lock = std::lock_guard(_mutex);
        rbuf.popInquire(available, start);
        if (available)
        {
            ssize_t count = 1;
            if (available > 1)
                count = (rand() % (available-1)) + 1;
            if (verbose >= 1)
                std::cout << "(will pop " << count << ")" << std::endl;
            ssize_t i = count;
            while (i-- > 0)
            {
                ++serial;
                auto observed = start->serialNumber;
                if (observed != serial)
                {
                    std::cout << "*** ERROR *** ";
                    std::cout << "Pop: expected " << serial << " got " <<
                        observed << std::endl;
                    exit(1);
                }
                ++start;
            }
            try
            {
                rbuf.pop(count);
            }
            catch (RingbufREmptyException)
            {
                std::cout << "Read failure " << std::endl;
                exit(1);
            }
        }
        else if (!running)
        {
            break;
        }
    }
}

static void Usage_exit (int exit_val)
{
    std::cerr << "Usage: test_ring [run_seconds]" << std::endl;
    exit (exit_val);
}
