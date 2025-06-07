
#pragma once

#include "epoller.h"
#include "tcp/acceptor.h"
#include "tcp/connection.h"
#include "tcp/connector.h"

#include <cassert>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <sys/eventfd.h>
#include <thread>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>
namespace tcp
{

class TaskRunner
{
    using Task = std::function<void()>;

  public:
    TaskRunner();
    ~TaskRunner();

    void start();

    // 以下接口线程安全
    void runTask(Task&& task, double delay = 0.0, double interval = 0.0);
    void addChannel(std::unique_ptr<Channel> channel);
    void removeChannel(int fd);
    Channel* getChannel(int fd);
    // 遵循先后原则，先添加的任务先执行
    void addTask(Task&& task, double delay = 0.0, double interval = 0.0);

    bool inOwnThread() const;

  private:
    enum HandleState
    {
        HandlingEvents,
        CallingTasks,
        Waiting
    };
    class Waker;
    void wakeup() const;
    Epoller epoller_;
    std::vector<Task> tasks_;
    int waker_;

    std::mutex mutex_;
    HandleState handleState_;
    std::thread::id threadId_;
    static constexpr std::size_t kInitialTaskLength = 10;
};
} // namespace tcp
