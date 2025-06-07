#pragma once
#include "tcp/inetaddress.h"
#include <any>
#include <cstddef>
#include <functional>
#include <memory>
#include <string_view>

namespace tcp
{
class Channel;
class Connection : public std::enable_shared_from_this<Connection>
{
  public:
    using Task = std::function<void()>;
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
    explicit Connection(int clientfd, Channel*, const InetAddress& peerAddr, const Tasks& tasks);
    ~Connection();
    // 线程安全
    void start();
    void stop();
    void send(std::string_view data);
    void send(std::string&& data);

    void doTask(Task&& task, double deley = 0.0, double interval = 0.0);
    void setContext(std::any context);
    std::any& getContext();

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace tcp