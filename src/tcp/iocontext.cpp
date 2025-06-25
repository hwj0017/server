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
        waker_.update();
        for (auto node : nodes)
        {
            handle_node(static_cast<Node*>(node));
        }
    }
}

void IoContext::handle_node(Node* node)
{
    // save read mode but not write mode
    node->type = node->type & Epoller::Type::Read;
    auto original_type = node->type;
    if (node->expired_type && Epoller::Type::Read)
    {
        // assert
        node->input_channel.push();
        // not pop again
        if (node->input_channel.is_full())
        {
            // not read
            node->type = node->type & Epoller::Type::Write;
            // iocontext and node is longer than input_channel and task
            // listen input channel
            listen_input_channel(*node);
        }
    }
    if (node->expired_type && Epoller::Type::Write)
    {
        // assert
        node->output_channel.push();
        // pop again
        if (!node->output_channel.is_full())
        {
            // write again
            node->type = node->type | Epoller::Type::Write;
            // iocontext and node is longer than output_channel and task
        }
        else
        {
            // listen output channel
            listen_output_channel(*node);
        }
    }
    if (original_type != node->type)
    {
        epoller_.update(node);
    }
}

auto IoContext::add(int fd, std::any data) -> std::tuple<utils::Channel<>&, utils::Channel<>&>
{
    assert(fd >= 0);
    auto it = nodes_.find(fd);
    assert(it == nodes_.end());
    auto node = std::make_unique<Node>(fd);
    node->data = std::move(data);
    auto node_ptr = node.get();
    epoller_.add(node_ptr);
    nodes_.emplace(fd, std::move(node));
    listen_input_channel(*node_ptr);
    listen_output_channel(*node_ptr);
    return {node_ptr->input_channel, node_ptr->output_channel};
}
void IoContext::remove(int fd)
{
    auto it = nodes_.find(fd);
    if (it != nodes_.end())
    {
        auto& node = it->second;
        epoller_.remove(node.get());
        nodes_.erase(it);
    }
}

auto IoContext::listen_input_channel(Node& node) -> utils::Task<>
{
    if (!co_await node.input_channel.not_full())
    {
        node.type = node.type | Epoller::Type::Read;
        epoller_.update(&node);
    }
}

auto IoContext::listen_output_channel(Node& node) -> utils::Task<>
{
    if (!co_await node.output_channel.not_full())
    {
        node.type = node.type | Epoller::Type::Write;
        epoller_.update(&node);
    }
}

} // namespace tcp