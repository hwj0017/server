#include "epoller.h"
#include <cassert>
#include <iostream>
#include <stdexcept>
#include <sys/epoll.h>
#include <unistd.h>
namespace tcp
{

Epoller::Epoller() : epollfd_(::epoll_create(1024))
{

    if (epollfd_ == -1)
    {
        throw std::runtime_error("Failed to create epoll file descriptor");
    }
}
Epoller::~Epoller()
{
    if (epollfd_ != -1)
    {
        ::close(epollfd_);
    }
}

void Epoller::add(Channel* channel)
{
    epoll_event event;
    event.data.ptr = channel;
    event.events = EPOLLET;
    event.events |= getEpollEvents(channel->type); // Set events based on the type
    ::epoll_ctl(epollfd_, EPOLL_CTL_ADD, channel->fd, &event);
}

void Epoller::remove(Channel* channel) { ::epoll_ctl(epollfd_, EPOLL_CTL_DEL, channel->fd, nullptr); }

void Epoller::update(Channel* channel)
{
    epoll_event event;
    event.data.ptr = channel;
    event.events = EPOLLET;
    event.events |= getEpollEvents(channel->type); // Set events based on the type
    if (::epoll_ctl(epollfd_, EPOLL_CTL_MOD, channel->fd, &event) == -1)
    {
        throw std::runtime_error("Failed to update epoll file descriptor");
    }
}

auto Epoller::poll() -> std::vector<Channel*>
{
    int event_count = ::epoll_wait(epollfd_, events_, kMaxEventNum_, -1);
    std::vector<Channel*> active_channels(event_count);
    for (int i = 0; i < event_count; ++i)
    {
        std::cout << "1" << std::endl;
        Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
        if (channel)
        {
            channel->expired_type = getTypeFromEpollEvents(events_[i].events); // set expired type
            active_channels[i] = channel;
        }
    }
    return active_channels;
}

} // namespace tcp