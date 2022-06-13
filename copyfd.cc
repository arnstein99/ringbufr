#include "ringbufr.h"

#include <sys/select.h>
#include <algorithm>
#include <cstring>
#include <iostream>
#include <unistd.h>

// #define VERBOSE

size_t copyfd(int readfd, int writefd, size_t chunk_size)
{
    int maxfd = std::max(readfd, writefd) + 1;
    fd_set read_set;
    fd_set write_set;

    RingbufR<unsigned char> bufr(3 * chunk_size);

    ssize_t bytes_read = 0;
    ssize_t bytes_write = 0;
    size_t bytes_processed = 0;
    do
    {
	fd_set* p_read_set = nullptr;
	fd_set* p_write_set = nullptr;

	unsigned char* read_start;
	size_t read_available;
	bufr.pushInquire(read_available, read_start);
	if (read_available)
	{
	    read_available = std::min(read_available, chunk_size);
	    FD_ZERO(&read_set);
	    FD_SET(readfd, &read_set);
	    p_read_set = &read_set;
	}

	bytes_write = 0;
	unsigned char* write_start;
	size_t write_available;
	bufr.popInquire(write_available, write_start);
	if (write_available)
	{
	    bytes_write = write(writefd, write_start, write_available);
	    if (bytes_write < 0)
	    {
		if ((errno == EWOULDBLOCK) || (errno == EAGAIN))
		{
		    FD_ZERO(&write_set);
		    FD_SET(writefd, &write_set);
		    p_write_set = &write_set;
		}
		else
		{
		    std::cerr << "write " << strerror(errno) << std::endl;
		    break;
		}
	    }
	    else
	    {
	        bufr.pop(bytes_write);
		bytes_processed += bytes_write;
	    }
	}

        int select_return = 
	    select(maxfd, p_read_set, p_write_set, nullptr, nullptr);
	if (select_return < 0)
	{
	    std::cerr << "select " << strerror(errno) << std::endl;
	    continue;
	}

	bytes_read = 0;
	if (p_read_set && FD_ISSET(readfd, p_read_set))
	{
	    bytes_read = read(readfd, read_start, read_available);
	    if (bytes_read < 0)
	    {
		std::cerr << "read " << strerror(errno) << std::endl;
		break;
	    }
	    else
	    {
	        bufr.push(bytes_read);
	    }
	}

#ifdef VERBOSE
	std::cerr << "read " << bytes_read << " write " << bytes_write <<
	std::endl;
#endif

    } while (bytes_read || bytes_write);

    return bytes_processed;
}
