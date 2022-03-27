#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::do_is_fin(bool flag) {
    if (flag) {
        _status = Status::FIN_RECEIVED;
    }
}

void TCPReceiver::segment_received(const TCPSegment &seg) {
    if (_status == Status::ERROR) return;
    if (_status == Status::WAIT_SYN) {
        if (seg.header().syn) {
            _status = Status::SYN_RECEIVED;
            _peer_isn = seg.header().seqno;

            // 如果 syn 带数据
            _reassembler.push_substring(seg.payload().copy(), 
                    0, seg.header().fin);
            // 如果 syn 带 fin
            do_is_fin(seg.header().fin);
        } else return;
    } else {
        // 已经收到 SYN 了
        do_is_fin(seg.header().fin);
        size_t asn = unwrap(seg.header().seqno, _peer_isn, getCheckpoint());
        // 如果收到的是 SYN TCP segment 但是我现在已经处于 SYN_RECEIVED 状态了！
        if (asn == 0) return;
        size_t byte_stream_index = asn - 1;
        _reassembler.push_substring(seg.payload().copy(), 
                byte_stream_index, 
                seg.header().fin);
    }
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if (_status == Status::SYN_RECEIVED || _status == Status::FIN_RECEIVED)
        return wrap(fromByteStreamIndex(), getPeerISN());
    return std::nullopt;
}

size_t TCPReceiver::window_size() const { return _capacity - stream_out().buffer_size(); }
