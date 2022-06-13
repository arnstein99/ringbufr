#ifndef __FD_COPY_H_
#define __FD_COPY_H_
#include <unistd.h>

size_t copyfd(int readfd, int writefd, size_t chunk_size);

void errorexit(const char* message);
#define ZEROCHECK(message,retval) \
    do { \
	if ((retval) != 0) \
	    errorexit(message); \
    } while (false)
#define NEGCHECK(message,retval) \
    do { \
	if ((retval) < 0) \
	    errorexit(message); \
    } while (false)

#endif // __FD_COPY_H_
