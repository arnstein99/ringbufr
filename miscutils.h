#ifndef __MISCUTILS_H_
#define __MISCUTILS_H_

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

#endif // __MISCUTILS_H_
