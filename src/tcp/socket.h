#pragma once

#include "tcp/inetaddress.h"
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace tcp
{
class IoContext;
class Channel;
class Socket
{
  public:
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

    auto accept() -> std::pair<Socket, bool>;
    auto recv() -> std::pair<std::string, bool>;
    auto send(std::string_view data) -> std::pair<size_t, bool>;
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