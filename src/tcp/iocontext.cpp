#include "iocontext.h"
#include "channel.h"
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
        auto channels = epoller_.poll();
        for (auto channel : channels)
        {
            handleEvent(channel);
        }
    }
}
void IoContext::add(std::unique_ptr<Channel> channel)
{
    auto it = channels_.find(channel->fd);
    assert(it == channels_.end());
    epoller_.add(channel.get());
    channels_.emplace(channel->fd, std::move(channel));
}
void IoContext::remove(int fd)
{
    auto it = channels_.find(fd);
    if (it != channels_.end())
    {
        epoller_.remove(it->second.get());
        channels_.erase(it);
    }
}

void IoContext::handleEvent(Channel* channel)
{
    if (channel->expired_type && Epoller::Type::Read)
    {
        if (channel->read_callBack)
        {
            channel->read_callBack();
        }
    }
    if (channel->expired_type && Epoller::Type::Write)
    {
        channel->type = channel->type & Epoller::Type::Read; // Reset to read type only
        if (channel->write_callBack)
        {
            channel->write_callBack();
        }
    }
}

auto IoContext::inThread() -> utils::Task<>
{
    if (waker_.isInThread())
    {
        co_return;
    }
    auto self_task = co_await utils::SelfTask();
    waker_.addTask(std::move(self_task));
    co_await std::suspend_always{};
}
} // namespace tcp