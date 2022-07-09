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
class RingbufRArgumentException
    : public RingbufRException
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

    RingbufR (size_t capacity, size_t push_pad=0, size_t pop_pad=0);
    virtual ~RingbufR ();
    RingbufR(const RingbufR&) = delete;
    RingbufR() = delete;
    RingbufR(RingbufR&&) = delete;
    RingbufR& operator=(const RingbufR&) = delete;
    RingbufR& operator=(RingbufR&&) = delete;

    virtual void pushInquire(size_t& available, _T*& start) const;
    void push(size_t newContent);
    virtual void popInquire(size_t& available, _T*& start) const;
    void pop(size_t oldContent);
    size_t size() const;

    // For debugging
    const _T* buffer_start() const;
    const _T* ring_start() const;

private:

    const size_t _capacity;
    const size_t _push_pad;
    const size_t _pop_pad;
    _T* const _edge_start;
    _T* const _edge_end;
    bool _empty;
    _T* _push_next;
    _T* _pop_next;
    _T* _ring_start;
    _T* _ring_end;
};

#endif // __RINGBUFR_H_
