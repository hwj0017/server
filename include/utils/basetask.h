#pragma once

#include <coroutine>
#include <cstddef>
#include <iostream>
#include <memory>
namespace utils
{
// RAII Base Task
class BaseTask;
struct base_promise_type
{
    size_t task_count_ = 0;
    std::suspend_never initial_suspend() { return {}; }
    // 当前协程运行完毕，在这里回到父协程，即continuation_
    std::suspend_always final_suspend() noexcept { return {}; }
    void unhandled_exception() { std::exit(-1); }
}; // struct base_promise_type

class BaseTask
{
  public:
    BaseTask() : handle_(nullptr) {}

    BaseTask(std::coroutine_handle<base_promise_type> handle) : handle_(handle) { ++handle.promise().task_count_; }
    template <typename promise_type>
    BaseTask(std::coroutine_handle<promise_type> handle)
        : BaseTask(std::coroutine_handle<base_promise_type>::from_promise(handle.promise()))
    {
        static_assert(std::is_base_of_v<base_promise_type, promise_type>);
    }
    BaseTask(BaseTask&& other) noexcept : handle_(other.handle_) { other.handle_ = nullptr; }
    BaseTask& operator=(BaseTask&& other) noexcept
    {
        if (this != &other)
        {
            if (handle_ && --handle_.promise().task_count_ == 0)
            {
                handle_.destroy();
            }
            handle_ = other.handle_;
            other.handle_ = nullptr;
        }
        return *this;
    }
    ~BaseTask() noexcept
    {
        auto& task_count = handle_.promise().task_count_;

        if (handle_ && --handle_.promise().task_count_ == 0)
        {
            std::cout << "task destroy" << std::endl;
            handle_.destroy();
        }
    }

    void resume() const noexcept { handle_.resume(); }
    bool done() noexcept { return handle_.done(); }
    operator bool() { return bool(handle_); }

  protected:
    std::coroutine_handle<base_promise_type> handle_;
};
} // namespace utils