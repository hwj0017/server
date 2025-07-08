#include "utils/threadpool.h"
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace utils
{
struct ThreadPool::Impl
{
    std::size_t threadNum_;
    bool isStopped_;
    std::vector<std::thread> workers_;
    std::queue<Task> tasks_;
    std::mutex queueMutex_;
    std::condition_variable condition_;
    Impl(std::size_t threadNum) : threadNum_(threadNum), isStopped_(false) {}
};
void ThreadPool::addTask(const Task& task)
{
    if (!impl_->isStopped_)
    {
        std::unique_lock<std::mutex> lock(impl_->queueMutex_);
        impl_->tasks_.push(task);
        impl_->condition_.notify_one();
    }
}

ThreadPool::ThreadPool(std::size_t threadNum) : impl_(std::make_unique<Impl>(threadNum))
{
    for (std::size_t i = 0; i < impl_->threadNum_; ++i)
    {
        impl_->workers_.emplace_back([this] {
            while (true)
            {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(this->impl_->queueMutex_);
                    this->impl_->condition_.wait(
                        lock, [this] { return this->impl_->isStopped_ || !this->impl_->tasks_.empty(); });

                    if (this->impl_->isStopped_ && this->impl_->tasks_.empty())
                        return;

                    task = std::move(this->impl_->tasks_.front());
                    this->impl_->tasks_.pop();
                }
                task();
            }
        });
    }
}

ThreadPool::~ThreadPool() = default;

} // namespace utils