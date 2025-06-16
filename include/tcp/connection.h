#pragma once
#include "tcp/inetaddress.h"
#include "utils/task.h"
#include <any>
#include <coroutine>
#include <cstddef>
#include <functional>
#include <memory>
#include <string_view>

namespace tcp
{
class IoContext;
class Socket;
class Connection : public std::enable_shared_from_this<Connection>
{
  public:
    using RecvResult = std::pair<bool, std::string>;
    using SendResult = std::pair<bool, size_t>;
    Connection() = default;
    Connection(Socket&& socket, IoContext* io_context);
    Connection(const Connection&) = delete;
    Connection(Connection&&) noexcept = default;
    auto operator=(Connection&&) noexcept -> Connection& = default;
    ~Connection();
    auto start() -> utils::Task<>;
    auto stop() -> utils::Task<>;

    auto async_recv() -> utils::Task<RecvResult>;
    auto async_send(std::string_view data) -> utils::Task<SendResult>;
    auto reset_recv() -> utils::Task<>;
    auto reset_send() -> utils::Task<>;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace tcp