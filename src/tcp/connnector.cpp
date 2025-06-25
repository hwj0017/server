#include "iocontext.h"
#include "socket.h"
#include "tcp/connector.h"
#include "utils/channel.h"
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
struct Connector::Impl
{
    enum class State
    {
        Started,
        Stopped
    };
    static constexpr size_t kMaxBufferSize = 1024;
    Socket socket;
    IoContext* io_context;
    utils::Channel<std::string> recv_channel{kMaxBufferSize};
    utils::Channel<std::string> send_channel{kMaxBufferSize};
    State state = State::Stopped;
    Impl(std::string_view server_ip, uint16_t port, IoContext* io_context)
        : socket(Socket::createConnectorSocket({server_ip, port})), io_context(io_context)
    {
    }
    ~Impl() = default;
    auto start(std::shared_ptr<Connector> self) -> utils::Task<>;
    auto stop() -> utils::Task<>;
    auto async_recv() -> utils::Task<RecvResult>;
    auto async_send(std::string_view data) -> utils::Task<SendResult>;
    // 重置读缓存区
    auto reset_recv() -> utils::Task<>;
    // 重置写缓存区
    auto reset_send() -> utils::Task<>;
    auto start_recv(utils::Channel<>& input_channel) -> utils::Task<>;
    auto start_send(utils::Channel<>& output_channel) -> utils::Task<>;
};
Connector::Connector(std::string_view server_ip, uint16_t port, IoContext* io_context)
    : impl_(std::make_unique<Impl>(server_ip, port, io_context))
{
}

Connector::~Connector() = default;
auto Connector::start() -> utils::Task<> { return impl_->start(shared_from_this()); }

auto Connector::stop() -> utils::Task<> { return impl_->stop(); }

auto Connector::async_recv() -> utils::Task<RecvResult> { return impl_->async_recv(); }
auto Connector::async_send(std::string_view data) -> utils::Task<SendResult> { return impl_->async_send(data); }

auto Connector::reset_recv() -> utils::Task<> { return impl_->reset_recv(); }
auto Connector::reset_send() -> utils::Task<> { return impl_->reset_send(); }

auto Connector::Impl::start(std::shared_ptr<Connector> self) -> utils::Task<>
{
    co_await io_context->in_thread();
    if (state == State::Started)
    {
        co_return;
    }
    state = State::Started;
    auto [input_channel, output_channel] = io_context->add(socket.fd(), std::move(self));
    start_recv(input_channel);
    start_send(output_channel);
}
auto Connector::Impl::stop() -> utils::Task<>
{
    co_await io_context->in_thread();
    if (state == State::Started)
    {
        state = State::Stopped;
        recv_channel.close();
        send_channel.close();
        io_context->remove(socket.fd());
    }
}
auto Connector::Impl::async_recv() -> utils::Task<RecvResult>
{
    co_await io_context->in_thread();
    co_return co_await recv_channel.async_pop();
}

auto Connector::Impl::async_send(std::string_view data) -> utils::Task<SendResult>
{
    co_await io_context->in_thread();
    co_return co_await send_channel.async_push(std::string(data));
}
auto Connector::Impl::reset_recv() -> utils::Task<>
{
    co_await io_context->in_thread();
    recv_channel.reset();
}

auto Connector::Impl::reset_send() -> utils::Task<>
{
    co_await io_context->in_thread();
    send_channel.reset();
}

auto Connector::Impl::start_recv(utils::Channel<>& input_channel) -> utils::Task<>
{
    // co_await io_context->inThread();

    while (state == State::Started)
    {
        // 监听
        if (!co_await input_channel.async_pop())
        {
            // conection close
            co_return;
        }
        auto result = socket.recv();
        if (result.has_value())
        {
            if (!co_await recv_channel.async_push(std::move(result.value())))
            {
                // conection close
                co_return;
            }
        }
        else
        {
            // socket close
            co_await stop();
            co_return;
        }
    }
}

auto Connector::Impl::start_send(utils::Channel<>& output_channel) -> utils::Task<>
{
    // co_await io_context->inThread();

    while (state == State::Started)
    {
        // 监听
        auto data = co_await send_channel.async_pop();
        if (data.has_value())
        {
            auto data_view = std::string_view(data.value());
            while (data_view.empty())
            {
                if (!co_await output_channel.async_pop())
                {
                    // conection close
                    co_return;
                }
                auto result = socket.send(data_view);
                if (result.has_value())
                {
                    data_view.remove_prefix(result.value());
                }
                else
                {
                    // socket close
                    co_await stop();
                    co_return;
                }
            }
        }
        else
        {
            // conection close
            co_return;
        }
    }
}
} // namespace tcp