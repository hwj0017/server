#pragma once
#include "utils/channel.h"
#include "utils/task.h"
#include <any>
#include <coroutine>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string_view>

namespace tcp
{
class IoContext;
class Socket;
class Connection
{
  public:
    using RecvResult = std::span<char>;
    Connection(Socket&& socket, IoContext* io_context);
    Connection(const Connection&) = delete;
    ~Connection();
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
