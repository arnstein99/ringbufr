#include <cstdlib>
#include <cstring>
#include <iostream>
#include <errno.h>
#include <unistd.h>
#include <string>
#include <pthread.h>
#include <stdio.h>
#include "posix_ringbufr.h"

// Tuning
static const int read_usleep = 1;
static const int write_usleep = 1;
static const int verbose = 1;
#define USE_POSIX
#define DEFAULT_RUN_SECONDS 30

class Dummy
{
public:
    int serialNumber;
    double doubleMember;
    char stringMember[32];
};

#ifdef USE_POSIX
static Posix_RingbufR<Dummy> rbuf (11);
#else
static RingbufR<Dummy> rbuf (11);
#endif

static void* Reader (void* arg);
static void* Writer (void* arg);
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
        Usage_exit (0);
        break;
    }
    
    pthread_t hReader, hWriter;
    if (pthread_create (&hReader, NULL, Reader, NULL) != 0)
    {
        std::cerr << "pthread_create failed (1) " << strerror(errno) <<
	             std::endl;
	exit (1);
    }
    if (pthread_create (&hWriter, NULL, Writer, NULL) != 0)
    {
        std::cerr << "pthread_create failed (2) " << strerror(errno) <<
	             std::endl;
	exit (1);
    }
    
    sleep (run_seconds);

    if (pthread_cancel (hWriter) != 0)
    {
        std::cerr << "pthread_cancel failed (1) " << strerror(errno) <<
	             std::endl;
	exit (1);
    }
    if (pthread_cancel (hReader) != 0)
    {
        std::cerr << "pthread_cancel failed (2) " << strerror(errno) <<
	             std::endl;
	exit (1);
    }

    std::cout << "No compare errors found" << std::endl;
    exit (0);
}

static void* Writer (void* arg)
{
    static __thread int serial = 0;

    while (true)
    {
	usleep (write_usleep);
	size_t available;
	Dummy* start;
	rbuf.pushInquire(available, start);
	start->serialNumber = ++serial;
        try
        {
	    rbuf.push(1);
        }
        catch (RingbufRFullException)
        {
	    if (verbose >= 2)
		std::cout << "Write failure " << serial << std::endl;
            --serial;
            continue;
        }
	if (verbose >= 3)
	    std::cout << "Put " << serial << std::endl;
    }
    
    return NULL;
}


static void* Reader (void* arg)
{
    static __thread int serial = 0;

    while (true)
    {
        usleep (read_usleep);
	++serial;
	size_t available;
	Dummy* start;
	rbuf.popInquire(available, start);
	if (start->serialNumber != serial)
	{
	    if (verbose >= 1)
	    {
		std::cout << "*** ERROR *** ";
		std::cout << "Pop " << serial << std::endl;
	    }
	}
	else
	{
	    if (verbose >= 3)
		std::cout << "Pop " << serial << std::endl;
	}
        try
        {
	    rbuf.pop(1);
        }
        catch (RingbufREmptyException)
        {
	    if (verbose >= 2)
		std::cout << "Read failure " << serial << std::endl;
            --serial;
            continue;
        }
    }

    return NULL;
}

static void Usage_exit (int exit_val)
{
    std::cerr << "Usage: test_ring [run_seconds]" << std::endl;
    exit (exit_val);
}
