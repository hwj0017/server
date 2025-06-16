#pragma once
#include "utils/task.h"
#include <coroutine>
#include <mutex>
#include <thread>
#include <vector>
namespace tcp
{
class IoContext;
class Waker
{
  public:
    Waker(IoContext* io_context);
    ~Waker();
    void start();
    void onRead();
    void addTask(utils::TaskBase&& task)
    {
        {
            std::lock_guard<std::mutex> guard(tasks_mutex_);
            tasks_.emplace_back(std::move(task));
        }
        wakeup();
    }
    bool isInThread() { return thread_id_ == std::this_thread::get_id(); }

  private:
    void wakeup();
    void clean();
    int fd_;
    IoContext* io_context_;
    std::thread::id thread_id_;
    std::vector<utils::TaskBase> tasks_;
    std::mutex tasks_mutex_;
};
} // namespace tcp