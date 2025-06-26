#pragma once
#include "node.h"
#include <sys/epoll.h>
#include <sys/types.h>
#include <vector>
namespace tcp
{
class Epoller
{
  public:
    Epoller();
    Epoller(const Epoller&) = delete;
    Epoller(Epoller&&) = delete;
    ~Epoller();
    void add(Node* node);
    void remove(Node* node);
    void update(Node* node);
    auto poll() -> std::vector<Node*>;

  private:
    static u_int32_t getEpollEvents(Node::Type type);
    static auto getTypeFromEpollEvents(u_int32_t events) -> Node::Type;
    static constexpr size_t kMaxEventNum_ = 1024;
    int epollfd_;
    epoll_event events_[kMaxEventNum_];
};

inline u_int32_t Epoller::getEpollEvents(Node::Type type)
{
    u_int32_t events = EPOLLET; // Edge-triggered mode
    if (type && Node::Type::Read)
    {
        events |= EPOLLIN;
    }
    if (type && Node::Type::Write)
    {
        events |= EPOLLOUT;
    }
    return events;
}
inline auto Epoller::getTypeFromEpollEvents(u_int32_t events) -> Node::Type
{
    Node::Type type = Node::Type::None;
    if (events & EPOLLIN)
    {
        type = type | Node::Type::Read;
    }
    if (events & EPOLLOUT)
    {
        type = type | Node::Type::Write;
    }
    return type;
}
} // namespace tcp