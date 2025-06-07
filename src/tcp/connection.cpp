#include "tcp/connection.h"
#include "buffer.h"
#include "channel.h"
#include "taskrunner.h"
#include "tcp/inetaddress.h"
#include <cassert>
#include <cerrno>
#include <cstddef>
#include <fcntl.h>
#include <memory>
#include <string>
#include <string_view>
#include <sys/types.h>
#include <unistd.h>
namespace tcp
{
struct Connection::Impl
{
    int fd_;
    Channel* channel_;
    Tasks tasks_;
    InetAddress peerAddr_;
    Buffer readBuffer_;
    Buffer writeBuffer_;
    std::any context_;
    Impl(int fd, Channel* channel, const InetAddress& peerAddr, const Tasks& tasks)
        : fd_(fd), channel_(channel), tasks_(tasks), peerAddr_(peerAddr)
    {
    }
    ~Impl()
    {
        ::close(fd_);
    }
};
Connection::Connection(int clientfd, Channel* channel, const InetAddress& peerAddr, const Tasks& tasks)
{
    impl_ = std::make_unique<Impl>(clientfd, channel, peerAddr, tasks);
    auto read_task = [this]() {
        if (impl_->readBuffer_.readSocket(impl_->fd_) >= 0)
        {
            auto& task = impl_->tasks_.messageTask;
            if (task)
                task(this, impl_->readBuffer_.begin(), impl_->readBuffer_.size());
            impl_->readBuffer_.clear();
        }
        else
        {
            stop();
        }
    };
    auto write_task = [this]() {
        if (impl_->writeBuffer_.writeSocket(impl_->fd_) >= 0)
        {
            if (impl_->writeBuffer_.size() == 0)
            {
                // 缓冲区没有数据，停止监听写事件
                impl_->channel_->setType(Channel::Type::kRead);
            };
        }
        else
        {
            stop();
        }
    };
    impl_->channel_->setReadTask([connection = shared_from_this(), task = std::move(read_task)]() { task(); });
    // 不需要再保留this
    impl_->channel_->setWriteTask(std::move(write_task));
}

Connection::~Connection() = default;

void Connection::start()
{
    impl_->channel_->enableRead();
}
// 发送数据
void Connection::send(std::string_view data)
{
    auto send_task = [this](std::string_view data) {
        if (impl_->writeBuffer_.writeSocket(impl_->fd_, data) >= 0)
        {
            if (impl_->writeBuffer_.size() > 0)
            {
                // 缓冲区有数据，开始监听写事件
                impl_->channel_->enableWrite();
            };
        }
        else
        {
            stop();
        }
    };
    if (impl_->channel_->inThread())
    {
        send_task(data);
    }
    else
    {
        impl_->channel_->addTask(
            [connection = shared_from_this(), task = std::move(send_task), data = std::string(data)]() { task(data); });
    }
}

void Connection::send(std::string&& data)
{
    auto send_task = [this](std::string_view data) {
        if (impl_->writeBuffer_.writeSocket(impl_->fd_, data) >= 0)
        {
            if (impl_->writeBuffer_.size() > 0)
            {
                // 缓冲区有数据，开始监听写事件
                impl_->channel_->enableWrite();
            };
        }
        else
        {
            stop();
        }
    };
    if (impl_->channel_->inThread())
    {
        send_task(data);
    }
    else
    {
        impl_->channel_->addTask(
            [connection = shared_from_this(), task = std::move(send_task), data = std::string(data)]() { task(data); });
    }
}

void Connection::stop()
{
    impl_->channel_->disableAll();
}

void Connection::doTask(Task&& task, double deley, double interval)
{
    impl_->channel_->runTask(std::move(task), deley, interval);
}
void Connection::setContext(std::any context)
{
    impl_->context_ = std::move(context);
}
std::any& Connection::getContext()
{
    return impl_->context_;
}
} // namespace tcp
