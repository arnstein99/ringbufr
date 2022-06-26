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

    Posix_RingbufR (
            size_t capacity, int verbose,
            size_t push_pad=0, size_t pop_pad=0)
        : RingbufR<_T> (capacity, push_pad, pop_pad),
          _verbose(verbose)
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
        dump("pop:");
    }

    void updateEnd(size_t increment) override
    {
        auto lock = std::lock_guard(_mutex);
        RingbufR<_T>::updateEnd(increment);
        dump("push:");
    }

private:

    void dump(const std::string& msg)
    {
        if (_verbose)
        {
            std::cout << msg <<
                " next pop " << this->_pop_next - this->_ring_start <<
                " next push " << this->_push_next - this->_ring_start <<
            " empty " << (this->_empty ? "true" : "false") << std::endl;
        }
    }

    std::mutex _mutex;
    bool _verbose;
};

#endif // __POSIX_RINGBUFR_H_
