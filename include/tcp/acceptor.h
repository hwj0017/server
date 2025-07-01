#pragma once
#include "tcp/awaitables.h"
#include "tcp/connection.h"
#include "utils/task.h"
#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>
#include <utility>

namespace tcp
{
class IoContext;
class IoContextPool;
class Acceptor : public std::enable_shared_from_this<Acceptor>
{
  public:
    using AcceptResult = std::shared_ptr<Connection>;
    Acceptor(std::string_view listen_ip, uint16_t port, IoContext* io_context, IoContextPool* io_context_pool);
    Acceptor(const Acceptor&) = delete;
    ~Acceptor();
    auto start() -> utils::Task<>;
    auto stop() -> utils::Task<>;
    auto async_accept() -> utils::Task<AcceptResult>;
    auto reset_accept() -> utils::Task<>;
    auto delay(double delay) -> Delay;
    auto cancel_delay(size_t id) -> utils::Task<>;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    static auto delay_destroy(std::unique_ptr<Impl> impl) -> utils::Task<>;
};

} // namespace tcp