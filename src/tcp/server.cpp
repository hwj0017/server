#include "tcp/server.h"
#include "baseobjectpool.h"
#include "iocontext.h"
#include "iocontextpool.h"
#include "tcp/baseobject.h"
#include <atomic>
#include <cassert>
#include <cstddef>
#include <memory>
#include <unordered_map>
namespace tcp
{
using size_t = std::size_t;
struct Server::Impl
{
    enum class State
    {
        kStarted,
        kStopped
    };
    std::size_t ioContextNum_;
    IoContext mainIoContext_;
    std::vector<IoContextThread> ioContexts_;
    // 对应线程id到IoContext的映射
    std::unordered_map<size_t, IoContext*> indexMap_;
    std::unordered_map<size_t, std::unique_ptr<Acceptor>> acceptors_;
    std::unordered_map<size_t, std::unique_ptr<Connection>> connections_;
    std::unordered_map<size_t, std::unique_ptr<Connector>> connectors_;
    std::unordered_map<size_t, IoContext*> attachedIoContexts_;
    std::mutex mutex_;
    State state_;
    static std::atomic<size_t> nextId_;
    static std::atomic<size_t> nextIoContextId_;

    void start();
    void stop();
    size_t newAcceptor(const InetAddress& listenAddr, const Acceptor::Tasks& tasks);
    size_t newConnection(int clientfd, const InetAddress& peerAddr, const Connection::Tasks& tasks);
    size_t newConnector(const InetAddress& serverAddr, const Connector::Tasks& tasks);
    template <typename T> void doTask(std::size_t id, const std::function<void(TempPtr<T>)>& task);
};
Server::Server() : impl_(std::make_unique<Impl>())
{
}
Server::~Server() = default;
void Server::start()
{
    impl_->start();
}

void Server::stop()
{
    impl_->stop();
}
// 加入一个Acceptor
std::size_t Server::newAcceptor(const InetAddress& listenAddr, const Acceptor::Tasks& tasks)
{
    return impl_->newAcceptor(listenAddr, tasks);
}
std::size_t Server::newConnection(int clientfd, const InetAddress& peerAddr, const Connection::Tasks& tasks)
{
    return impl_->newConnection(clientfd, peerAddr, tasks);
}
// 加入一个Connector
std::size_t Server::newConnector(const InetAddress& serverAddr, const Connector::Tasks& tasks)
{
    return impl_->newConnector(serverAddr, tasks);
}
// 线程安全，对id进行操作
template <typename T> void Server::doTask(std::size_t id, const std::function<void(TempPtr<T>)>& task)
{
    impl_->doTask(id, task);
}

void Server::Impl::start()
{
    state_ = State::kStarted;
    for (auto& ioContextThread : ioContexts_)
    {
        // 此处需保证ioContext的生命周期
        ioContextThread.start();
        indexMap_[std::hash<std::thread::id>()(ioContextThread.getThreadId())] = ioContextThread.getIoContext();
    }
    indexMap_[std::hash<std::thread::id>()(std::this_thread::get_id())] = &mainIoContext_;
    mainIoContext_.start();
}

void Server::Impl::stop()
{
    for (auto& ioContextThread : ioContexts_)
    {
        ioContextThread.stop();
    }
    mainIoContext_.stop();
}

size_t Server::Impl::newAcceptor(const InetAddress& listenAddr, const Acceptor::Tasks& tasks)
{
    size_t id = nextId_;
    // releaseTask由调用者负责线程安全
    auto releaseTask = [this](size_t id) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = acceptors_.find(id);
        assert(it != acceptors_.end());
        acceptors_.erase(it);
    };
    {
        std::lock_guard<std::mutex> lock(mutex_);
        acceptors_.emplace(id, std::make_unique<Acceptor>(&mainIoContext_, id, listenAddr, tasks, releaseTask));
    }
    // 用下标符会报错
    auto it = acceptors_.find(id);
    auto acceptor = it->second.get();
    mainIoContext_.runTask([acceptor]() { acceptor->start(); });
    nextId_++;
    return id;
}

size_t Server::Impl::newConnection(int clientfd, const InetAddress& peerAddr, const Connection::Tasks& tasks)
{
    size_t id = nextId_;
    // releaseTask由调用者负责线程安全
    auto releaseTask = [this](size_t id) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = connections_.find(id);
        assert(it != connections_.end());
        connections_.erase(it);
    };
    {
        std::lock_guard<std::mutex> lock(mutex_);
        connections_.emplace(id, std::make_unique<Connection>(clientfd, ioContext, id, peerAddr, tasks, releaseTask));
        // connections_[id]
    }
    auto it = connections_.find(id);
    auto connection = it->second.get();
    ioContext->runTask([it]() { it->second->start(); });
    nextId_++;
    return id;
}

size_t Server::Impl::newConnector(const InetAddress& serverAddr, const Connector::Tasks& tasks)
{
}

template <typename T> void Server::Impl::doTask(std::size_t id, const std::function<void(TempPtr<T>)>& task)
{
}
} // namespace tcp