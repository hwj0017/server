// #pragma once
// #include "utils/task.h"
// namespace utils
// {
// struct promise_type_base_with_id;
// template <typename T> struct promise_type_with_id;
// struct TaskBaseWithId
// {
//     TaskBaseWithId() noexcept : handle_(nullptr), promise_base_(nullptr) {}
//     template <typename T> TaskBaseWithId(coroutine_handle<promise_type<T>> handle);
//     TaskBaseWithId(const TaskBaseWithId&) = delete;
//     TaskBaseWithId& operator=(const TaskBaseWithId&) = delete;
//     TaskBaseWithId(TaskBaseWithId&& other) noexcept : handle_(other.handle_), promise_base_(other.promise_base_)
//     {
//         other.handle_ = nullptr;
//         other.promise_base_ = nullptr;
//     }
//     TaskBaseWithId& operator=(TaskBaseWithId&& other) noexcept;
//     ~TaskBaseWithId() noexcept;
//     bool await_ready() { return false; }
//     // 如果执行完，返回true;否则保存协程，返回false;
//     template <typename T> bool await_suspend(coroutine_handle<promise_type<T>> waiter);
//     void resume() { handle_.resume(); }

//     operator bool() { return bool(handle_); }
//     coroutine_handle<void> handle_;
//     promise_type_base_with_id* promise_base_;
// };
// struct promise_type_base_with_id : public promise_type_base
// {
//     size_t id_{next_id_++};
//     static std::atomic<size_t> next_id_;
// };

// } // namespace utils