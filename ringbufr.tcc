// Implementation of RingbufR

#include <cassert>
#include <algorithm>

template<typename _T>
RingbufR<_T>::RingbufR (size_t capacity, size_t edge_guard)
    : _capacity(capacity),
      _edge_guard(edge_guard),
      _edge_start(new _T[capacity + 2*edge_guard]),
      _ring_start(_edge_start + edge_guard),
      _ring_end(_ring_start + capacity),
      _edge_end(_ring_end + edge_guard)
{
    _empty = true;
    _push_next = _ring_start;
    _pop_next  = _ring_start;
}

template<typename _T>
RingbufR<_T>::~RingbufR()
{
    delete[] _edge_start;
}

template<typename _T>
size_t RingbufR<_T>::compute_move(size_t& guard_available) const
{
    guard_available = 0;
    size_t available = _ring_end - _push_next;
    if (available < _edge_guard)
    {
        if (_pop_next > _ring_start)
        {
            guard_available = _pop_next - _ring_start;
            std::size_t excess = _edge_guard - available;
            guard_available = std::min(guard_available, excess);
            available += guard_available;
        }
    }
    return available;
}

template<typename _T>
void RingbufR<_T>::pushInquire(size_t& available, _T*& start) const
{
    // Take care of a special case
    if (!_empty && (_push_next == _pop_next))
    {
        // Buffer is full
        available = 0;
        start = nullptr;
        return;
    }

    if (_push_next < _pop_next)
    {
        // Wrap-around is in effect
        available = _pop_next - _push_next;
    }
    else
    {
        size_t guard_available;
        available = compute_move(guard_available);
    }
    start = _push_next;
}

// push
template<typename _T>
void RingbufR<_T>::updateEnd(size_t increment)
{
    // Take care of special cases
    if (increment == 0) return;
    if (!_empty && (_push_next == _pop_next))
    {
        // Buffer is full, so corruption has already occurred.
        throw RingbufRFullException();
    }
    _empty = false;

    _T* limit;
    size_t guard_available = 0;
    if (_push_next < _pop_next)
    {
        // Wrap-around is in effect
        limit = _pop_next;
    }
    else
    {
        size_t available = compute_move(guard_available);
        limit = _push_next + available;
    }
    _push_next += increment;
    if (_push_next > limit)
    {
        // Corruption has already occurred.
        throw RingbufRFullException();
    }

    // Wrap-around could begin here
    if (guard_available > 0)
    {
        // Move excess data to start
        _push_next = _ring_start + guard_available;
        _T* source = _ring_end;
        _T* dest   = _ring_start;
        while (guard_available--)
            *dest++ = *source++;
    }
    if (_push_next == _ring_end) _push_next = _ring_start;
}

template<typename _T>
void RingbufR<_T>::push(size_t newContent)
{
    updateEnd(newContent);
}

template<typename _T>
void RingbufR<_T>::popInquire(size_t& available, _T*& start) const
{
    // Take care of special case
    if (_empty)
    {
        assert(_push_next == _pop_next);
        available = 0;
        start = nullptr;
        return;
    }

    if (_push_next <= _pop_next)
    {
        // Wrap-around is in effect
        available = _ring_end - _pop_next;
    }
    else
    {
        available = _push_next - _pop_next;
    }
    start = _pop_next;
}

// pop
template<typename _T>
void RingbufR<_T>::updateStart(size_t increment)
{
    // Take care of special cases
    if (increment == 0) return;
    if (_empty) throw RingbufREmptyException();

    _T* limit;
    _T* new_next = _pop_next + increment;
#ifdef LATER
    size_t stub_data = 0;
#endif
    if (_push_next <= _pop_next)
    {
        // Wrap around is in effect
        limit = _ring_end;
#ifdef LATER
        stub_data = _ring_end - new_next;
#endif
    }
    else
    {
        limit = _push_next;
    }
    if (new_next > limit)
    {
        // Invalid data has already been copied out
        throw RingbufREmptyException();
    }

#ifdef LATER
    // Edge guard: move data from end of buffer.
    if (stub_data < _edge_guard)
    {
        _T* source = new_next;
        new_next = _ring_start - stub_data;
        _T* dest = new_next;
        while (stub_data-- > 0)
            *dest++ = *source++;
    }
#endif

    if (new_next == _ring_end) new_next = _ring_start;
    _pop_next = new_next;
    _empty = (_pop_next == _push_next);
}

template<typename _T>
void RingbufR<_T>::pop(size_t oldContent)
{
    updateStart(oldContent);
}
