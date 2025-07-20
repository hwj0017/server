#include "doublebuffer.h"
#include "iocontext.h"
#include "ionode.h"
#include "socket.h"
#include "tcp/connector.h"
#include "utils/channel.h"
#include "utils/task.h"
#include <cassert>
#include <coroutine>
#include <cstddef>
#include <iostream>
#include <memory>
#include <queue>
#include <string>
#include <string_view>
#include <unistd.h>
#include <vector>

namespace tcp
{
struct Connector::Impl
{
    enum class State
    {
        Started,
        Stopped
    };
    static constexpr size_t MaxSendBufferSize = 1024;
    Socket socket_;
    IoContext* io_context_;
    IoNode node_;
    utils::Channel<RecvResult> recv_channel_{1};
    std::vector<char> recv_buffer_ = std::vector<char>(1024);
    utils::Channel<> send_channel_{1};
    DoubleBuffer send_buffer_{MaxSendBufferSize};

    State state_ = State::Stopped;
    Impl(std::string_view server_ip, uint16_t port, IoContext* io_context_)
        : socket_(Socket::createConnectorSocket({server_ip, port})), io_context_(io_context_), node_(socket_.fd())
    {
        assert(socket_.fd() >= 0);
    }
    // run in queue
    ~Impl()
    {
        stop_in_thread();
        std::cout << "comnector stop" << std::endl;
    }

    auto start() -> utils::Task<>
    {
        co_await io_context_->in_thread();
        if (state_ == State::Started)
        {
            co_return;
        }
        if (!co_await connect_complete())
        {
            VOID_TASK_ERROR
        }
        co_return start_really();
    }
    auto connect_complete() -> utils::Task<>
    {
        node_.type = IoNode::Type::Write;
        io_context_->add(&node_);
        utils::Channel<> channel{0};
        node_.on_write_ = [&channel] { channel.push(); };

        co_await channel.async_pop();
        io_context_->remove(&node_);
        if (!is_connected())
        {
            VOID_TASK_ERROR
        }
        co_return;
    }
    auto is_connected() -> bool
    {
        // 检查连接状态
        int error = 0;
        socklen_t len = sizeof(error);
        if (getsockopt(socket_.fd(), SOL_SOCKET, SO_ERROR, &error, &len) < 0)
        {
            perror("getsockopt failed");
            return false;
        }

        if (error != 0)
        {
            errno = error;
            return false;
        }
        return true; // 连接成功
    }
    void start_really()
    {
        state_ = State::Started;
        node_.type = IoNode::Type::None;
        io_context_->add(&node_);
        node_.on_read_ = [this] { on_read(); };
        node_.on_write_ = [this] { on_write(); };
    }
    void on_read()
    {
        if (auto result = socket_.recv(recv_buffer_); result.has_value())
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
Connector::Connector(std::string_view server_ip, uint16_t port, IoContext* io_context)
    : impl_(std::make_unique<Impl>(server_ip, port, io_context))
{
}
Connector::~Connector() { delay_destroy(std::move(impl_)); };
auto Connector::start() -> utils::Task<> { return impl_->start(); }
auto Connector::stop() -> utils::Task<> { return impl_->stop(); }
auto Connector::async_recv_local() -> utils::Channel<RecvResult>::AsyncPop { return impl_->async_recv_local(); }
auto Connector::async_recv() -> utils::Task<RecvResult> { return impl_->async_recv(); }

auto Connector::async_send_local(std::span<char> data) -> utils::Channel<>::NotFull
{
    return impl_->async_send_local(data);
}
auto Connector::async_send(std::span<char> data) -> utils::Task<> { return impl_->async_send(data); }

auto Connector::delay_destroy(std::unique_ptr<Impl> impl) -> utils::Task<> { co_await impl->io_context_->queue(); }
} // namespace tcp
