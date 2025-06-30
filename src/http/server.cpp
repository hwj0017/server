#include "http/server.h"
#include "context.h"
#include "http/request.h"
#include "http/response.h"
#include "tcp/server.h"
#include "utils/task.h"
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <sys/types.h>
namespace http
{
struct Server::Impl
{
    tcp::Server tcp_server_{};
    std::string ip_;
    uint16_t port_;
    std::string root_;

    Impl(std::string&& ip, uint16_t port_, std::string&& root)
        : ip_(std::move(ip)), port_(port_), root_(std::move(root))
    {
    }
    ~Impl() = default;
    void start()
    {
        serve();
        tcp_server_.start();
    }
    auto serve() -> utils::Task<>
    {
        auto acceptor = tcp_server_.new_acceptor(ip_, port_);
        acceptor->start();
        while (true)
        {
            auto connection = co_await acceptor->async_accept();
            if (connection.has_value())
            {
                connection.value()->start();
                http(std::move(connection.value()));
            }
            else
            {
                break;
            }
        }
    }
    auto http(std::shared_ptr<tcp::Connection> connection) -> utils::Task<>
    {
        Context context;
        while (true)
        {
            auto buf = co_await connection->async_recv();
            if (!buf.has_value())
            {
                connection->stop();
                break;
            }
            std::cout << "recv: " << buf.value() << std::endl;
            if (!context.parseRequest(std::move(buf.value())))
            {
                co_await connection->async_send("HTTP/1.1 400 Bad Request\r\n\r\n");
                connection->stop();
                break;
            }

            if (context.gotAll())
            {
                auto response = on_request(std::move(context.request()));
                co_await connection->async_send(response.message());
                if (response.closeConnection())
                {
                    connection->stop();
                    break;
                }
                context.reset();
            }
        }
    }

    auto on_request(Request request) -> Response
    {
        Response response;
        auto close_message = request.getHeader("Connection");
        bool is_close =
            (close_message == "close") || (request.getVersion() == Request::Http10 && close_message != "Keep-Alive");
        response.setCloseConnection(is_close);
        if (request.getMethod() == Request::Method::Get)
        {
            auto path = request.getPath();
            if (path == "/")
            {
                path = "/index.html";
            }
            std::string file_path(root_);
            file_path.append(path);
            // auto root
            auto file = std::ifstream(file_path);
            if (file.is_open())
            {
                std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                response.setStatusCode(Response::StatusCode::Ok);
                response.setBody(std::move(content));
                file.close();
            }
        }
        return response;
    }
};
Server::Server(std::string ip, uint16_t port, std::string root)
    : impl_(std::make_unique<Impl>(std::move(ip), port, std::move(root)))
{
}
Server::~Server() = default;

void Server::start() { impl_->start(); }

} // namespace http
