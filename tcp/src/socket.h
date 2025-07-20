#pragma once

#include "inetaddress.h"
#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace tcp
{

class IoContext;
class Socket
{
  public:
    using AcceptResult = std::optional<Socket>;
    using RecvResult = std::optional<size_t>;
    using SendResult = std::optional<size_t>;
    enum class Type
    {
        Acceptor,
        Connection,
        Connector,
    };
    Socket();
    Socket(const Socket&) = delete;
    Socket(Socket&&);
    auto operator=(const Socket&) -> Socket& = delete;
    auto operator=(Socket&&) -> Socket&;
    ~Socket();
    auto fd() const -> int { return fd_; }

    auto accept() -> AcceptResult;
    auto recv(std::vector<char>& data) -> RecvResult;
    auto send(std::span<char> data) -> SendResult;
    static auto createAcceptorSocket(const InetAddress& listen_address) -> Socket;
    static auto createConnecionSocket(int fd, const InetAddress& client_address) -> Socket;
    static auto createConnectorSocket(const InetAddress& server_address) -> Socket;

  private:
    Socket(int fd, const InetAddress& address);
    static constexpr size_t kRecvBufferSize = 4096;
    int fd_;
    InetAddress address_;
};
} // namespace tcp