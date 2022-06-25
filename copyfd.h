#ifndef __FD_COPY_H_
#define __FD_COPY_H_
#include <unistd.h>

size_t
copyfd(
    int readfd, int writefd,
    size_t buffer_size, size_t push_pad=0, size_t pop_pad=0);

#endif // __FD_COPY_H_
