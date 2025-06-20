#pragma once

#include "utils/task.h"
#include <cassert>
#include <cstddef>
#include <optional>
#include <queue>
namespace utils
{
// not thread safe
class ChannelBase;
template <typename T> class Channel;

class ChannelBase
{
  public:
    ChannelBase(size_t max_size, size_t init_size = 0) : empty_size_(max_size), full_size_(init_size) {}
    bool is_empty() { return full_size_ <= 0; }

    bool is_full() { return empty_size_ <= 0; }
    bool is_close() const { return is_close_; }

    struct Empty
    {
        ChannelBase& channel_;
        Empty(ChannelBase& channel) : channel_(channel) {}
        bool await_ready() { return channel_.is_close() || channel_.is_empty(); }
        template <typename R> void await_suspend(coroutine_handle<promise_type<R>> coro)
        {
            channel_.tasks_3_.push(coro);
        }
        bool await_resume() { return channel_.is_close(); }
    };
    struct Full
    {
        ChannelBase& channel_;
        Full(ChannelBase& channel) : channel_(channel) {}
        bool await_ready() { return channel_.is_close() || channel_.is_full(); }
        template <typename R> void await_suspend(coroutine_handle<promise_type<R>> coro)
        {
            channel_.tasks_3_.push(coro);
        }
        bool await_resume() { return channel_.is_close(); }
    };
    struct NotEmpty
    {
        ChannelBase& channel_;
        NotEmpty(ChannelBase& channel) : channel_(channel) {}
        bool await_ready() { return channel_.is_close() || !channel_.is_empty(); }
        template <typename R> void await_suspend(coroutine_handle<promise_type<R>> coro)
        {
            channel_.tasks_2_.push(coro);
        }
        bool await_resume() { return channel_.is_close(); }
    };
    struct NotFull
    {
        ChannelBase& channel_;
        NotFull(ChannelBase& channel) : channel_(channel) {}
        bool await_ready() { return channel_.is_close() || !channel_.is_full(); }
        template <typename R> void await_suspend(coroutine_handle<promise_type<R>> coro)
        {
            channel_.tasks_2_.push(coro);
        }
        bool await_resume() { return channel_.is_close(); }
    };
    void close()
    {
        is_close_ = true;
        while (!tasks_1_.empty())
        {
            auto task = std::move(tasks_1_.front());
            tasks_1_.pop();
            task.resume();
        }
        while (!tasks_2_.empty())
        {
            auto task = std::move(tasks_2_.front());
            tasks_2_.pop();
            task.resume();
        }
        while (!tasks_3_.empty())
        {
            auto task = std::move(tasks_3_.front());
            tasks_3_.pop();
            task.resume();
        }
    }

    auto empty() -> Empty { return {*this}; }
    auto full() -> Full { return {*this}; }
    auto not_empty() -> NotEmpty { return {*this}; }

  protected:
    int full_size_;
    int empty_size_;
    bool is_close_ = false;
    // await pop or push
    std::queue<TaskBase> tasks_1_;
    // await not empty or not full
    std::queue<TaskBase> tasks_2_;
    // await empty or full
    std::queue<TaskBase> tasks_3_;

