#pragma once
#include <string>
#include <vector>
namespace tcp
{

class Buffer
{
  public:
    const static int initialLength = 1024;
    const static int pretendedLength = 8;
    constexpr static char CRLF[3] = "\r\n";
    Buffer() : data_(initialLength), beginIndex_(pretendedLength), endIndex_(pretendedLength)
    {
    }

    void append(const char* data, int length);

    void prepend(const char* data, int length);
    const char* findCRLF(const char* start) const;

    Buffer& operator+=(const char*);
    Buffer& operator+=(const std::string&);
    std::string toString() const
    {
        return std::string(begin(), size());
    }
    void clear()
    {
        beginIndex_ = pretendedLength;
        endIndex_ = pretendedLength;
    }
    int size() const
    {
        return endIndex_ - beginIndex_;
    }
    int space() const
    {
        return data_.size() - endIndex_;
    }
    const char* begin() const
    {
        return data_.data() + beginIndex_;
    }
    const char* end() const
    {
        return data_.data() + endIndex_;
    }

    // 根据传入的长度length，更新beginIndex_和endIndex_的值
    void retrieve(int length)
    {
        // 如果length小于等于size()，则将beginIndex_增加length
        if (length <= size())
            beginIndex_ += length;
        // 否则，将beginIndex_和endIndex_都设置为pretendedLength
        else
            endIndex_ = beginIndex_ = pretendedLength;
    }
    // 返回读写的字节数，为-1则表示出错
    int readSocket(int fd);
    int writeSocket(int fd, const std::string& data);
    int writeSocket(int fd, const void* data = nullptr, size_t len = 0);

  private:
    // 确保有足够的后写空间
    void ableAppend(int length);
    // 确保有足够的前写空间
    void ablePrepend(int length);

    std::vector<char> data_;
    int beginIndex_;
    int endIndex_;
};
} // namespace tcp
