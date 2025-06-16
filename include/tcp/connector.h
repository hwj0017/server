#pragma once
#include "tcp/inetaddress.h"
#include "utils/task.h"
#include <cstddef>
#include <memory>
#include <string_view>

namespace tcp
{
class IoContext;
class Socket;
class Connector
{
  public:
    Connector() = default;
    Connector(const InetAddress& server_address, IoContext* io_context);
    Connector(const Connector&) = delete;
    Connector(Connector&&) = default;
    auto operator=(Connector&&) -> Connector& = default;
    ~Connector();
    auto async_read() -> utils::Task<std::string>;
    auto async_send(std::string_view data) -> utils::Task<size_t>;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace tcp