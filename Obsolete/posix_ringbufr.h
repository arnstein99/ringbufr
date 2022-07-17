// Specialization of RingbufR class with Posix mutex added

#ifndef __POSIX_RINGBUFR_H_
#define __POSIX_RINGBUFR_H_

#include <stdexcept>
#include <mutex>
#include <iostream>
#include <cstddef>
#include "ringbufr.h"

template<typename _T>
class Posix_RingbufR : public RingbufR<_T>
{
public:
    Posix_RingbufR ( size_t capacity, size_t push_pad=0, size_t pop_pad=0)
        : RingbufR<_T> (capacity, push_pad, pop_pad)
    {
    }

    ~Posix_RingbufR () override
    {
    }

protected:
    void updateStart(size_t increment) override
    {
        const std::lock_guard<std::mutex> lock(_mutex);
        RingbufR<_T>::updateStart(increment);
    }
    void updateEnd(size_t increment) override
    {
        const std::lock_guard<std::mutex> lock(_mutex);
        RingbufR<_T>::updateEnd(increment);
    }

private:
    std::mutex _mutex;
};

#endif // __POSIX_RINGBUFR_H_
