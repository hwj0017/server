#include "doublebuffer.h"
#include "iocontext.h"
#include "socket.h"
#include "tcp/connector.h"
#include "utils/channel.h"
#include "utils/log.h"
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
    utils::Channel<RecvResult> recv_channel_{0};
    std::vector<char> recv_buffer_{};
    DoubleBuffer send_buffer_{MaxSendBufferSize};

    State state_ = State::Stopped;
    Impl(std::string_view server_ip, uint16_t port, IoContext* io_context_)
        : socket_(Socket::createConnectorSocket({server_ip, port})), io_context_(io_context_)
    {
        assert(socket_.fd() >= 0);
    }
    // run in queue
    ~Impl()
    {
        stop_in_thread();
        utils::Logger::logger << "connector close" + std::to_string(socket_.fd()) + "\n";
    }

    auto start() -> utils::Task<>
    {
        co_await io_context_->in_thread();
        if (state_ == State::Started)
        {
            co_return;
        }
        state_ = State::Started;
        if (!co_await connect_complete())
        {
            stop_in_thread();
            VOID_TASK_ERROR
        }
        start_recv();
        start_send();
    }
    auto connect_complete() -> utils::Task<>
    {
        auto out_channel_ = io_context_->get_out_channel(socket_.fd());
        co_await out_channel_->async_pop();
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

    auto start_recv() -> utils::Task<>
    {
        auto in_channel = io_context_->get_in_channel(socket_.fd());

        while (true)
        {
            if (!co_await recv_channel_.not_full() || !co_await in_channel->async_pop())
            {
                co_return;
            }
            utils::Logger::logger << "recv" + std::to_string(socket_.fd()) + "\n";

            if (auto result = socket_.recv(recv_buffer_); !result.has_value())
            {
                stop_in_thread();
                VOID_TASK_ERROR
            }
            if (!recv_buffer_.empty())
            {
                recv_channel_.push({recv_buffer_});
                // TODO:
                recv_buffer_.clear();
            }
        }
    }

    auto start_send() -> utils::Task<>
    {
        auto out_channel = io_context_->get_out_channel(socket_.fd());

        while (true)
        {
            if (!co_await send_buffer_.not_empty() || !co_await out_channel->async_pop())
            {
                co_return;
            }
            utils::Logger::logger << "send" + std::to_string(socket_.fd()) + "\n";

            auto result = socket_.send(send_buffer_.get_data());
            if (!result.has_value())
            {
                stop_in_thread();
                VOID_TASK_ERROR
            }
            send_buffer_.remove(result.value());
        }
    }
    void stop_in_thread()
    {
        if (state_ == State::Stopped)
        {
            return;
        }
        state_ = State::Stopped;
        io_context_->remove(socket_.fd());
        // may add ~Impl in queue
        recv_channel_.close();
        send_buffer_.close();
    }

    auto stop() -> utils::Task<>
    {
        co_await io_context_->in_thread();
        stop_in_thread();
    }
    auto async_recv_local() -> utils::Channel<RecvResult>::AsyncPop { return recv_channel_.async_pop(); }

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
        if (send_buffer_.is_empty())
        {
            if (auto result = socket_.send(data); !result.has_value())
            {
                stop_in_thread();
            }
            else
            {
                data = {data.data() + result.value(), data.size() - result.value()};
            }
        }
        send_buffer_.append(data);
        return send_buffer_.not_full();
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
