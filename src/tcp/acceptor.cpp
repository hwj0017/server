
#include "tcp/acceptor.h"
// #include "connection.h"
#include "channel.h"
#include "iocontext.h"
#include "socket.h"
#include "tcp/connection.h"
#include "utils/task.h"
#include "waker.h"
#include <coroutine>
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
    Socket socket;
    IoContext* io_context;
    std::queue<Socket> accept_buffer;
    std::queue<utils::Awaitable<AcceptResult>*> accept_waiters;
    State state = State::Stopped;
    Channel* channel = nullptr;
    Impl(const InetAddress& listen_address, IoContext* io_context)
        : socket(Socket::createAcceptorSocket(listen_address)), io_context(io_context)
    {
    }
    ~Impl() { stop(); };
    auto start(std::shared_ptr<Acceptor> self) -> utils::Task<>;
    auto stop() -> utils::Task<>;
    void accept();
    auto reset_accept() -> utils::Task<>;
    void handle_error();
    void update_tasks();
    auto async_accept() -> utils::Task<AcceptResult>;
};
Acceptor::Acceptor(const InetAddress& listen_address, IoContext* io_context)
    : impl_(std::make_unique<Impl>(listen_address, io_context))
{
}
Acceptor ::~Acceptor() = default;
auto Acceptor::start() -> utils::Task<> { co_await impl_->start(shared_from_this()); }

auto Acceptor::stop() -> utils::Task<> { co_await impl_->stop(); }

auto Acceptor::async_accept() -> utils::Task<AcceptResult> { co_return co_await impl_->async_accept(); }

auto Acceptor::reset_accept() -> utils::Task<> { co_await impl_->reset_accept(); }

auto Acceptor::Impl::start(std::shared_ptr<Acceptor> self) -> utils::Task<>
{
    co_await io_context->inThread();
    if (state == State::Stopped)
    {
        state = State::Started;
        auto new_channel = std::make_unique<Channel>(socket.fd());
        new_channel->type = Channel::Type::Read;
        new_channel->read_callBack = [self = std::move(self)]() { self->impl_->accept(); };
        channel = new_channel.get();
        io_context->add(std::move(new_channel));
    }
}

auto Acceptor::Impl::stop() -> utils::Task<>
{
    co_await io_context->inThread();
    if (state == State::Started)
    {
        state = State::Stopped;
        io_context->remove(socket.fd());
    }
}
void Acceptor::Impl::accept()
{
    auto res = socket.accept();
    if (res.second)
    {
        accept_buffer.push(std::move(res.first));
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
