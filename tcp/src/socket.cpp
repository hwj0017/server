#include "socket.h"
#include "inetaddress.h"
#include "iocontext.h"
#include <cstddef>
#include <fcntl.h>
#include <string>
#include <string_view>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

namespace tcp
{

Socket::Socket() : fd_(-1), address_(InetAddress()) {}
Socket::Socket(int fd, const InetAddress& address) : fd_(fd), address_(address) {}
Socket::Socket(Socket&& other) : fd_(other.fd_), address_(std::move(other.address_))
{
    other.fd_ = -1; // Invalidate the moved-from socket
}
auto Socket::operator=(Socket&& other) -> Socket&
{
    if (this != &other)
    {
        if (fd_ != -1)
        {
            ::close(fd_);
        }
        fd_ = other.fd_;
        address_ = std::move(other.address_);
        other.fd_ = -1; // Invalidate the moved-from socket
    }
    return *this;
}
Socket::~Socket()
{
    if (fd_ != -1)
    {
        ::close(fd_);
    }
}
auto Socket::accept() -> AcceptResult
{
    InetAddress address;
    auto fd = ::accept(fd_, reinterpret_cast<sockaddr*>(&address.addr_), &address.addrLen_);
    if (fd < 0)
    {
        return {};
    }
    return {createConnecionSocket(fd, address)};
}

auto Socket::recv() -> RecvResult
{
    size_t total_bytes_received = 0;
    std::string data;
    while (true)
    {
        auto current_size = data.size();
        data.resize(current_size + kRecvBufferSize);
        auto bytes_received = ::recv(fd_, data.data() + current_size, kRecvBufferSize, 0);
        if (bytes_received <= 0 && errno != EAGAIN && errno != EWOULDBLOCK)
        {
            data.resize(current_size);
            return {};
        }
        if (bytes_received < kRecvBufferSize)
        {
            data.resize(current_size + bytes_received);
            return {std::move(data)};
        }
    }
}

auto Socket::send(std::string_view data) -> SendResult
{
    size_t total_bytes_sent = 0;
    auto bytes_sent = ::send(fd_, data.data(), data.size(), 0);
    if (bytes_sent <= 0 && errno != EAGAIN && errno != EWOULDBLOCK)
    {
        return {};
    }
    total_bytes_sent += bytes_sent;
    return {total_bytes_sent};
}

auto Socket::createAcceptorSocket(const InetAddress& listen_address) -> Socket
{
    int fd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (fd < 0)
    {
        return Socket(-1, InetAddress());
    }
    if (::bind(fd, reinterpret_cast<const sockaddr*>(&listen_address.addr_), listen_address.addrLen_) < 0)
    {
        ::close(fd);
        return Socket(-1, InetAddress());
    }
    if (::listen(fd, SOMAXCONN) < 0)
    {
        ::close(fd);
        return Socket(-1, InetAddress());
    }
    return Socket(fd, listen_address);
}

auto Socket::createConnecionSocket(int fd, const InetAddress& client_address) -> Socket
{
    fcntl(fd, F_SETFL, O_NONBLOCK | O_CLOEXEC); // Set non-blocking and close-on-exec flags
    if (fd < 0)
    {
        return Socket(-1, InetAddress());
    }
    return Socket(fd, client_address);
}

auto Socket::createConnectorSocket(const InetAddress& server_address) -> Socket
{
    int fd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (fd < 0)
    {
        return Socket(-1, InetAddress());
    }
    if (::connect(fd, reinterpret_cast<const sockaddr*>(&server_address.addr_), server_address.addrLen_) < 0)
    {
        if (errno != EINPROGRESS)
        {
            ::close(fd);
            return Socket(fd, InetAddress());
        }
    }
    return Socket(fd, server_address);
}
} // namespace tcp