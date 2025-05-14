#include "epoller.h"
#include <cassert>
#include <sys/epoll.h>
#include <unistd.h>
#include <vector>
namespace tcp
{
Epoller::Epoller() : epfd_(epoll_create1(EPOLL_CLOEXEC))
{
    assert(epfd_ >= 0);
}

Epoller::~Epoller()
{
    assert(epfd_ >= 0);
    close(epfd_);
}

auto Epoller::poll(int timeout) -> std::vector<Action>
{
    auto eventsNum = epoll_wait(epfd_, events_, kEventSize, timeout);
    assert(eventsNum >= 0);
    std::vector<Action> actions(eventsNum);

    for (auto i = 0; i < eventsNum; ++i)
    {
        auto fd = events_[i].data.fd;
        auto event = events_[i].events;
        Type type = Type::kNone;
        if (event & EPOLLIN && event & EPOLLOUT)
        {
            type = Type::kBoth;
        }
        else if (event & EPOLLIN)
        {
            type = Type::kReadable;
        }
        else
            type = Type::kWriteable;
        actions[i] = {fd, type};
    }
    return actions;
}

void Epoller::update(int fd, Type type)
{
    assert(fd >= 0);
    auto it = attachedFds_.find(fd);
    if (it == attachedFds_.end())
    {
        if (type != Type::kNone)
        {
            auto ev = getEvent(fd, type);
            epoll_ctl(epfd_, EPOLL_CTL_ADD, fd, &ev);
            attachedFds_[fd] = type;
        }
    }
    else if (it->second != type)
    {
        if (type != Type::kNone)
        {
            auto ev = getEvent(fd, type);
            epoll_ctl(epfd_, EPOLL_CTL_MOD, fd, &ev);
            it->second = type;
        }
        else
        {
            epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, nullptr);
            attachedFds_.erase(it);
        }
    }
}

epoll_event Epoller::getEvent(int fd, Type type)
{
    epoll_event ev;
    ev.events = 0;
    ev.data.fd = fd;
    if (type == Type::kReadable || type == Type::kBoth)
    {
        ev.events |= EPOLLIN;
    }
    if (type == Type::kWriteable || type == Type::kBoth)
    {
        ev.events |= EPOLLOUT;
    }
    return ev;
}

} // namespace tcp
