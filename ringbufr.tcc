// Implementation of RingbufR
#ifndef __RINGBUFR_TCC
#define __RINGBUFR_TCC

static int scarce_fail = 0;

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

    template<typename _T>
    inline void qmove(_T* dest, _T* src, size_t n);
    #define SPECIALIZE_QMOVE(atype)                                  \
        template<>                                                   \
        inline void qmove<atype>(atype* dest, atype* src, size_t n)  \
        {                                                            \
            memcpy(dest, src, sizeof(atype)*n);                      \
        }
    SPECIALIZE_QMOVE(bool)
    SPECIALIZE_QMOVE(unsigned char)
    SPECIALIZE_QMOVE(char)
    SPECIALIZE_QMOVE(unsigned short)
    SPECIALIZE_QMOVE(short)
    SPECIALIZE_QMOVE(unsigned int)
    SPECIALIZE_QMOVE(int)
    SPECIALIZE_QMOVE(unsigned long)
    SPECIALIZE_QMOVE(long)
    SPECIALIZE_QMOVE(unsigned long long)
    SPECIALIZE_QMOVE(long long)
    SPECIALIZE_QMOVE(float)
    SPECIALIZE_QMOVE(double)
    SPECIALIZE_QMOVE(long double)
    template<typename _T>
    inline void qmove(_T* dest, _T* src, size_t n)
    {
        assert(dest <= src);
        qcopy(dest, src, n);
    }
}

template<typename _T>
RingbufR<_T>::RingbufR (size_t capacity, size_t push_pad, size_t pop_pad)
    : _capacity(capacity - push_pad - pop_pad),
      _push_pad(push_pad),
      _pop_pad(pop_pad),
      _edge_start(new _T[capacity]),
      _edge_end(_edge_start + capacity),
      _neutral_start(_edge_start + pop_pad),
      _neutral_end(_edge_end - push_pad)
{
    if (capacity <= (push_pad + pop_pad))
    {
        throw (RingbufRArgumentException());
    }
    _ring_start = _neutral_start;
    _ring_end   = _neutral_end;
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
    size_t used;
    if (_empty)
    {
        assert(_push_next == _pop_next);
        used = 0;
        available = _ring_end - _push_next;
    }
    else
    {
        if (_push_next <= _pop_next)
        {
            // Wrap-around is in effect
            used = (_ring_end - _pop_next) + (_push_next - _ring_start);
            available = _pop_next - _push_next;
        }
        else
        {
            available = _ring_end - _push_next;
            used = _push_next - _pop_next;
        }
        // Over-fill not allowed
        if (used > _capacity)
        {
            available = 0;
        }
        else
        {
            available = std::min(available, (_capacity - used));
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

    // Update complete. Shift buffer to avoid short push later.
    _push_next = new_next;
    adjustEnd();
    if (_push_next == _ring_end) _push_next = _ring_start;

    // A stub may have been created.
    adjustStart();
}

template<typename _T>
void RingbufR<_T>::adjustEnd()
{
    if (_push_next > _pop_next)
    {
        // Wrap-around is not in effect, should proceed.
        size_t unused_ring = _ring_end - _push_next;
        if (unused_ring < _push_pad)
        {
            // Unused space at right of ring buffer is too small. May lead to
            // fragmentation.
            if (_push_next > _neutral_end)
            {
                // Push pad already in use. Don't use it again.
                _ring_end = _push_next;
            }
            else
            {
                // Make the push pad available
                _ring_end = _push_next + _push_pad;
            }
        }
    }
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
    auto state = getState();
    adjustStart();
    state = getState();

    // A small free space may have just been created
    adjustEnd();
    if (_push_next == _ring_end) _push_next = _ring_start;
}

template<typename _T>
void RingbufR<_T>::adjustStart()
{
    auto state = getState();
    if (!_empty)
    {
        // Buffer shifts
        if (_push_next < _pop_next)
        {
            validate(_ring_start, _push_next - _ring_start);
            validate(_pop_next, _ring_end - _pop_next);
            // Wrap-around is in effect, should proceed.
            size_t stub_data = _ring_end - _pop_next;

            // Programming note: we know * _pop_next != _ring_end,
            // so stub_data != 0.

            if (stub_data < _pop_pad)
            {
                // The stub data is too small. 
                if (_ring_start >= (_edge_start + stub_data))
                {
                    // There is room to move left.
                    _ring_start -= stub_data;
                    assert(_ring_start >= _edge_start);
                    qcopy(_ring_start, _pop_next, stub_data);
                    _pop_next = _ring_start;
                    validate(_pop_next, _push_next - _pop_next);
                }
                else
                {
                    // No room to move left.
                    // Instead, move old data into stub.

                    size_t available =
                        std::min(_neutral_start, _push_next) - _ring_start;
                    size_t reverse_stub = _pop_pad - stub_data;
                    if (available < reverse_stub)
                    {
                        reverse_stub = available;
                    }
                    _T* new_pop = _pop_next - reverse_stub;
                    if (new_pop >= _push_next)
                    {
                        // Shift stub to left to make room
                        validate(_ring_start, _push_next - _ring_start);
                        qmove(new_pop, _pop_next, _ring_end - _pop_next);
                        validate(new_pop, _ring_end - _pop_next);
                        // Now we can move the leftmost data into place
                        qcopy(
                            _ring_end - reverse_stub, _ring_start,
                            reverse_stub);
                        validate(_ring_end - reverse_stub, reverse_stub);
                        validate(new_pop, _ring_end - new_pop);
                        _ring_start += reverse_stub;
                        assert(_ring_start <= _neutral_start);
                        _pop_next = new_pop;
                        // assert((_pop_next + ???) == _ring_end);
                        // validate(_pop_next, ???);
                        validate(_ring_start, _push_next - _ring_start);
                        validate(_pop_next, _ring_end - _pop_next);
                    }
                    else
                    {
                        ++scarce_fail;
                    }
                }
            }
            state = getState();
        }
        else
        {
            // Wrap-around is not in effect. Conserve the pop pad.
            validate(_pop_next, _push_next - _pop_next);
            if (_pop_next != _push_next) // Experimental "if"
            {
                if (_pop_next <= _neutral_start)
                {
                    _ring_start = _pop_next;
                }
            }
            validate(_pop_next, _push_next - _pop_next);
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
const _T* RingbufR<_T>::buffer_start() const
{
    return _edge_start;
}

template<typename _T>
const _T* RingbufR<_T>::ring_start() const
{
    return _ring_start;
}

template<typename _T>
const _T* RingbufR<_T>::ring_end() const
{
    return _ring_end;
}

template<typename _T>
RingbufR<_T>::debugState RingbufR<_T>::getState() const
{
    debugState state;
    state.ring_start = _ring_start - _edge_start;
    state.ring_end = _ring_end - _edge_start;
    state.neutral_start = _neutral_start - _edge_start;
    state.neutral_end = _neutral_end - _edge_start;
    state.pop_next = _pop_next - _edge_start;
    state.push_next = _push_next - _edge_start;
    state.empty = _empty;
    return state;
}

template<typename _T>
void RingbufR<_T>::validate(const _T* /*start*/, size_t /*count*/)
{
    // The user can override as desired.
}

#endif // __RINGBUFR_TCC
