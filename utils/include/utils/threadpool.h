#pragma once

#include <cstddef>
#include <functional>
#include <memory>

namespace utils
{
class ThreadPool
{
  public:
    using Task = std::function<void()>;
    void addTask(const Task& task);
    static constexpr size_t kInitialThreadNum = 8;
    void stop();
    static ThreadPool& instance()
    {
        static ThreadPool instance;
        return instance;
    }
    ~ThreadPool();

  private:
    ThreadPool(size_t threadNum = kInitialThreadNum);
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace utils
