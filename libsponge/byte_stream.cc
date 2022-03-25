#include "byte_stream.hh"

#include <cstring>

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity) : _bytes(), _capacity(capacity) {
    _capacity = _capacity > 4 * 1024 * 1024 ? 4 * 1024 * 1024 : _capacity;
    _bytes.resize(_capacity);
}

size_t ByteStream::write(const string &data) {
    size_t writed = 0;
    if (writeable() < data.size())
        noCap();

    int need_write = writeable() > data.size() ? data.size() : writeable();
    ::memmove(_bytes.data() + _writePos, data.data(), need_write);
    writed += need_write;
    _writePos += need_write;
    _hasWrited += writed;
    return writed;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const { return {_bytes.data() + _readPos, len}; }

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    _readPos += len;
    _hasReaded += len;

    // _bytes = _bytes.substr(_read)
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    size_t bs = buffer_size();
    size_t need_read = len > bs ? bs : len;
    string ret{_bytes.data() + _readPos, need_read};
    pop_output(need_read);
    return ret;
}

void ByteStream::noCap() {
    if (buffer_size() == _capacity)
        return;
    _bytes = _bytes.substr(_readPos, buffer_size());
    _readPos = 0;
    _writePos = _bytes.size();
    _bytes.resize(_capacity);
}

void ByteStream::end_input() { _writeClosed = true; }

bool ByteStream::input_ended() const { return _writeClosed; }

size_t ByteStream::buffer_size() const { return _writePos - _readPos; }

bool ByteStream::buffer_empty() const { return buffer_size() == 0; }

bool ByteStream::eof() const { return _readPos == _writePos && _writeClosed; }

size_t ByteStream::bytes_written() const { return _hasWrited; }

size_t ByteStream::bytes_read() const { return _hasReaded; }

size_t ByteStream::remaining_capacity() const { return _capacity - _writePos + _readPos; }
size_t ByteStream::writeable() const { return _capacity - _writePos; }
