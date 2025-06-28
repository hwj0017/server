#include "tcp/server.h"
#include "tcp/connection.h"
#include "utils/task.h"
#include <iostream>
#include <memory>
#include <unistd.h>
auto echo(std::shared_ptr<tcp::Connection> connection) -> utils::Task<>
{
    auto massage = co_await connection->async_recv();
    if (massage.has_value())
    {
        std::cout << massage.value() << std::endl;
        co_await connection->async_send(massage.value());
    }
}

class EchoServer : public tcp::Server
{
  public:
    EchoServer() {}
    auto serve() -> utils::Task<> override
    {
        auto acceptor = new_acceptor("127.0.0.1", 8080);
        acceptor->start();
        co_await acceptor->delay(1);
        std::cout << " 1" << std::endl;
        // while (true)
        // {
        //     auto connection = co_await acceptor->async_accept();
        //     if (connection.has_value())
        //     {
        //         connection.value()->start();
        //         echo(std::move(connection.value()));
        //     }
        //     else
        //     {
        //         break;
        //     }
        // }
    }
};

int main()
{
    EchoServer server;
    server.start();
}
