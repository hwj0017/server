#pragma once
#include "tcp/inetaddress.h"
#include "utils/task.h"
#include <memory>
#include <utility>

namespace tcp
{
class IoContext;
class Socket;
class Acceptor : public std::enable_shared_from_this<Acceptor>
{
  public:
    using AcceptResult = std::pair<bool, Socket>;
    Acceptor(const InetAddress& listen_address, IoContext* io_context);
    ~Acceptor();
    auto start() -> utils::Task<>;
    auto stop() -> utils::Task<>;
    auto async_accept() -> utils::Task<AcceptResult>;
    auto reset_accept() -> utils::Task<>;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace tcp