
#include "tcp/acceptor.h"
// #include "connection.h"
#include "inetaddress.h"
#include "iocontext.h"
#include "iocontextpool.h"
#include "socket.h"
#include "tcp/connection.h"
#include "utils/channel.h"
#include "utils/task.h"
#include "waker.h"
#include <coroutine>
#include <cstddef>
#include <memory>
#include <queue>
namespace tcp
{

struct Acceptor::Impl
{
    enum class State
    {
        Started,
        Stopped
    };
    static constexpr size_t kMaxPendingConnections = 1024;
    Socket socket;
    IoContext* io_context;
    IoContextPool* io_context_pool;
    utils::Channel<Socket> accept_channel{kMaxPendingConnections};
    State state = State::Stopped;
    Impl(std::string_view listen_ip, uint16_t port, IoContext* io_context, IoContextPool* io_context_pool)
        : socket(Socket::createAcceptorSocket(InetAddress(listen_ip, port))), io_context(io_context),
          io_context_pool(io_context_pool)
    {
    }
    ~Impl() { stop(); };
    auto start(std::shared_ptr<Acceptor> self) -> utils::Task<>;
    auto stop() -> utils::Task<>;
    auto async_accept() -> utils::Task<AcceptResult>;
    auto reset_accept() -> utils::Task<>;
    auto accept(std::shared_ptr<Acceptor> self) -> utils::Task<>;
};
Acceptor::Acceptor(std::string_view listen_ip, uint16_t port, IoContext* io_context, IoContextPool* io_context_pool)
    : impl_(std::make_unique<Impl>(listen_ip, port, io_context, io_context_pool))
{
}
Acceptor ::~Acceptor() = default;
auto Acceptor::start() -> utils::Task<> { co_await impl_->start(shared_from_this()); }

auto Acceptor::stop() -> utils::Task<> { co_await impl_->stop(); }

auto Acceptor::async_accept() -> utils::Task<AcceptResult> { co_return co_await impl_->async_accept(); }

auto Acceptor::reset_accept() -> utils::Task<> { co_await impl_->reset_accept(); }

auto Acceptor::Impl::start(std::shared_ptr<Acceptor> self) -> utils::Task<>
{
    co_await io_context->thread_channel().pop();
    auto input_task = [self = std::move(self), this]() -> utils::Task<> {
        while (state == State::Started)
        {
            // 监听
            co_await std::suspend_always{};
            auto result = socket.accept();
            if (result.has_value())
            {
                co_await accept_channel.push(std::move(result.value()));
                io_context->continue_read(socket.fd());
            }
            else
            {
                // 必在当前线程,会将当前协程销毁
                co_await stop();
            }
        }
    }();
    io_context->add_io_task(socket.fd(), std::move(input_task), {});
}

auto Acceptor::Impl::stop() -> utils::Task<>
{
    if (!io_context->in_attached_thread())
    {
        co_await io_context->thread_channel().pop();
    }
    if (state == State::Started)
    {
        state = State::Stopped;
    }
}

// 定义一个异步接受函数，返回一个AcceptResult类型的Task
auto Acceptor::Impl::async_accept() -> utils::Task<AcceptResult>
{
    auto client_socket = co_await accept_channel.pop();
    if (client_socket.has_value())
    {
        co_return {Connection(std::move(client_socket.value()), io_context_pool->getIoContext())};
    }
    co_return {};
}

auto Acceptor::Impl::reset_accept() -> utils::Task<>
{
    if (!io_context->in_attached_thread())
    {
        co_await io_context->thread_channel().pop();
    }
    accept_channel.clear();
}

} // namespace tcp
