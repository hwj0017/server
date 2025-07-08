#pragma once

#include "basetask.h"
#include "channel.h"
#include <algorithm>
#include <atomic>
#include <cassert>
#include <coroutine>
#include <cstddef>
#include <optional>
#include <type_traits>
#define VALUE_TASK_ERROR                                                                                               \
    co_await std::suspend_always{};                                                                                    \
    co_return {};
#define VOID_TASK_ERROR                                                                                                \
    co_await std::suspend_always{};                                                                                    \
    co_return;

namespace utils
{
using std::coroutine_handle;
using std::suspend_always;
using std::suspend_never;
// can not co_await Task which will not be destroyed
template <typename T> class Task;
template <typename T = void> struct promise_type : base_promise_type
{
    Channel<T> channel_{1};
    void return_value(T value) { assert(channel_.push(std::move(value))); }
    auto get_return_object() -> Task<T>;
};
template <> struct promise_type<void> : base_promise_type
{
    Channel<> channel_{1};
    void return_void() { assert(channel_.push()); }
    auto get_return_object() -> Task<void>;
};

template <typename T = void> class Task : public BaseTask
{
  public:
    using promise_type = utils::promise_type<T>;
    Task() = default;
    Task(coroutine_handle<base_promise_type> handle) : BaseTask(handle) {}
    Task(Task&& other) noexcept : BaseTask(std::move(other)) {};
    auto& operator=(Task&& other) noexcept
    {
        BaseTask::operator=(std::move(other));
        return *this;
    }
    ~Task() = default;
    auto operator co_await() &&
    {
        Channel<T>* channel = &static_cast<promise_type&>(handle_.promise()).channel_;
        // other reference
        if (handle_.promise().task_count_ > 1)
        {
            // delete self
            operator=(Task<T>{});
        }
        else
        {
            if (channel->is_empty())
            {
                channel->close();
            }
        }
        return channel->async_pop();
    }
    auto operator co_await() &
    {
        Channel<T>* channel = &static_cast<promise_type&>(handle_.promise()).channel_;
        return channel->async_pop();
    }
};

// 用来获取自身句柄
template <typename promise_type> struct SelfTask
{
    bool await_ready() noexcept { return false; }
    bool await_suspend(coroutine_handle<promise_type> handle) noexcept
    {
        handle_ = handle;
        return false;
    }
    auto await_resume() noexcept { return handle_; }
    coroutine_handle<promise_type> handle_;
};

struct base_id_promise_type : base_promise_type
{
    size_t id{next_id++};
    static std::atomic<size_t> next_id;
};
inline std::atomic<size_t> base_id_promise_type::next_id{0};

template <typename T> class IdTask;
template <typename T = void> struct promise_type_with_id : base_id_promise_type
{
    Channel<T> channel_{1};
    void return_value(T value) { assert(channel_.push(std::move(value))); }
    auto get_return_object() -> IdTask<T>;
};
template <> struct promise_type_with_id<void> : base_id_promise_type
{
    Channel<> channel_{1};
    void return_void() { assert(channel_.push()); }
    auto get_return_object() -> IdTask<void>;
};
// task with id
class BaseIdTask : public BaseTask
{
  public:
    BaseIdTask() : BaseTask() {}
    BaseIdTask(coroutine_handle<base_promise_type> handle) : BaseTask(handle) {}
    BaseIdTask(BaseIdTask&& other) noexcept : BaseTask(std::move(other)) {}
    ~BaseIdTask() noexcept = default;
    size_t id() const noexcept { return static_cast<base_id_promise_type&>(handle_.promise()).id; }
};
template <typename T = void> class IdTask : public BaseIdTask
{
  public:
    using promise_type = promise_type_with_id<T>;
    IdTask() = default;
    IdTask(coroutine_handle<base_promise_type> handle) : BaseIdTask(handle) {}

    IdTask(IdTask&& other) noexcept : BaseTask(std::move(other)) {};
    auto& operator=(IdTask&& other) noexcept
    {
        BaseTask::operator=(std::move(other));
        return *this;
    }
    ~IdTask() = default;
    auto operator co_await() &&
    {
        Channel<T>* channel = &static_cast<promise_type&>(handle_.promise()).channel_;
        // other reference
        if (handle_.promise().task_count_ > 1)
        {
            // delete self
            operator=(IdTask<T>{});
        }
        else
        {
            if (channel->is_empty())
            {
                channel->close();
            }
        }
        return channel->async_pop();
    }
};

template <typename T> auto promise_type<T>::get_return_object() -> Task<T>
{
    return Task<T>(std::coroutine_handle<base_promise_type>::from_promise(*this));
}

inline auto promise_type<void>::get_return_object() -> Task<>
{
    return Task<>(std::coroutine_handle<base_promise_type>::from_promise(*this));
}
template <typename T> auto promise_type_with_id<T>::get_return_object() -> IdTask<T>
{
    return IdTask<T>(std::coroutine_handle<base_promise_type>::from_promise(*this));
}

inline auto promise_type_with_id<void>::get_return_object() -> IdTask<>
{
    return IdTask<>(std::coroutine_handle<base_promise_type>::from_promise(*this));
}
} // namespace utils
