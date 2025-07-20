#pragma once
#include "utils/task.h"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string_view>

namespace tcp
{
class IoContext;
class Connector
{
  public:
    using RecvResult = std::span<char>;

    Connector(std::string_view server_ip, uint16_t port, IoContext* io_context);
    Connector(const Connector&) = delete;
    ~Connector();
    auto start() -> utils::Task<>;
    auto stop() -> utils::Task<>;

    auto async_recv() -> utils::Task<RecvResult>;
    auto async_recv_local() -> utils::Channel<RecvResult>::AsyncPop;

    auto async_send(std::span<char> data) -> utils::Task<>;
    auto async_send_local(std::span<char> data) -> utils::Channel<>::NotFull;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    static auto delay_destroy(std::unique_ptr<Impl> impl) -> utils::Task<>;
};

} // namespace tcp
