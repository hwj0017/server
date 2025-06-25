
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
#include <cstddef>
#include <memory>
namespace tcp
{

struct Acceptor::Impl
{
    Socket socket;
    IoContext* io_context;
    IoContextPool* io_context_pool;
    std::unique_ptr<utils::Channel<Socket>> accept_channel;
    Impl(std::string_view listen_ip, uint16_t port, IoContextPool* io_context_pool)
        : socket(Socket::createAcceptorSocket({listen_ip, port})), io_context(io_context_pool->getIoContext()),
          io_context_pool(io_context_pool)
    {
        auto node = std::make_unique<IoContext::Node>(socket.fd());
        node->read_callback = [this, node = node.get()]() {
            if (accept_channel->is_full())
            {
                io_context->disable_read(node);
                [this, node]() -> utils::Task<> {
                    co_await accept_channel->not_full();
                    io_context->enable_read(node);
                }();
            }
        };
        node->read_callback = io_context->add();
    }
};

Acceptor::Acceptor(std::string_view listen_ip, uint16_t port, IoContextPool* io_context_pool) { io }
struct Acceptor::Impl:
{

    State state = State::Stopped;

    ~Impl() = default;

    void on_write() override
    {
        // Do nothing for acceptor
    }
    auto async_accept() -> utils::Task<AcceptResult>;
    auto reset_accept() -> utils::Task<>;
};

Acceptor ::~Acceptor() = default;
auto Acceptor::start() -> utils::Task<> { return impl_->start(); }

auto Acceptor::stop() -> utils::Task<> { return impl_->stop(); }

auto Acceptor::async_accept() -> utils::Task<AcceptResult> { return impl_->async_accept(); }

auto Acceptor::reset_accept() -> utils::Task<> { return impl_->reset_accept(); }

auto Acceptor::Impl::start() -> utils::Task<>
{
    co_await io_context->in_thread();
    if (state == State::Started)
    {
        co_return;
    }
    state = State::Started;
    // 协程不持有本身
    auto [input_channel, _] = io_context->add(socket.fd());
    accept_channel = io_context->get_channel<Socket>(socket.fd());
    start_accept(input_channel, accept_channel);
}

auto Acceptor::Impl::stop() -> utils::Task<>
{
    co_await io_context->in_thread();
    if (state == State::Started)
    {
        state = State::Stopped;
        io_context->remove(socket.fd());
    }
}

// 定义一个异步接受函数，返回一个AcceptResult类型的Task
auto Acceptor::Impl::async_accept() -> utils::Task<AcceptResult>
{
    auto client_socket = co_await accept_channel.async_pop();
    if (client_socket.has_value())
    {
        co_return {std::make_shared<Connection>(std::move(client_socket.value()), io_context_pool->getIoContext())};
    }
    co_return {};
}

auto Acceptor::Impl::reset_accept() -> utils::Task<>
{
    co_await io_context->in_thread();
    accept_channel.reset();
}

auto Acceptor::Impl::start_accept(utils::Channel<>* input_channel, utils::Channel<Socket>* accept_channel)
    -> utils::Task<>
{
    // Todo
    if (!co_await accept_channel->not_full())
    {
        // acceptor close
        co_return;
    }
    if (!co_await input_channel.async_pop())
    {
        // acceptor close
        co_return;
    }
    auto result = socket.accept();
    if (result.has_value())
    {
        accept_channel.push(std::move(result.value()));
    }
    else
    {
        // socket close
        co_await stop();
        co_return;
    }
}
} // namespace tcp
