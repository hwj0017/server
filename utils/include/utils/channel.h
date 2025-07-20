#pragma once

#include "basetask.h"
#include <cassert>
#include <coroutine>
#include <cstddef>
#include <memory>
#include <optional>
#include <queue>

namespace utils
{
// not thread safe
// class ChannelBase;
// TODO
class ChannelBase
{
  public:
    ChannelBase(size_t max_size, size_t init_size = 0) : empty_size_(max_size), full_size_(init_size) {}
    ~ChannelBase() { close(); }
    // get channel state
    bool is_empty() const { return full_size_ <= 0; }
    bool is_full() const { return empty_size_ <= 0; }
    bool is_closed() const { return is_closed_; }
    // close the channel
    void close()
    {
        if (is_closed_)
        {
            return;
        }
        is_closed_ = true;
        while (!res_tasks_.empty())
        {
            auto task = std::move(res_tasks_.front());
            res_tasks_.pop();
            task.resume();
        }
        while (!empty_tasks_.empty())
        {
            auto task = std::move(empty_tasks_.front());
            empty_tasks_.pop();
            task.resume();
        }
        while (!full_tasks_.empty())
        {
            auto task = std::move(full_tasks_.front());
            full_tasks_.pop();
            task.resume();
        }
    }

    struct Empty
    {
        ChannelBase& channel_;
        Empty(ChannelBase& channel) : channel_(channel) {}
        bool await_ready() { return channel_.is_closed() || channel_.is_empty(); }
        template <typename promise_type> void await_suspend(std::coroutine_handle<promise_type> coro)
        {
            channel_.empty_tasks_.push({coro});
        }
        bool await_resume() { return channel_.is_empty(); }
    };
    struct NotEmpty
    {
        ChannelBase& channel_;
        NotEmpty(ChannelBase& channel) : channel_(channel) {}
        bool await_ready() { return channel_.is_closed() || !channel_.is_empty(); }
        template <typename promise_type> void await_suspend(std::coroutine_handle<promise_type> coro)
        {
            channel_.empty_tasks_.push({coro});
        }
        bool await_resume() { return !channel_.is_empty(); }
    };
    struct Full
    {
        ChannelBase& channel_;
        Full(ChannelBase& channel) : channel_(channel) {}
        bool await_ready() { return channel_.is_closed() || channel_.is_full(); }
        template <typename promise_type> void await_suspend(std::coroutine_handle<promise_type> coro)
        {
            channel_.full_tasks_.push({coro});
        }
        bool await_resume() { return channel_.is_full(); }
    };

    struct NotFull
    {
        ChannelBase& channel_;
        NotFull(ChannelBase& channel) : channel_(channel) {}
        bool await_ready() { return channel_.is_closed() || !channel_.is_full(); }
        template <typename promise_type> void await_suspend(std::coroutine_handle<promise_type> coro)
        {
            channel_.full_tasks_.push({coro});
        }
        bool await_resume() { return !channel_.is_full(); }
    };

    // await empty, full, not_empty, not_full, closed, pushable, popable
    auto empty() -> Empty { return {*this}; }
    auto full() -> Full { return {*this}; }
    auto not_empty() -> NotEmpty { return {*this}; }
    auto not_full() -> NotFull { return {*this}; }

