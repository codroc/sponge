#include "wrapping_integers.hh"
#include <cmath>

// Dummy implementation of a 32-bit wrapping integer

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! Transform an "absolute" 64-bit sequence number (zero-indexed) into a WrappingInt32
//! \param n The input absolute 64-bit sequence number
//! \param isn The initial sequence number
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) {
    return isn + static_cast<uint32_t>(n % (1ul << 32));
}

//! Transform a WrappingInt32 into an "absolute" 64-bit sequence number (zero-indexed)
//! \param n The relative sequence number
//! \param isn The initial sequence number
//! \param checkpoint A recent absolute 64-bit sequence number
//! \returns the 64-bit sequence number that wraps to `n` and is closest to `checkpoint`
//!
//! \note Each of the two streams of the TCP connection has its own ISN. One stream
//! runs from the local TCPSender to the remote TCPReceiver and has one ISN,
//! and the other stream runs from the remote TCPSender to the local TCPReceiver and
//! has a different ISN.
uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
    int times = checkpoint >> 32;
    uint64_t diff = n.raw_value() - isn.raw_value();
    if (times == 0) {
        return abs(static_cast<int64_t>(checkpoint - diff)) < 
            abs(static_cast<int64_t>(checkpoint - (diff + (1ul << 32)))) ? 
            diff : diff + (1ul<<32);
    }
    int u = times - 1;
    uint64_t mindiff = static_cast<uint64_t>
        (abs(static_cast<int64_t>(checkpoint - (diff + u * (1ul << 32)))));
    for (int j = times - 1;j <= times + 1;++j) {
        if (static_cast<uint64_t>(abs(static_cast<int64_t>(checkpoint - (diff + j * (1ul << 32))))) < mindiff) {
            u = j;
            mindiff = static_cast<uint64_t>
                (abs(static_cast<int64_t>(checkpoint - (diff + u * (1ul << 32)))));
        }
    }
    return diff + u * (1ul << 32);
}
