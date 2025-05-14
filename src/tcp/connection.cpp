#include "tcp/connection.h"
#include "buffer.h"
#include "taskrunner.h"
#include "tcp/inetaddress.h"
#include <cassert>
#include <cerrno>
#include <cstddef>
#include <fcntl.h>
#include <memory>
#include <string>
#include <sys/types.h>
#include <unistd.h>
namespace tcp
{
struct Connection::Impl
{
    int fd_;
    TaskRunner* taskRunner_;
    Tasks tasks_;
    InetAddress peerAddr_;
    Buffer readBuffer_;
    Buffer writeBuffer_;
    Impl(int fd, TaskRunner* taskRunner, const InetAddress& peerAddr, const Tasks& tasks)
        : fd_(fd), taskRunner_(taskRunner), tasks_(tasks), peerAddr_(peerAddr)
    {
    }
    ~Impl()
    {
        ::close(fd_);
    }
};
Connection::Connection(int clientfd, TaskRunner* taskRunner, const InetAddress& peerAddr, const Tasks& tasks)
{
    impl_ = std::make_unique<Impl>(clientfd, taskRunner, peerAddr, tasks);
}

Connection::~Connection() = default;

void Connection::start()
{
    impl_->taskRunner_->runTask([this]() {
        impl_->taskRunner_->addObject(impl_->fd_, shared_from_this());
        impl_->taskRunner_->addIoTask(impl_->fd_, TaskRunner::IoType::kRead, [this]() {
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
        });
        if (impl_->tasks_.startTask)
            impl_->tasks_.startTask(this);
    });
}
void Connection::send(const std::string& data)
{
    return send(data.data(), data.size());
}

void Connection::send(const void* data, std::size_t len)
{
    return send(std::string(static_cast<const char*>(data), len));
}

void Connection::send(std::string&& data)
{
    impl_->taskRunner_->runTask([this, data = std::move(data)]() {
        if (impl_->writeBuffer_.writeSocket(impl_->fd_, data) >= 0)
        {
            if (impl_->writeBuffer_.size() > 0)
            {
                // 缓冲区未清空，继续监听写事件
                impl_->taskRunner_->addIoTask(impl_->fd_, TaskRunner::IoType::kWrite, [this]() {
                    if (impl_->writeBuffer_.writeSocket(impl_->fd_) >= 0)
                    {
                        // 发送缓冲区已清空，停止写事件监听
                        if (impl_->writeBuffer_.size() <= 0)
                        {
                            impl_->taskRunner_->removeIoTask(impl_->fd_, TaskRunner::IoType::kWrite);
                            impl_->writeBuffer_.clear();
                        }
                    }
                    else
                    {
                        stop();
                    }
                });
            }
        }
        else
        {
            stop();
        }
    });
}
} // namespace tcp
