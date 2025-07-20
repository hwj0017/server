#include "socket.h"
#include "inetaddress.h"
#include "iocontext.h"
#include "utils/log.h"
#include <cassert>
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
        if (errno != EAGAIN && errno != EWOULDBLOCK)
        {
            return {};
        }
        return Socket(-1, address);
    }
    utils::Logger::logger << "connection fd " + std::to_string(fd) + "\n";
    return {createConnecionSocket(fd, address)};
}

auto Socket::recv(std::vector<char>& data) -> RecvResult
{
    while (true)
    {
        auto current_size = data.size();
        data.resize(current_size + kRecvBufferSize);
        auto bytes_received = ::recv(fd_, data.data() + current_size, kRecvBufferSize, 0);
        if (bytes_received <= 0)
        {
            if (errno != EAGAIN && errno != EWOULDBLOCK)
            {
                data.resize(current_size);
                return {};
            }
            else
            {
                data.resize(current_size);
                return {current_size};
            }
        }
        if (bytes_received < kRecvBufferSize)
        {
            data.resize(current_size + bytes_received);
            return {current_size + bytes_received};
        }
    }
}

auto Socket::send(std::span<char> data) -> SendResult
{
    auto bytes_sent = ::send(fd_, data.data(), data.size(), 0);
    if (bytes_sent <= 0)
    {
        if (errno != EAGAIN && errno != EWOULDBLOCK)
        {
            return {};
        }
        else
        {
            return {0};
        }
    }
    return {bytes_sent};
}

auto Socket::createAcceptorSocket(const InetAddress& listen_address) -> Socket
{
    int fd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    assert(fd >= 0);
    assert(::bind(fd, reinterpret_cast<const sockaddr*>(&listen_address.addr_), listen_address.addrLen_) >= 0);
    assert(::listen(fd, SOMAXCONN) >= 0);
    return Socket(fd, listen_address);
}

auto Socket::createConnecionSocket(int fd, const InetAddress& client_address) -> Socket
{
    assert(fd >= 0);
    fcntl(fd, F_SETFL, O_NONBLOCK | O_CLOEXEC); // Set non-blocking and close-on-exec flags
    return Socket(fd, client_address);
}

auto Socket::createConnectorSocket(const InetAddress& server_address) -> Socket
{
    int fd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    assert(fd >= 0);
    assert(::connect(fd, reinterpret_cast<const sockaddr*>(&server_address.addr_), server_address.addrLen_) >= 0 ||
           errno == EINPROGRESS);
    return Socket(fd, server_address);
}
} // namespace tcp