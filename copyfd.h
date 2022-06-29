#ifndef __FD_COPY_H_
#define __FD_COPY_H_
#include <unistd.h>
#include <atomic>

// For read and write errors
class IOException
{
public:
    IOException(size_t bc) : byte_count(bc) {}
    size_t byte_count;
};
class ReadException : public IOException
{
public:
    ReadException(size_t bc) : IOException(bc) {}
};
class WriteException : public IOException
{
public:
    WriteException(size_t bc) : IOException(bc) {}
};

size_t copyfd(
    int readfd, int writefd,
    size_t buffer_size, size_t push_pad=0, size_t pop_pad=0);

size_t copyfd_while(
    int readfd, int writefd,
    const std::atomic<bool>& continue_flag, long check_usec,
    size_t buffer_size, size_t push_pad=0, size_t pop_pad=0);

#endif // __FD_COPY_H_
