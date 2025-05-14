
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
#include <variant>
#include <vector>
namespace tcp
{
class TaskRunner
{
    using Task = std::function<void()>;
    using object = std::variant<std::shared_ptr<Acceptor>, std::shared_ptr<Connection>, std::shared_ptr<Connector>>;
    template <typename T> using IoTask = std::function<void(T*)>;

  public:
    enum class IoType
    {
        kRead,
        kWrite,
        kBoth
    };
    TaskRunner();
    ~TaskRunner();
    // 以下接口线程安全
    void start();
    void stop();
    template <typename T>
    void addIoTask(int fd, std::shared_ptr<T>&& object, IoType startType, IoTask<T>&& readTask = nullptr,
                   IoTask<T>&& writeTask = nullptr)
    {
        objects_.emplace(fd, object);
        if (readTask)
            readTasks_.emplace(fd, readTask);
        if (writeTask)
            writeTasks_.emplace(fd, writeTask);
    }
    void removeIoTask(int fd);
    void updateIoTask(int fd, IoType type);
    template <typename T> void runTask(T&& task, double delay = 0.0, double interval = 0.0)
    {
        if (inOwnThread())
            task();
        else
            addTask(std::forward<T>(task), delay, interval);
    }
    // 遵循先后原则，先添加的任务先执行
    template <typename T> void addTask(T&& task, double delay = 0.0, double interval = 0.0)
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            tasks_.emplace_back(std::forward<T>(task));
        }
        // 无论是不是当前线程，只要不是在处理事件都唤醒
        // if (handleState_ != HandlingEvents)
        //     waker.wakeup();
    }
    bool inOwnThread() const;

  private:
    enum class State
    {
        kStarted,
        kStopped
    };
    enum HandleState
    {
        HandlingEvents,
        CallingTasks,
        Waiting
    };

    // class Waker
    // {
    //   public:
    //     Waker(IoContext* ioContext);
    //     void onRead();
    //     void wakeup();

    //   private:
    //     int fd_;
    //     IoContext* ioContext_;
    // };

    Epoller epoller_;
    std::unordered_map<int, object> objects_;
    std::unordered_map<int, Task> readTasks_;
    std::unordered_map<int, Task> writeTasks_;
    std::vector<Task> tasks_;
    // Waker waker;
    std::mutex mutex_;
    HandleState handleState_;
    State state_;
    std::thread::id threadId_;
    static constexpr std::size_t kInitialTaskLength = 10;
};
} // namespace tcp
