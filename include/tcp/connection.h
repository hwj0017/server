#pragma once
#include "tcp/inetaddress.h"
#include <any>
#include <cstddef>
#include <functional>
#include <memory>

namespace tcp
{
class TaskRunner;
class Connection : public std::enable_shared_from_this<Connection>
{
  public:
    using Task = std::function<void(Connection*)>;
    using StartTask = std::function<void(Connection*)>;
    using StopTask = std::function<void(Connection*)>;
    using MessageTask = std::function<void(Connection*, const void*, std::size_t)>;
    struct Tasks
    {
        StartTask startTask;
        StopTask stopTask;
        MessageTask messageTask;
    };
    // 由server实例化
    explicit Connection(int clientfd, TaskRunner* taskRunner, const InetAddress& peerAddr, const Tasks& tasks);
    ~Connection();
    // 线程安全
    void start();
    void stop();
    void send(const std::string& data);
    void send(std::string&& data);
    void send(const void* data, std::size_t len);
    void doTask(const Task& task, double deley = 0.0, double interval = 0.0);
    void setContext(std::any context);
    std::any& getContext();

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace tcp