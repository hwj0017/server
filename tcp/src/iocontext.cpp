#include "iocontext.h"
#include "epoller.h"
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
#include <utility>
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
            handle_node(static_cast<IoNode*>(node));
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
    if (node->expired_type && IoNode::Type::Read)
    {
        node->in_channel_.push();
    }
    if (node->expired_type && IoNode::Type::Write)
    {
        node->out_channel_.push();
    }
}

auto IoContext::get_in_channel(int fd) -> utils::Channel<>*
{
    assert(fd >= 0);
    utils::Channel<>* channel = nullptr;
    auto it = io_nodes_.find(fd);
    if (it == io_nodes_.end())
    {
        auto io_node = std::make_unique<IoNode>(fd);
        io_node->type = IoNode::Type::Read;
        channel = &io_node->in_channel_;
        epoller_.add(io_node.get());
        io_nodes_.emplace(fd, std::move(io_node));
    }
    else
    {
        channel = &it->second->in_channel_;
        if (!(it->second->type && Node::Type::Read))
        {
            it->second->type |= IoNode::Type::Read;
            epoller_.update(it->second.get());
        }
    }
    return channel;
}
auto IoContext::get_out_channel(int fd) -> utils::Channel<>*
{
    assert(fd >= 0);
    utils::Channel<>* channel = nullptr;
    auto it = io_nodes_.find(fd);
    if (it == io_nodes_.end())
    {
        auto io_node = std::make_unique<IoNode>(fd);
        io_node->type = IoNode::Type::Write;
        channel = &io_node->out_channel_;
        epoller_.add(io_node.get());
        io_nodes_.emplace(fd, std::move(io_node));
    }
    else
    {
        channel = &it->second->out_channel_;
        if (!(it->second->type && IoNode::Type::Write))
        {
            it->second->type |= IoNode::Type::Write;
            epoller_.update(it->second.get());
        }
    }
    return channel;
}

void IoContext::remove(int fd)
{
    if (auto it = io_nodes_.find(fd); it != io_nodes_.end())
    {
        epoller_.remove(it->second.get());
        io_nodes_.erase(it);
    }
}
} // namespace tcp