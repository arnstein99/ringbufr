#include <cstdlib>
#include <cstring>
#include <iostream>
#include <errno.h>
#include <unistd.h>
#include <string>
#include <thread>
#include <stdio.h>
#include <chrono>
using namespace std::chrono_literals;
#include "posix_ringbufr.h"

// Tuning
static const int read_usleep_range  = 500000;
static const int write_usleep_range = 500000;
static const size_t buffer_size = 37;
static const size_t push_pad = 7;
static const size_t pop_pad = 5;
static const size_t verbose = 1;
#define USE_POSIX
#define DEFAULT_RUN_SECONDS 300

class Dummy
{
public:
    int serialNumber;
    double doubleMember;
    char stringMember[32];
};
Dummy* buffer;

#ifdef USE_POSIX
static Posix_RingbufR<Dummy> rbuf(
    buffer_size, 0 /*verbose*/, push_pad, pop_pad);
#else
static RingbufR<Dummy> rbuf (buffer_size, push_pad, pop_pad);
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
    // Cheat
    size_t available;
    rbuf.pushInquire(available, buffer);
    buffer -= (push_pad + pop_pad);

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
        std::this_thread::sleep_for(write_usleep * 1us);
        size_t available;
        Dummy* start;
        rbuf.pushInquire(available, start);
        write_usleep = (rand() % write_usleep_range) + 1;
        std::this_thread::sleep_for(write_usleep * 1us);
        if (available)
        {
            size_t count = 1;
            if (available > 1)
                count = (rand() % (available-1)) + 1;
            // Temporary, test of edge pad
            if (available >= push_pad)
                count = std::max(count, push_pad);
            if (verbose >= 1)
            {
                std::cout << "(will push " << count <<
                    " at " << start - buffer <<
                    " starting with value " << serial + 1 << ")" << std::endl;
            }
            size_t i = count;
            while (i-- > 0)
            {
                start->serialNumber = ++serial;
                ++start;
            }
            try
            {
                rbuf.push(count);
            }
            catch (RingbufRFullException)
            {
                std::cout << "Write failure" << std::endl;
                exit(1);
            }
            std::cout << "size is now " << rbuf.size() <<
                ", ring start is " << rbuf.ring_start() - buffer <<std::endl;
        }
    }
}


static void Reader ()
{
    static __thread int serial = 0;

    while (true)
    {
        int read_usleep = (rand() % read_usleep_range) + 1;
        std::this_thread::sleep_for(read_usleep * 1us);
        size_t available;
        Dummy* start;
        rbuf.popInquire(available, start);
        read_usleep = (rand() % read_usleep_range) + 1;
        std::this_thread::sleep_for(read_usleep * 1us);
        if (available)
        {
            size_t count = 1;
            if (available > 1)
                count = (rand() % (available-1)) + 1;
            // Temporary, test of edge pad
            if (available >= pop_pad)
                count = std::max(count, pop_pad);
            if (verbose >= 1)
                std::cout << "(will pop " << count <<
                " starting at " << start - buffer << ")" << std::endl;
            for (size_t i = 0 ; i < count ; ++i)
            {
                ++serial;
                auto observed = start->serialNumber;
                if (observed != serial)
                {
                    std::cout << "*** ERROR *** ";
                    std::cout << "Pop: expected " << serial << " got " <<
                        observed << " offset " << i << std::endl;
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
            std::cout << "size is now " << rbuf.size() <<
                ", ring start is " << rbuf.ring_start() - buffer <<std::endl;
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

#include "ringbufr.tcc"
