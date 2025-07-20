#include "tcp/connection.h"
#include "doublebuffer.h"
#include "iocontext.h"
#include "iocontextpool.h"
#include "ionode.h"
#include "socket.h"
#include "utils/channel.h"
#include "utils/task.h"
#include <cassert>
#include <coroutine>
#include <cstddef>
#include <iostream>
#include <memory>
#include <queue>
#include <span>
#include <string>
#include <string_view>
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
    static constexpr size_t InitialRecvBufferSize = 1024;

    static constexpr size_t MaxSendBufferSize = 1024;
    Socket socket_;
    IoContext* io_context_;
    IoNode node_;
    utils::Channel<RecvResult> recv_channel_{1};
    std::vector<char> recv_buffer_;
    utils::Channel<> send_channel_{1};
    DoubleBuffer send_buffer_{MaxSendBufferSize};

    State state_ = State::Stopped;
    Impl(Socket&& socket, IoContext* io_context)
        : socket_(std::move(socket)), io_context_(io_context), node_(socket_.fd())
    {
    }
    ~Impl()
    {
        stop_in_thread();
        std::cout << "close" << std::endl;
    }
    auto start() -> utils::Task<>
    {
        co_await io_context_->in_thread();
        if (state_ == State::Started)
        {
            co_return;
        }
        state_ = State::Started;
        node_.type = IoNode::Type::None;
        io_context_->add(&node_);
        node_.on_read_ = [this]() { on_read(); };
        node_.on_write_ = [this]() { on_write(); };
    }
    void on_read()
    {
        if (auto result = socket_.recv(recv_buffer_); !result.has_value())
        {
            stop_in_thread();
            return;
        }
        if (!recv_buffer_.empty())
        {
            recv_channel_.push({recv_buffer_});
            recv_buffer_.clear();
            if (recv_channel_.is_full())
            {
                io_context_->disable_read(&node_);
            }
        }
    }

    void on_write()
    {
        if (auto result = socket_.send(send_buffer_.get_data()); result.has_value())
        {
            send_buffer_.remove(result.value());
            if (!send_buffer_.is_full())
            {
                send_channel_.pop();
            }
            if (!send_buffer_.is_empty())
            {
                io_context_->enable_write(&node_);
            }
        }
        else
        {
            stop_in_thread();
        }
    }
    void stop_in_thread()
    {
        if (state_ == State::Stopped)
        {
            return;
        }
        state_ = State::Stopped;
        node_.is_closed = true;
        io_context_->remove(&node_);
        // may add ~Impl in queue
        recv_channel_.close();
        send_channel_.close();
    }

    auto stop() -> utils::Task<>
    {
        co_await io_context_->in_thread();
        stop_in_thread();
    }
    auto async_recv_local() -> utils::Channel<RecvResult>::AsyncPop
    {
        // not read
        if (!(node_.type && IoNode::Type::Read))
        {
            on_read();
            if (recv_channel_.is_empty())
            {
                io_context_->enable_read(&node_);
            }
        }
        return recv_channel_.async_pop();
    }
    auto async_recv() -> utils::Task<RecvResult>
    {
        co_await io_context_->in_thread();
        auto recv_res = co_await async_recv_local();
        if (!recv_res.has_value())
        {
            VALUE_TASK_ERROR
        }
        co_return recv_res.value();
    }
    auto async_send_local(std::span<char> data) -> utils::Channel<>::NotFull
    {
        // not write
        if (!(node_.type && IoNode::Type::Write))
        {
            if (auto result = socket_.send(data); result.has_value())
            {
                data = {data.data() + result.value(), data.size() - result.value()};
                send_buffer_.append(data);
                io_context_->enable_write(&node_);
                if (send_buffer_.is_full())
                {
                    send_channel_.push();
                }
            }
            else
            {
                stop_in_thread();
            }
        }
        return send_channel_.not_full();
    }

    auto async_send(std::span<char> data) -> utils::Task<>
    {
        co_await io_context_->in_thread();
        if (!co_await async_send_local(data))
        {
            VOID_TASK_ERROR
        }
    }
};
Connection::Connection(Socket&& socket_, IoContext* io_context)
    : impl_(std::make_unique<Impl>(std::move(socket_), io_context))
{
}
Connection::~Connection() { delay_destroy(std::move(impl_)); };
auto Connection::start() -> utils::Task<> { return impl_->start(); }
auto Connection::stop() -> utils::Task<> { return impl_->stop(); }
auto Connection::async_recv_local() -> utils::Channel<RecvResult>::AsyncPop { return impl_->async_recv_local(); }
auto Connection::async_recv() -> utils::Task<RecvResult> { return impl_->async_recv(); }

auto Connection::async_send_local(std::span<char> data) -> utils::Channel<>::NotFull
{
    return impl_->async_send_local(data);
}
auto Connection::async_send(std::span<char> data) -> utils::Task<> { return impl_->async_send(data); }

auto Connection::delay_destroy(std::unique_ptr<Impl> impl) -> utils::Task<> { co_await impl->io_context_->queue(); }

} // namespace tcp
