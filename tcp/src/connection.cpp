#include "tcp/connection.h"
#include "doublebuffer.h"
#include "iocontext.h"
#include "iocontextpool.h"
#include "socket.h"
#include "utils/channel.h"
#include "utils/log.h"
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
    utils::Channel<RecvResult> recv_channel_{0};
    std::vector<char> recv_buffer_{};
    DoubleBuffer send_buffer_{MaxSendBufferSize};

    State state_ = State::Stopped;
    Impl(Socket&& socket, IoContext* io_context) : socket_(std::move(socket)), io_context_(io_context) {}
    ~Impl()
    {
        stop_in_thread();
        utils::Logger::logger << "connection close" + std::to_string(socket_.fd()) + "\n";
    }
    auto start() -> utils::Task<>
    {
        co_await io_context_->in_thread();
        if (state_ == State::Started)
        {
            co_return;
        }
        state_ = State::Started;
        start_recv();
        start_send();
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
