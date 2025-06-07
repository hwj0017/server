#include "epoller.h"
#include "channel.h"
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

auto Epoller::poll(int timeout) -> std::vector<Channel*>
{
    auto eventsNum = epoll_wait(epfd_, events_, kEventSize, timeout);
    assert(eventsNum >= 0);
    std::vector<Channel*> expired_channels(eventsNum);

    for (auto i = 0; i < eventsNum; ++i)
    {
        auto event = events_[i].events;
        auto channel = static_cast<Channel*>(events_[i].data.ptr);
        if (event & EPOLLIN && event & EPOLLOUT)
        {
            channel->setExpiredType(Channel::Type::kBoth);
        }
        else if (event & EPOLLIN)
        {
            channel->setExpiredType(Channel::Type::kRead);
        }
        else
        {
            channel->setExpiredType(Channel::Type::kWrite);
        }
        expired_channels[i] = channel;
    }
    return expired_channels;
}

void Epoller::update(Channel* channel)
{
    assert(channel != nullptr);
    auto fd = channel->fd();
    assert(fd >= 0);
    auto type = channel->type();
    if (channel->isStop())
    {
        epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, nullptr);
    }
    else
        switch (type)
        {
        case Channel::Type::kNone:
            epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, nullptr);
            break;
        case Channel::Type::kRead:
            epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, nullptr);
            break;
        case Channel::Type::kWrite:
            epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, nullptr);
            break;
        }
    if (type != Type::kNone)
    {
        auto ev = getEvent(fd, type);
        epoll_ctl(epfd_, EPOLL_CTL_ADD, fd, &ev);
        attachedFds_[fd] = type;
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
