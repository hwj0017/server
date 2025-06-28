
#include "tcp/acceptor.h"
// #include "connection.h"
#include "inetaddress.h"
#include "iocontext.h"
#include "iocontextpool.h"
#include "socket.h"
#include "tcp/awaitables.h"
#include "tcp/connection.h"
#include "utils/channel.h"
#include "utils/task.h"
#include "waker.h"
#include <coroutine>
#include <cstddef>
#include <iostream>
#include <memory>
#include <ostream>
namespace tcp
{

struct Acceptor::Impl
{
    enum class State
    {
        Started,
        Stopped
    };
    static constexpr size_t kMaxAcceptChannelSize = 1024;
    Socket socket_;
    IoContext* io_context_;
    IoContextPool* io_context_pool_;
    IoNode node_;
    utils::Channel<Socket> accept_channel{kMaxAcceptChannelSize};
    State state = State::Stopped;
    Impl(std::string_view listen_ip, uint16_t port, IoContext* io_context, IoContextPool* io_context_pool)
        : socket_(Socket::createAcceptorSocket({listen_ip, port})), io_context_pool_(io_context_pool),
          io_context_(io_context), node_(socket_.fd())
    {
    }
    // run in queue
    ~Impl() { stop_in_thread(); }
    auto start() -> utils::Task<>
    {
        co_await io_context_->in_thread();
        if (state == State::Started)
        {
            co_return;
        }
        state = State::Started;
        node_.type = IoNode::Type::Read;
        io_context_->add(&node_);
        node_.read_task = start_accept();
    }
    auto start_accept() -> utils::Task<>
    {
        while (true)
        {
            if (accept_channel.is_full())
            {
                io_context_->disable_read(&node_);
                if (!co_await accept_channel.not_full())
                {
                    co_return;
                }
                io_context_->enable_read(&node_);
            }
            co_await std::suspend_always{};
            auto client_socket_ = socket_.accept();
            if (client_socket_.has_value())
            {
                accept_channel.push(std::move(client_socket_.value()));
            }
            else
            {
                stop_in_thread();
            }
        }
    }
    void stop_in_thread()
    {
        if (state == State::Stopped)
        {
            return;
        }
        state = State::Stopped;
        io_context_->remove(&node_);
        // may add ~Impl in queue
        accept_channel.close();
    }
    auto stop() -> utils::Task<>
    {
        co_await io_context_->in_thread();
        stop_in_thread();
    }

    auto async_accept() -> utils::Task<AcceptResult>
    {
        co_await io_context_->in_thread();
        auto client_socket_ = co_await accept_channel.async_pop();
        if (client_socket_.has_value())
        {
            co_return {
                std::make_shared<Connection>(std::move(client_socket_.value()), io_context_pool_->getIoContext())};
        }
        co_return {};
    }
    auto reset_accept() -> utils::Task<>
    {
        co_await io_context_->in_thread();
        accept_channel.reset();
    }

    auto delay(double delay) -> Delay
    {
        if (delay > 0)
        {
        }
        return Delay{io_context_, delay};
    }
    auto cancel_delay(size_t id) -> utils::Task<> { return io_context_->cancel_delay(id); }
};

Acceptor::Acceptor(std::string_view listen_ip, uint16_t port, IoContext* io_context, IoContextPool* io_context__pool)
    : impl_(std::make_unique<Impl>(listen_ip, port, io_context, io_context__pool))
{
}

Acceptor ::~Acceptor()
{
    // delay destory
    delay_destroy(std::move(impl_));
}
auto Acceptor::start() -> utils::Task<> { return impl_->start(); }

auto Acceptor::stop() -> utils::Task<> { return impl_->stop(); }

auto Acceptor::async_accept() -> utils::Task<AcceptResult> { return impl_->async_accept(); }
auto Acceptor::reset_accept() -> utils::Task<> { return impl_->reset_accept(); }

auto Acceptor::delay(double delay) -> Delay { return impl_->delay(delay); }

auto Acceptor::cancel_delay(size_t id) -> utils::Task<> { return impl_->cancel_delay(id); }
auto Acceptor::delay_destroy(std::unique_ptr<Impl> impl) -> utils::Task<> { co_await impl->io_context_->queue(); }

} // namespace tcp
