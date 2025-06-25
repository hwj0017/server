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
    auto start() -> utils::Task<>;
    void addTask(utils::TaskBase&& task)
    {
        {
            std::lock_guard<std::mutex> guard(tasks_mutex_);
            tasks_.emplace_back(std::move(task));
        }
        if (need_wakeup_)
        {
            wakeup();
        }
    }
    void update() { need_wakeup_ = false; }
    bool isInThread() { return thread_id_ == std::this_thread::get_id(); }

  private:
    void wakeup();
    void clean();
    int fd_;
    IoContext* io_context_;
    // 是否需要唤醒
    bool need_wakeup_ = true;
    std::thread::id thread_id_{};
    std::vector<utils::TaskBase> tasks_{};
    std::mutex tasks_mutex_{};
};
} // namespace tcp