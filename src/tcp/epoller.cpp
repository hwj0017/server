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

void Epoller::add(Node* node)
{
    epoll_event event;
    event.data.ptr = node;
    event.events = EPOLLET;
    event.events |= getEpollEvents(node->type); // Set events based on the type
    ::epoll_ctl(epollfd_, EPOLL_CTL_ADD, node->fd, &event);
}

void Epoller::remove(Node* node) { ::epoll_ctl(epollfd_, EPOLL_CTL_DEL, node->fd, nullptr); }

void Epoller::update(Node* node)
{
    epoll_event event;
    event.data.ptr = node;
    event.events = EPOLLET;
    event.events |= getEpollEvents(node->type); // Set events based on the type
    if (::epoll_ctl(epollfd_, EPOLL_CTL_MOD, node->fd, &event) == -1)
    {
        throw std::runtime_error("Failed to update epoll file descriptor");
    }
}

auto Epoller::poll() -> std::vector<Node*>
{
    int event_count = ::epoll_wait(epollfd_, events_, kMaxEventNum_, -1);
    std::vector<Node*> active_nodes(event_count);
    for (int i = 0; i < event_count; ++i)
    {
        std::cout << "1" << std::endl;
        Node* node = static_cast<Node*>(events_[i].data.ptr);
        if (node)
        {
            node->expired_type = getTypeFromEpollEvents(events_[i].events); // set expired type
            active_nodes[i] = node;
        }
    }
    return active_nodes;
}

} // namespace tcp