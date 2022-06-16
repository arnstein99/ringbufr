#ifndef __FD_COPY_H_
#define __FD_COPY_H_
#include <unistd.h>

size_t copyfd(int readfd, int writefd, size_t chunk_size);

#endif // __FD_COPY_H_
