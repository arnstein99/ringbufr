#include "copyfd.h"
#include "miscutils.h"

#include "ringbufr.h"
#include <sys/select.h>
#include <algorithm>
#include <cstring>
#include <iostream>
#include <unistd.h>

#include <chrono>
using namespace std::chrono;

#define VERBOSE
#define CHECKPOINT \
    do { \
        /* std::cerr << "checkpoint " << __LINE__ << std::endl; */ \
    } while (false)

size_t copyfd(int readfd, int writefd, size_t chunk_size)
{
    int maxfd = std::max(readfd, writefd) + 1;
    fd_set read_set;
    fd_set write_set;

    RingbufR<unsigned char> bufr(3 * chunk_size);

    ssize_t bytes_read = 0;
    ssize_t bytes_write = 0;
    size_t bytes_processed = 0;
    size_t read_available;
    size_t write_available;
    do
    {
        fd_set* p_read_set = nullptr;
        fd_set* p_write_set = nullptr;

        unsigned char* read_start;
        bufr.pushInquire(read_available, read_start);
        CHECKPOINT;
        bytes_read = 0;
        if (read_available)
        {
            CHECKPOINT;
            read_available = std::min(read_available, chunk_size);
            auto before = system_clock::now();
            bytes_read = read(readfd, read_start, read_available);
            auto after = system_clock::now();
            auto dur = duration_cast<milliseconds>(after - before).count();
            std::cerr << "read time " << dur << std::endl;
            if (bytes_read < 0)
            {
                if ((errno == EWOULDBLOCK) || (errno == EAGAIN))
                {
                    // This is when to select()
                    FD_ZERO(&read_set);
                    FD_SET(readfd, &read_set);
                    p_read_set = &read_set;
                }
                else
                {
                    // Some other error on input
                    errorexit("read");
                }
            }
            else if (bytes_read == 0)
            {
                // End of input
                ;
            }
            else
            {
                // Some data was input, no need to select.
                bufr.push(bytes_read);
            }
        }

        bytes_write = 0;
        unsigned char* write_start;
        bufr.popInquire(write_available, write_start);
        if (write_available)
        {
            CHECKPOINT;
            auto before = system_clock::now();
            bytes_write = write(writefd, write_start, write_available);
            auto after = system_clock::now();
            auto dur = duration_cast<milliseconds>(after - before).count();
            std::cerr << "write time " << dur << std::endl;
            if (bytes_write < 0)
            {
                CHECKPOINT;
                if ((errno == EWOULDBLOCK) || (errno == EAGAIN))
                {
                    CHECKPOINT;
                    // This is when to select().
                    FD_ZERO(&write_set);
                    FD_SET(writefd, &write_set);
                    p_write_set = &write_set;
                }
                else
                {
                    CHECKPOINT;
                    // Some other error on write
                    errorexit("write");
                }
            }
            else if (bytes_write == 0)
            {
                CHECKPOINT;
                // Cannot accept EOF on write.
                std::cerr << "copyfd: EOF on write descriptor" << std::endl;
                exit(1);
            }
            else
            {
                CHECKPOINT;
                // Some data was output, no need to select.
                bufr.pop(bytes_write);
                bytes_processed += bytes_write;
            }
        }

        // Only block if really necessary
        if (bytes_read  > 0) p_write_set = nullptr;
        if (bytes_write > 0) p_read_set  = nullptr;

        if (p_read_set || p_write_set)
        {
            int select_return;
            NEGCHECK("select",
                (select_return = select(
                    maxfd, p_read_set, p_write_set, nullptr, nullptr)));
        }

#ifdef VERBOSE
    std::cerr << "read " << bytes_read << " " << " write " << bytes_write;
    std::cerr << " ";
    std::cerr << (p_read_set  ? "x" : "|");
    std::cerr << (p_write_set ? "x" : "|");
    std::cerr << std::endl;
#endif

    } while (bytes_read || bytes_write);
    // Includes negative values, meaning select() was just called.

    return bytes_processed;
}
