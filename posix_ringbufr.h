// Specialization of RingbufR class with Posix mutex added

#ifndef __POSIX_RINGBUFR_H_
#define __POSIX_RINGBUFR_H_

#include <stdexcept>
#include <mutex>
#include <pthread.h>
#include "ringbufr.h"

template<typename _T>
class Posix_RingbufR : public RingbufR<_T>
{
public:

    Posix_RingbufR (size_t capacity)
        : RingbufR<_T> (capacity)
    {
    }

    ~Posix_RingbufR ()
    {
    }

protected:

    void updateStart(size_t increment) override
    {
        auto lock = std::lock_guard(_mutex);
	RingbufR<_T>::updateStart(increment);
    }

    void updateEnd(size_t increment) override
    {
        auto lock = std::lock_guard(_mutex);
	RingbufR<_T>::updateEnd(increment);
    }

private:

    std::mutex _mutex;
};

#endif // __POSIX_RINGBUFR_H_
