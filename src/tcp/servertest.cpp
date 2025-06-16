#include "iocontext.h"
#include "socket.h"
#include "tcp/acceptor.h"
#include "tcp/inetaddress.h"
#include "utils/task.h"
#include <iostream>
#include <memory>
#include <thread>
#include <unistd.h>
auto fun(std::shared_ptr<tcp::Acceptor> acceptor) -> utils::Task<>
{
    auto connection = co_await acceptor->async_accept();
    std::cout << "1" << std::endl;
}
int main()
{
    tcp::IoContext io_conetxt;
    tcp::InetAddress listen_address("127.0.0.1", 8080);
    auto acceptor = std::make_shared<tcp::Acceptor>(listen_address, &io_conetxt);
    acceptor->start();
    // std::thread waker_thread([&io_conetxt]() -> utils::Task<> {
    //     sleep(2);
    //     co_await io_conetxt.inThread();
    //     std::cout << "Waker thread woke up the IoContext" << std::endl;
    // });
    auto task = fun(acceptor);
    io_conetxt.run();
}
