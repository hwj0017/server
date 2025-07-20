#pragma once

#include "utils/channel.h"
#include <cassert>
#include <cstddef>
#include <span>
#include <utility>
#include <vector>
namespace tcp
{
class DoubleBuffer
{

  private:
    static constexpr size_t InitialBufferSize = 1024;
    size_t size;
    std::vector<char> buffer0_;
    std::vector<char> buffer1_;
    ;
    size_t begin_pos_ = 0;
    utils::Channel<> channel_{2};

  public:
    DoubleBuffer(size_t size) : size(size) {}
    ~DoubleBuffer() = default;
    bool is_full() const { return buffer0_.size() + buffer1_.size() >= size; }
    bool is_empty() const { return buffer0_.empty() && buffer1_.empty(); }
    void append(std::span<char> data)
    {
        if (data.size() == 0)
        {
            return;
        }
        if (buffer0_.size() < size / 2)
        {
            buffer0_.insert(buffer0_.end(), data.begin(), data.end());
        }
        else
        {
            buffer1_.insert(buffer0_.end(), data.begin(), data.end());
        }
        if (channel_.is_empty())
        {
            channel_.push();
        }
        if (is_full())
        {
            channel_.push();
        }
    }

    void remove(size_t size)
    {
        if (size == 0)
        {
            return;
        }
        begin_pos_ += size;
        assert(begin_pos_ <= buffer0_.size());
        if (begin_pos_ >= buffer0_.size())
        {
            buffer0_.clear();
            std::swap(buffer0_, buffer1_);
            begin_pos_ = begin_pos_ - buffer0_.size();
        }
        if (channel_.is_full())
        {
            channel_.pop();
        }
        if (is_empty())
        {
            channel_.pop();
        }
    }
    std::span<char> get_data() { return std::span<char>(buffer0_.data() + begin_pos_, buffer0_.size() - begin_pos_); }
    auto not_empty() -> utils::Channel<>::NotEmpty { return channel_.not_empty(); }
    auto not_full() -> utils::Channel<>::NotFull { return channel_.not_full(); }
    void close() { channel_.close(); }
};

} // namespace tcp