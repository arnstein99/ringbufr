#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <cstring>
#include "copyfd.h"

void set_nonblock_flag(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    flags |= O_NONBLOCK;
    int retval = fcntl(fd, F_SETFL, flags);
    if (retval != 0)
    {
        std::cerr << "fcntl " << strerror(errno) << std::endl;
	exit(retval);
    }
}

int main (int, char* [])
{
    set_nonblock_flag(0);
    set_nonblock_flag(1);
    auto bytes_processed = copyfd(0, 1, 64*1024);
    std::cerr << bytes_processed << " copied" << std::endl;
}
