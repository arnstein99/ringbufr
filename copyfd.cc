#include "ringbufr.h"

#include <sys/select.h>
#include <algorithm>
#include <cstring>
#include <iostream>
#include <unistd.h>

// #define VERBOSE
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
            bytes_read = read(readfd, read_start, read_available);
            if (bytes_read < 0)
            {
                if ((errno == EWOULDBLOCK) || (errno == EAGAIN))
                {
                    FD_ZERO(&read_set);
                    FD_SET(readfd, &read_set);
                    p_read_set = &read_set;
                }
                else
                {
		    std::cerr << "read: " << strerror(errno) << std::endl;
                    break;
                }
            }
            else if (bytes_read == 0)
            {
                // End of input
                break;
            }
            else
            {
                bufr.push(bytes_read);
            }
	}

	bytes_write = 0;
	unsigned char* write_start;
	bufr.popInquire(write_available, write_start);
	if (write_available)
	{
            CHECKPOINT;
	    bytes_write = write(writefd, write_start, write_available);
	    if (bytes_write < 0)
	    {
                CHECKPOINT;
		if ((errno == EWOULDBLOCK) || (errno == EAGAIN))
		{
                    CHECKPOINT;
		    FD_ZERO(&write_set);
		    FD_SET(writefd, &write_set);
		    p_write_set = &write_set;
		}
		else
		{
                    CHECKPOINT;
		    std::cerr << "write: " << strerror(errno) << std::endl;
		    break;
		}
	    }
            else if (bytes_write == 0)
            {
                CHECKPOINT;
                std::cerr << "copyfd: EOF on write descriptor" << std::endl;
                break;
            }
            else
	    {
                CHECKPOINT;
	        bufr.pop(bytes_write);
		bytes_processed += bytes_write;
	    }
	}

        if (p_read_set || p_write_set)
        {
            int select_return = 
                select(maxfd, p_read_set, p_write_set, nullptr, nullptr);
            if (select_return < 0)
            {
                std::cerr << "select: " << strerror(errno) << std::endl;
                continue;
            }
        }

#ifdef VERBOSE
	std::cerr << "read " << bytes_read << " write " << bytes_write <<
	std::endl;
#endif

    } while (bytes_read || bytes_write);
    // Includes negative values, meaning select() was just called.

    return bytes_processed;
}
