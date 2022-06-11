// Implementation of RingbufR

template<typename _T>
RingbufR<_T>::RingbufR (size_t capacity)
    : _capacity(capacity)
{
    _ring_start = new _T[capacity];
    _ring_end  = _ring_start + capacity;
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
    if (_push_next < _pop_next)
    {
	// Deal with wrap-around
	available = _pop_next - _push_next;
    }
    else
    {
	available = _ring_end - _push_next;
    }
    start = _push_next;
}

template<typename _T>
void RingbufR<_T>::push(size_t newContent)
{
    auto new_next = _push_next + newContent;
    _T* limit;
    if (_push_next < _pop_next)
    {
	// Deal with wrap-around
	limit = _pop_next;
    }
    else
    {
        limit = _ring_end;
    }

    if (new_next > limit)
    {
        // Corruption has already occurred.
        throw RingbufRFullException();
    }
    if (new_next == _ring_end) new_next = _ring_start;
    updateEnd(new_next);
}

template<typename _T>
void RingbufR<_T>::popInquire(size_t& available, _T*& start) const
{
    if (_push_next < _pop_next)
    {
	// Deal with wrap-around
	available = _ring_end - _pop_next;
    }
    else
    {
	available = _push_next - _pop_next;
    }
    start = _pop_next;
}

template<typename _T>
void RingbufR<_T>::pop(size_t oldContent)
{
    auto new_next = _pop_next + oldContent;
    _T* limit;
    if (_push_next < _pop_next)
    {
	// Deal with wrap-around
	limit = _ring_end;
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
    if (new_next == _ring_end) new_next = _ring_start;
    updateStart(new_next);
}
