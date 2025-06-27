#pragma once

#include <atomic>
#include <coroutine>
#include <cstddef>
#include <iostream>
#include <memory>
#include <type_traits>

using std::coroutine_handle;
using std::suspend_always;
using std::suspend_never;

namespace utils
{
// struct base_promise_type;
// template <typename T> struct promise_type;
// RAII Base Task
class BaseTask
{
  public:
    struct base_promise_type;

    BaseTask() : handle_(nullptr) {}

    BaseTask(coroutine_handle<base_promise_type> handle) : handle_(handle) { ++handle.promise().count; }

    template <typename promise_type>
    BaseTask(coroutine_handle<promise_type> handle)
        : handle_(coroutine_handle<base_promise_type>::from_address(handle.address()))
    {
        static_assert(std::is_base_of_v<base_promise_type, promise_type>);
    }
    BaseTask(BaseTask&& other) noexcept : handle_(other.handle_) { other.handle_ = nullptr; }
    BaseTask& operator=(BaseTask&& other) noexcept
    {
        handle_ = other.handle_;
        other.handle_ = nullptr;
        return *this;
    }
    ~BaseTask() noexcept
    {
        if (handle_ && --handle_.promise().count == 0)
        {
            handle_.destroy();
        }
    }
    bool await_ready()
    {
        if (done())
        {
            return true;
        }
        return false;
    }
    // 如果执行完，返回true;否则保存协程，返回false;
    template <typename promise_type> void await_suspend(coroutine_handle<promise_type> waiter)
    {
        static_assert(std::is_base_of_v<base_promise_type, promise_type>);
        handle_.promise().continuation_ = std::make_unique<BaseTask>(waiter);
    }
    void resume() noexcept { handle_.resume(); }
    bool done() noexcept { return handle_.done(); }
    operator bool() { return bool(handle_); }
    struct base_promise_type
    {
        std::unique_ptr<BaseTask> continuation_; // who waits on this coroutine
        size_t count = 0;
        suspend_never initial_suspend() { return {}; }
        // 当前协程运行完毕，在这里回到父协程，即continuation_
        auto final_suspend() noexcept
        {
            if (continuation_)
            {
                continuation_->resume();
            }
            return std::suspend_always{};
        }
        void unhandled_exception()
        { // TODO:
            std::exit(-1);
        }
    }; // struct base_promise_type
  protected:
    coroutine_handle<base_promise_type> handle_;
};

// template <typename T> struct Task;
// template <typename T> struct promise_type final : base_promise_type
// {
//     T result;
//     void return_value(T value) { result = std::move(value); }
//     auto get_return_object() -> Task<T>;
// };

// template <> struct promise_type<void> final : base_promise_type
// {
//     void return_void() {}
//     auto get_return_object() -> Task<void>;
// };

// return type
template <typename T = void> class Task : public BaseTask
{
  public:
    struct promise_type : base_promise_type
    {
        T result;
        void return_value(T value) { result = std::move(value); }
        auto get_return_object() -> Task<T> { return Task<T>{coroutine_handle<promise_type>::from_promise(*this)}; }
    };
    Task() = default;
    Task(coroutine_handle<promise_type> handle) : BaseTask(handle) {}

    Task(Task&& other) noexcept : BaseTask(std::move(other)) {};
    auto& operator=(Task&& other) noexcept
    {
        BaseTask::operator=(std::move(other));
        return *this;
    }
    ~Task() = default;
    auto await_resume() -> T { return std::move(static_cast<promise_type&>(handle_.promise()).result); }
};

template <> class Task<void> : public BaseTask
{
  public:
    struct promise_type : base_promise_type
    {
        void return_void() {}
        auto get_return_object() -> Task<void>
        {
            return Task<void>{coroutine_handle<promise_type>::from_promise(*this)};
        }
    };
    Task() = default;
    Task(coroutine_handle<promise_type> handle) : BaseTask(handle) {}
    Task(Task&& other) noexcept : BaseTask(std::move(other)) {};
    ~Task() = default;
    auto& operator=(Task&& other) noexcept
    {
        BaseTask::operator=(std::move(other));
        return *this;
    }
    void await_resume() {}
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

// template <typename T> struct Awaitable
// {
//     T result;
//     BaseTask task;
//     bool await_ready() { return false; }
//     void await_suspend(coroutine_handle<promise_type<T>> waiter) { task = BaseTask(waiter, &waiter.promise()); }
//     T await_resume() { return std::move(result); }
// };
} // namespace utils
