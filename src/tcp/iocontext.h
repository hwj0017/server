#pragma once
#include "epoller.h"
#include "utils/channel.h"
#include "utils/task.h"
#include "waker.h"
#include <any>
#include <functional>
#include <memory>
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
        Waker* waker_;
        InThread(Waker* waker) : waker_(waker) {}
        bool await_ready() { return waker_->isInThread(); }
        template <typename T> void await_suspend(std::coroutine_handle<utils::promise_type<T>> handle)
        {
            waker_->addTask(utils::TaskBase(handle));
        }
        void await_resume() {}
    };
    struct Node : protected Epoller::Node
    {
        Node(int fd) : Epoller::Node(fd) {};
        std::function<void()> read_callback;
        std::function<void()> write_callback;
    };
    void add(std::unique_ptr<Node> node);
    void remove(Node* node);
    void enable_read(Node* node);
    void enable_write(Node* node);
    void disable_read(Node* node);
    void disable_write(Node* node);
    // auto get_input_channel(int fd) -> utils::Channel<>&;
    // auto get_output_channel(int fd) -> utils::Channel<>&;
    // remove the channel associated with the fd
    auto in_thread() -> InThread { return {&waker_}; }
    auto addTimer(Task, double delay, double interval) -> uint64_t;
    void remove_timer(uint64_t timer_id);
    void run();

  private:
    auto listen_input_channel(Node& node) -> utils::Task<>;
    auto listen_output_channel(Node& node) -> utils::Task<>;

    void handle_node(Node* node);
    Epoller epoller_;
    Waker waker_;
    std::unordered_map<int, std::unique_ptr<Node>> nodes_;
};

} // namespace tcp