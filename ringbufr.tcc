// Implementation of RingbufR

#include <cassert>

template<typename _T>
RingbufR<_T>::RingbufR (size_t capacity)
    : _capacity(capacity),
      _ring_start(new _T[capacity]),
      _ring_end(_ring_start + capacity)
{
    _empty = true;
    _push_next = _ring_start;
    _pop_next  = _ring_start;
}

template<typename _T>
RingbufR<_T>::~RingbufR()
{
    delete[] _ring_start;
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
            // Deal with wrap-around
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
void RingbufR<_T>::updateEnd(size_t increment)
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
            // Deal with wrap-around
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
    if (new_next == _ring_end) new_next = _ring_start;
    _push_next = new_next;
}

template<typename _T>
void RingbufR<_T>::push(size_t newContent)
{
    updateEnd(newContent);
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
            // Deal with wrap-around
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
void RingbufR<_T>::updateStart(size_t increment)
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
            // Deal with wrap-around
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
    if (new_next == _ring_end) new_next = _ring_start;
    _pop_next = new_next;
    _empty = (_pop_next == _push_next);
}

template<typename _T>
void RingbufR<_T>::pop(size_t oldContent)
{
    updateStart(oldContent);
}
