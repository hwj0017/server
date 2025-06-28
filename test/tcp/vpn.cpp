#include "tcp/server.h"
#include "utils/task.h"
#include <iostream>
#include <memory>
#include <unistd.h>
auto vpn(tcp::Server* server, std::shared_ptr<tcp::Connection> connection) -> utils::Task<>
{
    co_await connection->start();
    auto connector = server->new_connector("127.0.0.1", 8888);
    co_await connector->start();
    while (true)
    {
        auto message = co_await connection->async_recv();
        if (message.has_value())
        {
            std::cout << message.value() << std::endl;
            co_await connector->async_send(message.value());
        }
        else
        {
            break;
        }
        auto message1 = co_await connector->async_recv();
        if (message1.has_value())
        {
            std::cout << message1.value() << std::endl;
            co_await connection->async_send(message1.value());
        }
        else
        {
            break;
        }
    }
}

class VPNServer : public tcp::Server
{
  public:
    VPNServer() {}
    auto serve() -> utils::IdTask<> override
    {
        auto acceptor = new_acceptor("127.0.0.1", 8080);
        acceptor->start();
        while (true)
        {
            auto connection = co_await acceptor->async_accept();
            if (connection.has_value())
            {
                vpn(this, std::move(connection.value()));
            }
            else
            {
                break;
            }
        }
    }
};

auto fun() -> utils::IdTask<> { co_return; }
int main()
{
    VPNServer server;
    server.start();
}
