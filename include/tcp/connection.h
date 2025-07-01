#pragma once
#include "tcp/awaitables.h"
#include "utils/task.h"
#include <any>
#include <coroutine>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <string_view>

namespace tcp
{
class IoContext;
class Socket;
class Connection : public std::enable_shared_from_this<Connection>
{
  public:
    using RecvResult = std::string;
    using SendResult = void;
    Connection(Socket&& socket, IoContext* io_context);
    Connection(const Connection&) = delete;
    ~Connection();
    auto start() -> utils::Task<>;
    auto stop() -> utils::Task<>;

    auto async_recv() -> utils::Task<RecvResult>;
    auto async_send(std::string_view data) -> utils::Task<SendResult>;
    auto reset_recv() -> utils::Task<>;
    auto reset_send() -> utils::Task<>;
    auto delay(double delay) -> Delay;
    auto cancel_delay(size_t id) -> utils::Task<>;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    static auto delay_destroy(std::unique_ptr<Impl> impl) -> utils::Task<>;
};

} // namespace tcp