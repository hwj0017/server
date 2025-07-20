#pragma once

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
    std::vector<char> in_buffer_ = std::vector<char>(InitialBufferSize);
    std::vector<char> out_buffer_ = std::vector<char>(InitialBufferSize);
    size_t begin_pos_ = 0;

  public:
    DoubleBuffer(size_t size) : size(size) {}
    ~DoubleBuffer() = default;
    bool is_full() const { return in_buffer_.size() >= size; }
    bool is_empty() const { return out_buffer_.empty(); }
    void append(std::span<char> data) { in_buffer_.insert(in_buffer_.end(), data.begin(), data.end()); }
    void remove(size_t size)
    {
        begin_pos_ += size;
        if (begin_pos_ >= out_buffer_.size())
        {
            std::swap(out_buffer_, in_buffer_);
            begin_pos_ = 0;
        }
    }
    std::span<char> get_data()
    {
        return std::span<char>(out_buffer_.data() + begin_pos_, out_buffer_.size() - begin_pos_);
    }
};

} // namespace tcp