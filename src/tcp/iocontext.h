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
    void add_io_task(int fd, utils::Task<> input_task, utils::Task<> output_task);
    void remove_io_task(int fd);
    // auto get_output_channel(int fd) -> utils::Channel<>&;
    // auto thread_channel() -> utils::Channel<>&;
    bool in_attached_thread();
    auto addTimer(Task, double delay, double interval) -> uint64_t;
    void remove_timer(uint64_t timer_id);
    void continue_read(int fd);
    void continue_write(int fd);
    void run();

  private:
    struct Node : public Epoller::Node
    {
        utils::Task<> input_task;
        utils::Task<> output_task;
        Node(int fd, Epoller::Type type, utils::Task<>&& input_task, utils::Task<>&& output_task)
            : Epoller::Node(fd, type), input_task(std::move(input_task)), output_task(std::move(output_task))
        {
        }
    };
    void handle_node(Node* node);
    Epoller epoller_;
    Waker waker_;
    std::unordered_map<int, Node> nodes_;
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