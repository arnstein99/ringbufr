// Implementation of RingbufR
#ifndef __RINGBUFR_TCC
#define __RINGBUFR_TCC

#include <cassert>
#include <algorithm>
#include <string.h>

namespace {
    template<typename _T>
    inline void qcopy(_T* dest, _T* src, size_t n);
    #define SPECIALIZE_QCOPY(atype)                                  \
        template<>                                                   \
        inline void qcopy<atype>(atype* dest, atype* src, size_t n)  \
        {                                                            \
            memcpy(dest, src, sizeof(atype)*n);                      \
        }
    SPECIALIZE_QCOPY(bool)
    SPECIALIZE_QCOPY(unsigned char)
    SPECIALIZE_QCOPY(char)
    SPECIALIZE_QCOPY(unsigned short)
    SPECIALIZE_QCOPY(short)
    SPECIALIZE_QCOPY(unsigned int)
    SPECIALIZE_QCOPY(int)
    SPECIALIZE_QCOPY(unsigned long)
    SPECIALIZE_QCOPY(long)
    SPECIALIZE_QCOPY(unsigned long long)
    SPECIALIZE_QCOPY(long long)
    SPECIALIZE_QCOPY(float)
    SPECIALIZE_QCOPY(double)
    SPECIALIZE_QCOPY(long double)
    template<typename _T>
    inline void qcopy(_T* dest, _T* src, size_t n)
    {
        while (n-- > 0)
        {
            // *dest++ = std::move(src++);
            auto& des = *dest++;
            auto& sr  = *src++;
            des = std::move(sr);
        }
    }
}

template<typename _T>
RingbufR<_T>::RingbufR (size_t capacity, size_t push_pad, size_t pop_pad)
    : _capacity(capacity - push_pad - pop_pad),
      _push_pad(push_pad),
      _pop_pad(pop_pad),
      _edge_start(new _T[capacity]),
      _edge_end(_edge_start + capacity)
{
    if (capacity <= (push_pad + pop_pad))
    {
        throw (RingbufRArgumentException());
    }
    _ring_start = _edge_start + push_pad + pop_pad;
    _ring_end   = _ring_start + _capacity;
    _push_next  = _ring_start;
    _pop_next   = _ring_start;
    _empty = true;
}

template<typename _T>
RingbufR<_T>::~RingbufR()
{
    delete[] _edge_start;
}

template<typename _T>
void RingbufR<_T>::pushInquire(size_t& available, _T*& start) const
{
    if (_empty)
    {
        assert(_push_next == _pop_next);
        available = _ring_end - _push_next;
    }
    else
    {
        if (_push_next <= _pop_next)
        {
            // Wrap-around is in effect
            available = _pop_next - _push_next;
        }
        else
        {
            available = _ring_end - _push_next;
        }
    }
    start = _push_next;
}

template<typename _T>
void RingbufR<_T>::push(size_t increment)
{
    if (increment == 0) return;

    auto new_next = _push_next + increment;
    _T* limit;
    if (_empty)
    {
        assert(_push_next == _pop_next);
        limit = _ring_end;
        _empty = false;
    }
    else
    {
        if (_push_next <= _pop_next)
        {
            // Wrap-around is in effect
            limit = _pop_next;
        }
        else
        {
            limit = _ring_end;
        }
    }

    if (new_next > limit)
    {
        // Corruption has already occurred.
        throw RingbufRFullException();
    }

    // Update complete. Shift buffer to left if appropriate.
    _push_next = new_next;
    if (_push_next > _pop_next)
    {
        // Wrap-around is not in effect, can proceed.
        size_t unused_ring = _ring_end - _push_next;
        if (unused_ring < _push_pad)
        {
            // Unused space at right of ring buffer is too small. May lead to
            // fragmentation.
            size_t unused_edge = _ring_start - _edge_start;
            if (unused_edge >= unused_ring)
            {
                // Shift left
                _ring_start -= unused_ring;
                _ring_end   -= unused_ring;
            }
        }
    }

    if (_push_next == _ring_end) _push_next = _ring_start;
}

template<typename _T>
void RingbufR<_T>::popInquire(size_t& available, _T*& start) const
{
    if (_empty)
    {
        assert(_push_next == _pop_next);
        available = 0;
    }
    else
    {
        if (_push_next <= _pop_next)
        {
            // Wrap-around is in effect
            available = _ring_end - _pop_next;
        }
        else
        {
            available = _push_next - _pop_next;
        }
    }
    start = _pop_next;
}

// pop
template<typename _T>
void RingbufR<_T>::pop(size_t increment)
{
    if (increment == 0) return;

    auto new_next = _pop_next + increment;
    _T* limit;
    if (_empty)
    {
        assert(_push_next == _pop_next);
        limit = _ring_end;
    }
    else
    {
        if (_push_next <= _pop_next)
        {
            // Wrap-around is in effect
            limit = _ring_end;
        }
        else
        {
            limit = _push_next;
        }
    }
    if (new_next > limit)
    {
        // Invalid data has already been copied out
        throw RingbufREmptyException();
    }

    // Update complete.
    _pop_next = new_next;
    if (_pop_next == _ring_end) _pop_next = _ring_start;
    _empty = (_pop_next == _push_next);

    if (!_empty)
    {
        // Buffer shifts
        if (_push_next <= _pop_next)
        {
            // Wrap-around is in effect
            size_t stub_data = _ring_end - _pop_next;

            // Programming note: we know * _pop_next != _ring_end,
            // so stub_data != 0.

            if (stub_data < _pop_pad)
            {
                // The stub data is too small. Try to shift it.
                size_t buffer_available = _ring_start - _edge_start;
                if (buffer_available >= stub_data)
                {
                    _ring_start -= stub_data;
                    _ring_end   -= stub_data;
                    qcopy(_ring_start, _pop_next, stub_data);
                    _pop_next = _ring_start;
                }
            }
        }
        else
        {
            // Wrap-around is not in effect.
            size_t unused_ring = _pop_next - _ring_start;
            size_t unused_buffer = _edge_end - _ring_end;
            size_t right_shift = std::min(unused_ring, unused_buffer);
            // Shift buffer to the left to recover from past shifts.
            _ring_start += right_shift;
            _ring_end   += right_shift;
        }
    }
}

template<typename _T>
size_t RingbufR<_T>::size() const
{
    if (_empty) return 0;
    if (_push_next < _pop_next)
    {
        // Wrap-around is in effect
        return (_ring_end - _pop_next) + (_push_next - _ring_start);
    }
    else if (_push_next == _pop_next)
    {
        // Ring buffer is full
        return (_ring_end - _ring_start);
    }
    else // (_push_next > _pop_next)
    {
        // Wrap-around is not in effect
        return (_push_next - _pop_next);
    }
}

template<typename _T>
const _T* RingbufR<_T>::ring_start() const
{
    return _ring_start;
}

template<typename _T>
const _T* RingbufR<_T>::buffer_start() const
{
    return _edge_start;
}

#endif // __RINGBUFR_TCC
