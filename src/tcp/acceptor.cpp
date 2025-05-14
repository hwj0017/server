#include "tcp/acceptor.h"
#include "socket.h"
#include "taskrunner.h"
#include "tcp/inetaddress.h"
#include "utils/log.h"
#include <memory>
namespace tcp
{
struct Acceptor::Impl
{
    Impl(int fd, TaskRunner* taskrunner, const InetAddress& listenAddr, const Tasks& tasks)
        : fd_(fd), taskRunner_(taskrunner), listenAddr_(listenAddr), tasks_(tasks)
    {
    }
    int fd_;
    TaskRunner* taskRunner_;
    InetAddress listenAddr_;
    Tasks tasks_;
};
Acceptor::Acceptor(TaskRunner* taskRunner, const InetAddress& listenAddr, const Tasks& tasks)
{

    impl_ = std::make_unique<Impl>(socket::createAcceptorSocket(listenAddr), taskRunner, listenAddr, tasks);
}

void Acceptor::start()
{
    impl_->taskRunner_->runTask([acceptor = shared_from_this()]() mutable {
        // 读事件
        auto readTask = [](Acceptor* acceptor) {
            InetAddress clientAddr;
            int clientFd = socket::accept(acceptor->impl_->fd_, &clientAddr);
            acceptor->impl_->tasks_.acceptTask(acceptor, clientFd, clientAddr);
            Logger::logger << std::string("accepted a client from ") << clientAddr.toIpPort();
        };
        acceptor->impl_->taskRunner_->addIoTask<Acceptor>(acceptor->impl_->fd_, std::move(acceptor),
                                                          std::move(readTask));
    });
}

void Acceptor::stop()
{
    impl_->taskRunner_->removeIoTask(impl_->fd_);
}
Acceptor::~Acceptor() = default;

} // namespace tcp
