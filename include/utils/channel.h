#pragma once

#include "utils/task.h"
#include <cassert>
#include <coroutine>
#include <cstddef>
#include <optional>
#include <queue>
namespace utils
{

template <typename T = void> class Channel
{
  public:
    // max_size_ = 0 only if T = void
    Channel(size_t max_size) : empty_size_(max_size), full_size_(0) { assert(max_size > 0); }
    auto push(T value) -> Task<bool>
    {
        if (is_close_)
        {
            co_return false;
        }
        --empty_size_;
        ++full_size_;
        // 还有空间
        if (empty_size_ >= 0)
        {
            resource_.push(std::move(value));
            // 有task等待，唤醒一个
            if (full_size_ <= 0)
            {
                tasks_.front().resume();
                tasks_.pop();
            }
            co_return true;
        }
        // 没有空间，等待
        auto self = co_await SelfTask<>();
        tasks_.push(std::move(self.task));
        co_await std::suspend_always{};
        if (is_close_)
        {
            co_return false;
        }
        resource_.push(std::move(value));
        co_return true;
    }
    auto pop() -> Task<std::optional<T>>
    {
        if (is_close_)
        {
            co_return {};
        }
        --full_size_;
        ++empty_size_;
        // 还有资源
        if (full_size_ >= 0)
        {
            auto res = std::move(resource_.front());
            resource_.pop();
            // 有task在等待，唤醒一个
            if (empty_size_ <= 0)
            {
                auto task = std::move(tasks_.front());
                tasks_.pop();
                task.resume();
            }
            co_return {res};
        }
        // 没有资源，等待
        auto self = co_await SelfTask<>();
        tasks_.push(std::move(self));
        co_await std::suspend_always{}; // 暂停
        if (is_close_)
        {
            co_return std::nullopt;
        }
        auto res = std::move(resource_.front());
        resource_.pop();
        co_return {res};
    }
    void close()
    {
        is_close_ = true;
        while (!tasks_.empty())
        {
            auto task = std::move(tasks_.front());
            tasks_.pop();
            task.resume();
        }
    }

  private:
    int full_size_;
    int empty_size_;
    bool is_close_ = false;
    std::queue<T> resource_;
    std::queue<TaskBase> tasks_;
};

template <> class Channel<void>
{
  public:
    // max_size_ = 0 only if T = void
    Channel(size_t max_size) : empty_size_(max_size), full_size_(0) {}
    auto push() -> Task<bool>
    {
        if (is_close_)
        {
            co_return false;
        }
        --empty_size_;
        ++full_size_;
        // 还有空间
        if (empty_size_ >= 0)
        {
            // 有task等待，唤醒一个
            if (full_size_ <= 0)
            {
                tasks_.front().resume();
                tasks_.pop();
            }
            co_return true;
        }
        // 没有空间，等待
        auto self = co_await SelfTask<bool>();
        tasks_.push(std::move(self));
        co_await std::suspend_always{};
        if (is_close_)
        {
            co_return false;
        }
        co_return true;
    }
    auto pop() -> Task<bool>
    {
        if (is_close_)
        {
            co_return false;
        }
        --full_size_;
        ++empty_size_;
        // 还有资源
        if (full_size_ >= 0)
        {
            // 有task在等待，唤醒一个
            if (empty_size_ <= 0)
            {
                tasks_.front().resume();
                tasks_.pop();
            }
            co_return true;
        }
        // 没有资源，等待
        auto self = co_await SelfTask<bool>{};
        tasks_.push(std::move(self));
        co_await std::suspend_always{}; // 暂停
        if (is_close_)
        {
            co_return false;
        }
        co_return true;
    }

  private:
    int full_size_;
    int empty_size_;
    bool is_close_ = false;
    std::queue<TaskBase> tasks_;
};
} // namespace utils