#pragma once
#include "epoller.h"
#include "utils/channel.h"
#include "utils/task.h"
#include "waker.h"
#include <functional>
#include <memory>
#include <sys/select.h>
#include <tuple>
#include <unordered_map>
namespace tcp
{
class IoContext
{
  public:
    using Task = utils::Task<>;
    IoContext();
    ~IoContext() = default;
    // 由调用方保证在线程内
    // void add(std::unique_ptr<Channel> channel);
    // remove in the last
    // void remove(int fd);
    auto get_channels(int fd) -> std::tuple<utils::Channel<>, utils::Channel<>>;
    void close_channels(int fd);
    auto thread_channel() -> utils::Channel<>&;
    bool in_attached_thread();
    auto addTimer(Task, double delay, double interval) -> uint64_t;
    void remove_timer(uint64_t timer_id);
    void run();

  private:
    void handleEvent(Channel* channel);
    Epoller epoller_;
    Waker waker_;
    std::unordered_map<int, std::unique_ptr<Channel>> channels_;
};

struct InThread
{
    Waker* waker_;
    InThread(Waker* waker) : waker_(waker) {}
    bool await_ready() { return waker_->isInThread(); }
    template <typename T> void await_suspend(std::coroutine_handle<utils::promise_type<T>> handle)
    {
        waker_->addTask(utils::TaskBase(handle, &handle.promise()));
    }
};
} // namespace tcp