#pragma once
#include "tcp/connection.h"
#include "utils/task.h"
#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>
#include <utility>

namespace tcp
{
class IoContextPool;
class Acceptor : public std::enable_shared_from_this<Acceptor>
{
  public:
    using AcceptResult = std::optional<Connection>;
    using TimeTask = std::function<void()>();
    Acceptor(std::string_view listen_ip, uint16_t port, IoContext* io_context, IoContextPool* io_context_pool);
    ~Acceptor();
    auto start() -> utils::Task<>;
    auto stop() -> utils::Task<>;
    auto async_accept() -> utils::Task<AcceptResult>;
    auto reset_accept() -> utils::Task<>;
    auto addTimer(TimeTask, double delay, double interval) -> uint64_t;
    void removeTimer(uint64_t timer_id);

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace tcp