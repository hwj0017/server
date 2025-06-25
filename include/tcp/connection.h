#pragma once
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
    using RecvResult = std::optional<std::string>;
    using SendResult = std::optional<size_t>;
    using TimeTask = std::function<void()>();
    Connection(Socket&& socket, IoContext* io_context);
    Connection(const Connection&) = delete;
    ~Connection();
    auto start() -> utils::Task<>;
    auto stop() -> utils::Task<>;

    auto async_recv() -> utils::Task<RecvResult>;
    auto async_send(std::string_view data) -> utils::Task<SendResult>;
    auto reset_recv() -> utils::Task<>;
    auto reset_send() -> utils::Task<>;
    auto addTimer(TimeTask, double delay, double interval) -> uint64_t;
    void removeTimer(uint64_t timer_id);

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace tcp