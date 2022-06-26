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
static const size_t buffer_size = 37 + 7 + 6;
static const size_t push_pad = 7;
static const size_t pop_pad = 6;
static const size_t verbose = 1;
#define USE_POSIX
#define DEFAULT_RUN_SECONDS 300

void itoa(int num, char* str)
{
    sprintf(str, "%d", num);
}
class TestClass
{
public:
    TestClass(int serial=0)
     : serialNumber(serial)
    {
        name = new char[9];
        itoa(serial, name);
    }
    ~TestClass()
    {
        delete[] name;
    }
    TestClass(const TestClass&) = delete;
    TestClass& operator=(const TestClass&) = delete;
    TestClass& operator=(TestClass&& other)
    {
        serialNumber = other.serialNumber;
        auto tmp = name;
        name = other.name;
        other.name = tmp;
        return *this;
    }
    bool operator==(const TestClass& other)
    {
        return
            (serialNumber == other.serialNumber) &&
            (strcmp(name, other.name) == 0);
    }
    TestClass& operator=(int ser)
    {
        serialNumber = ser;
        itoa(ser, name);
        return *this;
    }
    bool operator!=(const TestClass& other)
    {
        return !operator==(other);
    }
    std::ostream& print(std::ostream& ost) const
    {
        ost << serialNumber << " (\"" << name << ")\"";
        return ost;
    }
    int serial() const
    {
        return serialNumber;
    }
private:
    int serialNumber;
    char* name;
};
std::ostream& operator<<(std::ostream& ost, const TestClass& tc)
{
    return tc.print(ost);
}

const TestClass* buffer;

int my_rand(int lower, int upper)
{
    if (lower == upper) return lower;
    return lower + rand() % (upper - lower);
}

#ifdef USE_POSIX
static Posix_RingbufR<TestClass> rbuf(
    buffer_size, push_pad, pop_pad);
#else
static RingbufR<TestClass> rbuf (buffer_size, push_pad, pop_pad);
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
    buffer = rbuf.buffer_start();

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
        int write_usleep = my_rand(1, write_usleep_range);
        std::this_thread::sleep_for(write_usleep * 1us);
        size_t available;
        TestClass* start;
        rbuf.pushInquire(available, start);
        write_usleep = my_rand(1, write_usleep_range);
        std::this_thread::sleep_for(write_usleep * 1us);
        if (available)
        {
            size_t count = my_rand(1, available);
            if (verbose >= 1)
            {
                std::cout << "(will push " << count <<
                    " at " << start - buffer <<
                    " starting with value " << serial + 1 << ")" << std::endl;
            }
            size_t i = count;
            while (i-- > 0)
            {
                *start++ = ++serial;
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
        size_t read_usleep = my_rand(1, read_usleep_range);
        std::this_thread::sleep_for(read_usleep * 1us);
        size_t available;
        TestClass* start;
        rbuf.popInquire(available, start);
        read_usleep = my_rand(1, read_usleep_range);
        std::this_thread::sleep_for(read_usleep * 1us);
        if (available)
        {
            size_t count = my_rand(1, available);
            if (verbose >= 1)
                std::cout << "(will pop " << count <<
                " starting at " << start - buffer << ")" << std::endl;
            for (size_t i = 0 ; i < count ; ++i)
            {
                ++serial;
                if (start->serial() != serial)
                {
                    std::cout << "*** ERROR *** ";
                    std::cout << "Pop: expected " << serial << " got " <<
                        start << " offset " << i << std::endl;
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