    friend struct Empty;
    friend struct Full;
    friend struct NotEmpty;
    friend struct NotFull;
};

template <typename T> class Channel : public ChannelBase
{
  public:
    Channel(size_t max_size) : ChannelBase(max_size, 0) {}
    struct Pop
    {
        Channel<T>& channel_;
        Pop(Channel<T>& channel) : channel_(channel) {}
        bool await_ready()
        {
            auto ret = channel_.is_close() || !channel_.is_empty();
            ++channel_.empty_size_;
            --channel_.full_size_;
            return ret;
        }
        template <typename R> void await_suspend(coroutine_handle<promise_type<R>> coro)
        {
            channel_.tasks_1_.push({coro});
        }
        auto await_resume() -> std::optional<T>;
    };
    struct Push
    {
        Channel<T>& channel_;
        T value_;
        Push(Channel<T>& channel, T&& value) : channel_(channel), value_(std::move(value)) {}
        bool await_ready()
        {
            auto ret = channel_.is_close() || !channel_.is_full();
            --channel_.empty_size_;
            ++channel_.full_size_;
            return ret;
        }
        template <typename R> void await_suspend(coroutine_handle<promise_type<R>> coro)
        {
            channel_.tasks_1_.push({coro});
        }
        bool await_resume();
    };
    void reset()
    {
        close();
        resources_ = {};
        is_close_ = false;
    }

  private:
    std::queue<T> resources_;

    friend struct Pop;
    friend struct Push;
};
template <> class Channel<void> : public ChannelBase
{
  public:
    Channel(size_t max_size, size_t init_size = 0) : ChannelBase(max_size, init_size) {}
    struct Pop
    {
        Channel& channel_;
        Pop(Channel& channel) : channel_(channel) {}
        bool await_ready()
        {
            auto ret = channel_.is_close() || !channel_.is_empty();
            ++channel_.empty_size_;
            --channel_.full_size_;
            return ret;
        }
        template <typename R> void await_suspend(coroutine_handle<promise_type<R>> coro)
        {
            channel_.tasks_1_.push({coro});
        }
        bool await_resume();
    };
    struct Push
    {
        Channel& channel_;
        Push(Channel& channel) : channel_(channel) {}
        bool await_ready()
        {
            auto ret = channel_.is_close() || !channel_.is_full();
            --channel_.empty_size_;
            ++channel_.full_size_;
            return ret;
        }
        template <typename R> void await_suspend(coroutine_handle<promise_type<R>> coro)
        {
            channel_.tasks_1_.push({coro});
        }
        bool await_resume();
    };
    void reset()
    {
        close();
        is_close_ = false;
    }
    auto push() -> Push { return Push{*this}; }
    auto pop() -> Pop { return Pop{*this}; }
    friend struct Pop;
    friend struct Push;
};
template <typename T> auto Channel<T>::Pop::await_resume() -> std::optional<T>
{
    if (channel_.is_close_)
    {
        return std::nullopt;
    }
    auto res = std::move(channel_.resources_.front());
    channel_.resources_.pop();
    // resume Push
    if (channel_.empty_size_ <= 0 && !channel_.tasks_1_.empty())
    {
        auto task = std::move(channel_.tasks_1_.front());
        channel_.tasks_1_.pop();
        task.resume();
    }
    // resume NotFull
    if (!channel_.is_full() && !channel_.tasks_2_.empty())
    {
        auto task = std::move(channel_.tasks_2_.front());
        channel_.tasks_2_.pop();
        task.resume();
    }
    // resume Empty
    if (channel_.is_empty() && !channel_.tasks_3_.empty())
    {
        auto task = std::move(channel_.tasks_3_.front());
        channel_.tasks_3_.pop();
        task.resume();
    }
    return {res};
}

template <typename T> auto Channel<T>::Push::await_resume() -> bool
{
    if (channel_.is_close_)
    {
        return false;
    }
    channel_.resources_.push(std::move(value_));
    // resume Pop
    if (channel_.full_size_ <= 0 && !channel_.tasks_1_.empty())
    {
        auto task = std::move(channel_.tasks_1_.front());
        channel_.tasks_1_.pop();
        task.resume();
    }
    // resume NotEmpty
    if (!channel_.is_empty() && !channel_.tasks_2_.empty())
    {
        auto task = std::move(channel_.tasks_2_.front());
        channel_.tasks_2_.pop();
        task.resume();
    }
    // resume Full
    if (channel_.is_full() && !channel_.tasks_3_.empty())
    {
        auto task = std::move(channel_.tasks_3_.front());
        channel_.tasks_3_.pop();
        task.resume();
    }
    return true;
}

inline auto Channel<void>::Pop::await_resume() -> bool
{
    --channel_.empty_size_;
    ++channel_.full_size_;
    if (channel_.is_close_)
    {
        return false;
    }
    // resume Push
    if (channel_.empty_size_ <= 0 && !channel_.tasks_1_.empty())
    {
        auto task = std::move(channel_.tasks_1_.front());
        channel_.tasks_1_.pop();
        task.resume();
    }
    // resume NotFull
    if (!channel_.is_full() && !channel_.tasks_2_.empty())
    {
        auto task = std::move(channel_.tasks_2_.front());
        channel_.tasks_2_.pop();
        task.resume();
    }
    // resume Empty
    if (channel_.is_empty() && !channel_.tasks_3_.empty())
    {
        auto task = std::move(channel_.tasks_3_.front());
        channel_.tasks_3_.pop();
        task.resume();
    }
    return true;
}

inline auto Channel<void>::Push::await_resume() -> bool
{
    ++channel_.empty_size_;
    --channel_.full_size_;
    if (channel_.is_close_)
    {
        return false;
    }
    // resume Pop
    if (channel_.full_size_ <= 0 && !channel_.tasks_1_.empty())
    {
        auto task = std::move(channel_.tasks_1_.front());
        channel_.tasks_1_.pop();
        task.resume();
    }
    // resume NotEmpty
    if (!channel_.is_empty() && !channel_.tasks_2_.empty())
    {
        auto task = std::move(channel_.tasks_2_.front());
        channel_.tasks_2_.pop();
        task.resume();
    }
    // resume Full
    if (channel_.is_full() && !channel_.tasks_3_.empty())
    {
        auto task = std::move(channel_.tasks_3_.front());
        channel_.tasks_3_.pop();
        task.resume();
    }
    return true;
}

} // namespace utils