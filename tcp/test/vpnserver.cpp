#include "tcp/server.h"
#include "utils/task.h"
#include <iostream>
#include <memory>
#include <unistd.h>

class VPNServer
{
  public:
    VPNServer() = default;
    ~VPNServer() = default;
    void start()
    {
        serve();
        server_.start();
    }

  private:
    auto serve() -> utils::IdTask<>
    {
        auto acceptor = server_.new_acceptor("127.0.0.1", 8888);
        acceptor->start();
        while (true)
        {
            auto connections = co_await acceptor->async_accept();
            if (!connections.has_value())
            {
                VOID_TASK_ERROR
            }
            for (auto&& connection : connections.value())
            {
                connection->start();
                vpn(std::move(connection));
            }
        }
    }
    auto vpn(std::shared_ptr<tcp::Connection> connection) -> utils::Task<>
    {
        co_await connection->start();
        auto connector = server_.new_connector("127.0.0.1", 8080);
        co_await connector->start();

        while (true)
        {
            auto message = co_await connection->async_recv();
            if (!message.has_value())
            {
                VOID_TASK_ERROR
            }
            std::cout << message->data() << std::endl;
            co_await connector->async_send(message.value());

            auto message1 = co_await connector->async_recv();
            if (!message1.has_value())
            {
                VOID_TASK_ERROR
            }
            std::cout << message1->data() << std::endl;
            co_await connection->async_send(message1.value());
        }
    }
    tcp::Server server_{};
};

auto fun() -> utils::IdTask<> { co_return; }
int main()
{
    VPNServer server;
    server.start();
}
