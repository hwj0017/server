#include "tcp/acceptor.h"
#include "channel.h"
#include "socket.h"
#include "taskrunner.h"
#include "tcp/inetaddress.h"
#include "utils/log.h"
#include <memory>
namespace tcp
{
struct Acceptor::Impl
{
    Impl(int fd, Channel* channel, const InetAddress& listenAddr, const Tasks& tasks)
        : fd_(fd), channel_(channel), listenAddr_(listenAddr), tasks_(tasks)
    {
    }
    int fd_;
    Channel* channel_;
    InetAddress listenAddr_;
    Tasks tasks_;
};
Acceptor::Acceptor(Channel* channel, const InetAddress& listenAddr, const Tasks& tasks)
{
    impl_ = std::make_unique<Impl>(socket::createAcceptorSocket(listenAddr), channel, listenAddr, tasks);
    channel->setReadTask([acceptor = shared_from_this(), this]() {
        InetAddress clientAddr;
        int clientFd = socket::accept(impl_->fd_, &clientAddr);
        acceptor->impl_->tasks_.acceptTask(this, clientFd, clientAddr);
        Logger::logger << std::string("accepted a client from ") << clientAddr.toIpPort();
    });
}

void Acceptor::start()
{
    impl_->channel_->enableRead();
}

void Acceptor::stop()
{
    impl_->channel_->disableAll();
}
Acceptor::~Acceptor() = default;

} // namespace tcp
