#include "iocontext.h"
#include "iocontextpool.h"
#include "socket.h"
#include "tcp/acceptor.h"
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
    tcp::IoContextPool pool;
    auto acceptor = std::make_shared<tcp::Acceptor>("127.0.0.1", 8080, &pool);
    acceptor->start();
    // std::thread waker_thread([&io_conetxt]() -> utils::Task<> {
    //     sleep(2);
    //     co_await io_conetxt.inThread();
    //     std::cout << "Waker thread woke up the IoContext" << std::endl;
    // });
    auto task = fun(acceptor);
    pool.run();
}
