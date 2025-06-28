#include "iocontext.h"
#include "epoller.h"
#include "ionode.h"
#include "timer.h"
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
IoContext::IoContext() : waker_(this), timer_(this), thread_id_(std::this_thread::get_id())
{
    waker_.start();
    timer_.start();
}
void IoContext::run()
{
    while (true)
    {
        timer_.update();
        auto nodes = epoller_.poll();
        need_wakeup_ = false;
        for (auto node : nodes)
        {
            handle_node(node);
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

void IoContext::handle_node(IoNode* node)
{
    // save read mode but not write mode
    node->type = node->type & IoNode::Type::Read;
    if (!node->is_closed && (node->expired_type && IoNode::Type::Read))
    {
        node->read_task.resume();
    }
    if (!node->is_closed && (node->expired_type && IoNode::Type::Write))
    {
        node->write_task.resume();
    }
}

void IoContext::add(IoNode* node)
{
    auto fd = node->fd;
    assert(fd >= 0);
    auto it = nodes_.find(fd);
    assert(it == nodes_.end());
    epoller_.add(node);
    nodes_.emplace(fd, node);
}
void IoContext::remove(IoNode* IoNode)
{
    if (auto it = nodes_.find(IoNode->fd); it != nodes_.end())
    {
        epoller_.remove(IoNode);
        nodes_.erase(it);
    }
}

} // namespace tcp