  public:
    int full_size_;
    int empty_size_;
    bool is_closed_ = false;
    // await pop or push
    std::queue<BaseTask> res_tasks_;
    // await empty or not empty
    std::queue<BaseTask> empty_tasks_;
    // await full or not full
    std::queue<BaseTask> full_tasks_;
};

// channel<T> is a bounded channel that can hold T type elements.
template <typename T = void> class Channel : public ChannelBase
{
  public:
    Channel(size_t max_size) : ChannelBase(max_size, 0) {}
    // TODO
    struct AsyncPop
    {
        Channel<T>& channel_;
        AsyncPop(Channel<T>& channel) : channel_(channel) {}
        bool await_ready() { return await_ready(channel_); }
        template <typename promiss_type> void await_suspend(std::coroutine_handle<promiss_type> coro)
        {
            channel_.res_tasks_.push({coro});
        }
        auto await_resume() -> std::optional<T> { return await_resume(channel_); }
        static bool await_ready(Channel<T>& channel)
        {
            // increment empty size and decrement full size
            ++channel.empty_size_;
            --channel.full_size_;
            if (!channel.is_closed())
            { // resume Push
                if (channel.is_full() && !channel.res_tasks_.empty())
                {
                    auto task = std::move(channel.res_tasks_.front());
                    channel.res_tasks_.pop();
                    task.resume();
                }
                while (channel.is_empty() && !channel.empty_tasks_.empty())
                {
                    auto task = std::move(channel.empty_tasks_.front());
                    channel.empty_tasks_.pop();
                    task.resume();
                }
                while (!channel.is_full() && !channel.full_tasks_.empty())
                {
                    auto task = std::move(channel.full_tasks_.front());
                    channel.full_tasks_.pop();
                    task.resume();
                }
            }

            return channel.is_closed() || channel.full_size_ >= 0;
        }
        static auto await_resume(Channel<T>& channel) -> std::optional<T>
        {
            if (channel.full_size_ < 0)
            {
                --channel.empty_size_;
                ++channel.full_size_;
                return {};
            }
            auto res = std::move(channel.resources_.front());
            channel.resources_.pop();
            return res;
        }
    };
    struct AsyncPush
    {
        Channel<T>& channel_;
        T value_;
        AsyncPush(Channel<T>& channel, T&& value) : channel_(channel), value_(std::move(value)) {}

        bool await_ready() { return await_ready(channel_, std::move(value_)); }

        template <typename promise_type> void await_suspend(std::coroutine_handle<promise_type> coro)
        {
            channel_.res_tasks_.push({coro});
        }
        bool await_resume() { return await_resume(channel_); }
        static bool await_ready(Channel<T>& channel, T&& value)
        {
            --channel.empty_size_;
            ++channel.full_size_;
            channel.resources_.push(std::move(value));
            if (!channel.is_closed())
            { // resume Pop
                if (channel.is_empty() && !channel.res_tasks_.empty())
                {
                    auto task = std::move(channel.res_tasks_.front());
                    channel.res_tasks_.pop();
                    task.resume();
                }
                while (channel.is_full() && !channel.full_tasks_.empty())
                {
                    auto task = std::move(channel.full_tasks_.front());
                    channel.full_tasks_.pop();
                    task.resume();
                }
                while (!channel.is_empty() && !channel.empty_tasks_.empty())
                {
                    auto task = std::move(channel.empty_tasks_.front());
                    channel.empty_tasks_.pop();
                    task.resume();
                }
            }
            return channel.is_closed() || channel.empty_size_ >= 0;
        }
        static bool await_resume(Channel<T>& channel)
        {
            if (channel.empty_size_ < 0)
            {
                ++channel.empty_size_;
                --channel.full_size_;
                return false;
            }
            return true;
        }
    };
    auto pop() -> std::optional<T>
    {
        AsyncPop::await_ready(*this);
        return AsyncPop::await_resume(*this);
    }

    bool push(T value)
    {
        AsyncPush::await_ready(*this, std::move(value));
        return AsyncPush::await_resume(*this);
    }

    auto async_pop() -> AsyncPop { return AsyncPop{*this}; }
    auto async_push(T value) -> AsyncPush { return AsyncPush{*this, std::move(value)}; }

  private:
    std::queue<T> resources_;
    friend struct AsyncPop;
    friend struct AsyncPush;
};
template <> class Channel<void> : public ChannelBase
{
  public:
    Channel(size_t max_size, size_t init_size = 0) : ChannelBase(max_size, init_size) {}
    struct AsyncPop
    {
        Channel<>& channel_;
        AsyncPop(Channel<>& channel) : channel_(channel) {}
        bool await_ready() { return await_ready(channel_); }
        template <typename promiss_type> void await_suspend(std::coroutine_handle<promiss_type> coro)
        {
            channel_.res_tasks_.push({coro});
        }
        auto await_resume() -> bool { return await_resume(channel_); }
        static bool await_ready(Channel<>& channel)
        {
            // increment empty size and decrement full size
            ++channel.empty_size_;
            --channel.full_size_;
            if (!channel.is_closed())
            { // resume Push
                if (channel.is_full() && !channel.res_tasks_.empty())
                {
                    auto task = std::move(channel.res_tasks_.front());
                    channel.res_tasks_.pop();
                    task.resume();
                }
                while (channel.is_empty() && !channel.empty_tasks_.empty())
                {
                    auto task = std::move(channel.empty_tasks_.front());
                    channel.empty_tasks_.pop();
                    task.resume();
                }
                while (!channel.is_full() && !channel.full_tasks_.empty())
                {
                    auto task = std::move(channel.full_tasks_.front());
                    channel.full_tasks_.pop();
                    task.resume();
                }
            }

            return channel.is_closed() || channel.full_size_ >= 0;
        }
        static auto await_resume(Channel<>& channel) -> bool
        {
            if (channel.full_size_ < 0)
            {
                --channel.empty_size_;
                ++channel.full_size_;
                return false;
            }
            return true;
        }
    };
    struct AsyncPush
    {
        Channel<>& channel_;
        AsyncPush(Channel<>& channel) : channel_(channel) {}

        bool await_ready() { return await_ready(channel_); }

        template <typename promise_type> void await_suspend(std::coroutine_handle<promise_type> coro)
        {
            channel_.res_tasks_.push({coro});
        }
        bool await_resume() { return await_resume(channel_); }
        static bool await_ready(Channel<>& channel)
        {
            --channel.empty_size_;
            ++channel.full_size_;
            if (!channel.is_closed())
            { // resume Pop
                if (channel.is_empty() && !channel.res_tasks_.empty())
                {
                    auto task = std::move(channel.res_tasks_.front());
                    channel.res_tasks_.pop();
                    task.resume();
                }
                while (channel.is_full() && !channel.full_tasks_.empty())
                {
                    auto task = std::move(channel.full_tasks_.front());
                    channel.full_tasks_.pop();
                    task.resume();
                }
                while (!channel.is_empty() && !channel.empty_tasks_.empty())
                {
                    auto task = std::move(channel.empty_tasks_.front());
                    channel.empty_tasks_.pop();
                    task.resume();
                }
            }
            return channel.is_closed() || channel.empty_size_ >= 0;
        }
        static bool await_resume(Channel<>& channel)
        {
            if (channel.empty_size_ < 0)
            {
                ++channel.empty_size_;
                --channel.full_size_;
                return false;
            }
            return true;
        }
    };
    auto pop() -> bool
    {
        AsyncPop::await_ready(*this);
        return AsyncPop::await_resume(*this);
    }

    bool push()
    {
        AsyncPush::await_ready(*this);
        return AsyncPush::await_resume(*this);
    }
    auto async_pop() -> AsyncPop { return AsyncPop{*this}; }
    auto async_push() -> AsyncPush { return AsyncPush{*this}; }
    friend struct AsyncPop;
    friend struct AsyncPush;
};
} // namespace utils
