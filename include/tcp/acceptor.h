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
    using AcceptResult = std::optional<std::shared_ptr<Connection>>;
    Acceptor(std::string_view listen_ip, uint16_t port, IoContextPool* io_context_pool);
    Acceptor(const Acceptor&) = delete;
    ~Acceptor();
    auto start() -> utils::Task<>;
    auto stop() -> utils::Task<>;
    auto async_accept() -> utils::Task<AcceptResult>;
    auto reset_accept() -> utils::Task<>;
    void removeTimer(uint64_t timer_id);

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    static auto delay_destroy(std::unique_ptr<Impl> impl) -> utils::Task<>;
};

} // namespace tcp