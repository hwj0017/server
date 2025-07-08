#include "rpc/server.h"

#include "rpc/common.pb.h"
#include "tcp/connection.h"
#include "tcp/server.h"
#include "utils/task.h"
#include <cstdint>
#include <iostream>
#include <memory>
#include <string_view>

namespace rpc
{
struct Server::Impl
{
    using Request = ::rpc::Request;
    using Response = ::rpc::Response;
    tcp::Server tcp_server_;
    std::string ip_;
    uint16_t port_;
    std::map<std::string, std::function<std::string(std::string)>> services_;
    Impl(std::string_view ip, uint16_t port) : ip_(ip), port_(port) {}
    void start()
    {
        serve();
        tcp_server_.start();
    }
    void register_service_impl(std::string&& method, std::function<std::string(std::string)> func)
    {
        services_.emplace(std::move(method), std::move(func));
    }
    auto serve() -> utils::Task<>
    {
        auto acceptor = tcp_server_.new_acceptor(ip_, port_);
        co_await acceptor->start();
        while (true)
        {
            auto connection = co_await acceptor->async_accept();
            if (connection.has_value())
            {
                connection.value()->start();
                rpc(std::move(connection.value()));
            }
            else
            {
                break;
            }
        }
    }
    auto rpc(std::shared_ptr<tcp::Connection> connection) -> utils::Task<>
    {
        co_await connection->start();
        while (true)
        {
            auto buffer = co_await connection->async_recv();
            if (!buffer.has_value())
            {
                break;
            }
            std::cout << "recv: " << buffer.value() << std::endl;
            ::rpc::Request request;
            if (!request.ParseFromString(buffer.value()))
            {
                std::cout << "error" << std::endl;
                continue;
            }
            auto response = on_request(std::move(request));
            std::cout << "send: " << response.SerializeAsString() << std::endl;
            co_await connection->async_send(response.SerializeAsString());
        }
    }
    auto on_request(Request request) -> Response
    {
        Response response;
        auto it = services_.find(request.method());
        response.set_method(std::move(request.method()));
        if (it == services_.end())
        {
            response.set_output({});
        }
        response.set_output(it->second(std::move(request.input())));
        return response;
    }
};

Server::Server(std::string_view ip, uint16_t port) : impl_(std::make_unique<Impl>(ip, port)) {}
Server::~Server() = default;

void Server::start() { impl_->start(); }
void Server::register_service_impl(std::string method, std::function<std::string(std::string)> func)
{
    impl_->register_service_impl(std::move(method), std::move(func));
}
} // namespace rpc
