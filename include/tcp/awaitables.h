#pragma once

#include <coroutine>
namespace tcp
{
class IoContext;
struct Delay
{
    IoContext* io_context_;
    double delay;
    bool await_ready() { return delay <= 0; }
    void await_suspend(std::coroutine_handle<> handle) {};
    void await_resume() {}
};
} // namespace tcp