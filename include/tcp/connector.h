#pragma once
#include "tcp/inetaddress.h"
#include <any>
#include <cstddef>
#include <functional>
#include <memory>

namespace tcp
{
class TaskRunner;
class Connector : public std::enable_shared_from_this<Connector>
{
  public:
    using Task = std::function<void(Connector*)>;
    using StartTask = std::function<void(Connector*)>;
    using StopTask = std::function<void(Connector*)>;
    using MessageTask = std::function<void(Connector*, const void*, std::size_t)>;

    struct Tasks
    {
        StartTask startTask;
        StopTask stopTask;
        MessageTask messageTask;
    };
    explicit Connector(TaskRunner* taskRunner, const InetAddress& serverAddr, const Tasks& tasks);
    ~Connector();
    void start();
    void stop();
    void send(const void* data, std::size_t size);
    void send(const std::string& data);
    void send(std::string&& data);
    void doTask(const Task& task, double deley = 0.0, double interval = 0.0);
    void setContext(std::any context);
    std::any& getContext();

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace tcp