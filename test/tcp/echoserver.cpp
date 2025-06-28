#include "tcp/server.h"
#include "utils/task.h"
#include <iostream>
#include <memory>
#include <unistd.h>
auto echo(std::shared_ptr<tcp::Connection> connection) -> utils::Task<>
{
    while (true)
    {
        auto message = co_await connection->async_recv();
        if (message.has_value())
        {
            std::cout << message.value() << std::endl;
            co_await connection->async_send(message.value());
        }
        else
        {
            break;
        }
    }
}

class EchoServer : public tcp::Server
{
  public:
    EchoServer() {}
    auto serve() -> utils::IdTask<> override
    {
        auto acceptor = new_acceptor("127.0.0.1", 8888);
        acceptor->start();
        while (true)
        {
            auto connection = co_await acceptor->async_accept();
            if (connection.has_value())
            {
                connection.value()->start();
                echo(std::move(connection.value()));
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
    EchoServer server;
    server.start();
}
