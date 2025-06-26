#include "iocontext.h"
#include "node.h"
#include "socket.h"
#include "tcp/connector.h"
#include "utils/channel.h"
#include "utils/task.h"
#include <cassert>
#include <coroutine>
#include <cstddef>
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
    static constexpr size_t kMaxBufferSize = 1024;
    Socket socket_;
    IoContext* io_context_;
    Node node_;
    utils::Channel<std::string> recv_channel_{kMaxBufferSize};
    utils::Channel<std::string> send_channel_{kMaxBufferSize};

    State state_ = State::Stopped;
    Impl(std::string_view server_ip, uint16_t port, IoContext* io_context_)
        : socket_(Socket::createConnectorSocket({server_ip, port})), io_context_(io_context_), node_(socket_.fd())
    {
    }
    // run in queue
    ~Impl() { stop_in_thread(); }
    auto start() -> utils::Task<>
    {
        co_await io_context_->in_thread();
        if (state_ == State::Started)
        {
            co_return;
        }
        state_ = State::Started;
        node_.type = Node::Type::Both;
        io_context_->add(&node_);
        node_.read_task = start_recv();
        node_.write_task = start_send();
    }
    auto start_recv() -> utils::Task<>
    {
        while (true)
        {
            if (recv_channel_.is_full())
            {
                io_context_->disable_read(&node_);
                if (!co_await recv_channel_.not_full())
                {
                    // recv_channel_ close
                    co_return;
                }
                io_context_->enable_read(&node_);
            }
            co_await std::suspend_always{};
            if (auto result = socket_.recv(); result.has_value())
            {
                recv_channel_.push(std::move(result.value()));
            }
            else
            {
                stop_in_thread();
            }
        }
    }

    auto start_send() -> utils::Task<>
    {
        while (true)
        {
            if (send_channel_.is_empty())
            {
                // default not write
                if (!co_await send_channel_.not_empty())
                {
                    // send_channel_ close
                    co_return;
                }
            }
            auto res = send_channel_.pop();
            assert(res.has_value());
            std::string_view data = res.value();
            if (auto result = socket_.send(data); result.has_value())
            {
                data.remove_prefix(result.value());
            }
            else
            {
                stop_in_thread();
            }
            while (!data.empty())
            {
                node_.type |= Node::Type::Write;
                io_context_->enable_write(&node_);
                co_await std::suspend_always{};
                if (auto result = socket_.send(data); result.has_value())
                {
                    data.remove_prefix(result.value());
                }
                else
                {
                    stop_in_thread();
                }
            }
        }
    }
    void stop_in_thread()
    {
        if (state_ == State::Stopped)
        {
            return;
        }
        state_ = State::Stopped;
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
    auto async_recv() -> utils::Task<RecvResult> { co_return co_await recv_channel_.async_pop(); }
    auto async_send(std::string_view data) -> utils::Task<SendResult>
    {
        co_return co_await send_channel_.async_push(std::string(data));
    }
    // 重置读缓存区
    auto reset_recv() -> utils::Task<>
    {
        co_await io_context_->in_thread();
        recv_channel_.reset();
    }
    // 重置写缓存区
    auto reset_send() -> utils::Task<>
    {
        co_await io_context_->in_thread();
        send_channel_.reset();
    }
};
Connector::Connector(std::string_view server_ip, uint16_t port, IoContext* io_context_)
    : impl_(std::make_unique<Impl>(server_ip, port, io_context_))
{
}
Connector::~Connector() { delay_destroy(std::move(impl_)); }
auto Connector::start() -> utils::Task<> { return impl_->start(); }
auto Connector::stop() -> utils::Task<> { return impl_->stop(); }

auto Connector::async_recv() -> utils::Task<RecvResult> { return impl_->async_recv(); }
auto Connector::async_send(std::string_view data) -> utils::Task<SendResult> { return impl_->async_send(data); }

auto Connector::reset_recv() -> utils::Task<> { return impl_->reset_recv(); }
auto Connector::reset_send() -> utils::Task<> { return impl_->reset_send(); }

auto Connector::delay_destroy(std::unique_ptr<Impl> impl) -> utils::Task<> { co_await impl->io_context_->queue(); }

} // namespace tcp