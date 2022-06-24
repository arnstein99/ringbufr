// RingbufR classes
// A ring buffer and associated classes that are mostly in the style
// of the C++ Standard Template Library

#ifndef __RINGBUFR_H_
#define __RINGBUFR_H_

#include <cstddef>

// Some exceptions for use with this class
class RingbufRException
{
};
class RingbufREmptyException
    : public RingbufRException
{
};
class RingbufRFullException
    : public RingbufRException
{
};

// Class RingbufR
template<typename _T>
class RingbufR
{
public:

    RingbufR (size_t capacity, size_t edge_guard = 0);
    virtual ~RingbufR ();
    RingbufR() = delete; // No default constructor
    RingbufR(const RingbufR<_T>&) = delete; // No copy constructor

    void pushInquire(size_t& available, _T*& start) const;
    void push(size_t newContent);
    void popInquire(size_t& available, _T*& start) const;
    void pop(size_t oldContent);
    size_t size() const;

    // For debugging
    const _T* ring_start() const;

protected:

    virtual void updateStart(size_t increment);
    virtual void updateEnd(size_t increment);

    const size_t _capacity;
    const size_t _edge_guard;
    _T* const _edge_start;
    _T* const _edge_end;
    bool _empty;
    _T* _push_next;
    _T* _pop_next;
    _T* _ring_start;
    _T* _ring_end;
};

// Implementation
#include "ringbufr.tcc"

#endif // __RINGBUFR_H_
