#include "tcp/connector.h"
#include "buffer.h"
#include "socket.h"
#include "taskrunner.h"
#include "utils/log.h"
#include <memory>
namespace tcp
{
struct Connector::Impl
{
    Connector* self_;
    int fd_;
    TaskRunner* taskRunner_;
    Tasks tasks_;
    InetAddress peerAddr_;
    Buffer readBuffer_;
    Buffer writeBuffer_;
    Impl(Connector* self, int fd, TaskRunner* taskRunner, const InetAddress& serverAddr, const Tasks& tasks)
        : self_(self), fd_(fd), taskRunner_(taskRunner), tasks_(tasks), peerAddr_(serverAddr), readBuffer_(),
          writeBuffer_()
    {
    }
    ~Impl()
    {
        ::close(fd_);
    }

    // 以下接口只在其所属线程中使用
    void stop();
    void start();
    void send(const void* data, std::size_t len);
};
Connector::Connector(TaskRunner* taskRunner, const InetAddress& serverAddr, const Tasks& tasks)
{
    impl_ = std::make_unique<Impl>(this, socket::createConnectorSocket(serverAddr), taskRunner, serverAddr, tasks);
}

Connector::~Connector() = default;

void Connector::start()
{
    impl_->taskRunner_->addTask([connector = shared_from_this()]() mutable {
        auto& impl = connector->impl_;
        auto& taskRunner = impl->taskRunner_;
        // 注册读事件监听
        taskRunner->addObject(impl->fd_, TaskRunner::IoType::kRead, [connector = connector.get()]() {
            auto& impl = connector->impl_;
            if (impl->readBuffer_.readSocket(impl->fd_) >= 0)
            {
                auto& task = impl->tasks_.messageTask;
                if (task)
                    task(connector, impl->readBuffer_.begin(), impl->readBuffer_.size());
                impl->readBuffer_.clear();
            }
            else
            {
                connector->stop();
            }
        });
        // 执行startTask
        auto& task = impl->tasks_.startTask;
        if (task)
            task(connector.get());
        // 将自己托管给TaskRunner
        taskRunner->addObject(impl->fd_, std::move(connector));
    });
}

void Connector::send(const std::string& data)
{
    return send(data.data(), data.size());
}
void Connector::send(const void* data, std::size_t len)
{
    return send(std::string(static_cast<const char*>(data), len));
    {
        impl_->taskRunner_->addTask([connector = this, data = std::string(static_cast<const char*>(data), len)]() {
            connector->impl_->send(data.data(), data.size());
        });
    }
}

void Connector::send(std::string&& data)
{
    impl_->taskRunner_->addTask([this, data = std::move(data)]() {
        if (impl_->writeBuffer_.writeSocket(impl_->fd_, data) >= 0)
        {
            if (impl_->writeBuffer_.size() > 0)
            {
                impl_->taskRunner_->addIoTask(impl_->fd_, TaskRunner::IoType::kWrite, [connector = ]() {
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
                        impl_->stop();
                    }
                });
            }
        }
        else
        {
            stop();
        }
    })
}
void Connector::Impl::start()
{
    taskRunner_->addIoTask(fd_, TaskRunner::IoType::kRead, [connector = self_->shared_from_this()]() {
        // 以下执行一定在TaskRunner线程中
    });
}
void Connector::Impl::stop()
{
    taskRunner_->removeIoTask(fd_, TaskRunner::IoType::kBoth);
}

void Connector::Impl::send(const void* data, std::size_t len)
{
}
} // namespace tcp