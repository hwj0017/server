#include "buffer.h"
#include "socket.h"
#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <vector>
namespace tcp
{
void Buffer::append(const char* data, int length)
{
    ableAppend(length);
    std::copy(data, data + length, data_.data() + endIndex_);
    endIndex_ += length;
}

void Buffer::prepend(const char* data, int length)
{
    ablePrepend(length);
    std::copy(data, data + length, data_.data() + beginIndex_ - length);
    beginIndex_ -= length;
}

const char* Buffer::findCRLF(const char* start) const
{
    const char* str = data_.data();
    return std::search(start, str + endIndex_, CRLF, CRLF + 2);
}

Buffer& Buffer::operator+=(const char* data)
{
    append(data, strlen(data));
    return *this;
}

Buffer& Buffer::operator+=(const std::string& data)
{
    append(data.data(), data.size());
    return *this;
}

void Buffer::ableAppend(int length)
{

    if (endIndex_ + length <= data_.size())
        return;
    int space = data_.size() - size();
    char* str = data_.data();

    if (space >= length + pretendedLength)
    {
        std::copy(str + beginIndex_, str + endIndex_, str + pretendedLength);
        endIndex_ = pretendedLength + size();
        beginIndex_ = pretendedLength;
    }
    else
    {
        data_.resize(endIndex_ + length);
    }
}

void Buffer::ablePrepend(int length)
{
    if (beginIndex_ >= length)
        return;
    int space = data_.size() - size();
    char* str = data_.data();
    if (space >= length + pretendedLength)
    {
        std::copy_backward(str + beginIndex_, str + endIndex_, str + length + pretendedLength + size());
        endIndex_ = pretendedLength + length + size();
        beginIndex_ = pretendedLength + length;
    }
    else
    {
        // 新建一个刚好大小的buffer
        std::vector<char> newData(length + size() + pretendedLength);
        std::copy(str + beginIndex_, str + endIndex_, newData.data() + length + pretendedLength);
        data_.swap(newData);
        endIndex_ = length + size() + pretendedLength;
        beginIndex_ = length + pretendedLength;
    }
}

int Buffer::readSocket(int socket)
{
    int readNumTotal = 0;
    while (true)
    {
        ableAppend(initialLength);
        auto ableAppendLength = data_.size() - endIndex_;
        int readNum = socket::readNoBlocking(socket, data_.data() + endIndex_, ableAppendLength);
        if (readNum > 0)
        {
            endIndex_ += readNum;
            readNumTotal += readNum;
        }
        else if (readNum < 0)
        {
            std::cout << "readSocket error" << std::endl;
            return -1;
        }
        else
            break;
    }
    return readNumTotal;
}

int Buffer::writeSocket(int fd, const void* data, size_t len)
{
    size_t writeNumTotal = 0;
    while (true)
    {
        int writeNum = socket::writeNoBlocking(fd, static_cast<const char*>(data) + writeNumTotal, len - writeNumTotal);
        if (writeNum > 0)
        {
            writeNumTotal += writeNum;
        }
        else if (writeNum < 0)
            return -1;
        else
            break;
    }
    size_t left = len - writeNumTotal;
    if (left > 0)
    {
        append(static_cast<const char*>(data) + writeNumTotal, left);
    }
    return writeNumTotal;
}

} // namespace tcp
