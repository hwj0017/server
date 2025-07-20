#pragma once
#include "epoller.h"
#include "ionode.h"
#include "timer.h"
#include "utils/channel.h"
#include "utils/task.h"
#include "waker.h"
#include <any>
#include <cassert>
#include <functional>
#include <memory>
#include <mutex>
#include <sys/select.h>
#include <tuple>
#include <unordered_map>
#include <vector>
namespace tcp
{

class IoContext
{
  public:
    using Task = utils::Task<>;

    IoContext();
    ~IoContext() = default;
    // 由调用方保证在线程内

    struct InThread
    {
        IoContext* io_context;
        InThread(IoContext* io_context) : io_context(io_context) {}
        bool await_ready() { return io_context->is_in_thread(); }
        template <typename promise_type> void await_suspend(std::coroutine_handle<promise_type> handle)
        {
            std::lock_guard<std::mutex> guard(io_context->tasks_mutex_);
            io_context->tasks_.emplace_back(handle);
            if (io_context->need_wakeup_)
            {
                io_context->waker_.wakeup();
            }
        }
        void await_resume() {}
    };

    struct Queue
    {
        IoContext* io_context;
        Queue(IoContext* io_context) : io_context(io_context) {}
        bool await_ready() { return false; }
        template <typename promise_type> void await_suspend(std::coroutine_handle<promise_type> handle)
        {
            std::lock_guard<std::mutex> guard(io_context->tasks_mutex_);
            io_context->tasks_.emplace_back(handle);
            if (io_context->need_wakeup_)
            {
                io_context->waker_.wakeup();
            }
        }
        void await_resume() {}
    };
    // not thread safe
    void run();
    void add(int fd);
    void remove(int fd);
    // void enable_read(IoNode* node)
    // {
    //     if (node->type && IoNode::Type::Read)
    //     {
    //         return;
    //     }
    //     node->type |= IoNode::Type::Read;
    //     epoller_.update(node);
    // }
    // void enable_write(IoNode* node)
    // {
    //     if (node->type && IoNode::Type::Write)
    //     {
    //         return;
    //     }
    //     node->type |= IoNode::Type::Write;
    //     epoller_.update(node);
    // }
    // void disable_read(IoNode* node)
    // {
    //     if (!(node->type && IoNode::Type::Read))
    //     {
    //         return;
    //     }
    //     node->type &= IoNode::Type::Write;
    //     epoller_.update(node);
    // }
    // void disable_write(IoNode* node)
    // {
    //     if (!(node->type && IoNode::Type::Write))
    //     {
    //         return;
    //     }
    //     node->type &= IoNode::Type::Read;
    //     epoller_.update(node);
    // }

    // thread safe
    bool is_in_thread() { return thread_id_ == std::this_thread::get_id(); }
    auto queue() -> Queue { return {this}; }

    auto add_delay(utils::BaseIdTask&& task, double delay) -> utils::Task<>
    {
        return timer_.add_delay(std::move(task), delay);
    }

    auto cancel_delay(size_t id) -> utils::Task<> { return timer_.cancel_delay(id); }
    auto in_thread() -> InThread { return {this}; }

    // auto delay() -> Delay;

  private:
    void handle_node(IoNode* node);
    Epoller epoller_;
    Waker waker_;
    Timer timer_;
    std::unordered_map<int, IoNode*> nodes_;
    bool need_wakeup_ = true;
    std::thread::id thread_id_{};
    std::vector<utils::BaseTask> tasks_{};
    std::mutex tasks_mutex_{};
    friend class InThread;
};
} // namespace tcp