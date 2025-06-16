#pragma once

#include <coroutine>
#include <cstddef>
#include <iostream>

using std::coroutine_handle;
using std::suspend_always;
using std::suspend_never;

namespace utils
{
struct promise_type_base;
template <typename T> struct promise_type;
struct TaskBase
{
    TaskBase() noexcept : handle_(nullptr), promise_base_(nullptr) {}
    TaskBase(coroutine_handle<> handle, promise_type_base* promise_base);
    TaskBase(const TaskBase&&) = delete;
    TaskBase& operator=(const TaskBase&&) = delete;
    TaskBase(TaskBase&& other) noexcept : handle_(other.handle_), promise_base_(other.promise_base_)
    {
        other.handle_ = nullptr;
        other.promise_base_ = nullptr;
    }
    TaskBase& operator=(TaskBase&& other) noexcept;
    ~TaskBase() noexcept;
    bool await_ready() { return false; }
    // 如果执行完，返回true;否则保存协程，返回false;
    template <typename T> bool await_suspend(coroutine_handle<promise_type<T>> waiter);
    void resume() { handle_.resume(); }
    operator bool() { return bool(handle_); }
    coroutine_handle<void> handle_;
    promise_type_base* promise_base_;
};
struct promise_type_base
{
    TaskBase continuation_; // who waits on this coroutine
    size_t count = 0;
    suspend_never initial_suspend() { return {}; }
    // 当前协程运行完毕，在这里回到父协程，即continuation_
    auto final_suspend() noexcept
    {
        if (continuation_)
        {
            continuation_.resume();
        }
        return std::suspend_always{};
    }

    void unhandled_exception()
    { // TODO:
        std::exit(-1);
    }
}; // struct promise_type_base

template <typename T> struct Task;
template <typename T> struct promise_type final : promise_type_base
{
    T result;
    void return_value(T value) { result = std::move(value); }
    auto get_return_object() -> Task<T>;
};

template <> struct promise_type<void> final : promise_type_base
{
    void return_void() {}
    auto get_return_object() -> Task<void>;
};
template <typename T = void> struct Task : TaskBase
{
    using promise_type = utils::promise_type<T>;
    Task() : TaskBase() {}
    Task(coroutine_handle<promise_type> handle) : TaskBase(handle, &handle.promise()) {}
    Task(const Task&&) = delete;
    Task& operator=(const Task&&) = delete;
    Task(Task&& other) = delete;
    Task& operator=(Task&&) = delete;
    ~Task() = default;
    auto await_resume() -> T;
};

// template <> struct Task<void>
// {
//     using promise_type = utils::promise_type<void>;
//     Task() : handle_(nullptr) {}
//     Task(coroutine_handle<promise_type_base> handle) : handle_(handle) {}
//     Task(const Task&&) = delete;
//     Task& operator=(const Task&&) = delete;
//     Task(Task&& other) noexcept : handle_(other.handle_) { other.handle_ = nullptr; }
//     ~Task();
//     bool await_ready() { return false; }
//     void await_resume();
//     bool await_suspend(coroutine_handle<> waiter);
//     void resume() { handle_.resume(); }

//     coroutine_handle<promise_type_base> handle_;
// };
// 用来获取自身句柄
struct SelfTask
{
    bool await_ready() noexcept { return false; }
    template <typename promise_type> bool await_suspend(coroutine_handle<promise_type> coro) noexcept
    {
        task = TaskBase(coro, &coro.promise());
        return false;
    }
    auto await_resume() noexcept -> TaskBase { return std::move(task); }
    TaskBase task;
};
inline TaskBase::TaskBase(coroutine_handle<> handle, promise_type_base* promise_base)
    : handle_(handle), promise_base_(promise_base)
{
    ++promise_base_->count;
}
inline TaskBase::~TaskBase() noexcept
{
    if (handle_ && --promise_base_->count == 0)
    {
        handle_.destroy();
    }
}
inline auto TaskBase::operator=(TaskBase&& other) noexcept -> TaskBase&
{
    if (this != &other)
    {
        if (handle_ && --promise_base_->count == 0)
        {
            handle_.destroy();
        }
        handle_ = other.handle_;
        promise_base_ = other.promise_base_;
        other.handle_ = nullptr;
        other.promise_base_ = nullptr;
    }
    return *this;
}
template <typename T> auto promise_type<T>::get_return_object() -> Task<T>
{
    return Task<T>{coroutine_handle<promise_type<T>>::from_promise(*this)};
}
inline auto promise_type<void>::get_return_object() -> Task<void>
{
    return Task<void>{coroutine_handle<promise_type<void>>::from_promise(*this)};
}
template <typename T> auto Task<T>::await_resume() -> T
{
    auto promise = static_cast<promise_type*>(promise_base_);
    auto result = std::move(promise->result);
    return result;
}
template <> inline void Task<void>::await_resume() {}
template <typename T> inline bool TaskBase::await_suspend(coroutine_handle<promise_type<T>> waiter)
{

    if (handle_.done())
    {
        return false;
    }
    promise_base_->continuation_ = TaskBase(waiter, &waiter.promise());
    return true; // return
                 // true，表示当前协程挂起，让子协程，即handle_所表示的协程恢复。子协程结束完以后又回到waiter。
}

template <typename T> struct Awaitable
{
    T result;
    TaskBase task;
    bool await_ready() { return false; }
    void await_suspend(coroutine_handle<promise_type<T>> waiter) { task = TaskBase(waiter, &waiter.promise()); }
    T await_resume() { return std::move(result); }
};
} // namespace utils
