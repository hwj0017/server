#pragma once
#include "utils/task.h"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string_view>

namespace tcp
{
class IoContext;
class Socket;
class Connector
{
  public:
    using RecvResult = std::optional<std::string>;
    using SendResult = std::optional<size_t>;
    using TimeTask = std::function<void()>();

    Connector() = default;
    Connector(std::string_view server_ip, uint16_t port, IoContext* io_context);
    Connector(const Connector&) = delete;
    Connector(Connector&&) = default;
    auto operator=(Connector&&) -> Connector& = default;
    ~Connector();
    auto async_read() -> utils::Task<std::string>;
    auto async_send(std::string_view data) -> utils::Task<size_t>;
    auto reset_recv() -> utils::Task<>;
    auto reset_send() -> utils::Task<>;
    auto addTimer(TimeTask, double delay, double interval) -> uint64_t;
    void removeTimer(uint64_t timer_id);

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace tcp