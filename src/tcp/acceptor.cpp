
#include "tcp/acceptor.h"
// #include "connection.h"
#include "inetaddress.h"
#include "iocontext.h"
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
    void accept();
    auto reset_accept() -> utils::Task<>;
    void handle_error();
    void update_tasks();
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
    auto io_channels = io_context->get_channels(socket.fd());
    while (true)
    {
        co_await std::get<0>(io_channels).pop();
        auto res = socket.accept();
        if (res.has_value())
        {
            accept_channel.push(std::move(res.value()));
        }
        else
        {
            accept_channel.close();
            break;
        }
    }
}

auto Acceptor::Impl::stop() -> utils::Task<>
{
    co_await io_context->thread_channel().pop();
    if (state == State::Started)
    {
        state = State::Stopped;
        io_context->remove_channel(socket.fd());
    }
}
// 接受连接
void Acceptor::Impl::accept()
{
    // 调用socket的accept函数，返回一个pair，第一个元素是连接的socket，第二个元素是bool值，表示是否成功
    auto res = socket.accept();
    if (res.has_value())
    {
        accept_buffer.push(std::move(res.value()));
        update_tasks();
    }
    else
    {
        handle_error();
    }
}

// 定义一个异步接受函数，返回一个AcceptResult类型的Task
auto Acceptor::Impl::async_accept() -> utils::Task<AcceptResult>
{
    // 如果接受缓冲区为空
    if (!accept_buffer.empty())
    {
        auto socket = std::move(accept_buffer.front());
        accept_buffer.pop();
        co_return {true, std::move(socket)};
    }
    utils::Awaitable<AcceptResult> waiter;
    accept_waiters.push(&waiter);
    co_return co_await waiter;
}

auto Acceptor::Impl::reset_accept() -> utils::Task<>
{
    co_await io_context->inThread();
    accept_buffer = {};
}

void Acceptor::Impl::handle_error()
{
    while (!accept_waiters.empty())
    {
        auto waiter = accept_waiters.front();
        accept_waiters.pop();
        waiter->result = {false, Socket()};
        waiter->task.resume();
    }
}
void Acceptor::Impl::update_tasks()
{
    if (!accept_waiters.empty())
    {
        auto waiter = accept_waiters.front();
        accept_waiters.pop();
        waiter->result = {true, std::move(accept_buffer.front())};
        waiter->task.resume();
    }
}
} // namespace tcp
