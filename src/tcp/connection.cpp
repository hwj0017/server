#include "tcp/connection.h"
#include "iocontext.h"
#include "socket.h"
#include "utils/task.h"
#include <coroutine>
#include <cstddef>
#include <memory>
#include <queue>
#include <string>
#include <unistd.h>
#include <vector>
namespace tcp
{
struct Connection::Impl
{
    enum class State
    {
        Started,
        Stopped
    };
    Socket socket;
    IoContext* io_context;
    std::queue<std::tuple<std::string, size_t>> recv_buffer;
    std::queue<std::tuple<std::string, size_t>> send_buffer;
    // save pointer
    std::queue<utils::Awaitable<RecvResult>*> recv_waiters;
    std::queue<utils::Awaitable<SendResult>*> send_waiters;
    State state = State::Stopped;
    Channel* channel = nullptr; // save pointer
    Impl(Socket&& socket, IoContext* io_context) : socket(std::move(socket)), io_context(io_context) {}
    ~Impl() = default;
    auto start(std::shared_ptr<Connection> self) -> utils::Task<>;
    auto stop() -> utils::Task<>;
    auto async_recv() -> utils::Task<RecvResult>;
    auto async_send(std::string_view data) -> utils::Task<SendResult>;
    // 重置读缓存区
    auto reset_recv() -> utils::Task<>;
    // 重置写缓存区
    auto reset_send() -> utils::Task<>;
    void recv();
    void send();
};
Connection::Connection(Socket&& socket, IoContext* io_context)
    : impl_(std::make_unique<Impl>(std::move(socket), io_context))
{
}
auto Connection::start() -> utils::Task<> { co_await impl_->start(shared_from_this()); }

auto Connection::stop() -> utils::Task<> { co_await impl_->stop(); }

auto Connection::async_recv() -> utils::Task<RecvResult> { co_return co_await impl_->async_recv(); }
auto Connection::async_send(std::string_view data) -> utils::Task<SendResult>
{
    co_return co_await impl_->async_send(data);
}

auto Connection::reset_recv() -> utils::Task<> { co_await impl_->reset_recv(); }
auto Connection::reset_send() -> utils::Task<> { co_await impl_->reset_send(); }

auto Connection::Impl::start(std::shared_ptr<Connection> self) -> utils::Task<>
{
    co_await io_context->inThread();
    if (state == State::Stopped)
    {
        state = State::Started;
        auto channel = std::make_unique<Channel>(socket.fd());
        channel->type = Channel::Type::Read;
        channel->read_callBack = [self = std::move(self), this]() { recv(); };
        channel->write_callBack = [this]() { send(); };
        io_context->add(std::move(channel));
    }
}
auto Connection::Impl::stop() -> utils::Task<>
{
    co_await io_context->inThread();
    if (state == State::Started)
    {
        state = State::Stopped;
        io_context->remove(socket.fd());
    }
}
auto Connection::Impl::async_recv() -> utils::Task<RecvResult>
{
    co_await io_context->inThread();
    if (!recv_buffer.empty())
    {
        auto data = std::move(std::get<0>(recv_buffer.front()));
        recv_buffer.pop();
        co_return {true, data};
    }
    utils::Awaitable<RecvResult> waiter;
    recv_waiters.push(&waiter);
    co_return co_await waiter;
}

auto Connection::Impl::async_send(std::string_view data) -> utils::Task<SendResult>
{
    co_await io_context->inThread();
    if (send_buffer.empty())
    {
        auto res = socket.send(data);
        // right or send conplete
        if (!res.second || res.first == data.size())
        {
            co_return {res.first, res.second};
        }
        // send not complete
        data.remove_prefix(res.first);
        utils::Awaitable<SendResult> waiter;
        send_buffer.push({std::string(data), 0});
        send_waiters.push(&waiter);
        start_send();
        co_return co_await waiter;
    }
    utils::Awaitable<SendResult> waiter;
    send_buffer.push({std::string(data), 0});
    send_waiters.push(&waiter);
    co_return co_await waiter;
}
auto Connection::Impl::reset_recv() -> utils::Task<>
{
    co_await io_context->inThread();
    recv_buffer = {};
}

auto Connection::Impl::reset_send() -> utils::Task<>
{
    co_await io_context->inThread();
    send_buffer = {};
}

auto Connection::Impl::start_recv() -> utils::Task<>
{
    // co_await io_context->inThread();
    while (state == State::Started)
    {
        co_await io_context->in(socket.fd());
        auto res = socket.recv();
        if (!res.second)
        {
            stop();
            break;
        }
        // recv right
        if (recv_waiters.empty())
        {
            recv_buffer.push({std::move(res.first), 0});
        }
        else
        {
            auto& waiter = recv_waiters.front();
            waiter->result = {true, std::move(res.first)};
            waiter->task.resume();
            recv_waiters.pop();
        }
    }
}

auto Connection::Impl::start_send() -> utils::Task<>
{
    // co_await io_context->inThread();
    while (state == State::Started && !send_buffer.empty())
    {
        co_await io_context->out(socket.fd());
        while (true)
        {
            auto& [data, offset] = send_buffer.front();
            std::string_view data_view(data);
            data_view.remove_prefix(offset);
            auto res = socket.send(data_view);
            if (!res.second)
            {
                stop();
                break;
            }
            // send right
            if (res.first < data.size())
            {
                offset += res.first;
                break;
            }
            send_buffer.pop();
        }
    }
}
} // namespace tcp