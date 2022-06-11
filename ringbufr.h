// RingbufR classes
// A ring buffer and associated classes that are mostly in the style
// of the C++ Standard Template Library

#ifndef __RINGBUFR_H_
#define __RINGBUFR_H_

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

    RingbufR (size_t capacity);
    virtual ~RingbufR ();
    void pushInquire(size_t& available, _T*& start) const;
    void push(size_t newContent);
    void popInquire(size_t& available, _T*& start) const;
    void pop(size_t oldContent);

protected:

    virtual void updateStart(size_t increment);
    virtual void updateEnd(size_t increment);

private:

    RingbufR() = delete; // No default constructor
    RingbufR(const RingbufR<_T>&) = delete; // No copy constructor

    const size_t _capacity;
    _T* const _ring_start;
    _T* const _ring_end;
    bool _empty;
    _T* _push_next;
    _T* _pop_next;
};

// Implementation
#include "ringbufr.tcc"

#endif // __RINGBUFR_H_
