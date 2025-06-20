#include "iocontext.h"
#include "epoller.h"
#include "utils/channel.h"
#include "utils/task.h"
#include "waker.h"
#include <cassert>
#include <coroutine>
#include <memory>
#include <unistd.h>
namespace tcp
{
IoContext::IoContext() : waker_(this) { waker_.start(); }
void IoContext::run()
{
    while (true)
    {
        auto nodes = epoller_.poll();
        for (auto node : nodes)
        {
            handle_node(static_cast<Node*>(node));
        }
    }
}

void IoContext::handle_node(Node* node)
{
    auto original_type = node->type & Epoller::Type::Read;
    if (node->expired_type && Epoller::Type::Read && node->input_task)
    {
        node->input_task.resume();
    }
    if (node->expired_type && Epoller::Type::Write && node->output_task)
    {
        node->output_task.resume();
    }
    if (original_type != node->type)
    {
        epoller_.update(node);
    }
}
void IoContext::continue_write(int fd) {}
auto IoContext::thread_channel() -> utils::Channel<>& { return waker_.thread_channel(); }

bool IoContext::in_attached_thread() { return waker_.isInThread(); }
} // namespace tcp