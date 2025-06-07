#pragma once

#include "tcp/inetaddress.h"
#include <any>
#include <functional>
#include <memory>
namespace tcp
{
class Channel;
class Acceptor : public std::enable_shared_from_this<Acceptor>
{
  public:
    using Task = std::function<void()>;
    using StartTask = std::function<void(Acceptor*)>;
    using StopTask = std::function<void(Acceptor*)>;
    using AcceptTask = std::function<void(Acceptor* acceptor, int clientfd, const InetAddress& peerAddr)>;

    struct Tasks
    {
        StartTask startTask;
        StopTask stopTask;
        AcceptTask acceptTask;
    };
    explicit Acceptor(Channel* taskRunner, const InetAddress& listenAddr, const Tasks& tasks);
    ~Acceptor();
    void start();
    void stop();
    // 后续考虑需不需要用shared_ptr
    void doTask(const Task& task, double deley = 0.0, double interval = 0.0);
    void setContext(std::any context);
    std::any& getContext();

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace tcp