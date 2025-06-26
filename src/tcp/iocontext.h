#pragma once
#include "epoller.h"
#include "node.h"
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
        template <typename T> void await_suspend(std::coroutine_handle<utils::promise_type<T>> handle)
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
        template <typename T> void await_suspend(std::coroutine_handle<utils::promise_type<T>> handle)
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
    void add(Node* node);
    void remove(Node* node);
    void enable_read(Node* node)
    {
        if (node->type && Node::Type::Read)
        {
            return;
        }
        node->type |= Node::Type::Read;
        epoller_.update(node);
    }
    void enable_write(Node* node)
    {
        if (node->type && Node::Type::Write)
        {
            return;
        }
        node->type |= Node::Type::Write;
        epoller_.update(node);
    }
    void disable_read(Node* node)
    {
        if (!(node->type && Node::Type::Read))
        {
            return;
        }
        node->type &= Node::Type::Write;
        epoller_.update(node);
    }
    void disable_write(Node* node)
    {
        if (!(node->type && Node::Type::Write))
        {
            return;
        }
        node->type &= Node::Type::Read;
        epoller_.update(node);
    }

    // thread safe
    bool is_in_thread() { return thread_id_ == std::this_thread::get_id(); }
    auto in_thread() -> InThread { return {this}; }
    auto queue() -> Queue { return {this}; }

    auto addTimer(Task, double delay, double interval) -> uint64_t;

    void remove_timer(uint64_t timer_id);

    void run();

  private:
    void handle_node(Node* node);
    Epoller epoller_;
    Waker waker_;
    std::unordered_map<int, Node*> nodes_;
    bool need_wakeup_ = true;
    std::thread::id thread_id_{};
    std::vector<utils::TaskBase> tasks_{};
    std::mutex tasks_mutex_{};
    friend class InThread;
};

} // namespace tcp