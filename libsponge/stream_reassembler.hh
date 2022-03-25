#ifndef SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
#define SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH

#include "byte_stream.hh"

#include <cstdint>
#include <string>
#include <set>
#include <memory>

//! \brief A class that assembles a series of excerpts from a byte stream (possibly out of order,
//! possibly overlapping) into an in-order byte stream.
class StreamReassembler {
  public:
    struct Node {
        Node(size_t idx, uint32_t l, std::shared_ptr<std::string> sp)
            : index(idx),
              length(l),
              spStr(std::move(sp))
        {}
        size_t index;
        uint32_t length;
        std::shared_ptr<std::string> spStr;
        size_t end() const { return index + length; }
    };
    struct NodeCmp {
        bool operator()(const Node& lhs, const Node& rhs) const { return lhs.index < rhs.index; }
    };
  private:
    // Your code here -- add private members as necessary.
    using SetType = std::set<Node, NodeCmp>;
    uint32_t _rcv_base{0}; // 下一个起始索引
    uint32_t _eof_index{0xffffffff};
    uint32_t _unassembled_bytes{0};

    SetType _aux_storage;

    ByteStream _output;  //!< The reassembled in-order byte stream
    size_t _capacity;    //!< The maximum number of bytes
  private:
    // 判断 data 是否有部分属于 assembled 但 unread
    // 把这一部分数据叫做有效数据 valid data
    std::pair<size_t, std::string>
    get_valid_data(const std::string& data, const size_t index);

    // remain capacity
    uint32_t remain_capacity() const { return _capacity - _unassembled_bytes - _output.buffer_size(); }
    
    // 把数据交付给 ByteStream，并更新 aux_storage
    uint32_t update();

    // 把数据写到 aux_storage
    // 会出现几种情况
    // 1. 数据是从 _rcv_base 开始的
    // 2. 数据不是从 _rcv_base 开始的
    void write_to_aux_storage(std::pair<size_t, std::string> p);

    // 合并碎片，可能会有多个碎片和输入碎片重叠
    void merge(Node node);

    void update_unassembled_bytes(SetType::iterator a, SetType::iterator b, const Node& node) {
        uint32_t total = 0;
        for (auto it = a; it != b; ++it)
            total += it->length;
        _unassembled_bytes += node.length - total;
    }
  public:
    //! \brief Construct a `StreamReassembler` that will store up to `capacity` bytes.
    //! \note This capacity limits both the bytes that have been reassembled,
    //! and those that have not yet been reassembled.
    StreamReassembler(const size_t capacity);

    //! \brief Receive a substring and write any newly contiguous bytes into the stream.
    //!
    //! The StreamReassembler will stay within the memory limits of the `capacity`.
    //! Bytes that would exceed the capacity are silently discarded.
    //!
    //! \param data the substring
    //! \param index indicates the index (place in sequence) of the first byte in `data`
    //! \param eof the last byte of `data` will be the last byte in the entire stream
    void push_substring(const std::string &data, const uint64_t index, const bool eof);

    //! \name Access the reassembled byte stream
    //!@{
    const ByteStream &stream_out() const { return _output; }
    ByteStream &stream_out() { return _output; }
    //!@}

    //! The number of bytes in the substrings stored but not yet reassembled
    //!
    //! \note If the byte at a particular index has been pushed more than once, it
    //! should only be counted once for the purpose of this function.
    size_t unassembled_bytes() const;

    //! \brief Is the internal state empty (other than the output stream)?
    //! \returns `true` if no substrings are waiting to be assembled
    bool empty() const;
};

#endif  // SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
