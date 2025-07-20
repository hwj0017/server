#include "tcp/server.h"
#include "utils/task.h"
#include <iostream>
#include <memory>
#include <unistd.h>
class EchoServer
{
  public:
    EchoServer() = default;
    ~EchoServer() = default;
    void start()
    {
        serve();
        server_.start();
    }

  private:
    auto serve() -> utils::IdTask<>
    {
        auto acceptor = server_.new_acceptor("127.0.0.1", 8080);
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
                echo(std::move(connection));
            }
        }
    }
    auto echo(std::shared_ptr<tcp::Connection> connection) -> utils::Task<>
    {
        while (true)
        {
            auto message = co_await connection->async_recv();
            if (!message.has_value())
            {
                VOID_TASK_ERROR
            }
            std::cout << message->data() << std::endl;
            co_await connection->async_send(message.value());
        }
    }
    tcp::Server server_{};
};

int main()
{
    EchoServer server;
    server.start();
}
