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
class Connector : public std::enable_shared_from_this<Connector>
{
  public:
    using RecvResult = std::optional<std::string>;
    using SendResult = std::optional<size_t>;
    using TimeTask = std::function<void()>();

    Connector(std::string_view server_ip, uint16_t port, IoContext* io_context);
    Connector(const Connector&) = delete;
    ~Connector();
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
    static auto delay_destroy(std::unique_ptr<Impl> impl) -> utils::Task<>;
};

} // namespace tcp