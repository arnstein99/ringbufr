// Specialization of RingbufR class with Posix mutex added

#ifndef __POSIX_RINGBUFR_H_
#define __POSIX_RINGBUFR_H_

#include <stdexcept>
#include <pthread.h>
#include "ringbufr.h"

template<typename _T>
class Posix_RingbufR : public RingbufR<_T>
{
public:

    Posix_RingbufR (size_t capacity)
        : RingbufR<_T> (capacity)
    {
        if (pthread_mutex_init (&_mutex, NULL) != 0)
	    throw std::runtime_error("mutex init");
    }

    ~Posix_RingbufR ()
    {
	// TODO: handle an error here?
        pthread_mutex_destroy (&_mutex);
    }

protected:

    void updateStart(_T* newStart) override
    {
        if (pthread_mutex_lock (&_mutex) != 0)
	    throw std::runtime_error("mutex lock");
	RingbufR<_T>::updateStart(newStart);
        if (pthread_mutex_unlock (&_mutex) != 0)
	    throw std::runtime_error("mutex unlock");
    }

    void updateEnd(_T* newEnd) override
    {
        if (pthread_mutex_lock (&_mutex) != 0)
	    throw std::runtime_error("mutex lock");
	RingbufR<_T>::updateEnd(newEnd);
        if (pthread_mutex_unlock (&_mutex) != 0)
	    throw std::runtime_error("mutex unlock");
    }

private:

    pthread_mutex_t _mutex;
};

#endif // __POSIX_RINGBUFR_H_
