#pragma once

#include "utils/task.h"
#include <coroutine>
namespace tcp
{
class IoContext;
struct Delay
{
    IoContext* io_context_;
    double delay;
    bool await_ready() { return delay <= 0; }
    template <typename promise_type> void await_suspend(std::coroutine_handle<promise_type> handle)
    {
        await_suspend(std::move(utils::BaseIdTask(handle)));
    };
    void await_suspend(utils::BaseIdTask&& task);
    void await_resume() {}
};
} // namespace tcp