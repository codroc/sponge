#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

#include <cassert>
// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;


//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity) 
    , _timer()
    , _outstandings()
    , _rto(retx_timeout)
{}

uint64_t TCPSender::bytes_in_flight() const {
    return _bytes_in_flight;
}

void TCPSender::fill_window() {
    if (!space_available()) return;
    // size_t last_size = _outstandings.size();
    if (_next_seqno == 0) { // 前两次握手
        size_t can_read = std::min(TCPConfig::MAX_PAYLOAD_SIZE, 
                static_cast<const size_t&>(_win_size - 1)); // 预留一个字节给 syn
        send_and_store(_stream.read(can_read), true, false);
    } else if (_stream.input_ended() && !_fin_sended) { // 后面四次挥手
        // 判断何时发送 fin
        if (_win_size >= _stream.buffer_size() + 1) {
            // 刚好能把 fin 塞入
            send_and_store(_stream.read(_win_size), false, true);
            _fin_sended = true;
        }
    }
    // 仅传输 payload
    while (!_stream.buffer_empty() && space_available()) {
        size_t can_read = std::min(TCPConfig::MAX_PAYLOAD_SIZE, 
                static_cast<const size_t&>(_win_size));
        send_and_store(_stream.read(can_read), 0, 0);
    }
    // 因为 ack_received 后会调用一次 fill_window 因此判断下是否已经 FIN_ACKED，如果 FIN_ACKED 了，那么 _outstandings 肯定为空
    if (!_timer.started() && !_outstandings.empty())
        _timer.start(_ms_since_alive + _rto);
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    uint64_t left = get_absolute_seqno(ackno, _isn, _checkpoint);
    if (left > _next_seqno) return;
    uint64_t right = left + window_size;
    _win_size = right - _next_seqno;

    if (window_size == 0) { _peer_busy = true; _win_size = 1; }
    else _peer_busy = false;

    bool has_new_data = false;
    while (!_outstandings.empty()) {
        TCPSegment segment = _outstandings.front();
        uint64_t ab_seqno = get_absolute_seqno(segment.header().seqno, _isn, _checkpoint);
        uint16_t length = segment.length_in_sequence_space();
        if (ab_seqno + length > left)
            break;

        has_new_data = true;
        _outstandings.pop();
        _bytes_in_flight -= length;

        _rto = _initial_retransmission_timeout;
        _rtx = 0;
    };
    if (_outstandings.empty()) _timer.stop();
    else if(has_new_data) _timer.start(_ms_since_alive + _rto);
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    if (_outstandings.empty()) return;
    _ms_since_alive += ms_since_last_tick;
    if (_timer.is_time_out(_ms_since_alive)) {
        if (is_peer_busy()) {
            // 如果是对端繁忙的情况，即 发送端还有数据发，但 receiver 窗口变成 0
        } else {
            // 网络环境差导致丢包或延迟送达
            _rtx++;
            _rto *= 2;
        }
        // 重传最早的那个 segment，并重启定时器
        // assert(!_outstandings.empty()); // 一定非空
        _segments_out.push(_outstandings.front());
        // _outstandings.pop();
        _timer.start(_ms_since_alive + _rto);
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return _rtx; }

void TCPSender::send_empty_segment() {
    _segments_out.push(make_segment("", false, false));
}

TCPSegment TCPSender::make_segment(std::string&& payload, bool syn, bool fin) {
    TCPSegment segment;
    segment.payload() = std::move(payload);
    segment.header().seqno = get_seqno();
    segment.header().syn = syn;
    segment.header().fin = fin;

    // _next_seqno += segment.length_in_sequence_space();
    return segment;
}

WrappingInt32 TCPSender::get_seqno() {
    return wrap(_next_seqno, _isn);
}

uint64_t TCPSender::get_absolute_seqno(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) const {
    return unwrap(n, isn, checkpoint);
}

void TCPSender::send_and_store(std::string&& payload, bool syn, bool fin) {
    TCPSegment segment = make_segment(std::move(payload), syn, fin);

    // send
    uint64_t length = segment.length_in_sequence_space();
    _segments_out.push(segment);
    _next_seqno += length;
    _win_size -= length;

    // store
    _outstandings.push(segment);
    _bytes_in_flight += length;

    // update checkpoint
    _checkpoint = _next_seqno;
}
