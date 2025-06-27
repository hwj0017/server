#include "iocontext.h"
#include "epoller.h"
#include "node.h"
#include "utils/channel.h"
#include "utils/task.h"
#include "waker.h"
#include <cassert>
#include <coroutine>
#include <memory>
#include <mutex>
#include <thread>
#include <unistd.h>
#include <vector>
namespace tcp
{
IoContext::IoContext() : waker_(this), thread_id_(std::this_thread::get_id()) { waker_.start(); }
void IoContext::run()
{
    while (true)
    {

        auto nodes = epoller_.poll();
        need_wakeup_ = false;
        for (auto node : nodes)
        {
            handle_node(static_cast<Node*>(node));
        }
        std::vector<utils::BaseTask> temp_tasks;
        {
            std::lock_guard<std::mutex> guard(tasks_mutex_);
            temp_tasks.swap(tasks_);
            need_wakeup_ = true;
        }
        for (auto& task : temp_tasks)
        {
            task.resume();
        }
    }
}

void IoContext::handle_node(Node* node)
{
    // save read mode but not write mode
    node->type = node->type & Node::Type::Read;
    if (node->expired_type && Node::Type::Read)
    {
        node->read_task.resume();
    }
    if (node->expired_type && Node::Type::Write)
    {
        node->write_task.resume();
    }
}

void IoContext::add(Node* node)
{
    auto fd = node->fd;
    assert(fd >= 0);
    auto it = nodes_.find(fd);
    assert(it == nodes_.end());
    epoller_.add(node);
    nodes_.emplace(fd, node);
}
void IoContext::remove(Node* node)
{
    if (auto it = nodes_.find(node->fd); it != nodes_.end())
    {
        epoller_.remove(node);
        nodes_.erase(it);
    }
}

} // namespace tcp