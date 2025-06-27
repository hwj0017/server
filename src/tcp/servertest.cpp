#include "iocontext.h"
#include "iocontextpool.h"
#include "socket.h"
#include "tcp/acceptor.h"
#include "tcp/connection.h"
#include "utils/task.h"
#include <iostream>
#include <memory>
#include <thread>
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
auto fun(std::shared_ptr<tcp::Acceptor> acceptor) -> utils::Task<>
{
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

int main()
{
    tcp::IoContextPool pool;
    auto acceptor = std::make_shared<tcp::Acceptor>("127.0.0.1", 8080, &pool);
    acceptor->start();
    std::thread waker_thread([&]() -> utils::Task<> {
        sleep(2);
        co_await pool.getIoContext()->in_thread();
        std::cout << "Waker thread woke up the IoContext" << std::endl;
    });
    auto task = fun(std::move(acceptor));
    pool.run();
}
