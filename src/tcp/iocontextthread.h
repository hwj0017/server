#pragma once
#include "iocontext.h"
#include <condition_variable>
#include <mutex>
#include <thread>
namespace tcp
{
class IoContext;
class IoContextThread
{
  public:
    IoContextThread() = default;
    auto start()
    {
        thread_ = std::thread([this] { this->thread_func(); });
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this] { return io_context_ != nullptr; });
        }
        return std::tuple{thread_.get_id(), io_context_};
    }

  private:
    void thread_func()
    {
        IoContext io_context;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            io_context_ = &io_context;
            cv_.notify_one();
        }
        io_context.run();
        {
            std::unique_lock<std::mutex> lock(mutex_);
            io_context_ = nullptr;
        }
    }
    IoContext* io_context_ = nullptr;
    std::thread thread_{};
    std::mutex mutex_;
    std::condition_variable cv_;
};

} // namespace tcp