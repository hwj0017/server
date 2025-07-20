
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
#include <cassert>
#include <coroutine>
#include <cstddef>
#include <iostream>
#include <memory>
#include <ostream>
#include <span>
#include <vector>

namespace tcp
{

struct Acceptor::Impl
{
    enum class State
    {
        Started,
        Stopped
    };
    static constexpr size_t InitialAcceptBufferSize = 1024;
    Socket socket_;
    IoContext* io_context_;
    IoContextPool* io_context_pool_;
    IoNode node_;
    utils::Channel<std::span<std::shared_ptr<Connection>>> accept_channel_{1};
    std::vector<std::shared_ptr<Connection>> accept_buffer_{};
    State state = State::Stopped;
    Impl(std::string_view listen_ip, uint16_t port, IoContext* io_context, IoContextPool* io_context_pool)
        : socket_(Socket::createAcceptorSocket({listen_ip, port})), io_context_pool_(io_context_pool),
          io_context_(io_context), node_(socket_.fd())
    {
        assert(socket_.fd() != -1);
        accept_buffer_.reserve(InitialAcceptBufferSize);
    }
    // run in queue
    ~Impl()
    {
        stop_in_thread();
        std::cout << "close" << std::endl;
    }
    auto start() -> utils::Task<>
    {
        co_await io_context_->in_thread();
        if (state == State::Started)
        {
            co_return;
        }
        state = State::Started;
        node_.type = IoNode::Type::None;
        io_context_->add(&node_);
        node_.on_read_ = [this]() { on_read(); };
    }
    void on_read()
    {
        accept_buffer_.reserve(InitialAcceptBufferSize);
        while (true)
        {
            auto client_socket_ = socket_.accept();
            if (!client_socket_.has_value())
            {
                stop_in_thread();
                return;
            }
            if (client_socket_->fd() == -1)
            {
                if (accept_buffer_.size() > 0)
                {
                    accept_channel_.push({accept_buffer_});
                    accept_buffer_.clear();
                }
                return;
            }
            accept_buffer_.emplace_back(
                std::make_shared<Connection>(std::move(client_socket_.value()), io_context_pool_->getIoContext()));
        }
    }
    void stop_in_thread()
    {
        if (state == State::Stopped)
        {
            return;
        }
        state = State::Stopped;
        // may add ~Impl in queue
        accept_channel_.close();
        io_context_->remove(&node_);
    }
    auto stop() -> utils::Task<>
    {
        co_await io_context_->in_thread();
        stop_in_thread();
    }

    auto async_accept_local() -> utils::Channel<AcceptResult>::AsyncPop
    {
        // not read
        if (!(node_.type && IoNode::Type::Read))
        {
            on_read();
            if (accept_channel_.is_empty())
            {
                io_context_->enable_read(&node_);
            }
        }
        return accept_channel_.async_pop();
    }
    auto async_accept() -> utils::Task<AcceptResult>
    {
        co_await io_context_->in_thread();
        if (auto res = co_await async_accept_local(); res.has_value())
        {
            co_return res.value();
        }
        else
        {
            VALUE_TASK_ERROR
        }
    }
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

auto Acceptor::async_accept_local() -> utils::Channel<AcceptResult>::AsyncPop { return impl_->async_accept_local(); }
auto Acceptor::delay_destroy(std::unique_ptr<Impl> impl) -> utils::Task<> { co_await impl->io_context_->queue(); }

} // namespace tcp
