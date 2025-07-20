#pragma once
#include "tcp/connection.h"
#include "utils/channel.h"
#include "utils/task.h"
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string_view>
#include <utility>

namespace tcp
{
class IoContext;
class IoContextPool;
class Acceptor
{
  public:
    using AcceptResult = std::span<std::shared_ptr<Connection>>;
    Acceptor(std::string_view listen_ip, uint16_t port, IoContext* io_context, IoContextPool* io_context_pool);
    Acceptor(const Acceptor&) = delete;
    ~Acceptor();
    auto start() -> utils::Task<>;
    auto stop() -> utils::Task<>;
    auto async_accept() -> utils::Task<AcceptResult>;
    auto async_accept_local() -> utils::Channel<AcceptResult>::AsyncPop;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    static auto delay_destroy(std::unique_ptr<Impl> impl) -> utils::Task<>;
};

} // namespace tcp
