#include "stream_reassembler.hh"
#include <assert.h>

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) 
    : _aux_storage(),
      _output(capacity),
      _capacity(capacity)
{}

void StreamReassembler::merge(StreamReassembler::Node node) {
    if (_aux_storage.size() == 0) {
        _aux_storage.insert(node);
        _unassembled_bytes += node.length;
        return;
    }

    auto pre = _aux_storage.lower_bound(node);
    if (pre == _aux_storage.end()) {
        // 有前，无后
        --pre;
        if (pre->end() < node.index) {
            _aux_storage.insert(node);
            update_unassembled_bytes(_aux_storage.end(), _aux_storage.end(), node);
            return;
        } else if (pre->end() >= node.end()) { return; }
        std::string tmp = *pre->spStr + 
            node.spStr->substr(pre->end() - node.index);
        node.spStr.reset(
                new std::string(std::move(tmp))
                );
        node.index = pre->index;
        node.length = node.spStr->size();

        _aux_storage.erase(pre);
        _aux_storage.insert(node);
        auto begin = pre;
        auto end   = ++pre;
        update_unassembled_bytes(begin, end, node);
        return;
    }
    if (pre == _aux_storage.begin()) {
        // 无前
        auto it = pre;
        while (it != _aux_storage.end()) {
            if (it->end() > node.end()) break;
            ++it;
        }
        if (it == _aux_storage.end()) {
            update_unassembled_bytes(_aux_storage.begin(), _aux_storage.end(), node);
            _aux_storage.clear();
            _aux_storage.insert(node);
            return;
        }
        // 无前，有后
        if (it->index > node.end()) {
            update_unassembled_bytes(pre, it, node);
            _aux_storage.erase(pre, it);
            _aux_storage.insert(node);
            return;
        } else {
            node.spStr.reset(
                    new std::string(*node.spStr + 
                        it->spStr->substr(
                            node.end() - it->index
                            )
                        )
                    );
            node.length = node.spStr->size();
            ++it;
            update_unassembled_bytes(pre, it, node);
            _aux_storage.erase(pre, it);
            _aux_storage.insert(node);
            return;
        }
    }
    // pre 在中间
    auto it = pre;
    while (it != _aux_storage.end()) {
        if (it->end() > node.end()) break;
        ++it;
    }
    if (it == _aux_storage.end()) {
        // 有前，无后
        auto t = --pre;
        if (t->end() < node.index) {
            ++t;
            update_unassembled_bytes(t, it, node);
            _aux_storage.erase(t, it);
            _aux_storage.insert(node);
            return;
        } else {
            node.spStr.reset(
                    new std::string(*t->spStr + 
                        node.spStr->substr(
                            t->end() - node.index
                            )
                        )
                    );
            node.index = t->index;
            update_unassembled_bytes(t, it, node);
            _aux_storage.erase(t, it);
            _aux_storage.insert(node);
            return;
        }
    } else {
        // 有前，有后
        auto a = --pre;
        auto b = it;
        if (a->end() < node.index && node.end() < b->index) {
            ++a;
            update_unassembled_bytes(a, b, node);
            _aux_storage.erase(a, b);
            _aux_storage.insert(node);
            return;
        } else if (a->end() < node.index) {
            node.spStr.reset(
                    new std::string(*node.spStr + 
                        b->spStr->substr(
                            node.end() - b->index
                            )
                        )
                    );
            node.length = node.spStr->size();

            ++a;
            ++b;
            update_unassembled_bytes(a, b, node);
            _aux_storage.erase(a, b);
            _aux_storage.insert(node);
            return;
        } else if (node.end() < b->index) {
            if (a->end() >= node.end()) return;
            node.spStr.reset(
                    new std::string(*a->spStr + 
                        node.spStr->substr(
                            a->end() - node.index
                            )
                        )
                    );
            node.index = a->index;
            node.length = node.spStr->size();

            update_unassembled_bytes(a, b, node);
            _aux_storage.erase(a, b);
            _aux_storage.insert(node);
            return;
        } else {
            node.spStr.reset(
                    new std::string(*a->spStr + 
                        node.spStr->substr(
                            a->end() - node.index
                            ) + 
                    b->spStr->substr(node.end() - b->index)
                        )
                    );
            node.index = a->index;
            node.length = node.spStr->size();

            ++b;
            update_unassembled_bytes(a, b, node);
            _aux_storage.erase(a, b);
            _aux_storage.insert(node);
            return;
        }
    }
}

void StreamReassembler::write_to_aux_storage(std::pair<size_t, std::string> p) {
    // std::string tmp = p.second.size() > remain_capacity() ? 
    //     p.second.substr(0, remain_capacity()) : p.second;
    std::string tmp = p.second.size() > _capacity ? 
        p.second.substr(0, _capacity) : p.second;
    Node node(p.first, tmp.size(), std::make_shared<std::string>(tmp));
    // _unassembled_bytes += tmp.size();
    merge(node);
}
uint32_t StreamReassembler::update() {
    auto it = _aux_storage.begin();
    // assert(it->index == _rcv_base);
    size_t writed = _output.write(*it->spStr);
    size_t ret = _rcv_base + writed;
    _unassembled_bytes -= writed;
    if (writed == it->length) {
        _aux_storage.erase(it);
    } else {
        Node node = {ret, it->length - static_cast<uint32_t>(writed), 
            std::make_shared<std::string>
                (it->spStr->substr(writed))
                };
        _aux_storage.erase(it);
        _aux_storage.insert(node);
    }
    if (ret == _eof_index) _output.end_input();
    return ret;
}

// data: 传到 push_substring 的字符串
// ret: index, valid_data
std::pair<size_t, std::string>
StreamReassembler::get_valid_data(const std::string& data, 
        const size_t index) {
    if (index >= _rcv_base) return {index, data};
    size_t end = index + data.size();
    if (end < _rcv_base) return {0, {}};
    return {_rcv_base, data.substr(_rcv_base - index)};
}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    if (eof) _eof_index = index + data.size();
    auto p = get_valid_data(data, index); // valid_index, valid_data
    // index + data.size() < _rcv_base
    if (p.second.empty()) {
        // 如果只是用来通知 eof 的
        if (_rcv_base == _eof_index)    _output.end_input();
        return;
    }

    write_to_aux_storage(p);
    // index + data.size() >= _rcv_base && index <= _rcv_base
    if (p.first == _rcv_base) {
        _rcv_base = update();
    } else {
        // index > _rcv_base
        
    }
}

size_t StreamReassembler::unassembled_bytes() const { return _unassembled_bytes; }

bool StreamReassembler::empty() const { return 0 == unassembled_bytes(); }
