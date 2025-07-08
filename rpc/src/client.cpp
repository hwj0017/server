#include "rpc/client.h"
#include "rpc/common.pb.h"
#include "tcp/server.h"
#include "utils/channel.h"
#include "utils/task.h"
#include <string>
#include <string_view>

namespace rpc
{

struct Client::Impl
{
    static constexpr size_t BufferSize = 1024;
    tcp::Server tcp_server_;
    std::string server_ip_;
    uint16_t server_port_;
    utils::Channel<Request> request_channel_;
    utils::Channel<Response> response_channel_;
    Impl(std::string_view server_ip, uint16_t server_port)
        : server_ip_(server_ip), server_port_(server_port), request_channel_(BufferSize), response_channel_(BufferSize)
    {
    }
    void start()
    {
        serve();
        tcp_server_.start();
    }
    auto serve() -> utils::Task<>
    {
        auto connector = tcp_server_.new_connector(server_ip_, server_port_);
        co_await connector->start();
        while (true)
        {
            auto request = co_await request_channel_.async_pop();
            if (!request.has_value())
            {
                break;
            }
            co_await connector->async_send(request.value().SerializeAsString());
            auto response_str = co_await connector->async_recv();
            if (response_str.has_value())
            {
                Response resonse;
                resonse.ParseFromString(response_str.value());
                response_channel_.push(std::move(resonse));
            }
        }
    }

    auto call_impl(Request&& request) -> utils::Task<Response>
    {
        co_await request_channel_.async_push(std::move(request));
        auto response = co_await response_channel_.async_pop();
        if (!response.has_value())
        {
            VALUE_TASK_ERROR
        }
        co_return response.value();
    }
};
Client::Client(std::string_view ip, uint16_t port) : impl_(std::make_unique<Impl>(ip, port)) {}
Client::~Client() = default;
void Client::start() { impl_->start(); }
auto Client::call_impl(Request&& request) -> utils::Task<Response> { return impl_->call_impl(std::move(request)); }
} // namespace rpc
